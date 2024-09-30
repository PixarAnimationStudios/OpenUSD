//
// Copyright 2024 Pixar
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_EXTERNAL_BOOST_PYTHON_DETAIL_GET_POINTER_HPP
#define PXR_EXTERNAL_BOOST_PYTHON_DETAIL_GET_POINTER_HPP

#include "pxr/pxr.h"
#include "pxr/external/boost/python/common.hpp"

#ifdef PXR_BOOST_PYTHON_HAS_BOOST_SHARED_PTR
#include <boost/get_pointer.hpp>
#include <boost/shared_ptr.hpp> // For get_pointer for boost::shared_ptr
#else
#include <memory>
#endif

namespace PXR_BOOST_NAMESPACE { namespace python { namespace detail {

#ifdef PXR_BOOST_PYTHON_HAS_BOOST_SHARED_PTR
using boost::get_pointer;
#else

// These are defined in boost/get_pointer.hpp and imported into this
// namespace when PXR_BOOST_PYTHON_HAS_BOOST_SHARED_PTR is defined.
// Otherwise, we need to provide the implementations ourselves.

template <class T>
T* get_pointer(T* p)
{
    return p;
}

template <class T> 
T* get_pointer(std::unique_ptr<T> const& p)
{
    return p.get();
}

template <class T>
T* get_pointer(std::shared_ptr<T> const& p)
{
    return p.get();
}

#endif

}}} // namespace PXR_BOOST_NAMESPACE::python::detail

#endif // PXR_EXTERNAL_BOOST_PYTHON_DETAIL_GET_POINTER_HPP
