/*************************************************
 *
 * Do the FFT/IFFT on the data.
 *
 * Author: Feng Zhang (zhjinf@gmail.com)
 * Date: 2018-10-13
 * 
 * Copyright:
 *   See LICENSE.
 * 
 ************************************************/

#ifndef _NAPI_FFT_INCLUDED_H_
#define _NAPI_FFT_INCLUDED_H_

#include <node_api.h>

// FFT
napi_value fft(napi_env env, napi_callback_info args);

// Inversed FFT
napi_value ifft(napi_env env, napi_callback_info args);

#endif // #ifndef _NAPI_FFT_INCLUDED_H_
