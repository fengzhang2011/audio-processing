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

#include <cmath>
#include <complex>
#include <cstring>
#include <vector>

#include "ffts.h"
#include "AudioFile.h"

#include "mfcc.h"

MFCC::MFCC(int frameLength, int sampleRate, int nbFilters, double lowerBound, double upperBound, double preEmphFactor)
{
  // Initialize
  m_hammingCoeff = initHammingCoeff(frameLength);
  m_dctCoeff = initDctCoeff(nbFilters);

  int fftSize = sizeForFFT(frameLength, 512); // by default, 512 points.
  m_melCoeff = initMelFilters(nbFilters, lowerBound, upperBound, sampleRate, fftSize);

  m_frameLength = frameLength;
  m_nbFilters = nbFilters;

  m_melBankFeatures.reserve(m_nbFilters);
  m_MFCCs.reserve(m_nbFilters);
}

MFCC::~MFCC()
{
  finalize();
}

// Initialize and finalze
void MFCC::finalize()
{
  if(m_hammingCoeff)
  {
    delete [] m_hammingCoeff;
    m_hammingCoeff = NULL;
  }

  if(m_dctCoeff)
  {
    delete [] m_dctCoeff;
    m_dctCoeff = NULL;
  }

  if(m_melCoeff)
  {
    delete [] m_melCoeff;
    m_melCoeff = NULL;
  }
}

// Initialize the Hamming Window with the frame length.
double* MFCC::initHammingCoeff(int frameLength)
{
  double* hammingCoeff = new double [ frameLength ];

  for(int i=0; i<frameLength; i++) 
  {
    hammingCoeff[i] = 0.53836 - 0.46164 * std::cos( 2 * M_PI * i / ( frameLength - 1 ) );
    // hammingCoeff[i] = 0.54 - 0.46 * std::cos( 2 * M_PI * i / ( frameLength - 1 ) );
  }

  return hammingCoeff;
}

// Initialize the DCT coefficients with the number of filters.
double* MFCC::initDctCoeff(int nbFilters)
{
  double* dctCoeff = new double [ nbFilters * nbFilters ];

  for(int k=0; k<nbFilters; k++)
  {
    for(int n=0; n<nbFilters; n++)
    {
      dctCoeff[ k * nbFilters + n ] = 2.0 * std::cos( M_PI / nbFilters * (n + 0.5) * k );
    }
  }

  return dctCoeff;
}

// Initialize the Mel coefficients with the frequency boundaries, the number of filters, the number of FFT-size, and the sample rate.
double* MFCC::initMelFilters(int nbFilters, double lowerBound, double upperBound, int sampleRate, int fftSize)
{
  // Frequency bins from 0 (DC), 1 to fftSize/2 (the first half. The second half is the mirroring part of the first part).
  // Therefore, we only get the DC + the non-duplicated frequency bins.
  int nbFreqBins = fftSize / 2 + 1;

  double* melCoeff = new double [ nbFilters * nbFreqBins ];
  memset( melCoeff, 0.0, nbFilters * nbFreqBins * sizeof(double) );

  // Convert the frequency from Hz to Mel-frequency.
  double lbMelFreq = hz2Mel(lowerBound);
  double upMelFreq = hz2Mel(upperBound);

  // Create the centering bins for each Mel-filter.
  int* freqBins = new int [ nbFilters + 2 ]; // +2 to add the two boundaries: lowest and greatest.
  {
    // Evenly created the bins in the Mel-frequency.
    double step = ( upMelFreq - lbMelFreq ) / ( nbFilters + 1 );

    for(int i=0; i<nbFilters+2; i++)
    {
      double freqMelCenter = mel2Hz( lbMelFreq + step * i );

      freqBins[i] = floor( ( fftSize + 1 ) * freqMelCenter / sampleRate );
    }
  }

  // Create the Mel-filter (from 1 to nbFilters, i.e., nbFilters in total).
  for(int i=1; i<nbFilters+1; i++)
  {
    int center = freqBins[ i ];
    int left = freqBins[ i - 1 ];
    int right = freqBins[ i + 1 ];

    for(int j=left; j<center; j++)
    {
      melCoeff[ ( i - 1 ) * nbFreqBins + j ] = 1.0 * ( j - left ) / ( center - left );
    }
    for(int j=center; j<=right; j++)
    {
      melCoeff[ ( i - 1 ) * nbFreqBins + j ] = 1.0 * ( right - j ) / ( right - center );
    }
  }

  return melCoeff;
}

