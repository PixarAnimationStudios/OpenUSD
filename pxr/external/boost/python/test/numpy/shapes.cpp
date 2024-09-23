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

namespace p = PXR_BOOST_NAMESPACE::python;
namespace np = PXR_BOOST_NAMESPACE::python::numpy;

np::ndarray reshape(np::ndarray old_array, p::tuple shape)
{
  np::ndarray local_shape =  old_array.reshape(shape);
  return local_shape;
}

PXR_BOOST_PYTHON_MODULE(shapes_ext)
{
  np::initialize();
  p::def("reshape", reshape);
}
