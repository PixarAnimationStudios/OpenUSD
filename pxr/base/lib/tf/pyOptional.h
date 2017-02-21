//
// Copyright 2016 Pixar
//
// Licensed under the Apache License, Version 2.0 (the "Apache License")
// with the following modification; you may not use this file except in
// compliance with the Apache License and the following modification to it:
// Section 6. Trademarks. is deleted and replaced with:
//
// 6. Trademarks. This License does not grant permission to use the trade
//    names, trademarks, service marks, or product names of the Licensor
//    and its affiliates, except as required to comply with Section 4(c) of
//    the License and to reproduce the content of the NOTICE file.
//
// You may obtain a copy of the Apache License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the Apache License with the above modification is
// distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
// KIND, either express or implied. See the Apache License for the specific
// language governing permissions and limitations under the Apache License.
//
#ifndef TF_PYOPTIONAL_H
#define TF_PYOPTIONAL_H

/// \file tf/pyOptional.h

#include "pxr/pxr.h"

#include "pxr/base/tf/pyUtils.h"
#include <boost/noncopyable.hpp>
#include <boost/optional.hpp>
#include <boost/python/converter/from_python.hpp>
#include <boost/python/extract.hpp>
#include <boost/python/to_python_converter.hpp>
#include <boost/python/to_python_value.hpp>

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
struct python_optional : public boost::noncopyable
{
    struct optional_to_python
    {
        static PyObject * convert(const boost::optional<T>& value)
        {
            if (value) {
                boost::python::object obj = TfPyObject(*value);
                Py_INCREF(obj.ptr());
                return obj.ptr();
            }
            return boost::python::detail::none();
        }
    };

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
                new (storage) boost::optional<T>(); // An uninitialized optional
            } else {
                new (storage) boost::optional<T>(boost::python::extract<T>(source));
            }

            data->convertible = storage;
        }
    };

    explicit python_optional() {
        register_python_conversion<boost::optional<T>,
                                   optional_to_python, optional_from_python>();
    }
};

} // namespace TfPyOptional

PXR_NAMESPACE_CLOSE_SCOPE

#endif // TF_PYOPTIONAL_H
