{
  "targets": [
    {
      "target_name": "feng_ap",
      "cflags_cc": [ "-ansi -pedantic -Werror -Wall -O3 -std=c++17 -fPIC -fext-numeric-literals -ffast-math" ],
      "ldflags": [ ],
      "libraries": [
        "../lib/libaudiofile.a",
        "../lib/libpitch_detection.a",
        "../lib/libffts.a"
      ],
      "sources": [
        "src/napi_module.cpp",
        "src/napi_audiofile.cpp",
        "src/napi_ampfreq.cpp",
        "src/napi_pitch.cpp",
        "src/napi_fft.cpp",
        "src/napi_mfcc.cpp",
        "src/mfcc.cpp"
      ],
      "include_dirs": [
        "./include"
      ]
    }
  ]
}
