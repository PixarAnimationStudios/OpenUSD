//
// Copyright 2024 Pixar
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
// Copyright David Abrahams 2001.
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
#ifndef PXR_EXTERNAL_BOOST_PYTHON_OBJECT_CLASS_WRAPPER_HPP
# define PXR_EXTERNAL_BOOST_PYTHON_OBJECT_CLASS_WRAPPER_HPP

#include "pxr/pxr.h"
#include "pxr/external/boost/python/common.hpp"

#ifndef PXR_USE_INTERNAL_BOOST_PYTHON
#include <boost/python/object/class_wrapper.hpp>
#else

# include "pxr/external/boost/python/to_python_converter.hpp"
#ifndef PXR_BOOST_PYTHON_NO_PY_SIGNATURES
# include "pxr/external/boost/python/converter/pytype_function.hpp"
#endif
# include <boost/ref.hpp>

namespace PXR_BOOST_NAMESPACE { namespace python { namespace objects { 

//
// These two classes adapt the static execute function of a class
// MakeInstance execute() function returning a new PyObject*
// reference. The first one is used for class copy constructors, and
// the second one is used to handle smart pointers.
//

template <class Src, class MakeInstance>
struct class_cref_wrapper
    : to_python_converter<Src,class_cref_wrapper<Src,MakeInstance> ,true>
{
    static PyObject* convert(Src const& x)
    {
        return MakeInstance::execute(boost::ref(x));
    }
#ifndef PXR_BOOST_PYTHON_NO_PY_SIGNATURES
    static PyTypeObject const *get_pytype() { return converter::registered_pytype_direct<Src>::get_pytype(); }
#endif
};

template <class Src, class MakeInstance>
struct class_value_wrapper
    : to_python_converter<Src,class_value_wrapper<Src,MakeInstance> ,true>
{
    static PyObject* convert(Src x)
    {
        return MakeInstance::execute(x);
    }
#ifndef PXR_BOOST_PYTHON_NO_PY_SIGNATURES
    static PyTypeObject const *get_pytype() { return MakeInstance::get_pytype(); }
#endif
};

}}} // namespace PXR_BOOST_NAMESPACE::python::objects

#endif // PXR_USE_INTERNAL_BOOST_PYTHON
#endif // PXR_EXTERNAL_BOOST_PYTHON_OBJECT_CLASS_WRAPPER_HPP
