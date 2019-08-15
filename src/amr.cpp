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

#include "bs.h"
#include "amr.h"
#include "samplerate.h"
#include "minimp3.h"


#define AMRNB_MAX_FRAME_TYPE  (8)    // SID Packet
#define AMRWB_MAX_FRAME_TYPE  (9)    // SID Packet
#define AMRNB_NUM_SAMPLES   (160)
#define AMRWB_NUM_SAMPLES   (320)
#define AMRNB_OUT_MAX_SIZE  (32)
#define AMRWB_OUT_MAX_SIZE  (61)
#define AMR_OUT_MAX_SIZE (61)

static const int amrnb_frame_sizes[] = {12, 13, 15, 17, 19, 20, 26, 31,  5, 0, 0, 0, 0, 0, 0, 0 };
static const int amrwb_frame_sizes[] = {17, 23, 32, 36, 40, 46, 50, 58, 60, 5, 0, 0, 0, 0, 0, 0 };

static const int b_octet_align = 1;


#define tocGetF(toc) ((toc) >> 7)
#define tocGetIndex(toc)  ((toc>>3) & 0xf)
static int tocListCheck(uint8_t *tl, size_t buflen) { size_t s = 1; while (tocGetF(*tl)) { tl++; s++; if (s > buflen) return -1; } return s; }
static int getFrameType(char* data) { return ((*(int*) data) & 0x78) >> 3; }
static int getFrameBytes(int frameType, enum AMR_TYPE type) { return (type==AMR_NB) ? amrnb_frame_sizes[frameType] + 1 : amrwb_frame_sizes[frameType] + 1; } // type == 0: AMR-NB,  type == 1: AMR-WB
static int getFrameBytesDirect(char* data, enum AMR_TYPE type) { int frameType = getFrameType(data); return getFrameBytes(frameType, type); }
static int getFrameCount(char* data, int size, enum AMR_TYPE type) { int count = 0; int i = (type==AMR_NB) ? strlen(AMRNB_HEADER): strlen(AMRWB_HEADER); while(i < size) { i += getFrameBytesDirect(data + i, type); count ++; } return count; }


enum AMR_TYPE getAMRType(char* data, int size)
{
  return (0==strncmp(data, AMRNB_HEADER, strlen(AMRNB_HEADER)))
         ? AMR_NB
         : ( (0==strncmp(data, AMRWB_HEADER, strlen(AMRWB_HEADER)))
           ? AMR_WB
           : AMR_UNKNOWN);
}

int getSampleCount(char* data, int size, enum AMR_TYPE type)
{
  int nbFrames = getFrameCount(data, size, type);
  return (type==AMR_NB) ? nbFrames * AMRNB_NUM_SAMPLES : nbFrames * AMRWB_NUM_SAMPLES;
}

