//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_BASE_TF_PY_OPTIONAL_H
#define PXR_BASE_TF_PY_OPTIONAL_H

/// \file tf/pyOptional.h

#include "pxr/pxr.h"

#include "pxr/base/tf/pyUtils.h"
#include <boost/optional.hpp>
#include <boost/python/converter/from_python.hpp>
#include <boost/python/extract.hpp>
#include <boost/python/to_python_converter.hpp>
#include <boost/python/to_python_value.hpp>

#include <optional>

PXR_NAMESPACE_OPEN_SCOPE

// Adapted from original at:
// http://mail.python.org/pipermail/cplusplus-sig/2007-May/012003.html

namespace TfPyOptional {

template <typename T, typename TfromPy>
struct object_from_python
{
    object_from_python() {
        boost::python::converter::registry::push_back
        (&TfromPy::convertible, &TfromPy::construct,
         boost::python::type_id<T>());
    }
};

template <typename T, typename TtoPy, typename TfromPy>
struct register_python_conversion
{
    register_python_conversion() {
        boost::python::to_python_converter<T, TtoPy>();
        object_from_python<T, TfromPy>();
    }
};

template <typename T>
struct python_optional
{
    python_optional(const python_optional&) = delete;
    python_optional& operator=(const python_optional&) = delete;
    template <typename Optional>
    struct optional_to_python
    {
        static PyObject * convert(const Optional& value)
        {
            if (value) {
                boost::python::object obj = TfPyObject(*value);
                Py_INCREF(obj.ptr());
                return obj.ptr();
            }
            return boost::python::detail::none();
        }
    };

    template <typename Optional>
    struct optional_from_python
    {
        static void * convertible(PyObject * source)
        {
            using namespace boost::python::converter;

            if ((source == Py_None) || boost::python::extract<T>(source).check())
                return source;

            return NULL;
        }

        static void construct(PyObject * source,
                              boost::python::converter::rvalue_from_python_stage1_data * data)
        {
            using namespace boost::python::converter;

            void * const storage =
                ((rvalue_from_python_storage<T> *)data)->storage.bytes;

            if (data->convertible == Py_None) {
                new (storage) Optional(); // An uninitialized optional
            } else {
                new (storage) Optional(boost::python::extract<T>(source));
            }

            data->convertible = storage;
        }
    };

    explicit python_optional() {
        register_python_conversion<
            std::optional<T>,
            optional_to_python<std::optional<T>>, 
            optional_from_python<std::optional<T>>>();
        register_python_conversion<
            boost::optional<T>,
            optional_to_python<boost::optional<T>>, 
            optional_from_python<boost::optional<T>>>();
    }
};

} // namespace TfPyOptional

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_BASE_TF_PY_OPTIONAL_H
