# Audio-Processing

A handy nodejs package for audio processing. For example, it can extract frequencies from the audio and compute the pitch.

The source code can be found on [https://github.com/fengzhang2011/audio-processing](feng_a://github.com/fengzhang2011/audio-processing).

## 1. HOW TO USE

### 1.1 A simple test on the library

Just execute the following command.
```bash
$ cd build
$ cmake ..
$ make
$ ./audio_processing
```

### 1.2 Use it in the Javascript.

This package has been published into the npm repository. Therefore, it becomes very easy to use this code.
```bash
$ mkdir your_project
$ cd your_project
$ npm init
$ npm install audio-processing
```

NOTE:

If you encounter some issues like ```permission denied``` while installing it, especially in a docker container, try the following command.

```bash
# npm config set unsafe-perm true
```
Reason: The ```unsafe-perm``` boolean set to true to suppress the UID/GID switching when running package scripts.

Nhow you could use the code. The example code is as follows.

```javascript
const ap = require('audio-processing');

console.log(ap.hello());

async function test() {
let audio = await ap.readAudio("./wav/female.wav");
  console.log(audio.samplerate);
  ap.saveAudio("haha.wav", audio.wavdataL, audio.wavdataR, audio.samplerate, audio.bitdepth, audio.channels);
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
}

test();
```

## 2. CREDITS

This code uses the [FFTS](https://github.com/anthonix/ffts.git), [Pitch-Detection](https://github.com/sevagh/pitch-detection.git), and [AudioFile](https://github.com/adamstark/AudioFile).
Thanks for their great work.

The detailed versions in use are as follows:

1. FFTS@[fe86885ecafd0d16eb122f3212403d1d5a86e24e](https://github.com/anthonix/ffts/tree/fe86885ecafd0d16eb122f3212403d1d5a86e24e) [Jun-17-2017]
2. Pitch-Detection@[7799a623c30ede739cb8d3c8fa3e0a9e5b200b58](https://github.com/sevagh/pitch-detection/tree/7799a623c30ede739cb8d3c8fa3e0a9e5b200b58) [Oct-07-2018]
3. AudioFile@[a6430a05c859e43f3379e0fe078ae4e094d71602](https://github.com/adamstark/AudioFile/tree/a6430a05c859e43f3379e0fe078ae4e094d71602) [Jun-06-2017]

### 2.1 Compile the FFTS static library

```bash
$ git clone https://github.com/anthonix/ffts.git
$ cd ffts
$ echo "set(CMAKE_C_FLAGS \"\${CMAKE_C_FLAGS} -fPIC\")" >> CMakeLists.txt
$ mkdir build
$ cd build
$ cmake ..
$ make ffts_static
```

NOTE:
  We must enable the ```-fPIC``` flag when compiling the ffts static library, to enable the "Position Independent Code". Otherwise, it will generate the follow error:
```
/usr/bin/ld: ../lib/libffts.a(ffts.c.o): relocation R_X86_64_32S against `.text' can not be used when making a shared object; recompile with -fPIC
../lib/libffts.a: error adding symbols: Bad value
collect2: error: ld returned 1 exit status
make: *** [Release/obj.target/ssrc.node] Error 1
```



Once these commands are done, the ```libffts.a``` will be generated under the ```build``` folder.

Copy the header file ```include/ffts.h``` and ```libffts.a``` to ```./include``` and ```./lib``` folders, respectively, into this repository.


### 2.2 Compile the Pitch-Detection static library

```bash
$ git clone https://github.com/sevagh/pitch-detection.git
$ cd pitch-detection
$ mkdir build
$ cd build
$ g++ -I../include -ansi -pedantic -Werror -Wall -O3 -std=c++17 -fPIC -fext-numeric-literals -ffast-math -c ../src/*.cpp
$ ar rcs --plugin $(gcc --print-file-name=liblto_plugin.so) libpitch_detection.a *.o
```

Once these commands are done, the ```libpitch_detection.a``` will be generated under the ```build``` folder.

Copy the header file ```include/pitch_detection.h``` and ```libpitch_detection.a``` to ```./include``` and ```./lib``` folders, respectively, into this repository.


### 2.3 Copy the AudioFile source code

```bash
$ git clone https://github.com/adamstark/AudioFile
$ cd AudioFile
$ mkdir build
$ cd build
$ g++ -ansi -pedantic -Werror -O3 -std=c++17 -fPIC -fext-numeric-literals -ffast-math -c ../*.cpp
$ ar rcs libaudiofile.a *.o
```

Once these commands are done, the ```libaudiofile.a``` will be generated under the ```build``` folder.

Copy the header file ```src/AudioFile.h``` and ```libaudiofile.a``` to ```./include``` and ```./lib``` folders, respectively, into this repository.

### 2.4 MFCC compuation

The code are written based on these great documents. Thanks for the authors.

[Mel Frequency Cepstral Coefficient (MFCC) tutorial. James Lyons. ](http://practicalcryptography.com/miscellaneous/machine-learning/guide-mel-frequency-cepstral-coefficients-mfccs)

[Speech Processing for Machine Learning: Filter banks, Mel-Frequency Cepstral Coefficients (MFCCs) and What's In-Between. Haytham Fayek. ](https://haythamfayek.com/2016/04/21/speech-processing-for-machine-learning.html)

[Hamming Window, SPECTRAL AUDIO SIGNAL PROCESSING. Julius Orion Smith III. ](https://ccrma.stanford.edu/~jos/sasp/Hamming_Window.html)

### 2.5 AMR WB/NB

### 2.6 MP3 decoder

[https://github.com/lieff/minimp3](https://github.com/lieff/minimp3)

### 2.7 Weiner filter for Noise Reduction and speech enhancement.

Pascal Scalart. [Wiener Noise Suppressor based on Decision-Directed method with TSNR and HRNR algorithms.](https://www.mathworks.com/matlabcentral/fileexchange/24462-wiener-filter-for-noise-reduction-and-speech-enhancement)


