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

#ifndef _NAPI_PITCH_INCLUDED_H_
#define _NAPI_PITCH_INCLUDED_H_

#include <node_api.h>

// Detect the pitch from a given mono-channel audio.
// arg[0]: wavdata (a single channel vector<float>)
// arg[1]: sample rate
// arg[2]: method  (available choices: acorr, yin, mpm, goertzel, dft)
napi_value detectPitch(napi_env env, napi_callback_info args);

#endif // #ifndef _NAPI_PITCH_INCLUDED_H_
