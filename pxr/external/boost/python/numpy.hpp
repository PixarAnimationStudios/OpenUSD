//
// Copyright 2024 Pixar
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
// Copyright Jim Bosch 2010-2012.
// Copyright Stefan Seefeld 2016.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef PXR_EXTERNAL_BOOST_PYTHON_NUMPY_HPP
#define PXR_EXTERNAL_BOOST_PYTHON_NUMPY_HPP

#include "pxr/pxr.h"
#include "pxr/external/boost/python/common.hpp"

#ifndef PXR_USE_INTERNAL_BOOST_PYTHON
#include <boost/python/numpy.hpp>
#else

#include "pxr/external/boost/python/numpy/dtype.hpp"
#include "pxr/external/boost/python/numpy/ndarray.hpp"
#include "pxr/external/boost/python/numpy/scalars.hpp"
#include "pxr/external/boost/python/numpy/matrix.hpp"
#include "pxr/external/boost/python/numpy/ufunc.hpp"
#include "pxr/external/boost/python/numpy/invoke_matching.hpp"
#include "pxr/external/boost/python/numpy/config.hpp"

namespace PXR_BOOST_NAMESPACE { namespace python { namespace numpy {

/**
 *  @brief Initialize the Numpy C-API
 *
 *  This must be called before using anything in boost.numpy;
 *  It should probably be the first line inside PXR_BOOST_PYTHON_MODULE.
 *
 *  @internal This just calls the Numpy C-API functions "import_array()"
 *            and "import_ufunc()", and then calls
 *            dtype::register_scalar_converters().
 */
BOOST_NUMPY_DECL void initialize(bool register_scalar_converters=true);

}}} // namespace PXR_BOOST_NAMESPACE::python::numpy

#endif // PXR_USE_INTERNAL_BOOST_PYTHON
#endif
