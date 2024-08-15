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
# include "pxr/external/boost/python/converter/registered.hpp"
# include "pxr/external/boost/python/converter/pointer_type_id.hpp"
# include "pxr/external/boost/python/converter/registry.hpp"
# include "pxr/external/boost/python/detail/type_traits.hpp"

namespace boost { namespace python { namespace converter { 

struct registration;

template <class T>
struct registered_pointee
    : registered<
        typename boost::python::detail::remove_pointer<
           typename boost::python::detail::remove_cv<
              typename boost::python::detail::remove_reference<T>::type
           >::type
        >::type
    >
{
};
}}} // namespace boost::python::converter

#endif // PXR_EXTERNAL_BOOST_PYTHON_CONVERTER_REGISTERED_POINTEE_HPP
