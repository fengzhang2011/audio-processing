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


class MFCC
{

public:

  MFCC(int frameLength, int sampleRate, int nbFilters = 26, double lowerBound = 300, double upperBound = 3500, double preEmphFactor = 0.97);
  virtual ~MFCC();

  // NOTE: The signal length is fixed to 'frameLength'.
  bool mfcc(double* signal);

  std::vector<double> getMFCCs(int idxStart, int idxEnd, bool isNormalize = true, int cepLifter = 22);
  std::vector<double> getMelBankFeatures(int idxStart, int idxEnd, bool isNormalize = true);


public:
  static double* loadWaveData(const char* wavFileName, int msFrame, int msStep, int &nbFrames, int &frameLength, int &frameStep, int &sampleRate);


private:

  // Initialize the Hamming Window with the frame length.
  double* initHammingCoeff(int frameLength);

  // Initialize the DCT coefficients with the number of filters.
  double* initDctCoeff(int nbFilters);

  // Initialize the Mel coefficients with the frequency boundaries, the number of filters, the number of FFT-size, and the sample rate.
  double* initMelFilters(int nbFilters, double lowerBound, double upperBound, int sampleRate, int fftSize);

  // Finalze.
  void finalize();

  // Get the size for FFT.
  int sizeForFFT(int frameLength, int fftSize = 512);

  // Expand the original signal for FFT.
  double* expandSignal(double* signal, int frameLength, int newSizeForFFT);

  // Pre-emphasize the original signal.
  void preEmphasize(double* signal, int length);

  // Add the Hamming window.
  void addHammingWindow(double* signal, int length);

  // Preprocess in one shot, combinations of 'expandSignal', 'preEmphasize', and 'addHammingWindow'.
  double* preprocessOneShot(double* signal, int frameLength, int &newSizeForFFT);

  // Compute the power spectral coefficients on the time-domain signal.
  double* computePowerSpectralCoeff(double* signal, int length);

  // Compute the Mel bank features.
  double* computeMelBankFeatures(double* powerSpectralCoef, int fftSize, int nbFilters);

  // Compute the MFCC.
  double* computeMFCC(double* melBankFeatures, int nbFilters);

  // Lift the MFCC values.
  void liftMFCCs(std::vector<double> &mfccs, int cepLifter);

  // Perform the normalization.
  void normalize(std::vector<double> &values);


private:
  // inline double hz2Mel(double hz){ return 1125 * std::log( 1 + (f) / 700 ); };
  inline double hz2Mel(double hz){ return 2595 * std::log10( 1 + (hz) / 700 ); };
  // inline double mel2Hz(double mel){ return 700 * ( std::exp( (m) / 1125 ) - 1 ); };
  inline double mel2Hz(double mel){ return 700 * ( std::pow(10, (mel) / 2595 ) - 1 ); };


private:

  double* m_hammingCoeff = NULL;
  double* m_dctCoeff = NULL;
  double* m_melCoeff = NULL;

  double m_preEmphasizeCoeff = 0.97;

  int m_frameLength;
  int m_nbFilters;

  std::vector<double> m_melBankFeatures;
  std::vector<double> m_MFCCs;
};

#endif // #ifndef _INCLUDE_MFCC_H_
