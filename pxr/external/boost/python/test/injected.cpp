//
// Copyright 2024 Pixar
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
// Copyright David Abrahams 2003.
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#include "pxr/external/boost/python/module.hpp"
#include "pxr/external/boost/python/class.hpp"
#include "test_class.hpp"
#include <memory>
#include <boost/shared_ptr.hpp>
#include "pxr/external/boost/python/make_constructor.hpp"
#include "pxr/external/boost/python/args.hpp"

using namespace boost::python;

typedef test_class<> X;

X* empty() { return new X(1000); }

std::shared_ptr<X> sum(int a, int b) { return std::shared_ptr<X>(new X(a+b)); }

boost::shared_ptr<X> product(int a, int b, int c)
{
    return boost::shared_ptr<X>(new X(a*b*c));
}

BOOST_PYTHON_MODULE(injected_ext)
{
    class_<X>("X", init<int>())
        .def("__init__", make_constructor(empty))
        .def("__init__", make_constructor(sum))
        .def("__init__", make_constructor(product
            , default_call_policies()
            , ( arg_("a"), arg_("b"), arg_("c"))
            ),
            "this is product's docstring")
        .def("value", &X::value)
        ;
}
