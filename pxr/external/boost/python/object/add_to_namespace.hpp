//
// Copyright 2024 Pixar
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
// Copyright David Abrahams 2002.
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
#ifndef PXR_EXTERNAL_BOOST_PYTHON_OBJECT_ADD_TO_NAMESPACE_HPP
# define PXR_EXTERNAL_BOOST_PYTHON_OBJECT_ADD_TO_NAMESPACE_HPP

#include "pxr/pxr.h"
#include "pxr/external/boost/python/common.hpp"

#ifndef PXR_USE_INTERNAL_BOOST_PYTHON
#include <boost/python/object/add_to_namespace.hpp>
#else

# include "pxr/external/boost/python/object_fwd.hpp"

namespace PXR_BOOST_NAMESPACE { namespace python { namespace objects { 

//
// A setattr that's "smart" about function overloading (and docstrings).
//
PXR_BOOST_PYTHON_DECL void add_to_namespace(
    object const& name_space, char const* name, object const& attribute);

PXR_BOOST_PYTHON_DECL void add_to_namespace(
    object const& name_space, char const* name, object const& attribute, char const* doc);

}}} // namespace PXR_BOOST_NAMESPACE::python::objects

#endif // PXR_USE_INTERNAL_BOOST_PYTHON
#endif // PXR_EXTERNAL_BOOST_PYTHON_OBJECT_ADD_TO_NAMESPACE_HPP
