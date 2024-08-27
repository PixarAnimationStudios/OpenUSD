//
// Copyright 2024 Pixar
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
// Copyright David Abrahams 2003.
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
#ifndef PXR_EXTERNAL_BOOST_PYTHON_DETAIL_DEF_HELPER_FWD_HPP
# define PXR_EXTERNAL_BOOST_PYTHON_DETAIL_DEF_HELPER_FWD_HPP

#include "pxr/pxr.h"
#include "pxr/external/boost/python/common.hpp"

#ifndef PXR_USE_INTERNAL_BOOST_PYTHON
#include <boost/python/detail/def_helper_fwd.hpp>
#else

# include "pxr/external/boost/python/detail/not_specified.hpp"

namespace PXR_BOOST_NAMESPACE { namespace python { namespace detail { 

template <class T1, class T2 = not_specified, class T3 = not_specified, class T4 = not_specified>
struct def_helper;

}}} // namespace PXR_BOOST_NAMESPACE::python::detail

#endif // PXR_USE_INTERNAL_BOOST_PYTHON
#endif // PXR_EXTERNAL_BOOST_PYTHON_DETAIL_DEF_HELPER_FWD_HPP
