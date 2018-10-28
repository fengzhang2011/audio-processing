/*************************************************
 *
 * Common tools for the NAPI interface.
 *
 * Author: Feng Zhang (zhjinf@gmail.com)
 * Date: 2018-10-28
 * 
 * Copyright:
 *   See LICENSE.
 * 
 ************************************************/

#ifndef _NAPI_COMMON_INCLUDED_H_
#define _NAPI_COMMON_INCLUDED_H_

#include <node_api.h>

void throwException(napi_env env, const char* error);

#endif // #ifndef _NAPI_COMMON_INCLUDED_H_
