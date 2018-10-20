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

short* amrConvert(char* data, int size);


#endif // #ifndef _INCLUDE_AMR_H_
