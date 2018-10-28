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


#include "napi_common.h"


void throwException(napi_env env, const char* error)
{
  char code[256];
  sprintf(code, "%s:%d", __FILE__, __LINE__);
  napi_throw_error(env, code, error);
}

