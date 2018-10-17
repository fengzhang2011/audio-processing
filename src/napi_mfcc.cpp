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

#include "mfcc.h"

#include "napi_mfcc.h"


// Compute the MFCCs from a given mono-channel audio.
// arg[0]: wavdata (a single channel vector<float>)
// arg[1]: sample rate
// arg[2]: nbFilters, the number of Mel-frequency filters. Default: 40.
// arg[3]: lowerBound, the lower bound of Mel-frequency filters. Default: 300.
// arg[4]: upperBound, the upper bound of Mel-frequency filters. Default: 3500.
// arg[5]: msFrame, the frame length in milliseconds. Default: 40 ms.
// arg[6]: msStep, the shifting step across frames. Default: 20 ms.
// arg[7]: preEmphFactor, the factor to pre-emphasize the original signal. Default: 0.97.
napi_value mfcc(napi_env env, napi_callback_info args)
{
  napi_status status;

  // Create the resulting object.
  napi_value result;
  status = napi_create_object(env, &result);
  if (status != napi_ok) return nullptr;

  // Parse the input arguments.
  size_t argc = 8;
  napi_value argv[8];
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

  // -- Get the nbFilters.
  int32_t nbFilters = 40;
  status = napi_get_value_int32(env, argv[2], &nbFilters);
  if (status != napi_ok) nbFilters = 40;

  // -- Get the lower bound.
  double lowerBound = 300;
  status = napi_get_value_double(env, argv[3], &lowerBound);
  if (status != napi_ok) lowerBound = 0.97;

  // -- Get the upper bound.
  double upperBound = 3500;
  status = napi_get_value_double(env, argv[4], &upperBound);
  if (status != napi_ok) upperBound = 0.97;

  // -- Get the msFrame.
  int32_t msFrame = 40;
  status = napi_get_value_int32(env, argv[5], &msFrame);
  if (status != napi_ok) msFrame = 40;

  // -- Get the msStep.
  int32_t msStep = 20;
  status = napi_get_value_int32(env, argv[6], &msStep);
  if (status != napi_ok) msStep = 20;

  // -- Get the preEmphFactor.
  double preEmphFactor = 0.97;
  status = napi_get_value_double(env, argv[7], &preEmphFactor);
  if (status != napi_ok) preEmphFactor = 0.97;


  // Prepare the data for MFCCs.
  int nbFrames = 0;
  int frameLength = 0;
  int frameStep = 0;

  float* signal = MFCC::padScaleOriginalWaveData(wavData, sampleRate, msFrame, msStep, nbFrames, frameLength, frameStep);

  MFCC m(frameLength, sampleRate, nbFilters, lowerBound, upperBound, preEmphFactor);

  // Prepare the buffers
  size_t byte_length = nbFilters * nbFrames * sizeof(float);
  byte_offset = 0;
  // -- First, create the ArrayBuffer to store MFCCs.
  napi_value abMFCCs;
  float* dataMFCCs = NULL;
  status = napi_create_arraybuffer(env, byte_length, (void**)&dataMFCCs, &abMFCCs);
  if (status != napi_ok) return nullptr;
  // -- Second, create the ArrayBuffer to store Mel bank features.
  napi_value abMelBankFeatures;
  float* dataMelBankFeatures = NULL;
  status = napi_create_arraybuffer(env, byte_length, (void**)&dataMelBankFeatures, &abMelBankFeatures);
  if (status != napi_ok) return nullptr;

  // Compute the MFCCs.
  for(int i=0; i<nbFrames; i++)
  {
    m.mfcc( signal + i * frameStep );

    float* mfccs = m.getMFCCArray(0, nbFilters-1, false, 0);
    float* melBankFeatures = m.getMelBankFeatureArray(0, nbFilters-1, false);

    int offset = i * nbFilters;

    memcpy(dataMFCCs + offset, mfccs, nbFilters * sizeof(float));
    memcpy(dataMelBankFeatures + offset, melBankFeatures, nbFilters * sizeof(float));
  }


  // Set the return value.
  // -- create the int32 to hold the number of frames.
  napi_value nv_nbFrames;
  status = napi_create_int32(env, nbFrames, &nv_nbFrames);
  if (status != napi_ok) return nullptr;
  int bufferSize = nbFrames * nbFilters;
  // -- First, create the TypedArray to store MFCCs.
  napi_value array_data_MFCCs;
  status = napi_create_typedarray(env, napi_float32_array, bufferSize, abMFCCs, byte_offset, &array_data_MFCCs);
  if (status != napi_ok) return nullptr;
  // -- Second, create the TypedArray to store Mel bank features.
  napi_value array_data_melBankFeatures;
  status = napi_create_typedarray(env, napi_float32_array, bufferSize, abMelBankFeatures, byte_offset, &array_data_melBankFeatures);
  if (status != napi_ok) return nullptr;

  // Set the named property.
  status = napi_set_named_property(env, result, "frames", nv_nbFrames);
  if (status != napi_ok) return nullptr;
  status = napi_set_named_property(env, result, "mfccs", array_data_MFCCs);
  if (status != napi_ok) return nullptr;
  status = napi_set_named_property(env, result, "melbfs", array_data_melBankFeatures);
  if (status != napi_ok) return nullptr;

  return result;
}

