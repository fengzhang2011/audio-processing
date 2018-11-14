/*************************************************
 *
 * Denoise.
 *
 * Author: Feng Zhang (zhjinf@gmail.com)
 * Date: 2018-11-13
 * 
 * Copyright:
 *   See LICENSE.
 * 
 ************************************************/

#ifndef _INCLUDE_DENOISE_H_
#define _INCLUDE_DENOISE_H_

#include <vector>

std::vector<float> weinerDenoiseTSNR(const std::vector<float>& noisySpeech, int sampleRate, int nbInitialSilentFrames);

#endif // #ifndef _INCLUDE_DENOISE_H_
