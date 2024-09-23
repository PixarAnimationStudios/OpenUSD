//
// Copyright 2024 Pixar
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/external/boost/python.hpp"
#include <memory>

struct vector
{
    virtual ~vector() {}
    
    vector operator+( const vector& ) const
    { return vector(); }

    vector& operator+=( const vector& )
    { return *this; }
    
    vector operator-() const
    { return *this; }
};

struct dvector : vector
{};

using namespace PXR_BOOST_NAMESPACE::python;

struct vector_wrapper
  : vector, wrapper< vector >
{
    vector_wrapper(vector const&) {}
    vector_wrapper() {}
};

PXR_BOOST_PYTHON_MODULE( operators_wrapper_ext )
{
    class_< vector_wrapper >( "vector" )
        .def( self + self )
        .def( self += self )
        .def( -self )
        ;
    
    scope().attr("v") = vector();
    std::shared_ptr<vector> dp(new dvector);
    register_ptr_to_python< std::shared_ptr<vector> >();
    scope().attr("d") = dp;
}
