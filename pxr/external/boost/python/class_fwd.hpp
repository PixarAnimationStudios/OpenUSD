//
// Copyright 2024 Pixar
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
// Copyright David Abrahams 2002.
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
#ifndef PXR_EXTERNAL_BOOST_PYTHON_CLASS_FWD_HPP
# define PXR_EXTERNAL_BOOST_PYTHON_CLASS_FWD_HPP

# include "pxr/external/boost/python/detail/prefix.hpp"
# include "pxr/external/boost/python/detail/not_specified.hpp"

namespace boost { namespace python { 

template <
    class T // class being wrapped
    // arbitrarily-ordered optional arguments. Full qualification needed for MSVC6
    , class X1 = ::boost::python::detail::not_specified
    , class X2 = ::boost::python::detail::not_specified
    , class X3 = ::boost::python::detail::not_specified
    >
class class_;

}} // namespace boost::python

#endif // PXR_EXTERNAL_BOOST_PYTHON_CLASS_FWD_HPP
