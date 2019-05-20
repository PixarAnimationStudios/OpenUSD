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
#include "pxr/usd/usd/schemaBase.h"

#include "pxr/usd/sdf/primSpec.h"

#include "pxr/base/tf/pyContainerConversions.h"
#include "pxr/base/tf/pyResultConversions.h"
#include "pxr/base/tf/wrapTypeHelpers.h"

#include <boost/python/class.hpp>
#include <boost/python/operators.hpp>

using std::string;

using namespace boost::python;

PXR_NAMESPACE_USING_DIRECTIVE

void wrapUsdSchemaBase()
{
    class_<UsdSchemaBase> cls("SchemaBase");

    cls
        .def(init<UsdPrim>(arg("prim")))
        .def(init<UsdSchemaBase const&>(arg("otherSchema")))
        .def(TfTypePythonClass())

        .def("GetPrim", &UsdSchemaBase::GetPrim)
        .def("GetPath", &UsdSchemaBase::GetPath)
        .def("GetSchemaClassPrimDefinition",
             &UsdSchemaBase::GetSchemaClassPrimDefinition)
        .def("GetSchemaAttributeNames", &UsdSchemaBase::GetSchemaAttributeNames,
             arg("includeInherited")=true,
             return_value_policy<TfPySequenceToList>())
        .staticmethod("GetSchemaAttributeNames")

        .def("IsAPISchema", &UsdSchemaBase::IsAPISchema)
        .def("IsConcrete", &UsdSchemaBase::IsConcrete) 
        .def("IsTyped", &UsdSchemaBase::IsTyped) 
        .def("IsAppliedAPISchema", &UsdSchemaBase::IsAppliedAPISchema) 
        .def("IsMultipleApplyAPISchema", &UsdSchemaBase::IsMultipleApplyAPISchema) 

        .def("GetSchemaType", &UsdSchemaBase::GetSchemaType)

        .def(!self)

        ;
}
