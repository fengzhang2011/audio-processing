/*************************************************
 *
 * Load/Save audio data from/to file.
 *
 * Author: Feng Zhang (zhjinf@gmail.com)
 * Date: 2018-10-13
 * 
 * Copyright:
 *   See LICENSE.
 * 
 ************************************************/

#include "AudioFile.h"

#include "napi_audiofile.h"
#include "napi_common.h"


// Read the Audio File (Only WAV/PCM and AIFF are supported.)
napi_value readAudio(napi_env env, napi_callback_info args)
{
  napi_value result;
  napi_deferred deferred;
  napi_value promise;

  napi_status status;

  // Create the promise.
  status = napi_create_promise(env, &deferred, &promise);
  if (status != napi_ok) { throwException(env, "Failed to create the promise object."); return nullptr; }

  // Create the resulting object.
  status = napi_create_object(env, &result);
  if (status != napi_ok) { throwException(env, "Failed to create the result object."); return nullptr; }

  // Parse the input arguments.
  size_t argc = 1;
  napi_value argv[1];
  status = napi_get_cb_info(env, args, &argc, argv, NULL, NULL);
  if (status != napi_ok) { throwException(env, "Failed to parse the arguments."); return nullptr; }

  char wavFileName[128];
  size_t lenFileName;
  status = napi_get_value_string_utf8(env, argv[0], wavFileName, 128, &lenFileName);
  if (status != napi_ok) { throwException(env, "Failed to read the wav file name."); return nullptr; }

  // -- READ IN THE WAVE FILE.
  AudioFile<float> audioFile;
  audioFile.load(wavFileName);

  //    -- Read the wave format information.
  int32_t sampleRate = audioFile.getSampleRate();
  napi_value samplerate;
  status = napi_create_int32(env, sampleRate, &samplerate);
  if (status != napi_ok) { throwException(env, "Failed to read the sample rate."); return nullptr; }

  int32_t bitDepth = audioFile.getBitDepth();
  napi_value bitdepth;
  status = napi_create_int32(env, bitDepth, &bitdepth);
  if (status != napi_ok) { throwException(env, "Failed to read the bit depth."); return nullptr; }

  int32_t nbChannels = audioFile.getNumChannels();
  napi_value channels;
  status = napi_create_int32(env, nbChannels, &channels);
  if (status != napi_ok) { throwException(env, "Failed to read the number of channels."); return nullptr; }

  //    -- Read the wave data
  std::vector<std::vector<float>> buffer = audioFile.samples;
  if (status != napi_ok) { throwException(env, "Failed to read the wav data using audioFile.samples."); return nullptr; }
  //printf("%s:%d\n", __FILE__, __LINE__);

  // We only use the first channel or at most the first two channels.
  napi_value wavdataL;
  {
    std::vector<float> wavData = buffer[0];
    size_t length = wavData.size();
    //       -- First, create the ArrayBuffer.
    napi_value arraybuffer;
    float* data = NULL;
    size_t byte_length = length*sizeof(float);
    status = napi_create_arraybuffer(env, byte_length, (void**)&data, &arraybuffer);
    if (status != napi_ok) { throwException(env, "Failed to create the arraybuffer."); return nullptr; }
    for (size_t i=0; i<length; i++) data[i] = wavData[i];

    //       -- Second, create the TypedArray.
    size_t byte_offset = 0;
    status = napi_create_typedarray(env, napi_float32_array, length, arraybuffer, byte_offset, &wavdataL);
    if (status != napi_ok) { throwException(env, "Failed to create the TypedArray: wavdataL."); return nullptr; }
  }
  napi_value wavdataR;
  if (buffer.size()>0)
  {
    std::vector<float> wavData = buffer[0];
    size_t length = wavData.size();
    //       -- First, create the ArrayBuffer.
    napi_value arraybuffer;
    float* data = NULL;
    size_t byte_length = length*sizeof(float);
    status = napi_create_arraybuffer(env, byte_length, (void**)&data, &arraybuffer);
    if (status != napi_ok) { throwException(env, "Failed to create the arraybuffer."); return nullptr; }
    for (size_t i=0; i<length; i++) data[i] = wavData[i];

    //       -- Second, create the TypedArray.
    size_t byte_offset = 0;
    status = napi_create_typedarray(env, napi_float32_array, length, arraybuffer, byte_offset, &wavdataR);
    if (status != napi_ok) { throwException(env, "Failed to create the TypedArray: wavdataR."); return nullptr; }
  }

  //printf("%s:%d\n", __FILE__, __LINE__);
  // Set the named property.
  status = napi_set_named_property(env, result, "samplerate", samplerate);
  if (status != napi_ok) { throwException(env, "Failed to set the result: samplerate."); return nullptr; }
  status = napi_set_named_property(env, result, "bitdepth", bitdepth);
  if (status != napi_ok) { throwException(env, "Failed to set the result: bitdepth."); return nullptr; }
  status = napi_set_named_property(env, result, "channels", channels);
  if (status != napi_ok) { throwException(env, "Failed to set the result: channels."); return nullptr; }
  status = napi_set_named_property(env, result, "wavdataL", wavdataL);
  if (status != napi_ok) { throwException(env, "Failed to set the result: wavdataL."); return nullptr; }
  status = napi_set_named_property(env, result, "wavdataR", wavdataR);
  if (status != napi_ok) { throwException(env, "Failed to set the result: wavdataR."); return nullptr; }

  status = napi_resolve_deferred(env, deferred, result);
  if (status != napi_ok) { throwException(env, "Failed to set the deferred result."); return nullptr; }

  // At this point the deferred has been freed, so we should assign NULL to it.
  deferred = NULL;

  return promise;
}

