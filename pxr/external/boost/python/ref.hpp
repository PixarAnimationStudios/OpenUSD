//
// Copyright 2024 Pixar
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#ifndef PXR_EXTERNAL_BOOST_PYTHON_REF_HPP
#define PXR_EXTERNAL_BOOST_PYTHON_REF_HPP

#include "pxr/pxr.h"
#include "pxr/external/boost/python/common.hpp"

#ifndef PXR_USE_INTERNAL_BOOST_PYTHON

#include <boost/core/ref.hpp>

namespace PXR_BOOST_NAMESPACE { namespace python {

using boost::ref;
using boost::cref;
using boost::reference_wrapper;
using boost::is_reference_wrapper;
using boost::unwrap_reference;

}} // namespace PXR_BOOST_NAMESPACE::python

#else

#include <functional>

namespace PXR_BOOST_NAMESPACE { namespace python {

using std::ref;
using std::cref;
using std::reference_wrapper;

template <class T>
struct is_reference_wrapper : std::false_type
{
};

template <class T>
struct is_reference_wrapper<std::reference_wrapper<T>> : std::true_type
{
};

template <class T>
struct unwrap_reference
{
    using type = T;
};

template <class U>
struct unwrap_reference<std::reference_wrapper<U>>
{
    using type = U;
};

}} // namespace PXR_BOOST_NAMESPACE::python

#endif // PXR_USE_INTERNAL_BOOST_PYTHON
#endif // PXR_EXTERNAL_BOOST_PYTHON_REF_HPP
