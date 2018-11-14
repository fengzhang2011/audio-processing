{
  "targets": [
    {
      "target_name": "feng_ap",
      "cflags_cc": [ "-ansi -pedantic -Werror -Wall -O3 -std=c++17 -fPIC -fext-numeric-literals -ffast-math -static" ],
      "ldflags": [ ],
      "libraries": [
        "../lib/libpitch_detection.a",
        "../lib/libffts.a",
        "../lib/libopencore-amrnb.a",
        "../lib/libopencore-amrwb.a",
        "../lib/libaudiofile.a"
      ],
      "sources": [
        "src/napi_module.cpp",
        "src/napi_audiofile.cpp",
        "src/napi_ampfreq.cpp",
        "src/napi_pitch.cpp",
        "src/napi_fft.cpp",
        "src/napi_mfcc.cpp",
        "src/mfcc.cpp",
        "src/napi_amr.cpp",
        "src/amr.cpp",
        "src/minimp3.cpp",
        "src/napi_mp3.cpp",
        "src/denoise.cpp"
      ],
      "include_dirs": [
        "./include"
      ]
    }
  ]
}
