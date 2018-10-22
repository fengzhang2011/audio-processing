/*************************************************
 *
 * A simple example that uses the built module.
 *
 * Author: Feng Zhang (zhjinf@gmail.com)
 * Date: 2018-10-13
 * 
 * Copyright:
 *   See LICENSE.
 * 
 ************************************************/

// test.js
const ap = require('./build/Release/feng_ap');

console.log(ap.hello());

async function test() {

  // let audio = await ap.readAudio('./wav/female.wav');
  let audio = await ap.readAudio('./wav/male.wav');
  // let audio = await ap.readAudio('./wav/OSR_us_000_0010_8k.wav');
  console.log(audio.samplerate);
  ap.saveAudio('haha.wav', audio.wavdataL, audio.wavdataR, audio.samplerate, audio.bitdepth, audio.channels);
  console.log(ap.detectPitch(audio.wavdataL, audio.samplerate, 'acorr'));
  console.log(ap.detectPitch(audio.wavdataL, audio.samplerate, 'yin'));
  console.log(ap.detectPitch(audio.wavdataL, audio.samplerate, 'mpm'));
  // console.log(ap.detectPitch(audio.wavdataL, audio.samplerate, 'goertzel'));
  // console.log(ap.detectPitch(audio.wavdataL, audio.samplerate, 'dft'));

  let ampfreq = await ap.ampfreq(audio.wavdataL, audio.samplerate);
  // console.log('ampfreq=', ampfreq);

  let data = new Float32Array([0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19]);
  // console.log(data);

  let freq_data = await ap.fft(data);
  // console.log(freq_data);

  let td_data = await ap.ifft(freq_data.real, freq_data.imag);
  // console.log(td_data);
/*
  // Test MFCCs
  audio = await ap.readAudio('./wav/OSR_us_000_0010_8k.wav');
  // console.log(audio2.samplerate);
  let mfcc_data = await ap.mfcc(audio.wavdataL, audio.samplerate, 40, 0, 3500, 25, 10, 0.97);
  // console.log(mfcc_data);
 */
  var fs = require("fs");
  fs.readFile("./wav/sample.amr", async function (err, data) {
    if (err) throw err;
    let pcm_data = await ap.amr2pcm(data, data.length);
    ap.saveAudio('sample.wav', pcm_data.pcm, pcm_data.pcm, pcm_data.samplerate, pcm_data.bitdepth, 1);
  });
}

test();
