#include <cmath>
#include <complex>
#include <string.h>
#include <vector>

#include "ffts.h"

#include "denoise.h"

void debug(std::vector<float> data) {
  for(int i=0; i<20; i++) {
    printf("%.10f\n", data[i]);
  }
}

void debug(std::vector<std::complex<float>> data) {
  for(int i=0; i<20; i++) {
    printf("%.10f + %.10fi\n", std::real(data[i]), std::imag(data[i]));
  }
}

void gainControl(std::vector<float>& gain, int constraintInLength, auto fft_forward, auto fft_backward)
{
  float fftSize = gain.size();
  float meanGain = 0.0;
  std::vector<std::complex<float>> gainc(fftSize);
  for(int i=0; i<fftSize; i++) {
    meanGain += gain[i]*gain[i];
    gainc[i] = {gain[i], 0.0};
  }
  meanGain /= fftSize;
  // printf("meanGain = %.20f\n", meanGain);

  int L2 = constraintInLength;
  // Compute the hammingWindow.
  std::vector<float> hammingWindow(L2);
  for(int i=0; i<L2; i++ ) {
    hammingWindow[i] = 0.53836 - 0.46164 * std::cos( 2 * M_PI * (i+1) / ( L2 - 1 ) );
  }
  // debug(hammingWindow);

  // Frequency -> Time
  // Computation of the non-constrained impulse response
  std::vector<std::complex<float>> out(fftSize);
  // auto fft_backward = ffts_init_1d(fftSize, FFTS_BACKWARD);
  ffts_execute(fft_backward, gainc.data(), out.data());
  std::vector<float> impulseR(fftSize);
  std::vector<float> impulseR2(fftSize);
  for(int i=0; i<fftSize; i++) {
    impulseR[i] = std::real(out[i])/fftSize;
  }
  // debug(impulseR);
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
  // debug(impulseR2);
  // ffts_free(fft_backward);

  // Time -> Frequency
  std::vector<std::complex<float>> input(fftSize);
  for(int i=0; i<fftSize; i++) {
    input[i] = {impulseR2[i], 0.0};
  }
  // auto fft_forward = ffts_init_1d(fftSize, FFTS_FORWARD);
  ffts_execute(fft_forward, input.data(), out.data());
  float meanNewGain = 0.0;
  for(int i=0; i<fftSize; i++) {
    gain[i] = std::abs(out[i]);
    meanNewGain += gain[i]*gain[i];
  }
  // debug(gain);
  meanNewGain /= fftSize;
  // printf("meanNewGain=%.20f\n", meanNewGain);
  for(int i=0; i<fftSize; i++) {
    // Normalisation to keep the same energy (if white r.v.)
    gain[i] *= std::sqrt(meanGain/meanNewGain);
  }
  // debug(gain);

  // ffts_free(fft_forward);

}

