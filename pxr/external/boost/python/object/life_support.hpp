//
// Copyright 2024 Pixar
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
// Copyright David Abrahams 2002.
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
#ifndef PXR_EXTERNAL_BOOST_PYTHON_OBJECT_LIFE_SUPPORT_HPP
# define PXR_EXTERNAL_BOOST_PYTHON_OBJECT_LIFE_SUPPORT_HPP
# include "pxr/external/boost/python/detail/prefix.hpp"

namespace boost { namespace python { namespace objects { 

BOOST_PYTHON_DECL PyObject* make_nurse_and_patient(PyObject* nurse, PyObject* patient);

}}} // namespace boost::python::object

#endif // PXR_EXTERNAL_BOOST_PYTHON_OBJECT_LIFE_SUPPORT_HPP
