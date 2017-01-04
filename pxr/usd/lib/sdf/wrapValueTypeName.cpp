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
#include "pxr/usd/sdf/valueTypeName.h"
#include "pxr/base/tf/type.h"
#include "pxr/base/vt/value.h"
#include "pxr/base/tf/pyResultConversions.h"

#include <boost/python.hpp>

using namespace boost::python;

void
wrapValueType()
{
    class_<SdfValueTypeName>("ValueTypeName", no_init)
        .def(init<>())
        .def(!self)
        .def(self == std::string())
        .def(self != std::string())
        .def(self == self)
        .def(self != self)
        .def("__hash__", &SdfValueTypeName::GetHash)
        .def("__str__", &SdfValueTypeName::GetAsToken)
        .add_property("type",
            make_function(&SdfValueTypeName::GetType,
                          return_value_policy<return_by_value>()))
        .add_property("role",
            make_function(&SdfValueTypeName::GetRole,
                          return_value_policy<return_by_value>()))
        .add_property("defaultValue",
            make_function(&SdfValueTypeName::GetDefaultValue,
                          return_value_policy<return_by_value>()))
        .add_property("defaultUnit",
            make_function(&SdfValueTypeName::GetDefaultUnit,
                          return_value_policy<return_by_value>()))
        .add_property("scalarType", &SdfValueTypeName::GetScalarType)
        .add_property("arrayType", &SdfValueTypeName::GetArrayType)
        .add_property("isScalar", &SdfValueTypeName::IsScalar)
        .add_property("isArray", &SdfValueTypeName::IsArray)
        .add_property("aliasesAsStrings",
            make_function(&SdfValueTypeName::GetAliasesAsTokens,
                          return_value_policy<return_by_value>()))
        ;
}
