//
// Copyright 2024 Pixar
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
// Copyright David Abrahams 2002.
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
#ifndef PXR_EXTERNAL_BOOST_PYTHON_DETAIL_IF_ELSE_HPP
# define PXR_EXTERNAL_BOOST_PYTHON_DETAIL_IF_ELSE_HPP

#include "pxr/pxr.h"
#include "pxr/external/boost/python/common.hpp"

#ifndef PXR_USE_INTERNAL_BOOST_PYTHON
#include <boost/python/detail/if_else.hpp>
#else

namespace PXR_BOOST_NAMESPACE { namespace python { namespace detail { 

template <class T> struct elif_selected;

template <class T>
struct if_selected
{
    template <bool b>
    struct elif : elif_selected<T>
    {
    };

    template <class U>
    struct else_
    {
        typedef T type;
    };
};

template <class T>
struct elif_selected
{
# if !(defined(__MWERKS__) && __MWERKS__ <= 0x2407)
    template <class U> class then;
# else
    template <class U>
    struct then : if_selected<T>
    {
    };
# endif 
};

# if !(defined(__MWERKS__) && __MWERKS__ <= 0x2407)
template <class T>
template <class U>
class elif_selected<T>::then : public if_selected<T>
{
};
# endif 

template <bool b> struct if_
{
    template <class T>
    struct then : if_selected<T>
    {
    };
};

struct if_unselected
{
    template <bool b> struct elif : if_<b>
    {
    };

    template <class U>
    struct else_
    {
        typedef U type;
    };
};

template <>
struct if_<false>
{
    template <class T>
    struct then : if_unselected
    {
    };
};

}}} // namespace PXR_BOOST_NAMESPACE::python::detail

#endif // PXR_USE_INTERNAL_BOOST_PYTHON
#endif // PXR_EXTERNAL_BOOST_PYTHON_DETAIL_IF_ELSE_HPP
