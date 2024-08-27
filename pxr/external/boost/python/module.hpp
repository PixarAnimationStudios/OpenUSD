//
// Copyright 2024 Pixar
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
// Copyright David Abrahams 2001.
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
#ifndef PXR_EXTERNAL_BOOST_PYTHON_MODULE_HPP
# define PXR_EXTERNAL_BOOST_PYTHON_MODULE_HPP

#include "pxr/pxr.h"
#include "pxr/external/boost/python/common.hpp"

#ifndef PXR_USE_INTERNAL_BOOST_PYTHON
#include <boost/python/module.hpp>
#else

# include "pxr/external/boost/python/detail/prefix.hpp"

# include "pxr/external/boost/python/module_init.hpp"
# define PXR_BOOST_PYTHON_MODULE PXR_BOOST_PYTHON_MODULE_INIT

#endif // PXR_USE_INTERNAL_BOOST_PYTHON
#endif // MODULE_DWA20011221_HPP
