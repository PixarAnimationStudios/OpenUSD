//
// Copyright 2023 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_BASE_TF_PY_SIGNATURE_EXT_H
#define PXR_BASE_TF_PY_SIGNATURE_EXT_H

#include "pxr/external/boost/python/common.hpp"
#include "pxr/external/boost/python/type_list.hpp"

// This file extends pxr_boost::python::detail::get_signature to support member
// function pointers that have lvalue ref-qualifiers.  For example:
//
// class Foo {
//     void f() &;
// };
//
// Without this extension, pxr_boost::python cannot wrap ref-qualified member
// functions like this.
//
// This utility does not support rvalue ref-qualifiers.  There isn't really such
// a thing as an rvalue in Python, so it doesn't make sense to wrap rvalue
// ref-qualified member functions.  And boost.python's infrastructure always
// requires an lvalue for 'this' accordingly.
//
// To use this utility, #include this file before any other file in your
// wrapXXX.cpp file; the order matters.

namespace PXR_BOOST_NAMESPACE { namespace python { namespace detail {

template <class Ret, class TheCls, class ... Args>
auto get_signature(Ret (TheCls::*)(Args...) &, void* =nullptr) {
    return python::type_list<Ret, TheCls &, Args...>();
}
template <class Ret, class TheCls, class ... Args>
auto get_signature(Ret (TheCls::*)(Args...) const &, void* =nullptr) {
    return python::type_list<Ret, TheCls &, Args...>();
}

}}}

#include "pxr/external/boost/python/signature.hpp"

#endif // PXR_BASE_TF_PY_SIGNATURE_EXT_H
