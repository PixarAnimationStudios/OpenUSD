//
// Copyright 2024 Pixar
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
// Copyright David Abrahams 2002.
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
#ifndef PXR_EXTERNAL_BOOST_PYTHON_CONVERTER_PYOBJECT_TYPE_HPP
# define PXR_EXTERNAL_BOOST_PYTHON_CONVERTER_PYOBJECT_TYPE_HPP

#include "pxr/pxr.h"
#include "pxr/external/boost/python/common.hpp"

#ifndef PXR_USE_INTERNAL_BOOST_PYTHON
#include <boost/python/converter/pyobject_type.hpp>
#else

# include "pxr/external/boost/python/cast.hpp"

namespace PXR_BOOST_NAMESPACE { namespace python { namespace converter { 

PXR_BOOST_PYTHON_DECL inline
PyObject* checked_downcast_impl(PyObject *obj, PyTypeObject *type)
{
  return (PyType_IsSubtype(Py_TYPE(obj), type) ? obj : NULL);
}
// Used as a base class for specializations which need to provide
// Python type checking capability.
template <class Object, PyTypeObject* pytype>
struct pyobject_type 
{
    static bool check(PyObject* x)
    {
        return ::PyObject_IsInstance(x, (PyObject*)pytype);
    }

    static Object* checked_downcast(PyObject* x)
    {
        return python::downcast<Object>(
            (checked_downcast_impl)(x, pytype)
            );
    }
#ifndef PXR_BOOST_PYTHON_NO_PY_SIGNATURES
    static PyTypeObject const* get_pytype() { return pytype; }
#endif
};

}}} // namespace PXR_BOOST_NAMESPACE::python::converter

#endif // PXR_USE_INTERNAL_BOOST_PYTHON
#endif // PXR_EXTERNAL_BOOST_PYTHON_CONVERTER_PYOBJECT_TYPE_HPP
