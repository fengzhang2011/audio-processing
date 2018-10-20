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
 * Credit:
 *   This version is modified based on the code by gansidui.
 *   Its git repository URL is: https://github.com/gansidui/pcm_amr_codec.
 *
 ************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <libgen.h>
#include <sys/types.h>

#include <interf_dec.h>
#include <interf_enc.h>
#include <dec_if.h>

#include "bs_read.h"
#include "amr.h"


#define AMRNB_MAX_FRAME_TYPE  (8)    // SID Packet
#define AMRWB_MAX_FRAME_TYPE  (9)    // SID Packet
#define AMRNB_NUM_SAMPLES   (160)
#define AMRWB_NUM_SAMPLES   (320)
#define AMRNB_OUT_MAX_SIZE  (32)
#define AMRWB_OUT_MAX_SIZE  (61)
#define AMR_OUT_MAX_SIZE (61)

static const int amrnb_frame_sizes[] = {12, 13, 15, 17, 19, 20, 26, 31, 5, 0 };
static const int amrwb_frame_sizes[] = {17, 23, 32, 36, 40, 46, 50, 58, 60, 5, 0 };

static const int b_octet_align = 1;


#define tocGetF(toc) ((toc) >> 7)
#define tocGetIndex(toc)  ((toc>>3) & 0xf)
static int tocListCheck(uint8_t *tl, size_t buflen) { size_t s = 1; while (tocGetF(*tl)) { tl++; s++; if (s > buflen) return -1; } return s; }
static int getFrameType(char* data) { return ((*(int*) data) & 0x78) >> 3; }
static int getFrameBytes(int frameType, enum AMR_TYPE type) { return (type==AMR_NB) ? amrnb_frame_sizes[frameType] + 1 : amrwb_frame_sizes[frameType] + 1; } // type == 0: AMR-NB,  type == 1: AMR-WB
static int getFrameBytesDirect(char* data, enum AMR_TYPE type) { int frameType = getFrameType(data); return getFrameBytes(frameType, type); }
static int getFrameCount(char* data, int size, enum AMR_TYPE type) { int count = 0; int i = (type==AMR_NB) ? strlen(AMRNB_HEADER): strlen(AMRWB_HEADER); while(i < size) { i += getFrameBytesDirect(data + i, type); count ++; } return count; }
static int getSampleCount(char* data, int size, enum AMR_TYPE type) { int nbFrames = getFrameCount(data, size, type); return (type==AMR_NB) ? nbFrames * AMRNB_NUM_SAMPLES : nbFrames * AMRWB_NUM_SAMPLES; }


enum AMR_TYPE getAMRType(char* data, int size)
{
  return (0==strncmp(data, AMRNB_HEADER, strlen(AMRNB_HEADER)))
         ? AMR_NB
         : ( (0==strncmp(data, AMRWB_HEADER, strlen(AMRWB_HEADER)))
           ? AMR_WB
           : AMR_UNKNOWN);
}

static int amrDecodeFrame(char *data, int nSize, short* pcmDataCurrentFrame, void* amrDecoder, enum AMR_TYPE type)
{
  if(nSize < 2) { printf("Too short packet\n"); return -1; }

  bs_t* payload = bs_new((uint8_t *)data, nSize);
  if(payload == NULL) { return -2; }

  int amrMaxFrameType = (type==AMR_NB) ? AMRNB_MAX_FRAME_TYPE : AMRWB_MAX_FRAME_TYPE;

  uint8_t tocs[20] = {0,};
  int nTocLen = 0, nFrameData = 0, nFbit = 1, nFTbits = 0, nQbit = 0;
  while(nFbit == 1)
  {
    // 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7
    // +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
    // |1|  FT   |Q|1|  FT   |Q|0|  FT   |Q|
    // +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
    nFbit = bs_read_u(payload, 1);
    nFTbits = bs_read_u(payload, 4);
    if(nFTbits > amrMaxFrameType) { printf("%s, Bad amr toc, index=%i (MAX=%d)\n", __func__, nFTbits, amrMaxFrameType); break; }
    nFrameData += (getFrameBytes(nFTbits, type) - 1);
    nQbit = bs_read_u(payload, 1);
    tocs[nTocLen++] = ((nFbit << 7) | (nFTbits << 3) | (nQbit << 2)) & 0xFC;
    if(b_octet_align == 1) { bs_read_u(payload, 2); } // two padding bits.
  }

  if ( -1 == tocListCheck(tocs, nSize) ) { printf("Bad AMR toc list\n"); bs_free(payload); return -3; }
  if((nFrameData) != bs_bytes_left(payload)) { printf("%s, invalid data mismatch, FrameData=%d, bytes_left=%d\n", __func__, nFrameData, bs_bytes_left(payload)); }

  uint8_t tmp[AMR_OUT_MAX_SIZE];
  for(int i=0; i<nTocLen; i++)
  {
    memset(tmp, 0, sizeof(tmp));
    tmp[0] = tocs[i];
    int index = tocGetIndex(tocs[i]);
    if (index > amrMaxFrameType) { printf("Bad amr toc, index=%i\n", index); break; }
    bs_read_bytes_ex(payload, &tmp[1], (getFrameBytes(index, type) - 1));

    (type==AMR_NB) ? Decoder_Interface_Decode(amrDecoder, tmp, pcmDataCurrentFrame, 0) : D_IF_decode(amrDecoder, tmp, pcmDataCurrentFrame, 0);

  }
  bs_free(payload);

  return 0;
}

short* amrConvert(char* data, int size)
{
  enum AMR_TYPE type = getAMRType(data, size);

  int samples = getSampleCount(data, size, type);
  short* pcmData = (short*) malloc( samples * sizeof(short) );
  short* pcmDataCurrentFrame = pcmData;

  void* amrDecoder = (type==AMR_NB) ? Decoder_Interface_init() : D_IF_init();

  int i = (type==AMR_NB) ? strlen(AMRNB_HEADER) : strlen(AMRWB_HEADER);
  while(i < size)
  {
    int frameBytes = getFrameBytesDirect(data + i, type);
    amrDecodeFrame(data + i, frameBytes, pcmDataCurrentFrame, amrDecoder, type);
    i += frameBytes;
    pcmDataCurrentFrame += ( (type==AMR_NB) ? AMRNB_NUM_SAMPLES : AMRWB_NUM_SAMPLES );
  }

  (type==AMR_NB) ? Decoder_Interface_exit(amrDecoder) : D_IF_exit(amrDecoder);
  amrDecoder = NULL;

  return pcmData;
}

