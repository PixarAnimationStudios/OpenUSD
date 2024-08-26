//
// Copyright 2024 Pixar
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
// Copyright David Abrahams 2002.
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
#ifndef PXR_EXTERNAL_BOOST_PYTHON_OBJECT_ITERATOR_CORE_HPP
# define PXR_EXTERNAL_BOOST_PYTHON_OBJECT_ITERATOR_CORE_HPP

#include "pxr/pxr.h"

#ifndef PXR_USE_INTERNAL_BOOST_PYTHON
#include <boost/python/object/iterator_core.hpp>
#else

# include "pxr/external/boost/python/object_fwd.hpp"

namespace boost { namespace python { namespace objects {

BOOST_PYTHON_DECL object const& identity_function();
BOOST_PYTHON_DECL void stop_iteration_error();

}}} // namespace boost::python::object

#endif // PXR_USE_INTERNAL_BOOST_PYTHON
#endif // PXR_EXTERNAL_BOOST_PYTHON_OBJECT_ITERATOR_CORE_HPP
