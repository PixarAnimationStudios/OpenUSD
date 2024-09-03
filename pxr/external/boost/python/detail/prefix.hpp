//
// Copyright 2024 Pixar
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
// Copyright David Abrahams 2003.
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
#ifndef PXR_EXTERNAL_BOOST_PYTHON_DETAIL_PREFIX_HPP
# define PXR_EXTERNAL_BOOST_PYTHON_DETAIL_PREFIX_HPP

#include "pxr/pxr.h"
#include "pxr/external/boost/python/common.hpp"

#ifndef PXR_USE_INTERNAL_BOOST_PYTHON
#include <boost/python/detail/prefix.hpp>
#else

// The rule is that <Python.h> must be included before any system
// headers (so it can get control over some awful macros).
// Unfortunately, Boost.Python needs to #include <limits.h> first, at
// least... but this gets us as close as possible.

# include "pxr/external/boost/python/detail/wrap_python.hpp"
# include "pxr/external/boost/python/detail/config.hpp"

#endif // PXR_USE_INTERNAL_BOOST_PYTHON
#endif // PXR_EXTERNAL_BOOST_PYTHON_DETAIL_PREFIX_HPP
