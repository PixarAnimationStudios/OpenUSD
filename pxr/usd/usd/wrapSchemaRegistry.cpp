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
#include "pxr/usd/usd/schemaRegistry.h"

#include "pxr/usd/sdf/attributeSpec.h"
#include "pxr/usd/sdf/propertySpec.h"
#include "pxr/usd/sdf/relationshipSpec.h"

#include "pxr/base/tf/pyResultConversions.h"

#include <boost/python.hpp>

using std::string;

using namespace boost::python;

PXR_NAMESPACE_USING_DIRECTIVE

static bool
_WrapIsAppliedAPISchema(const TfType &schemaType)
{
    return UsdSchemaRegistry::GetInstance().IsAppliedAPISchema(schemaType);
}

static bool
_WrapIsMultipleApplyAPISchema(const TfType &schemaType)
{
    return UsdSchemaRegistry::GetInstance().IsMultipleApplyAPISchema(
            schemaType);
}

void wrapUsdSchemaRegistry()
{
    class_<UsdSchemaRegistry>("SchemaRegistry", no_init)

        .def("GetSchematics", UsdSchemaRegistry::GetSchematics,
             return_value_policy<return_by_value>())
        .staticmethod("GetSchematics")

        .def("GetPrimDefinition", (SdfPrimSpecHandle (*)(const TfToken &))
             &UsdSchemaRegistry::GetPrimDefinition,
             arg("primType"))
        .def("GetPrimDefinition", (SdfPrimSpecHandle (*)(const TfType &))
             &UsdSchemaRegistry::GetPrimDefinition,
             arg("primType"))
        .staticmethod("GetPrimDefinition")

        .def("GetPropertyDefinition", &UsdSchemaRegistry::GetPropertyDefinition,
             (arg("primType"), arg("propName")))
        .staticmethod("GetPropertyDefinition")

        .def("GetAttributeDefinition",
             &UsdSchemaRegistry::GetAttributeDefinition,
             (arg("primType"), arg("attrName")))
        .staticmethod("GetAttributeDefinition")

        .def("GetRelationshipDefinition",
             &UsdSchemaRegistry::GetRelationshipDefinition,
             (arg("primType"), arg("relName")))
        .staticmethod("GetRelationshipDefinition")

        .def("GetDisallowedFields",
             &UsdSchemaRegistry::GetDisallowedFields,
             return_value_policy<TfPySequenceToList>())
        .staticmethod("GetDisallowedFields")

        .def("IsTyped",
             &UsdSchemaRegistry::IsTyped,
             (arg("primType")))
        .staticmethod("IsTyped")

        .def("IsConcrete",
             &UsdSchemaRegistry::IsConcrete,
             (arg("primType")))
        .staticmethod("IsConcrete")

        .def("IsAppliedAPISchema", &_WrapIsAppliedAPISchema,
             (arg("apiSchemaType")))
        .staticmethod("IsAppliedAPISchema")

        .def("IsMultipleApplyAPISchema", &_WrapIsMultipleApplyAPISchema,
             (arg("apiSchemaType")))
        .staticmethod("IsMultipleApplyAPISchema")

        .def("GetTypeFromName", &UsdSchemaRegistry::GetTypeFromName, 
            (arg("typeName")))
        .staticmethod("GetTypeFromName")
        ;
}
