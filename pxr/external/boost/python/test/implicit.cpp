//
// Copyright 2024 Pixar
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
// Copyright David Abrahams 2002.
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

// If BOOST_PYTHON_NO_PY_SIGNATURES was defined when building this module,
// boost::python will generate simplified docstrings that break the associated
// test unless we undefine it before including any headers.
#undef BOOST_PYTHON_NO_PY_SIGNATURES

#include "pxr/external/boost/python/class.hpp"
#include "pxr/external/boost/python/docstring_options.hpp"
#include "pxr/external/boost/python/implicit.hpp"
#include "pxr/external/boost/python/module.hpp"
#include "pxr/external/boost/python/def.hpp"
#include "test_class.hpp"

using namespace boost::python;

typedef test_class<> X;

int x_value(X const& x)
{
    return x.value();
}

X make_x(int n) { return X(n); }


// foo/bar -- a regression for a vc7 bug workaround
struct bar {};
struct foo
{
    virtual ~foo() {}; // silence compiler warnings
    virtual void f() = 0;
    operator bar() const { return bar(); }
};

BOOST_PYTHON_MODULE(implicit_ext)
{
    // Explicitly enable Python signatures in docstrings in case boost::python
    // was built with BOOST_PYTHON_NO_PY_SIGNATURES, which disables those
    // signatures by default.
    docstring_options doc_options;
    doc_options.enable_py_signatures();

    implicitly_convertible<foo,bar>();
    implicitly_convertible<int,X>();
    
    def("x_value", x_value);
    def("make_x", make_x);

    class_<X>("X", init<int>())
        .def("value", &X::value)
        .def("set", &X::set)
        ;
    
    implicitly_convertible<X,int>();
}

#include "module_tail.cpp"
