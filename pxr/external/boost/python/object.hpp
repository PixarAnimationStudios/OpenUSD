//
// Copyright 2024 Pixar
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
// Copyright David Abrahams 2002.
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
#ifndef PXR_EXTERNAL_BOOST_PYTHON_OBJECT_HPP
# define PXR_EXTERNAL_BOOST_PYTHON_OBJECT_HPP

# include "pxr/external/boost/python/ssize_t.hpp"
# include "pxr/external/boost/python/object_core.hpp"
# include "pxr/external/boost/python/object_attributes.hpp"
# include "pxr/external/boost/python/object_items.hpp"
# include "pxr/external/boost/python/object_slices.hpp"
# include "pxr/external/boost/python/object_operators.hpp"
# include "pxr/external/boost/python/converter/arg_to_python.hpp"

namespace boost { namespace python {

    inline ssize_t len(object const& obj)
    {
        ssize_t result = PyObject_Length(obj.ptr());
        if (PyErr_Occurred()) throw_error_already_set();
        return result;
    }

}} // namespace boost::python

#endif // PXR_EXTERNAL_BOOST_PYTHON_OBJECT_HPP
