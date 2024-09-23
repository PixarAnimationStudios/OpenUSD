#if !defined(BOOST_PP_IS_ITERATING)

//
// Copyright 2024 Pixar
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
// Copyright David Abrahams 2002.
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

# ifndef PXR_EXTERNAL_BOOST_PYTHON_CALL_HPP
#  define PXR_EXTERNAL_BOOST_PYTHON_CALL_HPP

#include "pxr/pxr.h"
#include "pxr/external/boost/python/common.hpp"

#ifndef PXR_USE_INTERNAL_BOOST_PYTHON
#include <boost/python/call.hpp>
#else

# include "pxr/external/boost/python/detail/prefix.hpp"

#  include <boost/type.hpp>

#  include "pxr/external/boost/python/converter/arg_to_python.hpp"
#  include "pxr/external/boost/python/converter/return_from_python.hpp"
#  include "pxr/external/boost/python/detail/preprocessor.hpp"
#  include "pxr/external/boost/python/detail/void_return.hpp"

#  include <boost/preprocessor/comma_if.hpp>
#  include <boost/preprocessor/iterate.hpp>
#  include <boost/preprocessor/repeat.hpp>
#  include <boost/preprocessor/debug/line.hpp>
#  include <boost/preprocessor/repetition/enum_trailing_params.hpp>
#  include <boost/preprocessor/repetition/enum_binary_params.hpp>

namespace PXR_BOOST_NAMESPACE { namespace python {

# define PXR_BOOST_PYTHON_FAST_ARG_TO_PYTHON_GET(z, n, _) \
    , converter::arg_to_python<A##n>(a##n).get()

#  define BOOST_PP_ITERATION_PARAMS_1 (3, (0, PXR_BOOST_PYTHON_MAX_ARITY, "pxr/external/boost/python/call.hpp"))
#  include BOOST_PP_ITERATE()

#  undef PXR_BOOST_PYTHON_FAST_ARG_TO_PYTHON_GET

}} // namespace PXR_BOOST_NAMESPACE::python

#endif // PXR_USE_INTERNAL_BOOST_PYTHON
# endif // PXR_EXTERNAL_BOOST_PYTHON_CALL_HPP

// For gcc 4.4 compatability, we must include the
// BOOST_PP_ITERATION_DEPTH test inside an #else clause.
#else // BOOST_PP_IS_ITERATING
#if BOOST_PP_ITERATION_DEPTH() == 1
#  line BOOST_PP_LINE(__LINE__, call.hpp)

# define N BOOST_PP_ITERATION()

template <
    class R
    BOOST_PP_ENUM_TRAILING_PARAMS_Z(1, N, class A)
    >
typename detail::returnable<R>::type
call(PyObject* callable
    BOOST_PP_COMMA_IF(N) BOOST_PP_ENUM_BINARY_PARAMS_Z(1, N, A, const& a)
    , boost::type<R>* = 0
    )
{
    PyObject* const result = 
        PyObject_CallFunction(
            callable
            , const_cast<char*>("(" BOOST_PP_REPEAT_1ST(N, PXR_BOOST_PYTHON_FIXED, "O") ")")
            BOOST_PP_REPEAT_1ST(N, PXR_BOOST_PYTHON_FAST_ARG_TO_PYTHON_GET, nil)
            );
    
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

# undef N

#endif // BOOST_PP_ITERATION_DEPTH()
#endif
