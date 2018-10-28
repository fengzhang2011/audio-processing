/*************************************************
 *
 * Do the FFT/IFFT on the data.
 *
 * Author: Feng Zhang (zhjinf@gmail.com)
 * Date: 2018-10-13
 * 
 * Copyright:
 *   See LICENSE.
 * 
 ************************************************/

#include <complex>
#include <vector>
#include <stdio.h>

#include "ffts.h"

#include "napi_fft.h"
#include "napi_common.h"



void doFFT(const std::vector<float> &data, std::vector<float> &real, std::vector<float> &imag)
{
  size_t N = data.size();

  std::vector<std::complex<float>> signalb_ext(N);
  for(size_t i=0; i<N; i++)
    signalb_ext[i] = {float(data[i]), 0.0};

  std::vector<std::complex<float>> out(N);

  auto fft_forward = ffts_init_1d(N, FFTS_FORWARD);
  ffts_execute(fft_forward, signalb_ext.data(), out.data());

  for(size_t i=0; i<N; i++)
  {
    real[i] = out[i].real();
    imag[i] = out[i].imag();
  }

  ffts_free(fft_forward);
}

void doIFFT(const std::vector<float> &data_real, const std::vector<float> &data_imag, std::vector<float> &td_data)
{
  size_t N = data_real.size();
  if ( N!=data_imag.size() )
  {
    return; // error, the real/imaginary parts must have equal length;
  }

  std::vector<std::complex<float>> freq_data(N);
  for(size_t i=0; i<N; i++)
  {
    freq_data[i] = {data_real[i], data_imag[i]};
    // printf("%f + %f i\n", data_real[i], data_imag[i]);
  }

  std::vector<std::complex<float>> out(N);

  auto fft_backward = ffts_init_1d(N, FFTS_BACKWARD);
  ffts_execute(fft_backward, freq_data.data(), out.data());

  for(size_t i=0; i<N; i++)
  {
    td_data[i] = out[i].real()/N; // Must divide the value by its length N.
  }

  ffts_free(fft_backward);
}

// Do the FFT transformation
// arg[0]: data
napi_value fft(napi_env env, napi_callback_info args)
{
  napi_value result;
  napi_deferred deferred;
  napi_value promise;

  napi_status status;

  // Create the promise.
  status = napi_create_promise(env, &deferred, &promise);
  if (status != napi_ok) { throwException(env, "Failed to create the promise object."); return nullptr; }

  // Create the resulting object.
  status = napi_create_object(env, &result);
  if (status != napi_ok) return nullptr;

  // Parse the input arguments.
  size_t argc = 1;
  napi_value argv[1];
  status = napi_get_cb_info(env, args, &argc, argv, NULL, NULL);

  // -- Get the data buffer.
  float* dataptr;
  napi_typedarray_type type;
  size_t length;
  napi_value arraybuffer;
  size_t byte_offset;
  status = napi_get_typedarray_info(env, argv[0], &type, &length, (void**) &dataptr, &arraybuffer, &byte_offset);
  if (status != napi_ok) return nullptr;
  // -- Save the data buffer.
  std::vector<float> data(length);
  for (size_t i=0; i<length; i++) data[i] = dataptr[i];

  // Perform the FFT
  std::vector<float> real(length);
  std::vector<float> imag(length);
  doFFT(data, real, imag);

  // Set the return value.
  size_t byte_length = length*sizeof(float);
  byte_offset = 0;
  // REAL part -- First, create the ArrayBuffer.
  float* data_real = NULL;
  status = napi_create_arraybuffer(env, byte_length, (void**)&data_real, &arraybuffer);
  if (status != napi_ok) return nullptr;
  for (size_t i=0; i<length; i++) data_real[i] = real[i];
  // REAL part -- Second, create the TypedArray.
  napi_value array_real;
  status = napi_create_typedarray(env, napi_float32_array, length, arraybuffer, byte_offset, &array_real);
  if (status != napi_ok) return nullptr;
  // IMAGINARY part -- First, create the ArrayBuffer.
  float* data_imag = NULL;
  status = napi_create_arraybuffer(env, byte_length, (void**)&data_imag, &arraybuffer);
  if (status != napi_ok) return nullptr;
  for (size_t i=0; i<length; i++) data_imag[i] = imag[i];
  // IMAGINARY part -- Second, create the TypedArray.
  napi_value array_imag;
  status = napi_create_typedarray(env, napi_float32_array, length, arraybuffer, byte_offset, &array_imag);
  if (status != napi_ok) return nullptr;

  // Set the named property.
  status = napi_set_named_property(env, result, "real", array_real);
  if (status != napi_ok) return nullptr;
  status = napi_set_named_property(env, result, "imag", array_imag);
  if (status != napi_ok) return nullptr;

  status = napi_resolve_deferred(env, deferred, result);
  if (status != napi_ok) { throwException(env, "Failed to set the deferred result."); return nullptr; }

  // At this point the deferred has been freed, so we should assign NULL to it.
  deferred = NULL;

  return promise;
}

