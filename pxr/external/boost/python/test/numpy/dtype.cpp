//
// Copyright 2024 Pixar
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
// Copyright Jim Bosch & Ankit Daftery 2010-2012.
// Copyright Stefan Seefeld 2016.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#include "pxr/external/boost/python/numpy.hpp"
#include <cstdint>

namespace p = PXR_BOOST_NAMESPACE::python;
namespace np = PXR_BOOST_NAMESPACE::python::numpy;

template <typename T>
np::dtype accept(T) {
  return np::dtype::get_builtin<T>();
}

PXR_BOOST_PYTHON_MODULE(dtype_ext)
{
  np::initialize();
  // wrap dtype equivalence test, since it isn't available in Python API.
  p::def("equivalent", np::equivalent);
  // integers, by number of bits
  p::def("accept_int8", accept<int8_t>);
  p::def("accept_uint8", accept<uint8_t>);
  p::def("accept_int16", accept<int16_t>);
  p::def("accept_uint16", accept<uint16_t>);
  p::def("accept_int32", accept<int32_t>);
  p::def("accept_uint32", accept<uint32_t>);
  p::def("accept_int64", accept<int64_t>);
  p::def("accept_uint64", accept<uint64_t>);
  // integers, by C name according to NumPy
  p::def("accept_bool_", accept<bool>);
  p::def("accept_byte", accept<signed char>);
  p::def("accept_ubyte", accept<unsigned char>);
  p::def("accept_short", accept<short>);
  p::def("accept_ushort", accept<unsigned short>);
  p::def("accept_intc", accept<int>);
  p::def("accept_uintc", accept<unsigned int>);
  // floats and complex
  p::def("accept_float32", accept<float>);
  p::def("accept_complex64", accept< std::complex<float> >);
  p::def("accept_float64", accept<double>);
  p::def("accept_complex128", accept< std::complex<double> >);
  if (sizeof(long double) > sizeof(double)) {
      p::def("accept_longdouble", accept<long double>);
      p::def("accept_clongdouble", accept< std::complex<long double> >);
  }
}
