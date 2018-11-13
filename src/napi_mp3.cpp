/*************************************************
 *
 * Decode the MP3 data.
 *
 * Author: Feng Zhang (zhjinf@gmail.com)
 * Date: 2018-11-13
 * 
 * Copyright:
 *   See LICENSE.
 * 
 ************************************************/

#include <stdio.h>
#include <vector>

#include "minimp3.h"

#include "napi_mp3.h"
#include "napi_common.h"



// Decode the MP3 data.
// arg[0]: mp3data  (uint8array)
// return: pcmdata  (float32array)
napi_value mp32pcm(napi_env env, napi_callback_info args)
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
  size_t argc = 1;
  napi_value argv[1];
  status = napi_get_cb_info(env, args, &argc, argv, NULL, NULL);

  // -- Get the data buffer.
  uint8_t* dataptr;
  napi_typedarray_type type;
  size_t length;
  napi_value arraybuffer;
  size_t byte_offset;
  status = napi_get_typedarray_info(env, argv[0], &type, &length, (void**) &dataptr, &arraybuffer, &byte_offset);
  if (status != napi_ok) return nullptr;

  // Convert MP3 data
  mp3dec_t mp3d;
  mp3dec_file_info_t info;
  mp3dec_load_buf(&mp3d, (const uint8_t*)dataptr, length, &info, 0, 0);
  int samples = info.samples;
  if(samples == 0 ) return nullptr;
  short* pcm = info.buffer;
  //int16_t *buffer = malloc(info.samples*sizeof(int16_t));
  //mp3dec_f32_to_s16(info.buffer, buffer, info.samples);
  //free(info.buffer);

  // Set the return value.
  size_t byte_length = samples*sizeof(float);
  byte_offset = 0;
  // -- First, create the ArrayBuffer.
  float* pcmdata = NULL;
  status = napi_create_arraybuffer(env, byte_length, (void**)&pcmdata, &arraybuffer);
  if (status != napi_ok) return nullptr;
  for (int i=0; i<samples; i++) pcmdata[i] = 1.0 * ((int) pcm[i]) / 32768; // NOTE: 1. Must convert 'short' to 'int'. 2. Must divide the value by 32768. Otherwise, no sound could be played, although the values are there.
  delete [] pcm;
  // -- Second, create the TypedArray.
  napi_value pcmarray;
  status = napi_create_typedarray(env, napi_float32_array, samples, arraybuffer, byte_offset, &pcmarray);
  if (status != napi_ok) return nullptr;

  // Set the sample rate;
  int sampleRate = info.hz;
  napi_value samplerate;
  status = napi_create_int32(env, sampleRate, &samplerate);
  if (status != napi_ok) return nullptr;

  // Set the bitdepth
  int bitDepth = 16;
  napi_value bitdepth;
  status = napi_create_int32(env, bitDepth, &bitdepth);
  if (status != napi_ok) return nullptr;

  // Set the named property.
  status = napi_set_named_property(env, result, "pcm", pcmarray);
  if (status != napi_ok) return nullptr;
  status = napi_set_named_property(env, result, "bitdepth", bitdepth);
  if (status != napi_ok) return nullptr;
  status = napi_set_named_property(env, result, "samplerate", samplerate);
  if (status != napi_ok) return nullptr;

  status = napi_resolve_deferred(env, deferred, result);
  if (status != napi_ok) { throwException(env, "Failed to set the deferred result."); return nullptr; }

  // At this point the deferred has been freed, so we should assign NULL to it.
  deferred = NULL;

  return promise;
}
