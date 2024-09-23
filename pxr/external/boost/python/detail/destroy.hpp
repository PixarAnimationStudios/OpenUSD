//
// Copyright 2024 Pixar
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
// Copyright David Abrahams 2002.
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
#ifndef PXR_EXTERNAL_BOOST_PYTHON_DETAIL_DESTROY_HPP
# define PXR_EXTERNAL_BOOST_PYTHON_DETAIL_DESTROY_HPP

#include "pxr/pxr.h"
#include "pxr/external/boost/python/common.hpp"

#ifndef PXR_USE_INTERNAL_BOOST_PYTHON
#include <boost/python/detail/destroy.hpp>
#else

# include "pxr/external/boost/python/detail/type_traits.hpp"
namespace PXR_BOOST_NAMESPACE { namespace python { namespace detail { 

template <bool array> struct value_destroyer;
    
template <>
struct value_destroyer<false>
{
    template <class T>
    static void execute(T const volatile* p)
    {
        p->~T();
    }
};

template <>
struct value_destroyer<true>
{
    template <class A, class T>
    static void execute(A*, T const volatile* const first)
    {
        for (T const volatile* p = first; p != first + sizeof(A)/sizeof(T); ++p)
        {
            value_destroyer<
                is_array<T>::value
            >::execute(p);
        }
    }
    
    template <class T>
    static void execute(T const volatile* p)
    {
        execute(p, *p);
    }
};

template <class T>
inline void destroy_referent_impl(void* p, T& (*)())
{
    // note: cv-qualification needed for MSVC6
    // must come *before* T for metrowerks
    value_destroyer<
         (is_array<T>::value)
    >::execute((const volatile T*)p);
}

template <class T>
inline void destroy_referent(void* p, T(*)() = 0)
{
    destroy_referent_impl(p, (T(*)())0);
}

}}} // namespace PXR_BOOST_NAMESPACE::python::detail

#endif // PXR_USE_INTERNAL_BOOST_PYTHON
#endif // PXR_EXTERNAL_BOOST_PYTHON_DETAIL_DESTROY_HPP
