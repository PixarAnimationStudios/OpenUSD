//
// Copyright 2024 Pixar
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
// Copyright David Abrahams 2002.
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

// If PXR_BOOST_PYTHON_NO_PY_SIGNATURES was defined when building this module,
// boost::python will generate simplified docstrings that break the associated
// test unless we undefine it before including any headers.
#undef PXR_BOOST_PYTHON_NO_PY_SIGNATURES

#include "pxr/external/boost/python/module.hpp"
#include "test_class.hpp"
#include "pxr/external/boost/python/def.hpp"
#include "pxr/external/boost/python/docstring_options.hpp"
#include "pxr/external/boost/python/args.hpp"
#include "pxr/external/boost/python/tuple.hpp"
#include "pxr/external/boost/python/class.hpp"
#include "pxr/external/boost/python/overloads.hpp"
#include "pxr/external/boost/python/raw_function.hpp"
#include "pxr/external/boost/python/return_internal_reference.hpp"

using namespace PXR_BOOST_NAMESPACE::python;


tuple f(int x = 1, double y = 4.25, char const* z = "wow")
{
    return make_tuple(x, y, z);
}

PXR_BOOST_PYTHON_FUNCTION_OVERLOADS(f_overloads, f, 0, 3)
    
typedef test_class<> Y;

struct X
{
    X(int a0 = 0, int a1 = 1) : inner0(a0), inner1(a1) {}
    tuple f(int x = 1, double y = 4.25, char const* z = "wow")
    {
        return make_tuple(x, y, z);
    }

    Y const& inner(bool n) const { return n ? inner1 : inner0; }
    
    Y inner0;
    Y inner1;
};

PXR_BOOST_PYTHON_MEMBER_FUNCTION_OVERLOADS(X_f_overloads, X::f, 0, 3)


tuple raw_func(tuple args, dict kw)
{
    return make_tuple(args, kw);
}

PXR_BOOST_PYTHON_MODULE(args_ext)
{
    // Explicitly enable Python signatures in docstrings in case boost::python
    // was built with PXR_BOOST_PYTHON_NO_PY_SIGNATURES, which disables those
    // signatures by default.
    docstring_options doc_options;
    doc_options.enable_py_signatures();

    def("f", f, (arg("x")=1, arg("y")=4.25, arg("z")="wow")
        , "This is f's docstring"
        );

    def("raw", raw_function(raw_func));
    
#if defined(BOOST_MSVC) && BOOST_MSVC <= 1200
    // MSVC6 gives a fatal error LNK1179: invalid or corrupt file:
    // duplicate comdat error if we try to re-use the exact type of f
    // here, so substitute long for int.
    tuple (*f)(long,double,char const*) = 0;
#endif 
    def("f1", f, f_overloads("f1's docstring", args("x", "y", "z")));
    def("f2", f, f_overloads(args("x", "y", "z")));
    def("f3", f, f_overloads(args("x", "y", "z"), "f3's docstring"));
    
    class_<Y>("Y", init<int>(args("value"), "Y's docstring"))
        .def("value", &Y::value)
        .def("raw", raw_function(raw_func))
        ;
            
    class_<X>("X", "This is X's docstring", init<>(args("self")))
        .def(init<int, optional<int> >(args("self", "a0", "a1")))
        .def("f", &X::f
             , "This is X.f's docstring"
             , args("self","x", "y", "z"))

        // Just to prove that all the different argument combinations work
        .def("inner0", &X::inner, return_internal_reference<>(), args("self", "n"), "docstring")
        .def("inner1", &X::inner, return_internal_reference<>(), "docstring", args("self", "n"))

        .def("inner2", &X::inner, args("self", "n"), return_internal_reference<>(), "docstring")
        .def("inner3", &X::inner, "docstring", return_internal_reference<>(), args("self", "n"))

        .def("inner4", &X::inner, args("self", "n"), "docstring", return_internal_reference<>())
        .def("inner5", &X::inner, "docstring", args("self", "n"), return_internal_reference<>())

        .def("f1", &X::f, X_f_overloads(args("self", "x", "y", "z")))
        .def("f2", &X::f, X_f_overloads(args("self", "x", "y", "z"), "f2's docstring"))
        .def("f2", &X::f, X_f_overloads(args("x", "y", "z"), "f2's docstring"))
        ;

    def("inner", &X::inner, "docstring", args("self", "n"), return_internal_reference<>());
}

#include "module_tail.cpp"
