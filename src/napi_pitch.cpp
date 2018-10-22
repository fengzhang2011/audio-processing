/*************************************************
 *
 * Compute the pitch of a given audio data.
 *
 * Author: Feng Zhang (zhjinf@gmail.com)
 * Date: 2018-10-13
 * 
 * Copyright:
 *   See LICENSE.
 * 
 ************************************************/

#include <stdio.h>
#include <cstring>
#include <vector>

#include "AudioFile.h"
#include "pitch_detection.h"

#include "napi_pitch.h"


double computePitchEfficiently(const std::vector<double> &wavData, int32_t sampleRate, const char* methodName)
{
  double sum = 0.0;
  int count = 0;
  int window = sampleRate/25; // 40 ms per frame.
  int overlap = window/2; // 20 ms as the overlap.

  for(size_t i=0; i<wavData.size()-window; i+=overlap)
  {
    std::vector<double>::const_iterator first = wavData.begin() + i;
    std::vector<double>::const_iterator last = wavData.begin() + i + window;
    std::vector<double> data(first, last);

    double pitch = -1.0;
    if ( 0 == strcmp(methodName, "acorr") ) {
      pitch = get_pitch_autocorrelation(data, sampleRate);
    } else if( 0 == strcmp(methodName, "yin") ) {
      pitch = get_pitch_yin(data, sampleRate);
    } else if( 0 == strcmp(methodName, "mpm") ) {
      pitch = get_pitch_mpm(data, sampleRate);
    } else if( 0 == strcmp(methodName, "goertzel") ) {
      pitch = get_pitch_goertzel(data, sampleRate);
    } else if( 0 == strcmp(methodName, "dft") ) {
      pitch = get_pitch_dft(data, sampleRate);
    }

    if(pitch>0 && pitch<1000) // Remove the abnormal points.
    {
      sum += pitch;
      count ++;
    }
  }

  // compute the average
  double mean = sum/count;

  return mean;
}

// Detect the pitch from a given mono-channel audio.
// arg[0]: wavdata (a single channel vector<float>)
// arg[1]: sample rate
// arg[2]: method  (available choices: acorr, yin, mpm, goertzel, dft)
napi_value detectPitch(napi_env env, napi_callback_info args)
{
  napi_value result;

  napi_status status;

  // Parse the input arguments.
  size_t argc = 3;
  napi_value argv[3];
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
  std::vector<double> wavData(length); // We only use the first channel or at most the first two channels.
  for (size_t i=0; i<length; i++) wavData[i] = data[i];

  // -- Get the sample rate.
  int32_t sampleRate;
  status = napi_get_value_int32(env, argv[1], &sampleRate);
  if (status != napi_ok) return nullptr;

  // -- Get the wave file name.
  char methodName[128];
  size_t lenMethodName;
  status = napi_get_value_string_utf8(env, argv[2], methodName, 128, &lenMethodName);
  if (status != napi_ok) return nullptr;

  // Compute the pitch.
  double pitch = computePitchEfficiently(wavData, sampleRate, methodName);

  // Set the pitch.
  napi_value retPitch;
  status = napi_create_double(env, pitch, &retPitch);
  if (status != napi_ok) return nullptr;

  // Create the resulting object.
  status = napi_create_object(env, &result);
  if (status != napi_ok) return nullptr;

  // Set the named property.
  status = napi_set_named_property(env, result, "pitch", retPitch);
  if (status != napi_ok) return nullptr;

  return result;
}

