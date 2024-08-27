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
#include "pxr/external/boost/python/slice.hpp"

namespace p = PXR_BOOST_NAMESPACE::python;
namespace np = PXR_BOOST_NAMESPACE::python::numpy;

p::object single(np::ndarray ndarr, int i) { return ndarr[i];}
p::object slice(np::ndarray ndarr, p::slice sl) { return ndarr[sl];}
p::object indexarray(np::ndarray ndarr, np::ndarray d1) { return ndarr[d1];}
p::object indexarray_2d(np::ndarray ndarr, np::ndarray d1,np::ndarray d2) { return ndarr[p::make_tuple(d1,d2)];}
p::object indexslice(np::ndarray ndarr, np::ndarray d1, p::slice sl) { return ndarr[p::make_tuple(d1, sl)];}

PXR_BOOST_PYTHON_MODULE(indexing_ext)
{
  np::initialize();
  p::def("single", single);
  p::def("slice", slice);
  p::def("indexarray", indexarray);
  p::def("indexarray", indexarray_2d);
  p::def("indexslice", indexslice);

}
