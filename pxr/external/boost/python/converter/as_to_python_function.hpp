//
// Copyright 2024 Pixar
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
// Copyright David Abrahams 2002.
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
#ifndef PXR_EXTERNAL_BOOST_PYTHON_CONVERTER_AS_TO_PYTHON_FUNCTION_HPP
# define PXR_EXTERNAL_BOOST_PYTHON_CONVERTER_AS_TO_PYTHON_FUNCTION_HPP

#include "pxr/pxr.h"
#include "pxr/external/boost/python/common.hpp"

#ifndef PXR_USE_INTERNAL_BOOST_PYTHON
#include <boost/python/converter/as_to_python_function.hpp>
#else
# include "pxr/external/boost/python/converter/to_python_function_type.hpp"

namespace PXR_BOOST_NAMESPACE { namespace python { namespace converter { 

// Given a typesafe to_python conversion function, produces a
// to_python_function_t which can be registered in the usual way.
template <class T, class ToPython>
struct as_to_python_function
{
    // Assertion functions used to prevent wrapping of converters
    // which take non-const reference parameters. The T* argument in
    // the first overload ensures it isn't used in case T is a
    // reference.
    template <class U>
    static void convert_function_must_take_value_or_const_reference(U(*)(T), int, T* = 0) {}
    template <class U>
    static void convert_function_must_take_value_or_const_reference(U(*)(T const&), long ...) {}
        
    static PyObject* convert(void const* x)
    {
        convert_function_must_take_value_or_const_reference(&ToPython::convert, 1L);
        
        // Yes, the const_cast below opens a hole in const-correctness,
        // but it's needed to convert auto_ptr<U> to python.
        //
        // How big a hole is it?  It allows ToPython::convert() to be
        // a function which modifies its argument. The upshot is that
        // client converters applied to const objects may invoke
        // undefined behavior. The damage, however, is limited by the
        // use of the assertion function. Thus, the only way this can
        // modify its argument is if T is an auto_ptr-like type. There
        // is still a const-correctness hole w.r.t. auto_ptr<U> const,
        // but c'est la vie.
        return ToPython::convert(*const_cast<T*>(static_cast<T const*>(x)));
    }
#ifndef PXR_BOOST_PYTHON_NO_PY_SIGNATURES
    static PyTypeObject const * get_pytype() { return ToPython::get_pytype(); }
#endif
};

}}} // namespace PXR_BOOST_NAMESPACE::python::converter

#endif // PXR_USE_INTERNAL_BOOST_PYTHON
#endif // PXR_EXTERNAL_BOOST_PYTHON_CONVERTER_AS_TO_PYTHON_FUNCTION_HPP
