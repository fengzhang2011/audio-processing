/*************************************************
 *
 * Load/Save audio data from/to file.
 *
 * Author: Feng Zhang (zhjinf@gmail.com)
 * Date: 2018-10-13
 * 
 * Copyright:
 *   See LICENSE.
 * 
 ************************************************/

#ifndef _NAPI_AUDIOFILE_INCLUDED_H_
#define _NAPI_AUDIOFILE_INCLUDED_H_

#include <node_api.h>

// Read the Audio File (Only WAV/PCM and AIFF are supported.)
// argv[0]: the wave file name.
napi_value readAudio(napi_env env, napi_callback_info args);

// Write wave data into the Audio File (Only support the WAV/PCM format.)
// argv[0]: the wave file name.
// argv[1]: the wave data buffer (Left channel). vector<float>
// argv[2]: the wave data buffer (Right channel, if exists). vector<float>
// argv[3]: the sample rate.
// argv[4]: the bit depth.
// argv[5]: the number of channels.
napi_value saveAudio(napi_env env, napi_callback_info args);

#endif // #ifndef _NAPI_AUDIOFILE_INCLUDED_H_
