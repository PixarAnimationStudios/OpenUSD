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

    // In Python 3.10 Py_REFCNT was changed from a macro that evaluated to the
    // ob_refcnt struct member to a function that returns its value. This breaks
    // the previous test, since taking the address of an rvalue is not allowed.
    //
    // To workaround this, we look at the struct members directly instead of
    // going through the API. These members are documented and are part of
    // the Python Stable ABI. We also look at ob_type instead of ob_refcnt since
    // the latter does not exist in Python builds with the GIL disabled.
#if PY_VERSION_HEX < 0x030a00f0
    assert(&Py_REFCNT(PXR_BOOST_NAMESPACE::python::upcast<PyObject>(&o)) == &Py_REFCNT(&o));
    assert(&Py_REFCNT(PXR_BOOST_NAMESPACE::python::upcast<PyObject>(&y)) == &Py_REFCNT(&y));
#else
    assert(&PXR_BOOST_NAMESPACE::python::upcast<PyObject>(&o)->ob_type == &o.ob_base.ob_base.ob_type);
    assert(&PXR_BOOST_NAMESPACE::python::upcast<PyObject>(&y)->ob_type == &y.ob_type);
#endif

    return 0;
}
