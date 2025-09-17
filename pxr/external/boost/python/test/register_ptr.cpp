//
// Copyright 2024 Pixar
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
// Copyright David Abrahams 2004. Distributed under the Boost
// Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include "pxr/external/boost/python.hpp"
#include <memory>

using namespace PXR_BOOST_NAMESPACE;
using namespace python;

struct A
{
    virtual int f() { return 0; }
};

std::shared_ptr<A> New() { return std::shared_ptr<A>( new A() ); }

int Call( const std::shared_ptr<A> & a )
{
    return a->f();
}

int Fail( std::shared_ptr<A> & a )
{
    return a->f();
}

struct A_Wrapper: A
{
    A_Wrapper(PyObject* self_): self(self_) {}
    A_Wrapper(PyObject* self_, const A& a): self(self_), A(a) {}

    int f() 
    {
        return call_method<int>(self, "f");
    }
    
    int default_f() 
    {
        return A::f();
    }
    
    PyObject* self;
};

PXR_BOOST_PYTHON_MODULE(register_ptr)
{
    class_<A, A_Wrapper>("A")
        .def("f", &A::f, &A_Wrapper::default_f)
    ;
    register_ptr_to_python< std::shared_ptr<A> >();
    def("New", &New);
    def("Call", &Call);
    def("Fail", &Fail);
}
