//
// Copyright 2024 Pixar
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
// Copyright Niall Douglas 2005.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

# include "pxr/external/boost/python/return_opaque_pointer.hpp"
# include "pxr/external/boost/python/def.hpp"
# include "pxr/external/boost/python/module.hpp"
# include "pxr/external/boost/python/return_value_policy.hpp"

static void *test=(void *) 78;

void *get()
{
    return test;
}

void *getnull()
{
    return 0;
}

void use(void *a)
{
    if(a!=test)
        throw std::runtime_error(std::string("failed"));
}

int useany(void *a)
{
    return a ? 1 : 0;
}


namespace bpl = PXR_BOOST_NAMESPACE::python;

PXR_BOOST_PYTHON_MODULE(voidptr_ext)
{
    bpl::def("get", &::get, bpl::return_value_policy<bpl::return_opaque_pointer>());
    bpl::def("getnull", &::getnull, bpl::return_value_policy<bpl::return_opaque_pointer>());
    bpl::def("use", &::use);
    bpl::def("useany", &::useany);
}