int MFCC::sizeForFFT(int frameLength, int fftSize)
{
  if( frameLength>0 && frameLength<fftSize )
  {
    return fftSize;
  }
}

double* MFCC::expandSignal(double* signal, int frameLength, int newSizeForFFT)
{
  double* expandedSignal = new double[ newSizeForFFT ];

  memset(expandedSignal, 0.0, newSizeForFFT * sizeof(double) );

  for(int i=0; i<frameLength; i++)
  {
    expandedSignal[i] = signal[i];
  }

  return expandedSignal;
}

// Compute the power spectral coefficients on the time-domain signal.
double* MFCC::computePowerSpectralCoeff(double* signal, int fftSize)
{
  // Frequencies from 0 (DC), 1 to fftSize/2 (the first half. The second half is the mirroring part of the first part).
  // Therefore, we only get the DC + the non-duplicated frequencies.
  int nbFreqs = fftSize / 2 + 1;

  double* powerSpectralCoef = new double[nbFreqs];

  auto fft_forward = ffts_init_1d(fftSize, FFTS_FORWARD);

  std::vector<std::complex<float>> signalb_ext(fftSize);
  for(int i=0; i<fftSize; i++)
  {
    signalb_ext[i] = {float(signal[i]), 0.0};
  }

  std::vector<std::complex<float>> out(fftSize);
  ffts_execute(fft_forward, signalb_ext.data(), out.data());

  for(int i=0; i<nbFreqs; i++)
  {
    powerSpectralCoef[i] = 1.0*pow(std::abs(out[i]), 2)/fftSize; // must divide it by length.
  }

  ffts_free(fft_forward);

  return powerSpectralCoef;
}

double* MFCC::computeMelBankFeatures(double* powerSpectralCoef, int fftSize, int nbFilters)
{
  // Frequencies from 0 (DC), 1 to fftSize/2 (the first half. The second half is the mirroring part of the first part).
  // Therefore, we only get the DC + the non-duplicated frequencies.
  int nbFreqs = fftSize / 2 + 1;

  double* melBankFeatures = new double[nbFilters];
  for(int i=0; i<nbFilters; i++)
  {
    melBankFeatures[i] = 0.0;

    for (int j=1; j<nbFreqs; j++)
    {
      melBankFeatures[i] += m_melCoeff[ i * nbFreqs + j ] * powerSpectralCoef[ j ];
    }

    if (melBankFeatures[i] < 1e-3)
    {
      melBankFeatures[i] = 1e-3;
    }

    melBankFeatures[i] = 20 * std::log10 (melBankFeatures[i]);
  }

  return melBankFeatures;
}

double* MFCC::computeMFCC(double* melBankFeatures, int nbFilters)
{
  // Perform the DCT transformation on the Mel bank features
  double* mfccs = new double[nbFilters];
  memset(mfccs, 0.0, nbFilters * sizeof(double));

  double factor = sqrt( 1.0 / ( 2 * nbFilters ) );

  for(int i=0; i<nbFilters; i++)
  {
    for(int j=0; j<nbFilters; j++)
    {
      mfccs[i] += melBankFeatures[j] * m_dctCoeff[ i * nbFilters + j ];
    }
    mfccs[i] *= factor;
  }

  return mfccs;
}

void MFCC::preEmphasize(double* signal, int length)
{
  double prev = signal[0];
  double emphasized = 0.0;
  for(int i=1; i<length; i++)
  {
    emphasized = signal[i] - m_preEmphasizeCoeff * prev;
    prev = signal[i];
    signal[i] = emphasized;
  }
}

void MFCC::addHammingWindow(double* signal, int length)
{
  for(int i=0; i<length; i++)
  {
    signal[i] *= m_hammingCoeff[i];
  }
}

// Perform the lifting
void MFCC::liftMFCCs(std::vector<double> &mfccs, int cepLifter)
{
  for(int i=0; i<mfccs.size(); i++)
  {
    mfccs[i] *= ( 1 + (cepLifter / 2.0) * std::sin(M_PI * i / cepLifter) );
  }
}

// Perform the normalization.
void MFCC::normalize(std::vector<double> &values)
{
  int length = values.size();
  double average = 0.0;
  for(int i=0; i<length; i++)
  {
    average += values[i];
  }
  average /= length;

  for(int i=0; i<length; i++)
  {
    values[i] -= average;
  }
}

