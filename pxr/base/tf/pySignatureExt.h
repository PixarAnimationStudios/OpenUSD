//
// Copyright 2023 Pixar
//
// Licensed under the Apache License, Version 2.0 (the "Apache License")
// with the following modification; you may not use this file except in
// compliance with the Apache License and the following modification to it:
// Section 6. Trademarks. is deleted and replaced with:
//
// 6. Trademarks. This License does not grant permission to use the trade
//    names, trademarks, service marks, or product names of the Licensor
//    and its affiliates, except as required to comply with Section 4(c) of
//    the License and to reproduce the content of the NOTICE file.
//
// You may obtain a copy of the Apache License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the Apache License with the above modification is
// distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
// KIND, either express or implied. See the Apache License for the specific
// language governing permissions and limitations under the Apache License.
//
#ifndef PXR_BASE_TF_PY_SIGNATURE_EXT_H
#define PXR_BASE_TF_PY_SIGNATURE_EXT_H

#include <boost/mpl/vector.hpp>

// This file extends boost::python::detail::get_signature to support member
// function pointers that have lvalue ref-qualifiers.  For example:
//
// class Foo {
//     void f() &;
// };
//
// Without this extension, boost::python cannot wrap ref-qualified member
// functions like this.
//
// This utility does not support rvalue ref-qualifiers.  There isn't really such
// a thing as an rvalue in Python, so it doesn't make sense to wrap rvalue
// ref-qualified member functions.  And boost.python's infrastructure always
// requires an lvalue for 'this' accordingly.
//
// To use this utility, #include this file before any other file in your
// wrapXXX.cpp file; the order matters.

namespace boost { namespace python { namespace detail {

template <class Ret, class TheCls, class ... Args>
auto get_signature(Ret (TheCls::*)(Args...) &, void* =nullptr) {
    return boost::mpl::vector<Ret, TheCls &, Args...>();
}
template <class Ret, class TheCls, class ... Args>
auto get_signature(Ret (TheCls::*)(Args...) const &, void* =nullptr) {
    return boost::mpl::vector<Ret, TheCls &, Args...>();
}

}}}

#include <boost/python/signature.hpp>

#endif // PXR_BASE_TF_PY_SIGNATURE_EXT_H
