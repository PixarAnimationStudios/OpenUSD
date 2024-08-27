//
// Copyright 2024 Pixar
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
// Copyright David Abrahams 2002.
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
#ifndef PXR_EXTERNAL_BOOST_PYTHON_OBJECT_VALUE_HOLDER_FWD_HPP
# define PXR_EXTERNAL_BOOST_PYTHON_OBJECT_VALUE_HOLDER_FWD_HPP

#include "pxr/pxr.h"
#include "pxr/external/boost/python/common.hpp"

#ifndef PXR_USE_INTERNAL_BOOST_PYTHON
#include <boost/python/object/value_holder_fwd.hpp>
#else

namespace PXR_BOOST_NAMESPACE { namespace python { namespace objects { 

struct no_back_reference;

template <class CallbackType = no_back_reference> struct value_holder_generator;

}}} // namespace PXR_BOOST_NAMESPACE::python::object

#endif // PXR_USE_INTERNAL_BOOST_PYTHON
#endif // PXR_EXTERNAL_BOOST_PYTHON_OBJECT_VALUE_HOLDER_FWD_HPP