// Do the inversed FFT transformation
// arg[0]: freq_data: REAL part
// arg[1]: freq_data: IMAGINARY part
napi_value ifft(napi_env env, napi_callback_info args)
{
  napi_value result;
  napi_deferred deferred;
  napi_value promise;

  napi_status status;

  // Create the promise.
  status = napi_create_promise(env, &deferred, &promise);
  if (status != napi_ok) { throwException(env, "Failed to create the promise object."); return nullptr; }

  // Create the resulting object.
  status = napi_create_object(env, &result);
  if (status != napi_ok) return nullptr;

  // Parse the input arguments.
  size_t argc = 2;
  napi_value argv[2];
  status = napi_get_cb_info(env, args, &argc, argv, NULL, NULL);

  // Get the data buffer.
  float* dataptr;
  napi_typedarray_type type;
  size_t length;
  napi_value arraybuffer;
  size_t byte_offset;
  // -- Get the REAL part.
  status = napi_get_typedarray_info(env, argv[0], &type, &length, (void**) &dataptr, &arraybuffer, &byte_offset);
  if (status != napi_ok) return nullptr;
  std::vector<float> real(length);
  for (size_t i=0; i<length; i++) real[i] = dataptr[i];
  // -- Get the IMAGINARY part.
  status = napi_get_typedarray_info(env, argv[1], &type, &length, (void**) &dataptr, &arraybuffer, &byte_offset);
  if (status != napi_ok) return nullptr;
  std::vector<float> imag(length);
  for (size_t i=0; i<length; i++) imag[i] = dataptr[i];

  // Perform the iFFT
  std::vector<float> td_data(length); // time domain data
  doIFFT(real, imag, td_data);

  // Set the return value.
  size_t byte_length = length*sizeof(float);
  byte_offset = 0;
  // -- First, create the ArrayBuffer.
  dataptr = NULL;
  status = napi_create_arraybuffer(env, byte_length, (void**)&dataptr, &arraybuffer);
  if (status != napi_ok) return nullptr;
  for (size_t i=0; i<length; i++) dataptr[i] = td_data[i];
  // -- Second, create the TypedArray.
  napi_value array_td_data;
  status = napi_create_typedarray(env, napi_float32_array, length, arraybuffer, byte_offset, &array_td_data);
  if (status != napi_ok) return nullptr;

  // Set the named property.
  status = napi_set_named_property(env, result, "data", array_td_data);
  if (status != napi_ok) return nullptr;

  status = napi_resolve_deferred(env, deferred, result);
  if (status != napi_ok) { throwException(env, "Failed to set the deferred result."); return nullptr; }

  // At this point the deferred has been freed, so we should assign NULL to it.
  deferred = NULL;

  return promise;
}

