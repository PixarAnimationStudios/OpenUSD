//
// Copyright 2024 Pixar
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
// Copyright David Abrahams 2002.
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#include "pxr/external/boost/python/module.hpp"
#include "pxr/external/boost/python/def.hpp"
#include "pxr/external/boost/python/long.hpp"
#include "pxr/external/boost/python/class.hpp"
#include <cassert>

using namespace PXR_BOOST_NAMESPACE::python;

object new_long()
{
    return long_();
}

long_ longify(object x)
{
    return long_(x);
}

object longify_string(char const* s)
{
    return long_(s);
}

char const* is_long1(long_& x)
{
    long_ y = x;
    x += 50;
    assert(x == y + 50);
    return "yes";
}

int is_long2(char const*)
{
    return 0;
}

// tests for accepting objects (and derived classes) in constructors
// from "Milind Patil" <milind_patil-at-hotmail.com>

struct Y
{
    Y(PXR_BOOST_NAMESPACE::python::long_) {}
};

PXR_BOOST_PYTHON_MODULE(long_ext)
{
    def("new_long", new_long);
    def("longify", longify);
    def("longify_string", longify_string);
    def("is_long", is_long1);
    def("is_long", is_long2);
    
    class_< Y >("Y", init< PXR_BOOST_NAMESPACE::python::long_ >())
        ;
}

#include "module_tail.cpp"
