//
// Copyright 2024 Pixar
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
// Copyright David Abrahams 2002.
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
#ifndef PXR_EXTERNAL_BOOST_PYTHON_BORROWED_HPP
# define PXR_EXTERNAL_BOOST_PYTHON_BORROWED_HPP

#include "pxr/pxr.h"
#include "pxr/external/boost/python/common.hpp"

#ifndef PXR_USE_INTERNAL_BOOST_PYTHON
#include <boost/python/borrowed.hpp>
#else

# include "pxr/external/boost/python/detail/prefix.hpp"
# include "pxr/external/boost/python/detail/borrowed_ptr.hpp"

namespace PXR_BOOST_NAMESPACE { namespace python { 

template <class T>
inline python::detail::borrowed<T>* borrowed(T* p)
{
    return (detail::borrowed<T>*)p;
}
    
}} // namespace PXR_BOOST_NAMESPACE::python

#endif // PXR_USE_INTERNAL_BOOST_PYTHON
#endif // PXR_EXTERNAL_BOOST_PYTHON_BORROWED_HPP
