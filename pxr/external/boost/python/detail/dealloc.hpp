//
// Copyright 2024 Pixar
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
// Copyright Gottfried Ganßauge 2003.
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

# ifndef PXR_EXTERNAL_BOOST_PYTHON_DETAIL_DEALLOC_HPP
# define PXR_EXTERNAL_BOOST_PYTHON_DETAIL_DEALLOC_HPP
namespace boost { namespace python { namespace detail {
    extern "C"
    {
        inline void dealloc(PyObject* self)
        {
          PyObject_Del(self);
        }
    }
}}} // namespace boost::python::detail
# endif    // PXR_EXTERNAL_BOOST_PYTHON_DETAIL_DEALLOC_HPP
