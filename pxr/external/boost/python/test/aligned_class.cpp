//
// Copyright 2024 Pixar
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
#include "pxr/external/boost/python/module.hpp"
#include "pxr/external/boost/python/def.hpp"
#include "pxr/external/boost/python/object.hpp"
#include "pxr/external/boost/python/class.hpp"
#include <cassert>
#include <cstdint>

using namespace PXR_BOOST_NAMESPACE::python;

struct alignas(32) X
{
    int x;
    alignas(32) float f;
    X(int n, float _f) : x(n), f(_f)
    {
        assert((reinterpret_cast<uintptr_t>(&f) % 32) == 0);
    }
};

int x_function(X& x) { return x.x;}
float f_function(X& x) { return x.f;}

PXR_BOOST_PYTHON_MODULE(aligned_class_ext)
{
    class_<X>("X", init<int, float>());
    def("x_function", x_function);
    def("f_function", f_function);
}

#include "module_tail.cpp"
