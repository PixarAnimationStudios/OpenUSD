//
// Copyright 2024 Pixar
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
// Copyright David Abrahams 2002.
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
#ifndef PXR_EXTERNAL_BOOST_PYTHON_OBJECT_ENUM_BASE_HPP
# define PXR_EXTERNAL_BOOST_PYTHON_OBJECT_ENUM_BASE_HPP

#include "pxr/pxr.h"
#include "pxr/external/boost/python/common.hpp"

#ifndef PXR_USE_INTERNAL_BOOST_PYTHON
#include <boost/python/object/enum_base.hpp>
#else

# include "pxr/external/boost/python/object_core.hpp"
# include "pxr/external/boost/python/type_id.hpp"
# include "pxr/external/boost/python/converter/to_python_function_type.hpp"
# include "pxr/external/boost/python/converter/convertible_function.hpp"
# include "pxr/external/boost/python/converter/constructor_function.hpp"

namespace PXR_BOOST_NAMESPACE { namespace python { namespace objects { 

struct PXR_BOOST_PYTHON_DECL enum_base : python::api::object
{
 protected:
    enum_base(
        char const* name
        , converter::to_python_function_t
        , converter::convertible_function
        , converter::constructor_function
        , type_info
        , const char *doc = 0
        );

    void add_value(char const* name, long value);
    void export_values();
    
    static PyObject* to_python(PyTypeObject* type, long x);
};

}}} // namespace PXR_BOOST_NAMESPACE::python::object

#endif // PXR_USE_INTERNAL_BOOST_PYTHON
#endif // PXR_EXTERNAL_BOOST_PYTHON_OBJECT_ENUM_BASE_HPP
