//
// Copyright 2024 Pixar
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
// Copyright David Abrahams 2004. Distributed under the Boost
// Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
#include "pxr/external/boost/python/module.hpp"
#include "pxr/external/boost/python/def.hpp"
#include "pxr/external/boost/python/exception_translator.hpp"

struct error {};

void translate(error const& /*e*/)
{
    PyErr_SetString(PyExc_RuntimeError, "!!!error!!!");
}

void throw_error()
{
    throw error();
    
}

BOOST_PYTHON_MODULE(exception_translator_ext)
{
  using namespace boost::python;
  register_exception_translator<error>(&translate);
  
  def("throw_error", throw_error);
}

