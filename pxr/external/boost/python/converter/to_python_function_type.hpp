//
// Copyright 2024 Pixar
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
// Copyright David Abrahams 2002.
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
#ifndef PXR_EXTERNAL_BOOST_PYTHON_CONVERTER_TO_PYTHON_FUNCTION_TYPE_HPP
# define PXR_EXTERNAL_BOOST_PYTHON_CONVERTER_TO_PYTHON_FUNCTION_TYPE_HPP
# include "pxr/external/boost/python/detail/prefix.hpp"
# include <boost/static_assert.hpp>

namespace boost { namespace python { namespace converter { 

// The type of stored function pointers which actually do conversion
// by-value. The void* points to the object to be converted, and
// type-safety is preserved through runtime registration.
typedef PyObject* (*to_python_function_t)(void const*);

}}} // namespace boost::python::converter

#endif // PXR_EXTERNAL_BOOST_PYTHON_CONVERTER_TO_PYTHON_FUNCTION_TYPE_HPP