// NOTE: The signal length is fixed to 'frameLength'.
bool MFCC::mfcc(double* signal)
{
  // Pre-emphasize the original signal with the 'frameLength' length.
  preEmphasize(signal, m_frameLength);

  // Add the Hamming Window to the emphasized signal.
  addHammingWindow(signal, m_frameLength);

  // Get the size for FFT, and expand the signal.
  int fftSize = sizeForFFT(m_frameLength);
  double* expandedSignalForFFT = expandSignal(signal, m_frameLength, fftSize);

  // Compute the power spectrum.
  double* powerSpectralCoef = computePowerSpectralCoeff(expandedSignalForFFT, fftSize);
  delete [] expandedSignalForFFT;

  // Compute the Mel bank features.
  double* melBankFeatures = computeMelBankFeatures(powerSpectralCoef, fftSize, m_nbFilters);
  delete [] powerSpectralCoef;

  // Copy to the instance variable.
  for(int i=0; i<m_nbFilters; i++) m_melBankFeatures[i] = melBankFeatures[i];

  // Compute the MFCC.
  double* mfccs = computeMFCC(melBankFeatures, m_nbFilters);
  delete [] melBankFeatures;

  // Copy to the instance variable.
  for(int i=0; i<m_nbFilters; i++) m_MFCCs[i] = mfccs[i];

  delete []mfccs;

  return true;
}

double* MFCC::preprocessOneShot(double* signal, int frameLength, int &newSizeForFFT)
{
  // Get the size for FFT, and expand the signal.
  int fftSize = sizeForFFT(frameLength);
  double* expandedSignal = new double[fftSize];
  memset(expandedSignal, 0.0, fftSize * sizeof(double) );

  // Pre-emphasize, and add the hamming window.
  expandedSignal[0] = signal[0];
  for(int i=1; i<frameLength; i++)
  {
    expandedSignal[i] = ( signal[i] - m_preEmphasizeCoeff * signal[i-1] ) * m_hammingCoeff[i];
  }

  newSizeForFFT = fftSize;

  return expandedSignal;
}

std::vector<double> MFCC::getMFCCs(int idxStart, int idxEnd, bool isNormalize, int cepLifter)
{
  std::vector<double>::const_iterator first = m_MFCCs.begin() + idxStart;
  std::vector<double>::const_iterator last = m_MFCCs.begin() + idxEnd + 1;

  std::vector<double> mfccs(first, last);

  // Do the lifting on the desired feature.
  if(cepLifter>0) // cepLifter = 0 means no lifting
  {
    liftMFCCs(mfccs, cepLifter);
  }

  // Normalize the results.
  if(isNormalize)
  {
    normalize(mfccs);
  }

  return mfccs;
}

std::vector<double> MFCC::getMelBankFeatures(int idxStart, int idxEnd, bool isNormalize)
{
  std::vector<double>::const_iterator first = m_melBankFeatures.begin() + idxStart;
  std::vector<double>::const_iterator last = m_melBankFeatures.begin() + idxEnd + 1;

  std::vector<double> melBankFeatures(first, last);

  // Normalize the results.
  if(isNormalize)
  {
    normalize(melBankFeatures);
  }

  return melBankFeatures;
}


double* MFCC::loadWaveData(const char* wavFileName, int msFrame, int msStep, int &nbFrames, int &frameLength, int &frameStep, int &sampleRate)
{
  AudioFile<double> audioFile;
  audioFile.load(wavFileName);

  sampleRate = audioFile.getSampleRate();

  std::vector<std::vector<double>> buffer = audioFile.samples;
  std::vector<double> wavData = buffer[0];

  // int length = wavData.size();
  int originalLength = 3.5 * sampleRate; // keep the first 3.5 seconds

  frameLength = 0.001 * msFrame * sampleRate;
  frameStep = 0.001 * msStep * sampleRate;

  nbFrames = (int)ceil(1.0*(originalLength-frameLength)/frameStep);

  int paddedLength = frameLength*nbFrames;
  double* signal = new double[paddedLength];
  memset(signal, 0.0, paddedLength*sizeof(double));
  for(int i=0; i<originalLength; i++) signal[i] = floor(32768*wavData[i]);
  // for(int i=0; i<originalLength; i++) signal[i] = floor(wavData[i]);

  return signal;
}

