//
// Copyright 2024 Pixar
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
// Copyright David Abrahams 2003.
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
#ifndef PXR_EXTERNAL_BOOST_PYTHON_DETAIL_IS_AUTO_PTR_HPP
# define PXR_EXTERNAL_BOOST_PYTHON_DETAIL_IS_AUTO_PTR_HPP

#include "pxr/pxr.h"
#include "pxr/external/boost/python/common.hpp"

#ifndef PXR_USE_INTERNAL_BOOST_PYTHON
#include <boost/python/detail/is_auto_ptr.hpp>
#else
#include <type_traits>

namespace PXR_BOOST_NAMESPACE { namespace python { namespace detail { 

template <class T>
struct is_auto_ptr : std::false_type
{
};
    
}}} // namespace PXR_BOOST_NAMESPACE::python::detail

#endif // PXR_USE_INTERNAL_BOOST_PYTHON
#endif // PXR_EXTERNAL_BOOST_PYTHON_DETAIL_IS_AUTO_PTR_HPP
