//
// Copyright 2024 Pixar
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
// Copyright David Abrahams 2002.
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
#include "pxr/external/boost/python/bases.hpp"
#include <boost/static_assert.hpp>
#include "pxr/external/boost/python/detail/type_traits.hpp"

struct A;
struct B;

template <class X, class Y, class Z>
struct choose_bases
    : PXR_BOOST_NAMESPACE::python::detail::select_bases<
    X
    , typename PXR_BOOST_NAMESPACE::python::detail::select_bases<
        Y
        , typename PXR_BOOST_NAMESPACE::python::detail::select_bases<Z>::type
    >::type>
{
    
};

int main()
{
    BOOST_STATIC_ASSERT((PXR_BOOST_NAMESPACE::python::detail::specifies_bases<
                         PXR_BOOST_NAMESPACE::python::bases<A,B> >::value));

    BOOST_STATIC_ASSERT((!PXR_BOOST_NAMESPACE::python::detail::specifies_bases<
                         PXR_BOOST_NAMESPACE::python::bases<A,B>& >::value));

    BOOST_STATIC_ASSERT((!PXR_BOOST_NAMESPACE::python::detail::specifies_bases<
                         void* >::value));

    BOOST_STATIC_ASSERT((!PXR_BOOST_NAMESPACE::python::detail::specifies_bases<
                         int >::value));

    BOOST_STATIC_ASSERT((!PXR_BOOST_NAMESPACE::python::detail::specifies_bases<
                         int[5] >::value));

    typedef PXR_BOOST_NAMESPACE::python::detail::select_bases<
        int
        , PXR_BOOST_NAMESPACE::python::detail::select_bases<char*>::type > collected1;

    BOOST_STATIC_ASSERT((PXR_BOOST_NAMESPACE::python::detail::is_same<collected1::type,PXR_BOOST_NAMESPACE::python::bases<> >::value));
    BOOST_STATIC_ASSERT((PXR_BOOST_NAMESPACE::python::detail::is_same<choose_bases<int,char*,long>::type,PXR_BOOST_NAMESPACE::python::bases<> >::value));
    
    typedef PXR_BOOST_NAMESPACE::python::detail::select_bases<
        int
        , PXR_BOOST_NAMESPACE::python::detail::select_bases<
                PXR_BOOST_NAMESPACE::python::bases<A,B>
                , PXR_BOOST_NAMESPACE::python::detail::select_bases<
                        A
            >::type
         >::type
     > collected2;

    BOOST_STATIC_ASSERT((PXR_BOOST_NAMESPACE::python::detail::is_same<collected2::type,PXR_BOOST_NAMESPACE::python::bases<A,B> >::value));
    BOOST_STATIC_ASSERT((PXR_BOOST_NAMESPACE::python::detail::is_same<choose_bases<int,PXR_BOOST_NAMESPACE::python::bases<A,B>,long>::type,PXR_BOOST_NAMESPACE::python::bases<A,B> >::value));
    
    return 0;
}
