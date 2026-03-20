//
// Copyright 2024 Pixar
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
// Copyright David Abrahams 2002.
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#include "pxr/external/boost/python/cast.hpp"
#include <cassert>

struct X { long x; };
struct Y : X, PyObject {};

int main()
{
    PyTypeObject o;
    Y y;
    assert(PXR_BOOST_NAMESPACE::python::upcast<PyObject>(&o) == reinterpret_cast<PyObject*>(&o));
    assert(PXR_BOOST_NAMESPACE::python::upcast<PyObject>(&y) == &y);
    return 0;
}
