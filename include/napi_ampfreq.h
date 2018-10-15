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

#ifndef _NAPI_AMPFREQ_INCLUDED_H_
#define _NAPI_AMPFREQ_INCLUDED_H_

#include <node_api.h>

// Compute the Amplifier-Frequency
// arg[0]: wavdata
// arg[1]: sample rate
napi_value ampfreq(napi_env env, napi_callback_info args);

#endif // #ifndef _NAPI_AMPFREQ_INCLUDED_H_
