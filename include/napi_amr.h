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

#ifndef _NAPI_AMR_INCLUDED_H_
#define _NAPI_AMR_INCLUDED_H_

#include <node_api.h>

napi_value amr2pcm(napi_env env, napi_callback_info args);
napi_value pcm2amr(napi_env env, napi_callback_info args);

#endif // #ifndef _NAPI_AMR_INCLUDED_H_
