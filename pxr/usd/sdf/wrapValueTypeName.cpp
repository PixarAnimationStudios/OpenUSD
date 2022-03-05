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
#include "pxr/usd/sdf/valueTypeName.h"
#include "pxr/base/tf/type.h"
#include "pxr/base/vt/value.h"
#include "pxr/base/tf/pyResultConversions.h"

#include <boost/python.hpp>


PXR_NAMESPACE_USING_DIRECTIVE

void
wrapValueType()
{
    boost::python::class_<SdfValueTypeName>("ValueTypeName", boost::python::no_init)
        .def(boost::python::init<>())
        .def(!boost::python::self)
        .def(boost::python::self == std::string())
        .def(boost::python::self != std::string())
        .def(boost::python::self == boost::python::self)
        .def(boost::python::self != boost::python::self)
        .def("__hash__", &SdfValueTypeName::GetHash)
        .def("__str__", &SdfValueTypeName::GetAsToken)
        .add_property("type",
            boost::python::make_function(&SdfValueTypeName::GetType,
                          boost::python::return_value_policy<boost::python::return_by_value>()))
        .add_property("cppTypeName",
            boost::python::make_function(&SdfValueTypeName::GetCPPTypeName,
                          boost::python::return_value_policy<boost::python::return_by_value>()))
        .add_property("role",
            boost::python::make_function(&SdfValueTypeName::GetRole,
                          boost::python::return_value_policy<boost::python::return_by_value>()))
        .add_property("defaultValue",
            boost::python::make_function(&SdfValueTypeName::GetDefaultValue,
                          boost::python::return_value_policy<boost::python::return_by_value>()))
        .add_property("defaultUnit",
            boost::python::make_function(&SdfValueTypeName::GetDefaultUnit,
                          boost::python::return_value_policy<boost::python::return_by_value>()))
        .add_property("scalarType", &SdfValueTypeName::GetScalarType)
        .add_property("arrayType", &SdfValueTypeName::GetArrayType)
        .add_property("isScalar", &SdfValueTypeName::IsScalar)
        .add_property("isArray", &SdfValueTypeName::IsArray)
        .add_property("aliasesAsStrings",
            boost::python::make_function(&SdfValueTypeName::GetAliasesAsTokens,
                          boost::python::return_value_policy<boost::python::return_by_value>()))
        ;
}
