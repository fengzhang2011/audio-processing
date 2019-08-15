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
int amrNBEncodeFrame(short *pcmDataCurrentFrame, uint8_t* pOutput, void* amrEncoder, enum Mode amrMode)
{
  // Prepare the samples (input data)
  short samples[sizeof(short) * AMRNB_NUM_SAMPLES];
  memcpy((uint8_t*)samples, pcmDataCurrentFrame, sizeof(short) * AMRNB_NUM_SAMPLES);

  // Create the output buffer and payload pointer
  uint8_t output[AMR_OUT_MAX_SIZE + 1];
  memset(output, 0, sizeof(output));
  bs_t* payload = bs_new(output, AMR_OUT_MAX_SIZE + 1);

  // Encode
  uint8_t tmp[AMR_OUT_MAX_SIZE];
  int ret = Encoder_Interface_Encode(amrEncoder, amrMode, samples, tmp, 0);
  if (ret <= 0 || ret > 32){ printf("Encoder returned %i\n", ret); return 0; }

  // Write TOC
  int nFbit = 0;
  bs_write_u(payload, 1, nFbit);

  int nFTbits = tmp[0] >> 3 & 0x0F;
  if(nFTbits > AMRNB_MAX_FRAME_TYPE){ printf("%s, Bad amr toc, index=%i (MAX=%d)\n", __func__, nFTbits, AMRNB_MAX_FRAME_TYPE); return 0; }
	bs_write_u(payload, 4, nFTbits);

  int nQbit = tmp[0] >> 2 & 0x01;
	bs_write_u(payload, 1, nQbit);

	if(b_octet_align == 1){	bs_write_u(payload, 2, 0); }// octet-align, add padding bit

  // Frame
  int framesz = amrnb_frame_sizes[nFTbits];
  uint8_t	tmp1[20*AMR_OUT_MAX_SIZE];
  memcpy(&tmp1[0], &tmp[1], framesz);
  bs_write_bytes_ex(payload, tmp1, framesz);

	int nOutputSize = 1 + framesz;
	memcpy(pOutput, output, nOutputSize);

	bs_free(payload);

  return nOutputSize;
}

int amrNBEncodeFrame2(short *pcmDataCurrentFrame, uint8_t* pOutput, void* amrEncoder, enum Mode amrMode)
{
  // Prepare the samples (input data)
  short samples[sizeof(short) * AMRNB_NUM_SAMPLES];
  memcpy((uint8_t*)samples, pcmDataCurrentFrame, sizeof(short) * AMRNB_NUM_SAMPLES);

  // Create the output buffer and payload pointer
  uint8_t output[AMR_OUT_MAX_SIZE + 1];
  memset(output, 0, sizeof(output));
  bs_t* payload = bs_new(output, AMR_OUT_MAX_SIZE + 1);

  // Encode
  uint8_t tmp[AMR_OUT_MAX_SIZE];
  int ret = Encoder_Interface_Encode(amrEncoder, amrMode, samples, tmp, 0);
  if (ret <= 0 || ret > 32){ printf("Encoder returned %i\n", ret); return 0; }
printf("ret = %d\n", ret);
  // Write TOC
  int nFbit = 0;
  bs_write_u(payload, 1, nFbit);

  int nFTbits = tmp[0] >> 3 & 0x0F;
  if(nFTbits > AMRNB_MAX_FRAME_TYPE){ printf("%s, Bad amr toc, index=%i (MAX=%d)\n", __func__, nFTbits, AMRNB_MAX_FRAME_TYPE); return 0; }
	bs_write_u(payload, 4, nFTbits);

  int nQbit = tmp[0] >> 2 & 0x01;
	bs_write_u(payload, 1, nQbit);

	if(b_octet_align == 1){	bs_write_u(payload, 2, 0); }// octet-align, add padding bit

  // Frame
  int framesz = amrnb_frame_sizes[nFTbits];
  uint8_t	tmp1[20*AMR_OUT_MAX_SIZE];
  memcpy(&tmp1[0], &tmp[1], framesz);
  bs_write_bytes_ex(payload, tmp1, framesz);

	int nOutputSize = 1 + framesz;
	memcpy(pOutput, output, nOutputSize);

	bs_free(payload);

  return nOutputSize;
}

