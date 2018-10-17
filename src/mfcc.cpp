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

#include <complex>
#include <cstring>
#include <vector>

#include "ffts.h"
#include "AudioFile.h"

#include "mfcc.h"

MFCC::MFCC(int frameLength, int sampleRate, int nbFilters, float lowerBound, float upperBound, float preEmphFactor)
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

  m_melBankFeatureArray = NULL;
  m_mfccArray = NULL;
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

  if(m_melBankFeatureArray)
  {
    delete [] m_melBankFeatureArray;
    m_melBankFeatureArray = NULL;
  }

  if(m_mfccArray)
  {
    delete []m_mfccArray;
    m_mfccArray = NULL;
  }
}

// Initialize the Hamming Window with the frame length.
float* MFCC::initHammingCoeff(int frameLength)
{
  float* hammingCoeff = new float [ frameLength ];

  for(int i=0; i<frameLength; i++) 
  {
    hammingCoeff[i] = 0.53836 - 0.46164 * std::cos( 2 * M_PI * i / ( frameLength - 1 ) );
    // hammingCoeff[i] = 0.54 - 0.46 * std::cos( 2 * M_PI * i / ( frameLength - 1 ) );
  }

  return hammingCoeff;
}

// Initialize the DCT coefficients with the number of filters.
float* MFCC::initDctCoeff(int nbFilters)
{
  float* dctCoeff = new float [ nbFilters * nbFilters ];

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
float* MFCC::initMelFilters(int nbFilters, float lowerBound, float upperBound, int sampleRate, int fftSize)
{
  // Frequency bins from 0 (DC), 1 to fftSize/2 (the first half. The second half is the mirroring part of the first part).
  // Therefore, we only get the DC + the non-duplicated frequency bins.
  int nbFreqBins = fftSize / 2 + 1;

  float* melCoeff = new float [ nbFilters * nbFreqBins ];
  memset( melCoeff, 0.0, nbFilters * nbFreqBins * sizeof(float) );

  // Convert the frequency from Hz to Mel-frequency.
  float lbMelFreq = hz2Mel(lowerBound);
  float upMelFreq = hz2Mel(upperBound);

  // Create the centering bins for each Mel-filter.
  int* freqBins = new int [ nbFilters + 2 ]; // +2 to add the two boundaries: lowest and greatest.
  {
    // Evenly created the bins in the Mel-frequency.
    float step = ( upMelFreq - lbMelFreq ) / ( nbFilters + 1 );

    for(int i=0; i<nbFilters+2; i++)
    {
      float freqMelCenter = mel2Hz( lbMelFreq + step * i );

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
  if( frameLength<0 )
  {
    return -1;
  }

  if( frameLength<fftSize )
  {
    return fftSize;
  }

  int newFFTSize = fftSize;
  for(int i=0; i<10; i++)
  {
    if( frameLength > newFFTSize )
    {
      newFFTSize *= 2;
    }
    else
    {
      break;
    }
  } 

  return newFFTSize;
}

float* MFCC::expandSignal(float* signal, int frameLength, int newSizeForFFT)
{
  float* expandedSignal = new float[ newSizeForFFT ];

  memset(expandedSignal, 0.0, newSizeForFFT * sizeof(float) );

  for(int i=0; i<frameLength; i++)
  {
    expandedSignal[i] = signal[i];
  }

  return expandedSignal;
}

// Compute the power spectral coefficients on the time-domain signal.
float* MFCC::computePowerSpectralCoeff(float* signal, int fftSize)
{
  // Frequencies from 0 (DC), 1 to fftSize/2 (the first half. The second half is the mirroring part of the first part).
  // Therefore, we only get the DC + the non-duplicated frequencies.
  int nbFreqs = fftSize / 2 + 1;

  float* powerSpectralCoef = new float[nbFreqs];

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

float* MFCC::computeMelBankFeatures(float* powerSpectralCoef, int fftSize, int nbFilters)
{
  // Frequencies from 0 (DC), 1 to fftSize/2 (the first half. The second half is the mirroring part of the first part).
  // Therefore, we only get the DC + the non-duplicated frequencies.
  int nbFreqs = fftSize / 2 + 1;

  float* melBankFeatures = new float[nbFilters];
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

float* MFCC::computeMFCC(float* melBankFeatures, int nbFilters)
{
  // Perform the DCT transformation on the Mel bank features
  float* mfccs = new float[nbFilters];
  memset(mfccs, 0.0, nbFilters * sizeof(float));

  float factor = sqrt( 1.0 / ( 2 * nbFilters ) );

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

void MFCC::preEmphasize(float* signal, int length)
{
  float prev = signal[0];
  float emphasized = 0.0;
  for(int i=1; i<length; i++)
  {
    emphasized = signal[i] - m_preEmphasizeCoeff * prev;
    prev = signal[i];
    signal[i] = emphasized;
  }
}

void MFCC::addHammingWindow(float* signal, int length)
{
  for(int i=0; i<length; i++)
  {
    signal[i] *= m_hammingCoeff[i];
  }
}

// Perform the lifting
void MFCC::liftMFCCs(std::vector<float> &mfccs, int cepLifter)
{
  for(size_t i=0; i<mfccs.size(); i++)
  {
    mfccs[i] *= ( 1 + (cepLifter / 2.0) * std::sin(M_PI * i / cepLifter) );
  }
}
void MFCC::liftMFCCs(float* mfccs, int length, int cepLifter)
{
  for(int i=0; i<length; i++)
  {
    mfccs[i] *= ( 1 + (cepLifter / 2.0) * std::sin(M_PI * i / cepLifter) );
  }
}

// Perform the normalization.
void MFCC::normalize(std::vector<float> &values)
{
  size_t length = values.size();
  float average = 0.0;
  for(size_t i=0; i<length; i++)
  {
    average += values[i];
  }
  average /= length;

  for(size_t i=0; i<length; i++)
  {
    values[i] -= average;
  }
}
void MFCC::normalize(float* values, int length)
{
  float average = 0.0;
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
bool MFCC::mfcc(float* signal)
{
  // Pre-emphasize the original signal with the 'frameLength' length.
  preEmphasize(signal, m_frameLength);

  // Add the Hamming Window to the emphasized signal.
  addHammingWindow(signal, m_frameLength);

  // Get the size for FFT, and expand the signal.
  int fftSize = sizeForFFT(m_frameLength);
  float* expandedSignalForFFT = expandSignal(signal, m_frameLength, fftSize);

  // Compute the power spectrum.
  float* powerSpectralCoef = computePowerSpectralCoeff(expandedSignalForFFT, fftSize);
  delete [] expandedSignalForFFT;

  // Compute the Mel bank features.
  float* melBankFeatures = computeMelBankFeatures(powerSpectralCoef, fftSize, m_nbFilters);
  delete [] powerSpectralCoef;

  // Copy to the instance variable.
  for(int i=0; i<m_nbFilters; i++) m_melBankFeatures[i] = melBankFeatures[i];

  // Compute the MFCC.
  float* mfccs = computeMFCC(melBankFeatures, m_nbFilters);
  // delete [] melBankFeatures;
  if( m_melBankFeatureArray ) delete [] m_melBankFeatureArray;
  m_melBankFeatureArray = melBankFeatures;

  // Copy to the instance variable.
  for(int i=0; i<m_nbFilters; i++) m_MFCCs[i] = mfccs[i];
  // delete []mfccs;
  if( m_mfccArray ) delete [] m_mfccArray;
  m_mfccArray = mfccs;

  return true;
}

float* MFCC::preprocessOneShot(float* signal, int frameLength, int &newSizeForFFT)
{
  // Get the size for FFT, and expand the signal.
  int fftSize = sizeForFFT(frameLength);
  float* expandedSignal = new float[fftSize];
  memset(expandedSignal, 0.0, fftSize * sizeof(float) );

  // Pre-emphasize, and add the hamming window.
  expandedSignal[0] = signal[0];
  for(int i=1; i<frameLength; i++)
  {
    expandedSignal[i] = ( signal[i] - m_preEmphasizeCoeff * signal[i-1] ) * m_hammingCoeff[i];
  }

  newSizeForFFT = fftSize;

  return expandedSignal;
}

std::vector<float> MFCC::getMFCCs(int idxStart, int idxEnd, bool isNormalize, int cepLifter)
{
  std::vector<float>::const_iterator first = m_MFCCs.begin() + idxStart;
  std::vector<float>::const_iterator last = m_MFCCs.begin() + idxEnd + 1;

  std::vector<float> mfccs(first, last);

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

float* MFCC::getMFCCArray(int idxStart, int idxEnd, bool isNormalize, int cepLifter)
{
  // Do the lifting on the desired feature.
  if(cepLifter>0) // cepLifter = 0 means no lifting
  {
    liftMFCCs(m_mfccArray + idxStart, idxEnd - idxStart + 1, cepLifter);
  }

  // Normalize the results.
  if(isNormalize)
  {
    normalize(m_mfccArray + idxStart, idxEnd - idxStart + 1);
  }

  return m_mfccArray;
}

std::vector<float> MFCC::getMelBankFeatures(int idxStart, int idxEnd, bool isNormalize)
{
  std::vector<float>::const_iterator first = m_melBankFeatures.begin() + idxStart;
  std::vector<float>::const_iterator last = m_melBankFeatures.begin() + idxEnd + 1;

  std::vector<float> melBankFeatures(first, last);

  // Normalize the results.
  if(isNormalize)
  {
    normalize(melBankFeatures);
  }

  return melBankFeatures;
}

float* MFCC::getMelBankFeatureArray(int idxStart, int idxEnd, bool isNormalize)
{
  // Normalize the results.
  if(isNormalize)
  {
    normalize(m_melBankFeatureArray + idxStart, idxEnd - idxStart + 1);
  }
  return m_melBankFeatureArray + idxStart;
}


float* MFCC::loadWaveData(const char* wavFileName, int msFrame, int msStep, int &nbFrames, int &frameLength, int &frameStep, int &sampleRate)
{
  AudioFile<double> audioFile;
  audioFile.load(wavFileName);

  sampleRate = audioFile.getSampleRate();
  std::vector<std::vector<double>> buffer = audioFile.samples;

  std::vector<double> wavData = buffer[0]; // only use the first channel

  return padScaleOriginalWaveData(wavData, sampleRate, msFrame, msStep, nbFrames, frameLength, frameStep);
}

float* MFCC::padScaleOriginalWaveData(const std::vector<double> &wavData, int sampleRate, int msFrame, int msStep, int &nbFrames, int &frameLength, int &frameStep)
{
  int length = wavData.size();

  frameLength = 0.001 * msFrame * sampleRate;
  frameStep = 0.001 * msStep * sampleRate;

  nbFrames = (int)ceil(1.0*(length-frameLength)/frameStep);

  int paddedLength = frameLength*nbFrames;
  float* signal = new float[paddedLength];
  memset(signal, 0.0, paddedLength*sizeof(float));
  for(int i=0; i<length; i++) signal[i] = floor(32768*wavData[i]);
  // for(int i=0; i<length; i++) signal[i] = floor(wavData[i]);

  return signal;
}