std::vector<float> weinerDenoiseTSNR(const std::vector<float>& noisySpeech, int sampleRate, int nbInitialSilentFrames)
{
  // debug(noisySpeech);
  int samples = noisySpeech.size();
  int frameLength = std::floor(0.020*sampleRate); // frame length is fixed to 20 ms.
  int fftSize = 2*frameLength; // FFT size is twice the frame length.  change from 640 to 512 can significantly reduce the time consumed.
  int isFrameLength = nbInitialSilentFrames * frameLength; // Initial Silence or Noise Only part in samples (= ten frames)

  // Compute the hannWindow.
  std::vector<float> hannWindow(frameLength);
  for(int i=0; i<frameLength; i++ ) {
    hannWindow[i] = 0.5 * ( 1 - cos( 2 * M_PI * (i+1) / (frameLength-1) ) );
  }
  // debug(hannWindow);

  // Compute the noise statistics
  std::vector<std::complex<float>> signal_hanwin(fftSize);
  std::vector<std::complex<float>> out(fftSize);

  auto fft_forward = ffts_init_1d(fftSize, FFTS_FORWARD);
  auto fft_backward = ffts_init_1d(fftSize, FFTS_BACKWARD);

  std::vector<float> nsum(fftSize);
  for(int i=0; i<=isFrameLength-frameLength; i+=1) {
    for(int j=0; j<fftSize; j++) {
      if(j<frameLength) {
        float hanwin = noisySpeech[i+j] * hannWindow[j];
        signal_hanwin[j] = {hanwin, 0.0};
      } else {
        signal_hanwin[j] = {0.0, 0.0};
      }
    }
    ffts_execute(fft_forward, signal_hanwin.data(), out.data());
    for(int j=0; j<fftSize; j++)
    {
      nsum[j] += 1.0*pow(std::abs(out[j]), 2);
    }
  }
//    ffts_free(fft_forward);

  for(int i=0; i<fftSize; i++) {
    nsum[i] /= (isFrameLength-frameLength+1);
  }

  // printf("isFrameLength=%d frameLength=%d\n", isFrameLength, frameLength);
  // debug(nsum);

  // Main algorithm
  float SP = 0.25; // Shift percentage is 25 % Overlap-Add method works good with this value
  float normFactor = 1/SP;
  int overlap = std::floor((1-SP)*frameLength); // overlap between sucessive frames
  int offset = frameLength - overlap;
  int max_m = std::floor((samples-fftSize)/offset);
  // printf("overlap: %d  offset: %d  max_m: %d\n", overlap, offset, max_m);

  std::vector<float> oldmag(fftSize);
  std::vector<float> news(samples);
  std::vector<std::vector<float>> phasea(max_m); // fftSize * max_m
  std::vector<std::vector<float>> xmaga(max_m); // fftSize * max_m
  std::vector<std::vector<float>> tsnra(max_m); // fftSize * max_m
  std::vector<std::vector<float>> newmags(max_m); // fftSize * max_m
  float alpha = 0.98;

  // Iteration to remove noise
  // --------------- TSNR ---------------------
  std::vector<std::complex<float>> winy(fftSize);
  std::vector<std::complex<float>> ffty(fftSize);
  std::vector<float> phasey(fftSize);
  std::vector<float> magy(fftSize);
  std::vector<float> newmag(fftSize);
  std::vector<float> Gtsnr(fftSize);
  for(int i=0; i<max_m; i++) {
    // Extract speech segment, and perform the hanning window.
    for(int j=0; j<fftSize; j++ ) {
      if(j<frameLength) {
        float sample_value = noisySpeech[i*offset+j];
        winy[j] = {hannWindow[j] * sample_value, 0.0};
      } else {
        winy[j] = {0.0, 0.0};
      }
    }
    // Perform fast fourier transform
    // auto fft_forward = ffts_init_1d(fftSize, FFTS_FORWARD);
    ffts_execute(fft_forward, winy.data(), ffty.data());
    // debug(ffty);
    // Extract phase, magnitude, and etc.
    for(int j=0; j<fftSize; j++ ) {
      // Phase
      phasey[j] = std::arg(ffty[j]);
      // Magnitude
      magy[j] = std::abs(ffty[j]);
      // Calculate a posteriori SNR
      float postsnr = ((magy[j]*magy[j])/nsum[j])-1 ;
      // Limitation to prevent distorsion
      if(postsnr<0.1) postsnr = 0.1;

      // Calculate a priori SNR using decision directed approach
      float eta = alpha * ( (oldmag[j]*oldmag[j])/nsum[j] ) + (1-alpha) * postsnr;
      newmag[j] = (eta/(eta+1)) * magy[j];

      // Calculate TSNR
      float tsnr = (newmag[j]*newmag[j]) / nsum[j];
      // Gain of TSNR
      Gtsnr[j] = tsnr / (tsnr+1);
    }
    // debug(phasey);
    // debug(magy);
    // debug(newmag);
    // debug(Gtsnr);

    // For HRNR use
    phasea[i] = phasey;
    xmaga[i] = magy;
    tsnra[i] = Gtsnr;

    // Gtsnr=max(Gtsnr,0.1);  
    gainControl(Gtsnr, frameLength, fft_forward, fft_backward);
    // debug(Gtsnr);

    for(int j=0; j<fftSize; j++ ) {
      newmag[j] = Gtsnr[j] * magy[j];
      ffty[j] = std::polar(newmag[j], phasey[j]);
      oldmag[j] = abs(newmag[j]);
    }
    // debug(newmag);
    // debug(ffty);
    // For HRNR use
    newmags[i] = newmag;

    // auto fft_backward = ffts_init_1d(fftSize, FFTS_BACKWARD);
    ffts_execute(fft_backward, ffty.data(), out.data());

    for(int j=0; j<frameLength; j++) {
      news[i*offset+j] += std::real(out[j])/normFactor/fftSize;
      // news[i*offset+j] *= 100000000;
    }
    // debug(news);

    // ffts_free(fft_forward);
    // ffts_free(fft_backward);
    // break;
  }
  // debug(news);
  ffts_free(fft_forward);
  ffts_free(fft_backward);

  return news; // noisySpeech;
}

std::vector<float> weinerDenoiseHRNR(const std::vector<float>& noisySpeech, int sampleRate, int nbInitialSilentFrames)
{

}


