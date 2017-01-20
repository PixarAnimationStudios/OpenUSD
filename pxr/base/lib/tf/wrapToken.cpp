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

#include "pxr/pxr.h"

#include "pxr/base/tf/token.h"

#include "pxr/base/tf/pyContainerConversions.h"

#include <boost/python/def.hpp>
#include <boost/python/to_python_converter.hpp>
#include <boost/python/str.hpp>
#include <boost/python/object.hpp>

#include <set>
#include <string>

namespace bp = boost::python;

PXR_NAMESPACE_OPEN_SCOPE

struct Tf_TokenFromPythonString
{
    Tf_TokenFromPythonString() {
        bp::converter::registry::insert
            (&convertible, &construct, bp::type_id<TfToken>());
    }
    static void *convertible(PyObject *obj) {
        bp::extract<std::string> s(obj);
        return s.check() ? obj : 0;
    }
    static void construct(PyObject *src,
                          bp::converter::rvalue_from_python_stage1_data *data) {
        bp::extract<std::string> s(src);
        void *storage =
            ((bp::converter::
              rvalue_from_python_storage<TfToken> *)data)->storage.bytes;
        new (storage) TfToken( s() );
        data->convertible = storage;
    }
};

struct Tf_TokenToPythonString {
    static PyObject* convert(TfToken const &val) {
        return bp::incref(bp::str(val.GetString()).ptr());
    }
};

void TfDumpTokenStats(); // Defined in token.cpp.

void wrapToken()
{    
    TfPyContainerConversions::from_python_sequence<
        std::set<TfToken> , 
        TfPyContainerConversions::set_policy >();

    TfPyContainerConversions::from_python_sequence<
        std::vector<TfToken>,
        TfPyContainerConversions::variable_capacity_policy >();

    boost::python::to_python_converter<
        std::vector<TfToken>, 
        TfPySequenceToPython<std::vector<TfToken> > >();

    // Tokens are represented directly as Python strings in Python.
    bp::to_python_converter<TfToken, Tf_TokenToPythonString>();
    Tf_TokenFromPythonString();

    // Stats.
    bp::def("DumpTokenStats", TfDumpTokenStats);
}

PXR_NAMESPACE_CLOSE_SCOPE
