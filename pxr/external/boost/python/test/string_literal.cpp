//
// Copyright 2024 Pixar
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
// Copyright David Abrahams 2004. Distributed under the Boost
// Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
#include "pxr/external/boost/python/detail/string_literal.hpp"
//#include <stdio.h>

#include <boost/detail/lightweight_test.hpp>

using namespace PXR_BOOST_NAMESPACE::python::detail;
    

template <class T>
void expect_string_literal(T const&)
{
    static_assert(is_string_literal<T const>::value);
}

int main()
{
    expect_string_literal("hello");
    static_assert(!is_string_literal<int*&>::value);
    static_assert(!is_string_literal<int* const&>::value);
    static_assert(!is_string_literal<int*volatile&>::value);
    static_assert(!is_string_literal<int*const volatile&>::value);
    
    static_assert(!is_string_literal<char const*>::value);
    static_assert(!is_string_literal<char*>::value);
    static_assert(!is_string_literal<char*&>::value);
    static_assert(!is_string_literal<char* const&>::value);
    static_assert(!is_string_literal<char*volatile&>::value);
    static_assert(!is_string_literal<char*const volatile&>::value);
    
    static_assert(!is_string_literal<char[20]>::value);
    static_assert(is_string_literal<char const[20]>::value);
    static_assert(is_string_literal<char const[3]>::value);

    static_assert(!is_string_literal<int[20]>::value);
    static_assert(!is_string_literal<int const[20]>::value);
    static_assert(!is_string_literal<int const[3]>::value);
    return boost::report_errors();
}
