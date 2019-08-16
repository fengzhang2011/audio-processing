/*************************************************
 *
 * Decode the AMR NB/WB data.
 *
 * Author: Feng Zhang (zhjinf@gmail.com)
 * Date: 2018-10-22
 * 
 * Copyright:
 *   See LICENSE.
 * 
 ************************************************/

#include <vector>

#include "amr.h"

#include "napi_amr.h"
#include "napi_common.h"


#include <stdio.h>


// Decode the AMR/NB/WB data.
// arg[0]: amrdata  (uint8array)
// return: pcmdata  (float32array)
napi_value amr2pcm(napi_env env, napi_callback_info args)
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

  // Convert AMR data
  AMR_TYPE amr_type = getAMRType((char*)dataptr, length);
  int samples = getSampleCount((char*)dataptr, length, amr_type);
  short* pcm = amr2pcm((char*)dataptr, length);
  if (pcm == NULL) return nullptr;

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
  int sampleRate = (amr_type==AMR_NB) ? 8000 : ( (amr_type==AMR_WB?16000:0) );
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


// Encode the PCM data to the AMR/NB/WB data.
// arg[0]: pcmdata  (float32array)
// arg[1]: sample rate
// arg[2]: mode: (0: 4.75k, 1: 5.15k, 2: 5.90k, 3: 6.70k, 4: 7.40k, 5: 7.95k, 6: 10.2k, 7: 12.2k)
// return: arg[0]: amrdata  (uint8array)
napi_value pcm2amr(napi_env env, napi_callback_info args)
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

  // -- Get the data buffer.
  // -- Get the wave data buffer. (Only accepts one channel).
  float* data;
  napi_typedarray_type type;
  size_t length;
  napi_value arraybuffer;
  size_t byte_offset;
  status = napi_get_typedarray_info(env, argv[0], &type, &length, (void**) &data, &arraybuffer, &byte_offset);
  if (status != napi_ok) return nullptr;
  // -- Save the wave data buffer. (Only accepts one channel).
  short* pcmData = new short[length]; // We only use the first channel or at most the first two channels.
  for (size_t i=0; i<length; i++) pcmData[i] = data[i]*32768;

  // -- Get the sample rate.
  int32_t sampleRate;
  status = napi_get_value_int32(env, argv[1], &sampleRate);
  if (status != napi_ok) return nullptr;

  // -- Get the AMR rate mode.
  int32_t mode;
  status = napi_get_value_int32(env, argv[2], &mode);
  if (status != napi_ok) return nullptr;

  // Convert PCM data
  int byte_length = 0;
  char* amr = pcm2amr(pcmData, length, sampleRate, &byte_length, mode);
  if (amr == NULL) return nullptr;
  delete []pcmData;

  // Set the return value.
  byte_offset = 0;
  // -- First, create the ArrayBuffer.
  uint8_t* amrdata = NULL;
  status = napi_create_arraybuffer(env, byte_length, (void**)&amrdata, &arraybuffer);
  if (status != napi_ok) return nullptr;
  for (size_t i=0; i<(size_t)byte_length; i++) amrdata[i] = amr[i];
  delete [] amr;
  // -- Second, create the TypedArray.
  napi_value amrarray;
  status = napi_create_typedarray(env, napi_uint8_array, byte_length, arraybuffer, byte_offset, &amrarray);
  if (status != napi_ok) return nullptr;

  // Set the named property.
  status = napi_set_named_property(env, result, "data", amrarray);
  if (status != napi_ok) return nullptr;

  status = napi_resolve_deferred(env, deferred, result);
  if (status != napi_ok) { throwException(env, "Failed to set the deferred result."); return nullptr; }

  // At this point the deferred has been freed, so we should assign NULL to it.
  deferred = NULL;

  return promise;
}

// Encode the PCM data from the WAVE buffer to the AMR/NB/WB data.
// arg[0]: wavbuffer  (buffer)
// arg[1]: mode: (0: 4.75k, 1: 5.15k, 2: 5.90k, 3: 6.70k, 4: 7.40k, 5: 7.95k, 6: 10.2k, 7: 12.2k)
// return: arg[0]: amrdata  (uint8array)
napi_value wav2amr(napi_env env, napi_callback_info args)
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
  size_t argc = 2;
  napi_value argv[2];
  status = napi_get_cb_info(env, args, &argc, argv, NULL, NULL);

  // -- Get the data buffer.
  void* data;
  size_t length;
  status = napi_get_buffer_info(env, argv[0], &data, &length);
  if (status != napi_ok) return nullptr;
  // -- Save the wave data buffer. (Only accepts one channel).
  char* wavData = new char[length];
  memcpy(wavData, data, length);

  // -- Get the AMR rate mode.
  int32_t mode;
  status = napi_get_value_int32(env, argv[1], &mode);
  if (status != napi_ok) return nullptr;

  // Convert PCM data
  int byte_length = 0;
  char* amr = wav2amr(wavData, length, &byte_length, mode);
  if (amr == NULL) return nullptr;
  delete []wavData;

  // Set the return value.
  size_t byte_offset = 0;
  // -- First, create the ArrayBuffer.
  napi_value arraybuffer;
  uint8_t* amrdata = NULL;
  status = napi_create_arraybuffer(env, byte_length, (void**)&amrdata, &arraybuffer);
  if (status != napi_ok) return nullptr;
  for (size_t i=0; i<(size_t)byte_length; i++) amrdata[i] = amr[i];
  delete [] amr;
  // -- Second, create the TypedArray.
  napi_value amrarray;
  status = napi_create_typedarray(env, napi_uint8_array, byte_length, arraybuffer, byte_offset, &amrarray);
  if (status != napi_ok) return nullptr;

  // Set the named property.
  status = napi_set_named_property(env, result, "data", amrarray);
  if (status != napi_ok) return nullptr;

  status = napi_resolve_deferred(env, deferred, result);
  if (status != napi_ok) { throwException(env, "Failed to set the deferred result."); return nullptr; }

  // At this point the deferred has been freed, so we should assign NULL to it.
  deferred = NULL;

  return promise;
}

//napi_value mp32amr(napi_env env, napi_callback_info args)
//{
//}
