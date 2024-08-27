//
// Copyright 2024 Pixar
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
// Copyright David Abrahams 2004. Distributed under the Boost
// Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include "pxr/external/boost/python/class.hpp"
#include "pxr/external/boost/python/raw_function.hpp"
#include "pxr/external/boost/python/make_constructor.hpp"
#include "pxr/external/boost/python/dict.hpp"
#include "pxr/external/boost/python/tuple.hpp"
#include "pxr/external/boost/python/module.hpp"

using namespace PXR_BOOST_NAMESPACE::python;

class Foo
{
 public:
    Foo(tuple args, dict kw)
      : args(args), kw(kw) {}
    
    tuple args;
    dict kw;
};

object init_foo(tuple args, dict kw)
{
    tuple rest(args.slice(1,_));
    return args[0].attr("__init__")(rest, kw);
}

PXR_BOOST_PYTHON_MODULE(raw_ctor_ext)
{
    // using no_init postpones defining __init__ function until after
    // raw_function for proper overload resolution order, since later
    // defs get higher priority.
    class_<Foo>("Foo", no_init) 
        .def("__init__", raw_function(&init_foo))
        .def(init<tuple, dict>())
        .def_readwrite("args", &Foo::args)
        .def_readwrite("kw", &Foo::kw)
        ;
}

#include "module_tail.cpp"
