//
// Copyright 2024 Pixar
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
// Copyright David Abrahams 2002.
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
#include "pxr/external/boost/python/pointee.hpp"
#include "pxr/external/boost/python/detail/type_traits.hpp"
#include <memory>

struct A;

int main()
{
    static_assert(
        (PXR_BOOST_NAMESPACE::python::detail::is_same<
             PXR_BOOST_NAMESPACE::python::pointee<std::shared_ptr<A> >::type
             , A>::value));

    static_assert(
        (PXR_BOOST_NAMESPACE::python::detail::is_same<
                PXR_BOOST_NAMESPACE::python::pointee<char*>::type
                , char
         >::value));

    return 0;
}
