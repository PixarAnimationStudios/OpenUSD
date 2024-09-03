//
// Copyright 2024 Pixar
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
//  Copyright David Abrahams 2001.
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
#ifndef PXR_EXTERNAL_BOOST_PYTHON_CONVERTER_REGISTRY_HPP
# define PXR_EXTERNAL_BOOST_PYTHON_CONVERTER_REGISTRY_HPP

#include "pxr/pxr.h"
#include "pxr/external/boost/python/common.hpp"

#ifndef PXR_USE_INTERNAL_BOOST_PYTHON
#include <boost/python/converter/registry.hpp>
#else
# include "pxr/external/boost/python/type_id.hpp"
# include "pxr/external/boost/python/converter/to_python_function_type.hpp"
# include "pxr/external/boost/python/converter/rvalue_from_python_data.hpp"
# include "pxr/external/boost/python/converter/constructor_function.hpp"
# include "pxr/external/boost/python/converter/convertible_function.hpp"

namespace PXR_BOOST_NAMESPACE { namespace python { namespace converter {

struct registration;

// This namespace acts as a sort of singleton
namespace registry
{
  // Get the registration corresponding to the type, creating it if necessary
  PXR_BOOST_PYTHON_DECL registration const& lookup(type_info);

  // Get the registration corresponding to the type, creating it if
  // necessary.  Use this first when the type is a shared_ptr.
  PXR_BOOST_PYTHON_DECL registration const& lookup_shared_ptr(type_info);

  // Return a pointer to the corresponding registration, if one exists
  PXR_BOOST_PYTHON_DECL registration const* query(type_info);
  
  PXR_BOOST_PYTHON_DECL void insert(to_python_function_t, type_info, PyTypeObject const* (*to_python_target_type)() = 0);

  // Insert an lvalue from_python converter
  PXR_BOOST_PYTHON_DECL void insert(convertible_function, type_info, PyTypeObject const* (*expected_pytype)() = 0);

  // Insert an rvalue from_python converter
  PXR_BOOST_PYTHON_DECL void insert(
      convertible_function
      , constructor_function
      , type_info
      , PyTypeObject const* (*expected_pytype)()  = 0
      );
  
  // Insert an rvalue from_python converter at the tail of the
  // chain. Used for implicit conversions
  PXR_BOOST_PYTHON_DECL void push_back(
      convertible_function
      , constructor_function
      , type_info
      , PyTypeObject const* (*expected_pytype)()  = 0
      );
}

}}} // namespace PXR_BOOST_NAMESPACE::python::converter

#endif // PXR_USE_INTERNAL_BOOST_PYTHON
#endif // PXR_EXTERNAL_BOOST_PYTHON_CONVERTER_REGISTRY_HPP