static int amrDecodeFrame(char *data, int nSize, short* pcmDataCurrentFrame, void* amrDecoder, enum AMR_TYPE type)
{
  if(nSize < 2) { printf("Too short packet\n"); return -1; } // it means that the framesize is 0, needs to abort.

  hbs_t* payload = bs_new((uint8_t *)data, nSize);
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

short* amr2pcm(char* data, int size)
{
  enum AMR_TYPE type = getAMRType(data, size);
  if ( type == AMR_UNKNOWN ) return NULL;

  int samples = getSampleCount(data, size, type);
  short* pcmData = (short*) malloc( samples * sizeof(short) );
  memset(pcmData, 0, samples * sizeof(short));
  short* pcmDataCurrentFrame = pcmData;

  void* amrDecoder = (type==AMR_NB) ? Decoder_Interface_init() : D_IF_init();

  int i = (type==AMR_NB) ? strlen(AMRNB_HEADER) : strlen(AMRWB_HEADER);
  while(i < size)
  {
    int frameBytes = getFrameBytesDirect(data + i, type);
    int rc = amrDecodeFrame(data + i, frameBytes, pcmDataCurrentFrame, amrDecoder, type);
    if ( rc < 0 ) break; // there is something wrong with the data, needs to abort.
    i += frameBytes;
    pcmDataCurrentFrame += ( (type==AMR_NB) ? AMRNB_NUM_SAMPLES : AMRWB_NUM_SAMPLES );
  }

  (type==AMR_NB) ? Decoder_Interface_exit(amrDecoder) : D_IF_exit(amrDecoder);
  amrDecoder = NULL;

  return pcmData;
}

/* PCM to AMR NB */
int pcm2amr_execute(char* data, int size, char* pOutput, void* amrEncoder, enum Mode amrMode)
{
	int nRet = 0;
	int amrPTime = 20;
	int amrDTX = 0;

	unsigned int unitaryBuffSize = sizeof (int16_t) * AMRNB_NUM_SAMPLES;
	unsigned int buffSize = unitaryBuffSize * amrPTime / 20;

	int16_t samples[buffSize];
	uint8_t tmp[AMR_OUT_MAX_SIZE];
	uint8_t	tmp1[20*AMR_OUT_MAX_SIZE];

  char* pData = data;
	uint8_t output[AMR_OUT_MAX_SIZE * buffSize / unitaryBuffSize + 1];
	while (size >= buffSize)
	{
		memcpy((uint8_t*)samples, pData, buffSize);

    memset(output, 0, sizeof(output));
	  hbs_t* payload = bs_new(output, AMR_OUT_MAX_SIZE * buffSize / unitaryBuffSize + 1);

		int nFrameData = 0;
  	int offset = 0;
		for (offset = 0; offset < buffSize; offset += unitaryBuffSize)
		{
			int ret = Encoder_Interface_Encode(amrEncoder, amrMode, &samples[offset / sizeof (int16_t)], tmp, amrDTX);
			if (ret <= 0 || ret > 32){ printf("Encoder returned %i\n", ret); continue; }

			int nFbit = tmp[0] >> 7;
			nFbit = (offset+buffSize >= unitaryBuffSize) ? 0 : 1;
			int nFTbits = tmp[0] >> 3 & 0x0F;
			if(nFTbits > AMRNB_MAX_FRAME_TYPE){ printf("%s, Bad amr toc, index=%i (MAX=%d)\n", __func__, nFTbits, AMRNB_MAX_FRAME_TYPE); break; }
			int nQbit = tmp[0] >> 2 & 0x01;

			// Frame
			int framesz = amrnb_frame_sizes[nFTbits];
			memcpy(&tmp1[nFrameData], &tmp[1], framesz);
			nFrameData += framesz;

			// write TOC
			bs_write_u(payload, 1, nFbit);
			bs_write_u(payload, 4, nFTbits);
			bs_write_u(payload, 1, nQbit);
			if(b_octet_align == 1) bs_write_u(payload, 2, 0); // octet-align, add padding bit

		} // end of for
		if(offset > 0) bs_write_bytes_ex(payload, tmp1, nFrameData);

		int nOutputSize = 1 + nFrameData;
		memcpy(pOutput+nRet, output, nOutputSize);
		nRet += nOutputSize;

		bs_free(payload);
		size -= buffSize;

    pData += buffSize;
	} // end of while

	return nRet;
}

short* resampleTo8K(short* data, int size, int sampleRate, int* out_size)
{
  float* data_in = (float*)malloc(sizeof(float)*size);
  for(int i=0; i<size; i++) data_in[i] = data[i];

  double src_ratio = 1.0*8000/sampleRate;
  long output_frames = (int)(size*src_ratio);

  float* data_out = (float*)malloc(sizeof(float)*output_frames);
  short* out_data = (short*)malloc(sizeof(short)*output_frames);
  *out_size = output_frames;

  // Resampling preparation
  SRC_DATA* src_data = (SRC_DATA*)malloc(sizeof(SRC_DATA));
  if (!src_data) {
    free(data_out);
    free(out_data);
    return NULL;
  }
  src_data->data_in = data_in;
  src_data->data_out = data_out;
  src_data->input_frames = size;
  src_data->output_frames = output_frames;
  src_data->src_ratio = src_ratio;

  // Resample
  src_simple(src_data, SRC_SINC_FASTEST, 1);//  (SRC_DATA *data, int converter_type, int channels);

  // Copy the data back to 'short' type
  for(int i=0; i<output_frames; i++) out_data[i] = (short) data_out[i];

  free(src_data);
  free(data_in);
  free(data_out);

  return out_data;
}

enum Mode getAMRMode(int amrMode) {
  enum Mode mode = MR475;
  switch(amrMode) {
    case 0:
      mode = MR475;
      break;
    case 1:
      mode = MR515;
      break;
    case 2:
      mode = MR59;
      break;
    case 3:
      mode = MR67;
      break;
    case 4:
      mode = MR74;
      break;
    case 5:
      mode = MR795;
      break;
    case 6:
      mode = MR102;
      break;
    case 7:
      mode = MR122;
      break;
    default:
      mode = MR122;
      break;
  }
  return mode;
}

char* pcm2amr(short* data, int size, int sampleRate, int* out_size, int amrMode)
{
  short* resampled = data;
  int new_size = size;
  if (sampleRate != 8000) resampled = resampleTo8K(data, size, sampleRate, &new_size);

  // amrnb_encode_init(nMode);
  void* amrEncoder = Encoder_Interface_init(0);
  uint8_t* output = (uint8_t*) malloc(size*sizeof(short));

  *out_size = pcm2amr_execute((char*)resampled, 2*new_size, (char*)output, amrEncoder, getAMRMode(amrMode));
  *out_size += 6;
  char header[6] = {'#', '!', 'A', 'M', 'R', '\n'};
  char* result = (char*) malloc(*out_size);
  memset(result, 0, *out_size);
  memcpy(result, header, 6);
  memcpy(result+6, output, *out_size-6);

	free(output);

  // amrnb_encode_uninit();
  Encoder_Interface_exit(amrEncoder);

  if (sampleRate != 8000 && resampled != NULL) free(resampled);

  return result;
}

char* wav2amr(short* data, int size, int sampleRate, int* out_size, int mode)
{
  // Skip the WAVE header, which is 44-byte long.
  return pcm2amr(data+44, size-44, sampleRate, out_size, mode);
}

char* mp32amr(short* data, int size, int* out_size, int mode)
{
  // Convert MP3 data
  mp3dec_t mp3d;
  mp3dec_file_info_t info;
  mp3dec_load_buf(&mp3d, (uint8_t*)data, size*2, &info, 0, 0);
  int samples = info.samples;
  if(samples == 0 ) return NULL;
  short* pcm = info.buffer;

  return pcm2amr(info.buffer, info.samples, info.hz, out_size, mode);

}

