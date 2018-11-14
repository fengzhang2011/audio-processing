#include <cmath>
#include <complex>
#include <string.h>
#include <vector>

#include "ffts.h"

#include "denoise.h"

void gainControl(std::vector<float>& gain, int constraintInLength)
{
  float fftSize = gain.size();
  float meanGain = 0.0;
  for(int i=0; i<fftSize; i++) {
    meanGain += gain[i]*gain[i];
  }
  meanGain /= fftSize;

  int L2 = constraintInLength;
  // Compute the hammingWindow.
  std::vector<float> hammingWindow(L2);
  for(int i=0; i<L2; i++ ) {
    hammingWindow[i] = 0.53836 - 0.46164 * std::cos( 2 * M_PI * i / ( L2 - 1 ) );
  }

  // Frequency -> Time
  // Computation of the non-constrained impulse response
  std::vector<std::complex<float>> out(fftSize);
  auto fft_backward = ffts_init_1d(fftSize, FFTS_BACKWARD);
  ffts_execute(fft_backward, gain.data(), out.data());
  std::vector<float> impulseR(fftSize);
  std::vector<float> impulseR2(fftSize);
  for(int i=0; i<fftSize; i++) {
    impulseR[i] = std::real(out[i]);
  }
  // Application of the constraint in the time domain
  int split = fftSize-L2/2;
  for(int i=0; i<fftSize; i++) {
    if(i<L2/2) {
      impulseR2[i] = impulseR[i] * hammingWindow[L2/2+i];
    } else if(i>split) {
      impulseR2[i] = impulseR[i] * hammingWindow[i-split];
    } else {
      impulseR2[i] = 0.0;
    }
  }
  ffts_free(fft_backward);

  // Time -> Frequency
  auto fft_forward = ffts_init_1d(fftSize, FFTS_FORWARD);
  ffts_execute(fft_forward, impulseR2.data(), out.data());
  float meanNewGain = 0.0;
  for(int i=0; i<fftSize; i++) {
    gain[i] = std::abs(out[i]);
    meanNewGain += gain[i]*gain[i];
  }
  meanNewGain /= fftSize;
  for(int i=0; i<fftSize; i++) {
    // Normalisation to keep the same energy (if white r.v.)
    gain[i] *= std::sqrt(meanGain/meanNewGain);
  }

  ffts_free(fft_forward);

}

