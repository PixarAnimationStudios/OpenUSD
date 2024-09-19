//
// Copyright 2024 Pixar
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
// Copyright David Abrahams 2002.
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
# ifndef PXR_EXTERNAL_BOOST_PYTHON_CALL_METHOD_HPP
#  define PXR_EXTERNAL_BOOST_PYTHON_CALL_METHOD_HPP

#include "pxr/pxr.h"
#include "pxr/external/boost/python/common.hpp"

#ifndef PXR_USE_INTERNAL_BOOST_PYTHON
#include <boost/python/call_method.hpp>
#else

# include "pxr/external/boost/python/detail/prefix.hpp"

#  include "pxr/external/boost/python/converter/arg_to_python.hpp"
#  include "pxr/external/boost/python/converter/return_from_python.hpp"
#  include "pxr/external/boost/python/detail/preprocessor.hpp"
#  include "pxr/external/boost/python/detail/void_return.hpp"

namespace PXR_BOOST_NAMESPACE { namespace python {

template <class R, class... A>
typename detail::returnable<R>::type
call_method(PyObject* self, char const* name, A const&... a)
{
    PyObject* const result = 
        PyObject_CallMethodObjArgs(
            self
            , handle<>(PyUnicode_FromString(name)).get()
            , converter::arg_to_python<A>(a).get()...
            , NULL);
    
    // This conversion *must not* be done in the same expression as
    // the call, because, in the special case where the result is a
    // reference a Python object which was created by converting a C++
    // argument for passing to PyObject_CallFunction, its reference
    // count will be 2 until the end of the full expression containing
    // the conversion, and that interferes with dangling
    // pointer/reference detection.
    converter::return_from_python<R> converter;
    return converter(result);
}

}} // namespace PXR_BOOST_NAMESPACE::python


#endif // PXR_USE_INTERNAL_BOOST_PYTHON
# endif // PXR_EXTERNAL_BOOST_PYTHON_CALL_METHOD_HPP
