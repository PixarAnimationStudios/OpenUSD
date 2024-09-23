//
// Copyright 2024 Pixar
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
// Copyright David Abrahams 2002.
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
#include "pxr/external/boost/python/detail/if_else.hpp"
#include "pxr/external/boost/python/detail/type_traits.hpp"

    typedef char c1;
    typedef char c2[2];
    typedef char c3[3];
    typedef char c4[4];

template <unsigned size>
struct choose
{
    typedef typename PXR_BOOST_NAMESPACE::python::detail::if_<
        (sizeof(c1) == size)
    >::template then<
        c1
    >::template elif<
        (sizeof(c2) == size)
    >::template then<
        c2
    >::template elif<
        (sizeof(c3) == size)
    >::template then<
        c3
    >::template elif<
        (sizeof(c4) == size)
    >::template then<
        c4
    >::template else_<void*>::type type;
};

int main()
{
    static_assert((PXR_BOOST_NAMESPACE::python::detail::is_same<choose<1>::type,c1>::value));
    static_assert((PXR_BOOST_NAMESPACE::python::detail::is_same<choose<2>::type,c2>::value));
    static_assert((PXR_BOOST_NAMESPACE::python::detail::is_same<choose<3>::type,c3>::value));
    static_assert((PXR_BOOST_NAMESPACE::python::detail::is_same<choose<4>::type,c4>::value));
    static_assert((PXR_BOOST_NAMESPACE::python::detail::is_same<choose<5>::type,void*>::value));
    return 0;
}