std::vector<float> weinerDenoiseTSNR(const std::vector<float>& noisySpeech, int sampleRate, int nbInitialSilentFrames)
{
  int samples = noisySpeech.size();
  int frameLength = std::floor(0.020*sampleRate); // frame length is fixed to 20 ms.
  int fftSize = 2*frameLength; // FFT size is twice the frame length.
  int isFrameLength = nbInitialSilentFrames * frameLength; // Initial Silence or Noise Only part in samples (= ten frames)

  // Compute the hannWindow.
  std::vector<float> hannWindow(frameLength);
  for(int i=0; i<frameLength; i++ ) {
    hannWindow[i] = 0.5 * ( 1 - cos( 2 * M_PI * i / (frameLength-1) ) );
  }

  // Compute the noise statistics
  std::vector<std::complex<float>> signal_hanwin(fftSize);
  std::vector<std::complex<float>> out(fftSize);

  std::vector<float> nsum(fftSize);
  for(int i=0; i<isFrameLength-frameLength; i++) {
    for(int j=0; j<fftSize; j++) {
      if(j<frameLength) {
        float hanwin = noisySpeech[i+j] * hannWindow[j];
        signal_hanwin[j] = {hanwin, 0.0};
      } else {
        signal_hanwin[j] = {0.0, 0.0};
      }
    }
    auto fft_forward = ffts_init_1d(fftSize, FFTS_FORWARD);
    ffts_execute(fft_forward, signal_hanwin.data(), out.data());
    for(int j=0; j<fftSize; j++)
    {
      nsum[j] += 1.0*pow(std::abs(out[j]), 2)/fftSize; // must divide it by length.
    }
    ffts_free(fft_forward);
  }

  for(int i=0; i<fftSize; i++) {
    nsum[i] /= (isFrameLength-frameLength);
  }

  // Main algorithm
  float SP = 0.25; // Shift percentage is 25 % Overlap-Add method works good with this value
  float normFactor = 1/SP;
  int overlap = std::floor((1-SP)*frameLength); // overlap between sucessive frames
  int offset = frameLength - overlap;
  int max_m = std::floor((samples-fftSize)/offset);

  std::vector<float> zvector(fftSize);
  std::vector<float> oldmag(fftSize);
  std::vector<float> news(samples);
  std::vector<std::vector<float>> phasea(max_m); // fftSize * max_m
  std::vector<std::vector<float>> xmaga(max_m); // fftSize * max_m
  std::vector<std::vector<float>> tsnra(max_m); // fftSize * max_m
  std::vector<std::vector<float>> newmags(max_m); // fftSize * max_m
  float alpha = 0.98;

  // Iteration to remove noise
  // --------------- TSNR ---------------------
  std::vector<float> speech(frameLength);
  std::vector<std::complex<float>> winy(fftSize);
  std::vector<std::complex<float>> ffty(fftSize);
  std::vector<float> phasey(fftSize);
  std::vector<float> magy(fftSize);
  std::vector<float> postsnr(fftSize);
  std::vector<float> eta(fftSize);
  std::vector<float> newmag(fftSize);
  std::vector<float> tsnr(fftSize);
  std::vector<float> Gtsnr(fftSize);
  for(int i=0; i<max_m; i++) {
    // Extract speech segment
    for(int j=0; j<frameLength; j++) {
      speech[j] = noisySpeech[i*offset+j];
    }
    // Perform hanning window
    for(int j=0; j<fftSize; j++ ) {
      if(j<frameLength) {
        winy[j] = {float(hannWindow[j] * speech[j]), 0.0};
      } else {
        winy[j] = {0.0, 0.0};
      }
    }
    // Perform fast fourier transform
    auto fft_forward = ffts_init_1d(fftSize, FFTS_FORWARD);
    ffts_execute(fft_forward, winy.data(), ffty.data());
    // Extract phase, magnitude, and etc.
    for(int j=0; j<fftSize; j++ ) {
      // Phase
      phasey[j] = std::arg(ffty[j]);
      // Magnitude
      magy[j] = std::abs(ffty[j]);
      // Calculate a posteriori SNR
      postsnr[j] = ((magy[j]*magy[j])/nsum[j])-1 ;
      // Limitation to prevent distorsion
      if(postsnr[j]<0.1) postsnr[j] = 0.1;

      // Calculate a priori SNR using decision directed approach
      eta[j] = alpha * ( (oldmag[j]*oldmag[j])/nsum[j] ) + (1-alpha) * postsnr[j];
      newmag[j] = (eta[j]/(eta[j]+1)) * magy[j];

      // Calculate TSNR
      tsnr[j] = (newmag[j]*newmag[j]) / nsum[j];
      // Gain of TSNR
      Gtsnr[j] = tsnr[j] / (tsnr[j]+1);
    }

    // For HRNR use
    phasea[i] = phasey;
    xmaga[i] = magy;
    tsnra[i] = Gtsnr;

    // Gtsnr=max(Gtsnr,0.1);  
    gainControl(Gtsnr, frameLength);

    for(int j=0; j<fftSize; j++ ) {
      newmag[j] = Gtsnr[j] * magy[j];
      ffty[j] = std::polar(newmag[j], phasey[j]);
      oldmag[j] = abs(newmag[j]);
    }
    // For HRNR use
    newmags[i] = newmag;

    auto fft_backward = ffts_init_1d(fftSize, FFTS_BACKWARD);
    ffts_execute(fft_backward, ffty.data(), out.data());

    for(int j=0; j<frameLength; j++) {
      news[i*offset+j] += std::real(out[j])/normFactor;
      news[i*offset+j] /= 32768;
    }

    ffts_free(fft_forward);
    ffts_free(fft_backward);
    
  }

  return news; // noisySpeech;
}

std::vector<float> weinerDenoiseHRNR(const std::vector<float>& noisySpeech, int sampleRate, int nbInitialSilentFrames)
{

}


