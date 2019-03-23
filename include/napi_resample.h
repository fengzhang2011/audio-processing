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

#ifndef _NAPI_RESAMPLE_INCLUDED_H_
#define _NAPI_RESAMPLE_INCLUDED_H_

#include <node_api.h>

napi_value resample(napi_env env, napi_callback_info args);

#endif // #ifndef _NAPI_RESAMPLE_INCLUDED_H_
