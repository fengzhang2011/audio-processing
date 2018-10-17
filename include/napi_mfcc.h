/*************************************************
 *
 * Compute the MFCC of a given audio data.
 *
 * Author: Feng Zhang (zhjinf@gmail.com)
 * Date: 2018-10-18
 * 
 * Copyright:
 *   See LICENSE.
 * 
 ************************************************/

#ifndef _NAPI_MFCC_INCLUDED_H_
#define _NAPI_MFCC_INCLUDED_H_

#include <node_api.h>

// Compute the MFCCs from a given mono-channel audio.
// arg[0]: wavdata (a single channel vector<float>)
// arg[1]: sample rate
// arg[2]: nbFilters, the number of Mel-frequency filters. Default: 40.
// arg[3]: lowerBound, the lower bound of Mel-frequency filters. Default: 300.
// arg[4]: upperBound, the upper bound of Mel-frequency filters. Default: 3500.
// arg[5]: msFrame, the frame length in milliseconds. Default: 40 ms.
// arg[6]: msStep, the shifting step across frames. Default: 20 ms.
// arg[7]: preEmphFactor, the factor to pre-emphasize the original signal. Default: 0.97.
napi_value mfcc(napi_env env, napi_callback_info args);

#endif // #ifndef _NAPI_MFCC_INCLUDED_H_
