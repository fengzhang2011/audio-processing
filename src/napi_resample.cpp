/*************************************************
 *
 * Resample the audio data.
 *
 * Author: Feng Zhang (zhjinf@gmail.com)
 * Date: 2019-03-23
 * 
 * Copyright:
 *   See LICENSE.
 * 
 ************************************************/

#include <stdio.h>
#include <vector>

#include "samplerate.h"

#include "napi_resample.h"
#include "napi_common.h"



// Resample the audio data.
// arg[0]: pcmdata  (float32array)
// arg[1]: samplerate_in (int)
// arg[2]: samplerate_out (int)
// return: resampled  (float32array)
napi_value resample(napi_env env, napi_callback_info args)
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
  if (status != napi_ok) return nullptr;

  // Parse the input arguments.
  size_t argc = 3;
  napi_value argv[3];
  status = napi_get_cb_info(env, args, &argc, argv, NULL, NULL);

  // -- Get the wave data buffer. (Only accepts one channel).
  float* pcmdata;
  napi_typedarray_type type;
  size_t length;
  napi_value arraybuffer;
  size_t byte_offset;
  status = napi_get_typedarray_info(env, argv[0], &type, &length, (void**) &pcmdata, &arraybuffer, &byte_offset);
  if (status != napi_ok) return nullptr;

  // -- Get the original sample rate.
  int32_t originalSampleRate;
  status = napi_get_value_int32(env, argv[1], &originalSampleRate);
  if (status != napi_ok) return nullptr;

  // -- Get the target sample rate.
  int32_t targetSampleRate;
  status = napi_get_value_int32(env, argv[2], &targetSampleRate);
  if (status != napi_ok) return nullptr;

  // Resample the audio data.
  // Preparing the input and output data
  long input_frames = length;
  double src_ratio = 1.0*targetSampleRate/originalSampleRate;
  long output_frames = (int)(length*src_ratio);
  // Create the output data buffer to be returned to nodejs.
  size_t byte_length = output_frames*sizeof(float);
  float* resampled = NULL;
  status = napi_create_arraybuffer(env, byte_length, (void**)&resampled, &arraybuffer);
  if (status != napi_ok) return nullptr;

  SRC_DATA* src_data = new SRC_DATA();
  if (!src_data) return nullptr;
  src_data->data_in = pcmdata;
  src_data->data_out = resampled;
  src_data->input_frames = input_frames;
  src_data->output_frames = output_frames;
  src_data->src_ratio = src_ratio;

  int converter = SRC_SINC_FASTEST;
  int channels = 1;
  src_simple(src_data, converter, channels);//  (SRC_DATA *data, int converter_type, int channels);

  // Set the return value.
  byte_offset = 0;
  napi_value resampledarray;
  status = napi_create_typedarray(env, napi_float32_array, output_frames, arraybuffer, byte_offset, &resampledarray);
  if (status != napi_ok) return nullptr;

  // Set the sample rate;
  napi_value samplerate;
  status = napi_create_int32(env, targetSampleRate, &samplerate);
  if (status != napi_ok) return nullptr;

  // Set the bitdepth
  int bitDepth = 16;
  napi_value bitdepth;
  status = napi_create_int32(env, bitDepth, &bitdepth);
  if (status != napi_ok) return nullptr;

  // Set the named property.
  status = napi_set_named_property(env, result, "wavdata", resampledarray);
  if (status != napi_ok) return nullptr;
  status = napi_set_named_property(env, result, "bitdepth", bitdepth);
  if (status != napi_ok) return nullptr;
  status = napi_set_named_property(env, result, "samplerate", samplerate);
  if (status != napi_ok) return nullptr;

  status = napi_resolve_deferred(env, deferred, result);
  if (status != napi_ok) { throwException(env, "Failed to set the deferred result."); return nullptr; }

  // At this point the deferred has been freed, so we should assign NULL to it.
  deferred = NULL;

  delete src_data;

  return promise;
}
