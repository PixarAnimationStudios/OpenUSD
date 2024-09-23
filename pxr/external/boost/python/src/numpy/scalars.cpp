//
// Copyright 2024 Pixar
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
// Copyright Jim Bosch 2010-2012.
// Copyright Stefan Seefeld 2016.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#define PXR_BOOST_PYTHON_NUMPY_INTERNAL
#include "pxr/external/boost/python/numpy/internal.hpp"

namespace PXR_BOOST_NAMESPACE { namespace python {
namespace converter 
{
NUMPY_OBJECT_MANAGER_TRAITS_IMPL(PyVoidArrType_Type, numpy::void_)
} // namespace PXR_BOOST_NAMESPACE::python::converter

namespace numpy 
{

void_::void_(Py_ssize_t size)
  : object(python::detail::new_reference
      (PyObject_CallFunction((PyObject*)&PyVoidArrType_Type, const_cast<char*>("i"), size)))
{}

void_ void_::view(dtype const & dt) const 
{
  return void_(python::detail::new_reference
    (PyObject_CallMethod(this->ptr(), const_cast<char*>("view"), const_cast<char*>("O"), dt.ptr())));
}

void_ void_::copy() const 
{
  return void_(python::detail::new_reference
    (PyObject_CallMethod(this->ptr(), const_cast<char*>("copy"), const_cast<char*>(""))));
}

}}} // namespace PXR_BOOST_NAMESPACE::python::numpy
