/*************************************************
 *
 * MFCC computation.
 *
 * Author: Feng Zhang (zhjinf@gmail.com)
 * Date: 2018-10-18
 * 
 * Copyright:
 *   See LICENSE.
 * 
 ************************************************/

#ifndef _INCLUDE_MFCC_H_
#define _INCLUDE_MFCC_H_

#include <cmath>

class MFCC
{

public:

  MFCC(int frameLength, int sampleRate, int nbFilters = 26, float lowerBound = 300, float upperBound = 3500, float preEmphFactor = 0.97);
  virtual ~MFCC();

  // NOTE: The signal length is fixed to 'frameLength'.
  bool mfcc(float* signal);

  std::vector<float> getMFCCs(int idxStart, int idxEnd, bool isNormalize = true, int cepLifter = 22);
  std::vector<float> getMelBankFeatures(int idxStart, int idxEnd, bool isNormalize = true);

  float* getMFCCArray(int idxStart, int idxEnd, bool isNormalize = true, int cepLifter = 22);
  float* getMelBankFeatureArray(int idxStart, int idxEnd, bool isNormalize = true);

public:
  static float* loadWaveData(const char* wavFileName, int msFrame, int msStep, int &nbFrames, int &frameLength, int &frameStep, int &sampleRate);
  static float* padScaleOriginalWaveData(const std::vector<double> &wavData, int sampleRate, int msFrame, int msStep, int &nbFrames, int &frameLength, int &frameStep);


private:

  // Initialize the Hamming Window with the frame length.
  float* initHammingCoeff(int frameLength);

  // Initialize the DCT coefficients with the number of filters.
  float* initDctCoeff(int nbFilters);

  // Initialize the Mel coefficients with the frequency boundaries, the number of filters, the number of FFT-size, and the sample rate.
  float* initMelFilters(int nbFilters, float lowerBound, float upperBound, int sampleRate, int fftSize);

  // Finalze.
  void finalize();

  // Get the size for FFT.
  int sizeForFFT(int frameLength, int fftSize = 512);

  // Expand the original signal for FFT.
  float* expandSignal(float* signal, int frameLength, int newSizeForFFT);

  // Pre-emphasize the original signal.
  void preEmphasize(float* signal, int length);

  // Add the Hamming window.
  void addHammingWindow(float* signal, int length);

  // Preprocess in one shot, combinations of 'expandSignal', 'preEmphasize', and 'addHammingWindow'.
  float* preprocessOneShot(float* signal, int frameLength, int &newSizeForFFT);

  // Compute the power spectral coefficients on the time-domain signal.
  float* computePowerSpectralCoeff(float* signal, int length);

  // Compute the Mel bank features.
  float* computeMelBankFeatures(float* powerSpectralCoef, int fftSize, int nbFilters);

  // Compute the MFCC.
  float* computeMFCC(float* melBankFeatures, int nbFilters);

  // Lift the MFCC values.
  void liftMFCCs(std::vector<float> &mfccs, int cepLifter);
  void liftMFCCs(float* mfccs, int length, int cepLifter);

  // Perform the normalization.
  void normalize(std::vector<float> &values);
  void normalize(float* values, int length);


private:
  // inline float hz2Mel(float hz){ return 1125 * std::log( 1 + (f) / 700 ); };
  inline float hz2Mel(float hz){ return 2595 * std::log10( 1 + (hz) / 700 ); };
  // inline float mel2Hz(float mel){ return 700 * ( std::exp( (m) / 1125 ) - 1 ); };
  inline float mel2Hz(float mel){ return 700 * ( std::pow(10, (mel) / 2595 ) - 1 ); };


private:

  float* m_hammingCoeff = NULL;
  float* m_dctCoeff = NULL;
  float* m_melCoeff = NULL;

  float m_preEmphasizeCoeff = 0.97;

  int m_frameLength;
  int m_nbFilters;

  std::vector<float> m_melBankFeatures;
  std::vector<float> m_MFCCs;

  float* m_melBankFeatureArray;
  float* m_mfccArray;
};

#endif // #ifndef _INCLUDE_MFCC_H_
