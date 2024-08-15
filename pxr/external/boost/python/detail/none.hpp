//
// Copyright 2024 Pixar
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
//  (C) Copyright David Abrahams 2000.
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//
//  The author gratefully acknowleges the support of Dragon Systems, Inc., in
//  producing this work.

#ifndef PXR_EXTERNAL_BOOST_PYTHON_DETAIL_NONE_HPP
# define PXR_EXTERNAL_BOOST_PYTHON_DETAIL_NONE_HPP

# include "pxr/external/boost/python/detail/prefix.hpp"

namespace boost { namespace python { namespace detail {

inline PyObject* none() { Py_INCREF(Py_None); return Py_None; }
    
}}} // namespace boost::python::detail

#endif // PXR_EXTERNAL_BOOST_PYTHON_DETAIL_NONE_HPP
