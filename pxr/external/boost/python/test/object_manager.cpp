//
// Copyright 2024 Pixar
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
// Copyright David Abrahams 2002.
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
#include "pxr/external/boost/python/converter/object_manager.hpp"
#include "pxr/external/boost/python/borrowed.hpp"
#include "pxr/external/boost/python/handle.hpp"

using namespace PXR_BOOST_NAMESPACE::python;
using namespace PXR_BOOST_NAMESPACE::python::converter;

struct X {};

int main()
{
    static_assert(is_object_manager<handle<> >::value);
    static_assert(!is_object_manager<int>::value);
    static_assert(!is_object_manager<X>::value);
    
    static_assert(is_reference_to_object_manager<handle<>&>::value);
    static_assert(is_reference_to_object_manager<handle<> const&>::value);
    static_assert(is_reference_to_object_manager<handle<> volatile&>::value);
    static_assert(is_reference_to_object_manager<handle<> const volatile&>::value);

    static_assert(!is_reference_to_object_manager<handle<> >::value);
    static_assert(!is_reference_to_object_manager<X>::value);
    static_assert(!is_reference_to_object_manager<X&>::value);
    static_assert(!is_reference_to_object_manager<X const&>::value);
    
    return 0;
}

