/*************************************************
 *
 * Export c++ methods into a NodeJS module.
 *
 * Author: Feng Zhang (zhjinf@gmail.com)
 * Date: 2018-10-13
 * 
 * Copyright:
 *   See LICENSE.
 * 
 ************************************************/

#include <node_api.h>
#include "napi_audiofile.h"
#include "napi_ampfreq.h"
#include "napi_fft.h"
#include "napi_mfcc.h"
#include "napi_pitch.h"
#include "napi_amr.h"
#include "napi_mp3.h"


napi_value Method(napi_env env, napi_callback_info args) {
  napi_value greeting;
  napi_status status;

  status = napi_create_string_utf8(env, "hello", NAPI_AUTO_LENGTH, &greeting);
  if (status != napi_ok) return nullptr;
  return greeting;
}

napi_value init(napi_env env, napi_value exports) {

  napi_status status;
  napi_value fn;

  // 'Export' the 'hello' function.
  status = napi_create_function(env, nullptr, 0, Method, nullptr, &fn);
  if (status != napi_ok) return nullptr;
  status = napi_set_named_property(env, exports, "hello", fn);
  if (status != napi_ok) return nullptr;

  // 'Export' the 'readAudio' function.
  status = napi_create_function(env, nullptr, 0, readAudio, nullptr, &fn);
  if (status != napi_ok) return nullptr;
  status = napi_set_named_property(env, exports, "readAudio", fn);
  if (status != napi_ok) return nullptr;

  // 'Export' the 'saveAudio' function.
  status = napi_create_function(env, nullptr, 0, saveAudio, nullptr, &fn);
  if (status != napi_ok) return nullptr;
  status = napi_set_named_property(env, exports, "saveAudio", fn);
  if (status != napi_ok) return nullptr;

  // 'Export' the 'detectPitch' function.
  status = napi_create_function(env, nullptr, 0, detectPitch, nullptr, &fn);
  if (status != napi_ok) return nullptr;
  status = napi_set_named_property(env, exports, "detectPitch", fn);
  if (status != napi_ok) return nullptr;

  // 'Export' the 'ampfreq' function.
  status = napi_create_function(env, nullptr, 0, ampfreq, nullptr, &fn);
  if (status != napi_ok) return nullptr;
  status = napi_set_named_property(env, exports, "ampfreq", fn);
  if (status != napi_ok) return nullptr;

  // 'Export' the 'fft' function.
  status = napi_create_function(env, nullptr, 0, fft, nullptr, &fn);
  if (status != napi_ok) return nullptr;
  status = napi_set_named_property(env, exports, "fft", fn);
  if (status != napi_ok) return nullptr;

  // 'Export' the 'ifft' function.
  status = napi_create_function(env, nullptr, 0, ifft, nullptr, &fn);
  if (status != napi_ok) return nullptr;
  status = napi_set_named_property(env, exports, "ifft", fn);
  if (status != napi_ok) return nullptr;

  // 'Export' the 'mfcc' function.
  status = napi_create_function(env, nullptr, 0, mfcc, nullptr, &fn);
  if (status != napi_ok) return nullptr;
  status = napi_set_named_property(env, exports, "mfcc", fn);
  if (status != napi_ok) return nullptr;

  // 'Export' the 'amr2pcm' function.
  status = napi_create_function(env, nullptr, 0, amr2pcm, nullptr, &fn);
  if (status != napi_ok) return nullptr;
  status = napi_set_named_property(env, exports, "amr2pcm", fn);
  if (status != napi_ok) return nullptr;

  // 'Export' the 'mp32pcm' function.
  status = napi_create_function(env, nullptr, 0, mp32pcm, nullptr, &fn);
  if (status != napi_ok) return nullptr;
  status = napi_set_named_property(env, exports, "mp32pcm", fn);
  if (status != napi_ok) return nullptr;

  return exports;
}

NAPI_MODULE(NODE_GYP_MODULE_NAME, init)
