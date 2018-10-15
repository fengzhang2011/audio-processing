/*************************************************
 *
 * Compute the amplifier-frequency of an input wave data.
 *
 * Author: Feng Zhang (zhjinf@gmail.com)
 * Date: 2018-10-13
 * 
 * Copyright:
 *   See LICENSE.
 * 
 ************************************************/

#include <complex>
#include <vector>

#include "ffts.h"

#include "napi_ampfreq.h"

// #include <stdio.h>

void computeAmplifierFrequency(const std::vector<float> &wavData, std::vector<float> &amplifiers, int32_t sampleRate, size_t window, size_t overlap)
{
  for(size_t i=0; i<window; i++)
  {
    amplifiers[i] = 0.0;
  }

  int count = 0;
  for(size_t i=0; i<wavData.size()-window; i+=overlap)
  {
    // printf("size=%d window=%d overlap=%d i=%d\n", (int)wavData.size(), (int)window, (int)overlap, (int)i);
    std::vector<float>::const_iterator first = wavData.begin() + i;
    std::vector<float>::const_iterator last = wavData.begin() + i + window;
    std::vector<float> data(first, last);

    // Perform the FFT on the windowed data.
    size_t N = data.size();
    std::vector<std::complex<float>> signalb_ext(N);
    for(size_t j=0; j<N; j++)
      signalb_ext[j] = {float(data[j]), 0.0};

    std::vector<std::complex<float>> out(N);
    auto fft_forward = ffts_init_1d(N, FFTS_FORWARD);
    ffts_execute(fft_forward, signalb_ext.data(), out.data());

    for(size_t j=0; j<N; j++)
      amplifiers[j] += std::abs(out[j]);

    ffts_free(fft_forward);

    count ++;
  }

  for(size_t i=0; i<window/2+1; i++) // fs/2+1: 0 is the DC, 1 to fs/2 are the amplifiers at each frequency.
  {
    amplifiers[i] /= count;
    // printf("af[%d]=%f\n", (int)i, amplifiers[i]);
  }
}

// Compute the Amplifier-Frequency
// arg[0]: wavdata
// arg[1]: sample rate
napi_value ampfreq(napi_env env, napi_callback_info args)
{
  napi_status status;

  // Create the resulting object.
  napi_value result;
  status = napi_create_object(env, &result);
  if (status != napi_ok) return nullptr;

  // Parse the input arguments.
  size_t argc = 2;
  napi_value argv[2];
  status = napi_get_cb_info(env, args, &argc, argv, NULL, NULL);

  // -- Get the wave data buffer. (Only accepts one channel).
  float* data;
  napi_typedarray_type type;
  size_t length;
  napi_value arraybuffer;
  size_t byte_offset;
  status = napi_get_typedarray_info(env, argv[0], &type, &length, (void**) &data, &arraybuffer, &byte_offset);
  if (status != napi_ok) return nullptr;
  // -- Save the wave data buffer. (Only accepts one channel).
  std::vector<float> wavData(length); // We only use the first channel or at most the first two channels.
  for (size_t i=0; i<length; i++) wavData[i] = data[i];

  // -- Get the sample rate.
  int32_t sampleRate;
  status = napi_get_value_int32(env, argv[1], &sampleRate);
  if (status != napi_ok) return nullptr;

  // Compute the amplifier-frequency.
  size_t window = sampleRate; // As as result, each data point in the result represents 1 Hz.
  size_t overlap = window/2;
  // Resize the data if it is less than the sample rate.
  if ((size_t)sampleRate>length) wavData.resize(sampleRate, 0.0);

  std::vector<float> amplifiers(window);
  computeAmplifierFrequency(wavData, amplifiers, sampleRate, window, overlap);

  // Set the return value.
  length = amplifiers.size();
  // -- First, create the ArrayBuffer.
  data = NULL;
  size_t byte_length = length*sizeof(float);
  status = napi_create_arraybuffer(env, byte_length, (void**)&data, &arraybuffer);
  if (status != napi_ok) return nullptr;
  for (size_t i=0; i<length; i++) data[i] = amplifiers[i];
  // -- Second, create the TypedArray.
  byte_offset = 0;
  napi_value ampfreq_value;
  status = napi_create_typedarray(env, napi_float32_array, length, arraybuffer, byte_offset, &ampfreq_value);
  if (status != napi_ok) return nullptr;

  // Set the named property.
  status = napi_set_named_property(env, result, "ampfreq", ampfreq_value);
  if (status != napi_ok) return nullptr;

  return result;
}

