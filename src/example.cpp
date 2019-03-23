/*************************************************
 *
 * A simple example that uses the three libraries: AudioFile, FFTS, and Pitch_Detection.
 *
 * Author: Feng Zhang (zhjinf@gmail.com)
 * Date: 2018-10-13
 * 
 * Copyright:
 *   See LICENSE.
 * 
 ************************************************/

#include <algorithm> 
#include <complex>
#include <cmath>
#include <dirent.h> 
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>


#include "AudioFile.h"
#include "ffts.h"
#include "pitch_detection.h"
#include "mfcc.h"
#include "amr.h"
#include "minimp3.h"
#include "denoise.h"
#include "samplerate.h"



void pitch_detection (const char* filename)
{

  AudioFile<double> audioFile;

  audioFile.load(filename);

  //fprintf(stdout, "%d\n", audioFile.getNumChannels());
  printf("Sample rate = %d\n", audioFile.getSampleRate());

  uint32_t sampleRate = audioFile.getSampleRate();

  std::vector<std::vector<double>> buffer = audioFile.samples;
  if(buffer.size()>0) // We only use the first channel
  {
    std::vector<double> wavData = buffer[0];
    //printf("samples=%d\n", (int)wavData.size());

    float sum = 0.0;
    int count = 0;
    int step = 8192;
    for(int i=0; i<wavData.size()-step; i+=1024)
    {
      //printf("[%d, %d]\n", i, i+step);
      std::vector<double>::const_iterator first = wavData.begin() + i;
      std::vector<double>::const_iterator last = wavData.begin() + i + step;
      std::vector<double> data(first, last);
      //printf("data size=%d\n", (int)data.size());
  
      //float pitch1 = get_pitch_autocorrelation(data, sampleRate);
      //printf("pitch [autocorrelation] = %f\n", pitch1);
  
      //float pitch2 = get_pitch_yin(data, sampleRate);
      //printf("pitch [yin] = %f\n", pitch2);
  
      float pitch3 = get_pitch_mpm(data, sampleRate);
      //printf("pitch [mpm] = %f\n", pitch3);

      //float pitch4 = get_pitch_goertzel(data, sampleRate);
      //printf("pitch [goertzel] = %f\n", pitch4);
  
      //float pitch5 = get_pitch_dft(data, sampleRate);
      //printf("pitch [dft] = %f\n", pitch5);

      //if(pitch2>0 || pitch3>0)
      //{
      //  printf("pitch [yin] = %f\n", pitch2);
      //  printf("pitch [mpm] = %f\n", pitch3);
      //  //printf("pitch [goertzel] = %f\n", pitch4);
      //}

      if(pitch3>0 && pitch3<1000)
      {
        sum += pitch3;
        count ++;
      }
    }

    double mean = sum/count;
    
    printf("Pitch = %f Hz\n", mean);

  }
}

void mfcc_test ()
{
  std::string wavFileName = "../wav/OSR_us_000_0010_8k.wav";

  int sampleRate = 0;
  int frameLength = 0;
  int frameStep = 0;
  int nbFrames = 0;

  float* signal = MFCC::loadWaveData(wavFileName.c_str(), 25, 10, nbFrames, frameLength, frameStep, sampleRate);

  double lowerBound = 0;
  double upperBound = 3500;

  int nbFilters = 40;
  double preEmphFactor = 0.97;

  MFCC m(frameLength, sampleRate, nbFilters, lowerBound, upperBound, preEmphFactor);

  for(int i=0; i<nbFrames; i++)
  {
//    printf("%d,", i);
    m.mfcc(signal+i*frameStep);
    std::vector<float> mfccs = m.getMFCCs(1, 12);
    // mfccs = m.getMelBankFeatures(0, 40, false);
    for(int j=0; j<mfccs.size(); j++)
    {
      printf("%f,", mfccs[j]);
    }
    printf("\n");
//    break;
  }
}

void amr_test()
{
  char szInputFileName[] = "../wav/sample.amr";
  char szOutputFileName[256] = {0};

  strcpy(szOutputFileName, basename(szInputFileName));
  strcat(szOutputFileName, ".pcm");

  FILE *fp = NULL;
  if (!(fp = fopen(szInputFileName, "rb"))) return;

  fseek(fp, 0L, SEEK_END);
  int sz = ftell(fp);
  printf("file size = %d\n", sz);
  fseek(fp, 0L, SEEK_SET);
  char* data = (char*)malloc(sz);
  fread(data, (size_t)1, sz, fp);

  short* output = amrConvert(data, sz);
  free(output);

  free(data);
  fclose(fp);
}

