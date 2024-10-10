//
// Copyright 2024 Pixar
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
// Copyright David Abrahams 2003.
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
#include "pxr/external/boost/python/detail/copy_ctor_mutates_rhs.hpp"
#include <memory>
#include <string>

int main()
{
    using namespace PXR_BOOST_NAMESPACE::python::detail;
    static_assert(!copy_ctor_mutates_rhs<int>::value);
    static_assert(!copy_ctor_mutates_rhs<std::string>::value);
    return 0;
}
