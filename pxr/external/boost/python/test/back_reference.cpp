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

#include "pxr/external/boost/python/class.hpp"
#include "pxr/external/boost/python/docstring_options.hpp"
#include "pxr/external/boost/python/module.hpp"
#include "pxr/external/boost/python/def.hpp"
#include "pxr/external/boost/python/has_back_reference.hpp"
#include "pxr/external/boost/python/back_reference.hpp"
#include <boost/ref.hpp>
#include <memory>
#include <cassert>
#include "pxr/external/boost/python/copy_const_reference.hpp"
#include "pxr/external/boost/python/return_value_policy.hpp"
#include "pxr/external/boost/python/detail/mpl2/bool.hpp"

// This test shows that a class can be wrapped "as itself" but also
// acquire a back-reference iff has_back_reference<> is appropriately
// specialized.
using namespace PXR_BOOST_NAMESPACE::python;

struct X
{
    explicit X(int x) : x(x), magic(7654321) { ++counter; }
    X(X const& rhs) : x(rhs.x), magic(7654321) { ++counter; }
    virtual ~X() { assert(magic == 7654321); magic = 6666666; x = 9999; --counter; }

    void set(int _x) { assert(magic == 7654321); this->x = _x; }
    int value() const { assert(magic == 7654321); return x; }
    static int count() { return counter; }
 private:
    void operator=(X const&);
 private:
    int x;
    long magic;
    static int counter;
};

int X::counter;

struct Y : X
{
    Y(PyObject* self, int x) : X(x), self(self) {}
    Y(PyObject* self, Y const& rhs) : X(rhs), self(self) {}
 private:
    Y(Y const&);
    PyObject* self;
};

struct Z : X
{
    Z(PyObject* self, int x) : X(x), self(self) {}
    Z(PyObject* self, Z const& rhs) : X(rhs), self(self) {}
 private:
    Z(Z const&);
    PyObject* self;
};

Y const& copy_Y(Y const& y) { return y; }
Z const& copy_Z(Z const& z) { return z; }

namespace PXR_BOOST_NAMESPACE { namespace python
{
  template <>
  struct has_back_reference<Y>
      : detail::mpl2::true_
  {
  };

  template <>
  struct has_back_reference<Z>
      : detail::mpl2::true_
  {
  };
}}

// prove that back_references get initialized with the right PyObject*
object y_identity(back_reference<Y const&> y)
{
    return y.source();
}

// prove that back_references contain the right value
bool y_equality(back_reference<Y const&> y1, Y const& y2)
{
    return &y1.get() == &y2;
}

PXR_BOOST_PYTHON_MODULE(back_reference_ext)
{
    // Explicitly enable Python signatures in docstrings in case boost::python
    // was built with PXR_BOOST_PYTHON_NO_PY_SIGNATURES, which disables those
    // signatures by default.
    docstring_options doc_options;
    doc_options.enable_py_signatures();

    def("copy_Y", copy_Y, return_value_policy<copy_const_reference>());
    def("copy_Z", copy_Z, return_value_policy<copy_const_reference>());
    def("x_instances", &X::count);
    
    class_<Y>("Y", init<int>())
        .def("value", &Y::value)
        .def("set", &Y::set)
        ;

    class_<Z,std::unique_ptr<Z> >("Z", init<int>())
        .def("value", &Z::value)
        .def("set", &Z::set)
        ;

    def("y_identity", y_identity);
    def("y_equality", y_equality);

}

#include "module_tail.cpp"
