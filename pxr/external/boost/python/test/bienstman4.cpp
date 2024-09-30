//
// Copyright 2024 Pixar
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
// Copyright David Abrahams 2002.
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#include "pxr/external/boost/python/module.hpp"
#include "pxr/external/boost/python/def.hpp"
#include "pxr/external/boost/python/class.hpp"
#include "pxr/external/boost/python/implicit.hpp"

struct Type1 {};

struct Term {Term(Type1 const&) {} };

struct Expression {void add(Term const&) {} };

PXR_BOOST_PYTHON_MODULE(bienstman4_ext)
{
  using namespace PXR_BOOST_NAMESPACE::python;

  implicitly_convertible<Type1,Term>();

  class_<Expression>("Expression")
      .def("add", &Expression::add)
      ;
  
  class_<Type1>("T1")
      ;
  
  class_<Term>("Term", init<Type1&>())
      ;
  
  Type1 t1;
  Expression e;
  e.add(t1);
}