void mp3_test(const char* fileName)
{
  // Convert MP3 data
  mp3dec_t mp3d;
  mp3dec_file_info_t info;
  mp3dec_load(&mp3d, fileName, &info, 0, 0);
  int samples = info.samples;
  if(samples == 0 ) return;
  short* pcm = info.buffer;

  int sampleRate = info.hz;
  printf("Sample rate = %d\n", sampleRate);
}

void denoise_test(const char* fileName)
{
  AudioFile<float> audioFile;
  audioFile.load(fileName);
  int sampleRate = audioFile.getSampleRate();

  printf("Sample rate = %d\n", sampleRate);

  std::vector<std::vector<float>> buffer = audioFile.samples;
  if(buffer.size()==0) return;
  // We only use the first channel
  int samples = buffer[0].size();

  std::vector<float> noisySpeech(samples);
  for(int i=0; i<samples; i++) {
    noisySpeech[i] = buffer[0][i];
  }

  std::vector<float> clean = weinerDenoiseTSNR(noisySpeech, sampleRate, 10);
  //for(int i=0; i<20; i++) {//clean.size(); i++) {
  //  printf("%.20f\n", clean[i]);
  //}

  std::vector<std::vector<float>> cleanedBuffer;
  cleanedBuffer.push_back(clean);
  AudioFile<float> audioFileOut;
  audioFileOut.setAudioBuffer(cleanedBuffer);
  audioFileOut.setSampleRate(sampleRate);
  audioFileOut.setNumChannels(1);
  audioFileOut.save("./clean.wav");
}

void resample_test(const char* fileName)
{
  // Read in the audio data
  AudioFile<float> audioFile;
  audioFile.load(fileName);
  int sampleRate = audioFile.getSampleRate();

  printf("Sample rate = %d\n", sampleRate);

  std::vector<std::vector<float>> buffer = audioFile.samples;
  if(buffer.size()==0) return;
  // We only use the first channel
  int samples = buffer[0].size();

  // Preparing the input and output data
  float* data_in = (float*)malloc(sizeof(float)*samples);
  long input_frames = samples;
  for(int i=0; i<input_frames; i++) {
    data_in[i] = buffer[0][i];
  }
  double src_ratio = 1.0*16000/sampleRate;
  long output_frames = (int)(samples*src_ratio);
  float* data_out = (float*)malloc(sizeof(float)*output_frames);

  // Resampling
  SRC_DATA* src_data = (SRC_DATA*)malloc(sizeof(SRC_DATA));
  if (!src_data) return;
  src_data->data_in = data_in;
  src_data->data_out = data_out;
  src_data->input_frames = input_frames;
  src_data->output_frames = output_frames;
  src_data->src_ratio = src_ratio;

  int converter = SRC_SINC_FASTEST;
  int channels = 1;
  src_simple(src_data, converter, channels);//  (SRC_DATA *data, int converter_type, int channels);

  printf("Resample: expected samples = %ld actual samples = %ld\n", output_frames, src_data->output_frames_gen);

  // Write the resampled audio
  std::vector<float> resampled;
  for(int i=0; i<output_frames; i++) {
    resampled.push_back(data_out[i]);
  }
  std::vector<std::vector<float>> resampledBuffer;
  resampledBuffer.push_back(resampled);
  AudioFile<float> audioFileOut;
  audioFileOut.setAudioBuffer(resampledBuffer);
  // audioFileOut.setAudioBuffer(buffer);
  audioFileOut.setSampleRate(16000);
  audioFileOut.setNumChannels(1);
  audioFileOut.save("./resampled.wav");

  free(src_data);
  free(data_in);
  free(data_out);
}

int main (int argc, char *argv[])
{

  // pitch_detection("../wav/female.wav");

  // mfcc_test();

  // mp3_test("../wav/t2.mp3");

  // denoise_test("../wav/noisy.wav");

  resample_test("../wav/female.wav");

  return 0;

}

