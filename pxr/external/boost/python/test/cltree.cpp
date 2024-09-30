//
// Copyright 2024 Pixar
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
// Copyright David Abrahams 2005. Distributed under the Boost
// Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include "pxr/external/boost/python/module.hpp"
#include "pxr/external/boost/python/def.hpp"
#include "pxr/external/boost/python/class.hpp"

/*  Non-modifiable definitions */

class basic {
public:
  basic() { name = "cltree.basic"; }
  std::string repr() { return name+"()"; }
protected:
  std::string name;
};

class constant: public basic {
public:
  constant() { name = "cltree.constant"; }
};

class symbol: public basic {
public:
  symbol() { name = "cltree.symbol"; }
};

class variable: public basic {
public:
  variable() { name = "cltree.variable"; }
};

/*  EOF: Non-modifiable definitions */

class symbol_wrapper: public symbol {
public:
  symbol_wrapper(PyObject* /*self*/): symbol() { 
    name = "cltree.wrapped_symbol"; 
  }
};

class variable_wrapper: public variable {
public:
  variable_wrapper(PyObject* /*self*/): variable() { 
    name = "cltree.wrapped_variable";
  }

  // This constructor is introduced only because cannot use
  // noncopyable, see below.
  variable_wrapper(PyObject* /*self*/,variable v): variable(v) {} 

};

PXR_BOOST_PYTHON_MODULE(cltree)
{
    PXR_BOOST_NAMESPACE::python::class_<basic>("basic")
        .def("__repr__",&basic::repr)
        ;

    PXR_BOOST_NAMESPACE::python::class_<constant, PXR_BOOST_NAMESPACE::python::bases<basic>, PXR_BOOST_NAMESPACE::python::noncopyable>("constant")
        ;


    PXR_BOOST_NAMESPACE::python::class_<symbol, symbol_wrapper, PXR_BOOST_NAMESPACE::python::noncopyable>("symbol")
        ;

    PXR_BOOST_NAMESPACE::python::class_<variable, PXR_BOOST_NAMESPACE::python::bases<basic>, variable_wrapper>("variable")
        ;
}

#include "module_tail.cpp"

