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

  double* signal = MFCC::loadWaveData(wavFileName.c_str(), 25, 10, nbFrames, frameLength, frameStep, sampleRate);

  double lowerBound = 0;
  double upperBound = 3500;

  int nbFilters = 40;
  double preEmphFactor = 0.97;

  MFCC m(frameLength, sampleRate, nbFilters, lowerBound, upperBound, preEmphFactor);

  for(int i=0; i<nbFrames; i++)
  {
//    printf("%d,", i);
    m.mfcc(signal+i*frameStep);
    std::vector<double> mfccs = m.getMFCCs(1, 12);
    // mfccs = m.getMelBankFeatures(0, 40, false);
    for(int j=0; j<mfccs.size(); j++)
    {
      printf("%f,", mfccs[j]);
    }
    printf("\n");
    break;
  }
}


int main (int argc, char *argv[])
{

  pitch_detection("../wav/female.wav");

  mfcc_test();

  return 0;

}