// Write wave data into the Audio File (Only support the WAV/PCM format.)
// argv[0]: the wave file name.
// argv[1]: the wave data buffer. vector<float>
// argv[2]: the sample rate.
// argv[3]: the bit depth.
napi_value saveAudio(napi_env env, napi_callback_info args)
{
  napi_value result;
  napi_deferred deferred;
  napi_value promise;

  napi_status status;

  // Create the promise.
  status = napi_create_promise(env, &deferred, &promise);
  if (status != napi_ok) { throwException(env, "Failed to create the promise object."); return nullptr; }

  // Parse the input arguments.
  size_t argc = 6;
  napi_value argv[6];
  status = napi_get_cb_info(env, args, &argc, argv, NULL, NULL);

  // -- Get the wave file name.
  char wavFileName[128];
  size_t lenFileName;
  status = napi_get_value_string_utf8(env, argv[0], wavFileName, 128, &lenFileName);
  if (status != napi_ok) return nullptr;
  // printf("wavFileName=%s\n", wavFileName);

  // -- Get the sample rate.
  int32_t sampleRate;
  status = napi_get_value_int32(env, argv[3], &sampleRate);
  if (status != napi_ok) return nullptr;
  // printf("sampleRate=%d\n", sampleRate);

  // -- Get the bit depth.
  int32_t bitDepth;
  status = napi_get_value_int32(env, argv[4], &bitDepth);
  if (status != napi_ok) return nullptr;
  // printf("bitDepth=%d\n", bitDepth);

  // -- Get the number of channels.
  int32_t nbChannels;
  status = napi_get_value_int32(env, argv[5], &nbChannels);
  if (status != napi_ok) return nullptr;

  // Set the Audio format
  AudioFile<float> audioFile;
  audioFile.setSampleRate(sampleRate);
  audioFile.setBitDepth(bitDepth);
  audioFile.setNumChannels(nbChannels);

  // -- Get the wave data buffer. (Left channel).
  float* dataL;
  napi_typedarray_type type;
  size_t length;
  napi_value arraybuffer;
  size_t byte_offset;
  status = napi_get_typedarray_info(env, argv[1], &type, &length, (void**) &dataL, &arraybuffer, &byte_offset);
  if (status != napi_ok) return nullptr;
  // -- Save the wave data buffer. (Left channel).
  std::vector<std::vector<float>> samples(nbChannels);
  std::vector<float> wavDataL(length); // We only use the first channel or at most the first two channels.
  for (size_t i=0; i<length; i++) wavDataL[i] = dataL[i];
  samples[0] = wavDataL;

  // If there are two channels.
  if (nbChannels == 2)
  {
    // -- Get the wave data buffer. (Right channel).
    float* dataR;
    status = napi_get_typedarray_info(env, argv[2], &type, &length, (void**) &dataR, &arraybuffer, &byte_offset);
    // napi_extended_error_info* error = new napi_extended_error_info;
    // napi_get_last_error_info(env, (const napi_extended_error_info**)&error);
    if (status != napi_ok) return nullptr;
    // -- Save the wave data buffer. (Right channel).
    std::vector<float> wavDataR(length); // We only use the first channel or at most the first two channels.
    for (size_t i=0; i<length; i++) wavDataR[i] = dataR[i];
    samples[1] = wavDataR;
  }

  audioFile.setAudioBuffer(samples);
  audioFile.save(wavFileName);

  result = argv[0];

  status = napi_resolve_deferred(env, deferred, result);
  if (status != napi_ok) { throwException(env, "Failed to set the deferred result."); return nullptr; }

  // At this point the deferred has been freed, so we should assign NULL to it.
  deferred = NULL;

  return promise;
}
