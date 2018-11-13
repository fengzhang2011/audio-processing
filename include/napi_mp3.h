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

#ifndef _NAPI_MP3_INCLUDED_H_
#define _NAPI_MP3_INCLUDED_H_

#include <node_api.h>

napi_value mp32pcm(napi_env env, napi_callback_info args);

#endif // #ifndef _NAPI_MP3_INCLUDED_H_
