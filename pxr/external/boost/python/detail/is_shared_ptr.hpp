//
// Copyright 2024 Pixar
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
// Copyright David Abrahams 2003.
// Copyright Stefan Seefeld 2016.
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef PXR_EXTERNAL_BOOST_PYTHON_DETAIL_IS_SHARED_PTR_HPP
#define PXR_EXTERNAL_BOOST_PYTHON_DETAIL_IS_SHARED_PTR_HPP

#include "pxr/pxr.h"
#include "pxr/external/boost/python/common.hpp"

#ifndef PXR_USE_INTERNAL_BOOST_PYTHON
#include <boost/python/detail/is_shared_ptr.hpp>
#else

#include "pxr/external/boost/python/detail/is_xxx.hpp"
#include <boost/shared_ptr.hpp>

namespace PXR_BOOST_NAMESPACE { namespace python { namespace detail { 

PXR_BOOST_PYTHON_IS_XXX_DEF(shared_ptr, shared_ptr, 1)
#if !defined(BOOST_NO_CXX11_SMART_PTR)
template <typename T>
struct is_shared_ptr<std::shared_ptr<T> > : std::true_type {};
#endif

}}} // namespace PXR_BOOST_NAMESPACE::python::detail

#endif // PXR_USE_INTERNAL_BOOST_PYTHON
#endif
