//
// Copyright 2024 Pixar
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
// Copyright David Abrahams 2002.
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
#ifndef PXR_EXTERNAL_BOOST_PYTHON_CONVERTER_REGISTERED_POINTEE_HPP
# define PXR_EXTERNAL_BOOST_PYTHON_CONVERTER_REGISTERED_POINTEE_HPP

#include "pxr/pxr.h"
#include "pxr/external/boost/python/common.hpp"

#ifndef PXR_USE_INTERNAL_BOOST_PYTHON
#include <boost/python/converter/registered_pointee.hpp>
#else
# include "pxr/external/boost/python/converter/registered.hpp"
# include "pxr/external/boost/python/converter/pointer_type_id.hpp"
# include "pxr/external/boost/python/converter/registry.hpp"
# include "pxr/external/boost/python/detail/type_traits.hpp"

namespace PXR_BOOST_NAMESPACE { namespace python { namespace converter { 

struct registration;

template <class T>
struct registered_pointee
    : registered<
        typename PXR_BOOST_NAMESPACE::python::detail::remove_pointer<
           typename PXR_BOOST_NAMESPACE::python::detail::remove_cv<
              typename PXR_BOOST_NAMESPACE::python::detail::remove_reference<T>::type
           >::type
        >::type
    >
{
};
}}} // namespace PXR_BOOST_NAMESPACE::python::converter

#endif // PXR_USE_INTERNAL_BOOST_PYTHON
#endif // PXR_EXTERNAL_BOOST_PYTHON_CONVERTER_REGISTERED_POINTEE_HPP
