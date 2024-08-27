//
// Copyright 2024 Pixar
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
// Copyright David Abrahams 2002.
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
#ifndef PXR_EXTERNAL_BOOST_PYTHON_OBJECT_FUNCTION_OBJECT_HPP
# define PXR_EXTERNAL_BOOST_PYTHON_OBJECT_FUNCTION_OBJECT_HPP

#include "pxr/pxr.h"
#include "pxr/external/boost/python/common.hpp"

#ifndef PXR_USE_INTERNAL_BOOST_PYTHON
#include <boost/python/object/function_object.hpp>
#else
# include "pxr/external/boost/python/detail/prefix.hpp"
# include <boost/function/function2.hpp>
# include "pxr/external/boost/python/object_core.hpp"
# include "pxr/external/boost/python/args_fwd.hpp"
# include "pxr/external/boost/python/object/py_function.hpp"

namespace PXR_BOOST_NAMESPACE { namespace python {

namespace objects
{ 
  PXR_BOOST_PYTHON_DECL api::object function_object(
      py_function const& f
      , python::detail::keyword_range const&);

  PXR_BOOST_PYTHON_DECL api::object function_object(
      py_function const& f
      , python::detail::keyword_range const&);

  PXR_BOOST_PYTHON_DECL api::object function_object(py_function const& f);

  // Add an attribute to the name_space with the given name. If it is
  // a Boost.Python function object
  // (boost/python/object/function.hpp), and an existing function is
  // already there, add it as an overload.
  PXR_BOOST_PYTHON_DECL void add_to_namespace(
      object const& name_space, char const* name, object const& attribute);

  PXR_BOOST_PYTHON_DECL void add_to_namespace(
      object const& name_space, char const* name, object const& attribute, char const* doc);
}

}} // namespace PXR_BOOST_NAMESPACE::python::objects

#endif // PXR_USE_INTERNAL_BOOST_PYTHON
#endif // PXR_EXTERNAL_BOOST_PYTHON_OBJECT_FUNCTION_OBJECT_HPP
