/*************************************************
 *
 * AMR Decoder to PCM (both AMR-NB and AMR-WB).
 *
 * Author: Feng Zhang (zhjinf@gmail.com)
 * Date: 2018-10-20
 *
 * Copyright:
 *   See LICENSE.
 *
 ************************************************/

#ifndef _INCLUDE_AMR_H_
#define _INCLUDE_AMR_H_

#include <string.h>


#define AMRNB_HEADER "#!AMR\n"
#define AMRWB_HEADER "#!AMR-WB\n"


enum AMR_TYPE
{
  AMR_NB,
  AMR_WB,
  AMR_UNKNOWN
};


enum AMR_TYPE getAMRType(char* data, int size);
int getSampleCount(char* data, int size, enum AMR_TYPE type);

short* amr2pcm(char* data, int size);
char* pcm2amr(short* data, int size, int sampleRate, int* out_size, int mode);
char* wav2amr(char* data, int size, int* out_size, int mode);
char* mp32amr(short* data, int size, int* out_size, int mode);
int amr_remove_silence(char* data, int size, float threshold, char** pOutput, int* szOutput);

#endif // #ifndef _INCLUDE_AMR_H_
