//
// Copyright 2024 Pixar
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
// Copyright David Abrahams 2002.
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
#ifndef PXR_EXTERNAL_BOOST_PYTHON_CONVERTER_CONSTRUCTOR_FUNCTION_HPP
# define PXR_EXTERNAL_BOOST_PYTHON_CONVERTER_CONSTRUCTOR_FUNCTION_HPP

#include "pxr/pxr.h"
#include "pxr/external/boost/python/common.hpp"

#ifndef PXR_USE_INTERNAL_BOOST_PYTHON
#include <boost/python/converter/constructor_function.hpp>
#else

namespace PXR_BOOST_NAMESPACE { namespace python { namespace converter { 

// Declares the type of functions used to construct C++ objects for
// rvalue from_python conversions.
struct rvalue_from_python_stage1_data;
typedef void (*constructor_function)(PyObject* source, rvalue_from_python_stage1_data*);

}}} // namespace PXR_BOOST_NAMESPACE::python::converter

#endif // PXR_USE_INTERNAL_BOOST_PYTHON
#endif // PXR_EXTERNAL_BOOST_PYTHON_CONVERTER_CONSTRUCTOR_FUNCTION_HPP
