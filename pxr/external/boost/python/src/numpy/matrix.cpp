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
#include "pxr/external/boost/python/numpy/matrix.hpp"

namespace PXR_BOOST_NAMESPACE { namespace python { namespace numpy
{
namespace detail 
{
inline object get_matrix_type()
{
  object module = import("numpy");
  return module.attr("matrix");
}
} // namespace PXR_BOOST_NAMESPACE::python::numpy::detail
} // namespace PXR_BOOST_NAMESPACE::python::numpy

namespace converter 
{

PyTypeObject const * object_manager_traits<numpy::matrix>::get_pytype()
{
  return reinterpret_cast<PyTypeObject*>(numpy::detail::get_matrix_type().ptr());
}

} // namespace PXR_BOOST_NAMESPACE::python::converter

namespace numpy 
{

object matrix::construct(object const & obj, dtype const & dt, bool copy)
{
  return numpy::detail::get_matrix_type()(obj, dt, copy);
}

object matrix::construct(object const & obj, bool copy)
{
  return numpy::detail::get_matrix_type()(obj, object(), copy);
}

matrix matrix::view(dtype const & dt) const 
{
  return matrix(python::detail::new_reference
    (PyObject_CallMethod(this->ptr(), const_cast<char*>("view"), const_cast<char*>("O"), dt.ptr())));
}

matrix matrix::copy() const 
{
  return matrix(python::detail::new_reference
    (PyObject_CallMethod(this->ptr(), const_cast<char*>("copy"), const_cast<char*>(""))));
}

matrix matrix::transpose() const 
{
  return matrix(extract<matrix>(ndarray::transpose()));
}

}}} // namespace PXR_BOOST_NAMESPACE::python::numpy
