//
// Copyright 2024 Pixar
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#ifndef PXR_EXTERNAL_BOOST_PYTHON_TYPE_LIST_HPP
#define PXR_EXTERNAL_BOOST_PYTHON_TYPE_LIST_HPP

#include "pxr/pxr.h"
#include "pxr/external/boost/python/common.hpp"

#ifndef PXR_USE_INTERNAL_BOOST_PYTHON

#include <boost/mpl/vector.hpp>

namespace PXR_BOOST_NAMESPACE { namespace python {

template <class ...T>
using type_list = ::boost::mpl::vector<T...>;

}} // namespace PXR_BOOST_NAMESPACE::python

#else

#include "pxr/external/boost/python/detail/type_list.hpp"

namespace PXR_BOOST_NAMESPACE { namespace python {

template <class ...T>
using type_list = detail::type_list<T...>;

}} // namespace PXR_BOOST_NAMESPACE::python

#endif // PXR_USE_INTERNAL_BOOST_PYTHON

#endif // PXR_EXTERNAL_BOOST_PYTHON_TYPE_LIST_HPP
