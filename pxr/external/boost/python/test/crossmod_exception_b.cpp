//
// Copyright 2024 Pixar
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
// Copyright (C) 2003 Rational Discovery LLC
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#include "pxr/external/boost/python.hpp"

namespace python = boost::python;

void tossit(){
  PyErr_SetString(PyExc_IndexError,"b-blah!");
  throw python::error_already_set();
}

BOOST_PYTHON_MODULE(crossmod_exception_b)
{
    python::def("tossit",tossit);
}