int pcm2amr2(char* data, int size, char* pOutput, void* amrEncoder, enum Mode amrMode)
{
	int nRet = 0;
	unsigned int unitary_buff_size = sizeof (int16_t) * AMRNB_NUM_SAMPLES;
	unsigned int buff_size = unitary_buff_size;
	uint8_t tmp[AMR_OUT_MAX_SIZE];
	int16_t samples[buff_size];
	uint8_t	tmp1[20*AMR_OUT_MAX_SIZE];
	bs_t	*payload = NULL;
	int		nCmr = 0xF;
	int		nFbit = 1, nFTbits = 0, nQbit = 0;
	int		nReserved = 0, nPadding = 0;
	int		nFrameData = 0, framesz = 0, nWrite = 0;
	int		offset = 0;
	int amrnb_dtx = 0;

	uint8_t output[AMR_OUT_MAX_SIZE * buff_size / unitary_buff_size + 1];
	int 	nOutputSize = 0;

  int totalFrames = 0;
  char* p2 = data;
	while (size >= buff_size)
	{
	  printf("offset = %d\n", int(p2-data));
		memset(output, 0, sizeof(output));
		memcpy((uint8_t*)samples, p2, buff_size);
		payload = bs_new(output, AMR_OUT_MAX_SIZE * buff_size / unitary_buff_size + 1);

		nFrameData = 0; nWrite = 0;
		for (offset = 0; offset < buff_size; offset += unitary_buff_size)
		{
			int ret = Encoder_Interface_Encode(amrEncoder, amrMode, &samples[offset / sizeof (int16_t)], tmp, amrnb_dtx);
			if (ret <= 0 || ret > 32)
			{
				printf("Encoder returned %i\n", ret);
				continue;
			}
			if (nRet == 0) {
					  char* p = (char*) tmp;
            for(int i=0; i<ret; i++) {
              if (i>0 && i%2==0) printf(" ");
              printf("%02x", p[i] & 0xFF);
            }
            printf("\n");
			}

			nFbit = tmp[0] >> 7;
			nFbit = (offset+buff_size >= unitary_buff_size) ? 0 : 1;
			nFTbits = tmp[0] >> 3 & 0x0F;
			if(nFTbits > AMRNB_MAX_FRAME_TYPE)
			{
				printf("%s, Bad amr toc, index=%i (MAX=%d)\n", __func__, nFTbits, AMRNB_MAX_FRAME_TYPE);
				break;
			}
			nQbit = tmp[0] >> 2 & 0x01;
			framesz = amrnb_frame_sizes[nFTbits];

			// Frame 데이터를 임시로 복사
			memcpy(&tmp1[nFrameData], &tmp[1], framesz);
			nFrameData += framesz;

			// write TOC
			bs_write_u(payload, 1, nFbit);
			bs_write_u(payload, 4, nFTbits);
			bs_write_u(payload, 1, nQbit);
			if(b_octet_align == 1)
			{	// octet-align, add padding bit
				bs_write_u(payload, 2, nPadding);
			}

		} // end of for
		if(offset > 0)
		{
			nWrite = bs_write_bytes_ex(payload, tmp1, nFrameData);
		}

		nOutputSize = 1 + framesz;
		/* nRet = fwrite(output, (size_t)1, nOutputSize, fp); */
		memcpy(pOutput+nRet, output, nOutputSize);
		if (nRet==0) {
		  char* p = (char*) output;
      for(int i=0; i<nOutputSize; i++) {
        if (i>0 && i%2==0) printf(" ");
        printf("%02x", p[i] & 0xFF);
      }
      printf("\n");
		}
		nRet += nOutputSize;
//    printf("nRet == %d, size == %d\n", nRet, nOutputSize);

		bs_free(payload);
		size -= buff_size;

    p2 += buff_size;
    totalFrames ++;

	} // end of while

	printf("total frames=%d\n", totalFrames);

	return nRet;
}

short* resampleTo8K(short* data, int size, int sampleRate, int* out_size)
{
  double src_ratio = 1.0*8000/sampleRate;
  long output_frames = (int)(size*src_ratio);
  float* data_in = (float*)malloc(sizeof(float)*size);
  for(int i=0; i<size; i++) data_in[i] = data[i];
  float* data_out = (float*)malloc(sizeof(float)*output_frames);
  short* out_data = (short*)malloc(sizeof(short)*output_frames);

  // Resampling
  SRC_DATA* src_data = (SRC_DATA*)malloc(sizeof(SRC_DATA));
  if (!src_data) return NULL;
  src_data->data_in = data_in;
  src_data->data_out = data_out;
  src_data->input_frames = size;
  src_data->output_frames = output_frames;
  src_data->src_ratio = src_ratio;

  int converter = SRC_SINC_FASTEST;
  int channels = 1;
  src_simple(src_data, converter, channels);//  (SRC_DATA *data, int converter_type, int channels);
  *out_size = output_frames;
  for(int i=0; i<output_frames; i++) {
    out_data[i] = (short) data_out[i];
  }

  free(src_data);
  free(data_in);
  free(data_out);

  return out_data;
}

char* pcm2amr(short* data, int size, int sampleRate, int* out_size)
{
  int new_size = size;
  short* resampled = data;

  if (sampleRate != 8000) resampled = resampleTo8K(data+44, size-44, sampleRate, &new_size);
  printf("size=%d new_size=%d\n", size, new_size);
  // amrnb_encode_init(nMode);
  enum Mode amrMode = MR122;
  void* amrEncoder = Encoder_Interface_init(0);
  uint8_t* output = (uint8_t*) malloc(size*sizeof(short));

  char* p = (char*) data;
//  for(int i=44; i<204; i++) {
//    if (i>0 && i%2==0) printf(" ");
//    printf("%02x", p[i] & 0xFF);
//  }
//  printf("\n");

  char* pcmCurrentFrame = (char*) data;
  int offset_out = 0;
	int nPcmSize = 320;
//	while(pcmCurrentFrame < (char*)data + size)
//	{
//    offset_out += amrNBEncodeFrame2((short*)pcmCurrentFrame, output + offset_out, amrEncoder, amrMode);
//    pcmCurrentFrame += nPcmSize;
//	}
  p = (char*) resampled;
//  p = (char*) data;
//  p += 44;
  *out_size = pcm2amr2(p, 2*new_size, (char*)output, amrEncoder, amrMode);
  printf("out_size=%d\n", *out_size);
  *out_size += 6;
  char* result = (char*) malloc(*out_size);
  memset(result, 0, *out_size);
  result[0] = '#';
  result[1] = '!';
  result[2] = 'A';
  result[3] = 'M';
  result[4] = 'R';
  result[5] = '\n';
  memcpy(result+6, output, *out_size-6);

	free(output);

  // amrnb_encode_uninit();
  Encoder_Interface_exit(amrEncoder);

  if (sampleRate != 8000 && resampled != NULL) free(resampled);

  return result;
}
