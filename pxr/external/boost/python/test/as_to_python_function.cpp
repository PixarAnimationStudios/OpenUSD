//
// Copyright 2024 Pixar
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
// Copyright David Abrahams 2002.
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#include "pxr/external/boost/python/converter/as_to_python_function.hpp"

struct hopefully_illegal
{
    static PyObject* convert(int&);
};

PyObject* x = PXR_BOOST_NAMESPACE::python::converter::as_to_python_function<int, hopefully_illegal>::convert(0);
