//
// Copyright 2024 Pixar
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
// Copyright David Abrahams 2002.
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#include "pxr/external/boost/python/class.hpp"
#include "pxr/external/boost/python/module.hpp"
#include "pxr/external/boost/python/def.hpp"
#include "pxr/external/boost/python/return_internal_reference.hpp"
#include "pxr/external/boost/python/call_method.hpp"

#include <cassert>
#include <functional>

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

struct Y : X
{
    Y(int x) : X(x) {};
};

struct abstract : X
{
    abstract(int x) : X(x) {};
    int call_f(Y const& y) { return f(y); }
    virtual int f(Y const& y) = 0;
    abstract& call_g(Y const& y) { return g(y); }
    virtual abstract& g(Y const& y) = 0;
};

struct concrete : X
{
    concrete(int x) : X(x) {};
    int call_f(Y const& y) { return f(y); }
    virtual int f(Y const& y) { set(y.value()); return y.value(); }
};

struct abstract_callback : abstract
{
    abstract_callback(PyObject* p, int x)
        : abstract(x), self(p)
    {}

    int f(Y const& y)
    {
        return call_method<int>(self, "f", std::ref(y));
    }

    abstract& g(Y const& y)
    {
        return call_method<abstract&>(self, "g", std::ref(y));
    }

    PyObject* self;
};

struct concrete_callback : concrete
{
    concrete_callback(PyObject* p, int x)
        : concrete(x), self(p)
    {}

    concrete_callback(PyObject* p, concrete const& x)
        : concrete(x), self(p)
    {}

    int f(Y const& y)
    {
        return call_method<int>(self, "f", std::ref(y));
    }

    int f_impl(Y const& y)
    {
        return this->concrete::f(y);
    }
    
    PyObject* self;
};

int X::counter;

PXR_BOOST_PYTHON_MODULE(virtual_functions_ext)
{
    class_<concrete, concrete_callback>("concrete", init<int>())
        .def("value", &concrete::value)
        .def("set", &concrete::set)
        .def("call_f", &concrete::call_f)
        .def("f", &concrete_callback::f_impl)
        ;
        
    class_<abstract, noncopyable, abstract_callback
        >("abstract", init<int>())
            
        .def("value", &abstract::value)
        .def("call_f", &abstract::call_f)
        .def("call_g", &abstract::call_g, return_internal_reference<>())
        .def("set", &abstract::set)
        ;
        
    class_<Y>("Y", init<int>())
        .def("value", &Y::value)
        .def("set", &Y::set)
        ;
}

#include "module_tail.cpp"
