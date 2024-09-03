//
// Copyright 2024 Pixar
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
// Copyright David Abrahams 2003.
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
#ifndef PXR_EXTERNAL_BOOST_PYTHON_DETAIL_COPY_CTOR_MUTATES_RHS_HPP
# define PXR_EXTERNAL_BOOST_PYTHON_DETAIL_COPY_CTOR_MUTATES_RHS_HPP

#include "pxr/pxr.h"
#include "pxr/external/boost/python/common.hpp"

#ifndef PXR_USE_INTERNAL_BOOST_PYTHON
#include <boost/python/detail/copy_ctor_mutates_rhs.hpp>
#else

#include "pxr/external/boost/python/detail/is_auto_ptr.hpp"
#include <boost/mpl/bool.hpp>

namespace PXR_BOOST_NAMESPACE { namespace python { namespace detail { 

template <class T>
struct copy_ctor_mutates_rhs
    : is_auto_ptr<T>
{
};

}}} // namespace PXR_BOOST_NAMESPACE::python::detail

#endif // PXR_USE_INTERNAL_BOOST_PYTHON
#endif // PXR_EXTERNAL_BOOST_PYTHON_DETAIL_COPY_CTOR_MUTATES_RHS_HPP
