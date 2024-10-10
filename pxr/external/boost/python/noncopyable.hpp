//
// Copyright 2024 Pixar
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_EXTERNAL_BOOST_PYTHON_NONCOPYABLE_HPP
#define PXR_EXTERNAL_BOOST_PYTHON_NONCOPYABLE_HPP

#include "pxr/pxr.h"
#include "pxr/external/boost/python/common.hpp"

#ifndef PXR_USE_INTERNAL_BOOST_PYTHON

#include <boost/noncopyable.hpp>

namespace PXR_BOOST_NAMESPACE { namespace python {

using boost::noncopyable;

}} // namespace PXR_BOOST_NAMESPACE::python

#else

namespace PXR_BOOST_NAMESPACE { namespace python {

// Tag structure used with class_ to indicate that the wrapped C++ type
// is not copyable. Note this struct is marked final to avoid client
// code accidentally inheriting from it instead of boost::noncopyable.
struct noncopyable final
{
};

}} // namespace PXR_BOOST_NAMESPACE::python

#endif // PXR_USE_INTERNAL_BOOST_PYTHON

#endif // PXR_EXTERNAL_BOOST_PYTHON_NONCOPYABLE_HPP
