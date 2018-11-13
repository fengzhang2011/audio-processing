cmake_minimum_required (VERSION 2.6)

project (audio_processing)

include_directories(./include)

#set(CMAKE_CXX_FLAGS "-ansi -pedantic -Werror -Wall -O3 -std=c++17 -fPIC -fext-numeric-literals -ffast-math")
set(CMAKE_CXX_FLAGS "-std=c++17")

add_executable(audio_processing ./src/example.cpp ./src/mfcc.cpp ./src/amr.cpp ./src/minimp3.cpp)

# audiofile library
add_library(audiofile STATIC IMPORTED)
set_target_properties(audiofile PROPERTIES
  IMPORTED_LOCATION "${CMAKE_SOURCE_DIR}/lib/libaudiofile.a"
  INTERFACE_INCLUDE_DIRECTORIES "${CMAKE_SOURCE_DIR}/include"
)

# ffts library
add_library(ffts STATIC IMPORTED)
set_target_properties(ffts PROPERTIES
  IMPORTED_LOCATION "${CMAKE_SOURCE_DIR}/lib/libffts.a"
  INTERFACE_INCLUDE_DIRECTORIES "${CMAKE_SOURCE_DIR}/include"
)

# pitch_detection library
add_library(pitch_detection STATIC IMPORTED)
set_target_properties(pitch_detection PROPERTIES
  IMPORTED_LOCATION "${CMAKE_SOURCE_DIR}/lib/libpitch_detection.a"
  INTERFACE_INCLUDE_DIRECTORIES "${CMAKE_SOURCE_DIR}/include"
)

# OpenCore AMR NB library
add_library(opencore_amr_nb STATIC IMPORTED)
set_target_properties(opencore_amr_nb PROPERTIES
  IMPORTED_LOCATION "${CMAKE_SOURCE_DIR}/lib/libopencore-amrnb.a"
  INTERFACE_INCLUDE_DIRECTORIES "${CMAKE_SOURCE_DIR}/include"
)

# OpenCore AMR WB library
add_library(opencore_amr_wb STATIC IMPORTED)
set_target_properties(opencore_amr_wb PROPERTIES
  IMPORTED_LOCATION "${CMAKE_SOURCE_DIR}/lib/libopencore-amrwb.a"
  INTERFACE_INCLUDE_DIRECTORIES "${CMAKE_SOURCE_DIR}/include"
)


target_link_libraries(audio_processing audiofile pitch_detection ffts opencore_amr_nb opencore_amr_wb)
