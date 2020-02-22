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
#include "pxr/usd/usd/primDefinition.h"

#include "pxr/usd/sdf/attributeSpec.h"
#include "pxr/usd/sdf/propertySpec.h"
#include "pxr/usd/sdf/relationshipSpec.h"

#include "pxr/base/tf/pyResultConversions.h"
#include "pxr/base/tf/pySingleton.h"

#include <boost/python.hpp>

using std::string;

using namespace boost::python;

PXR_NAMESPACE_USING_DIRECTIVE

void wrapUsdSchemaRegistry()
{
    typedef UsdSchemaRegistry This;
    typedef TfWeakPtr<UsdSchemaRegistry> ThisPtr;

    class_<This, ThisPtr, boost::noncopyable>("SchemaRegistry", no_init)
        .def(TfPySingleton())

        .def("GetSchemaTypeName",
             (TfToken (This::*)(const TfType &) const)
                &This::GetSchemaTypeName,
             arg("schemaType"))

        .def("GetSchemaPrimSpec", (SdfPrimSpecHandle (*)(const TfToken &))
             &This::GetSchemaPrimSpec,
             arg("primType"))
        .def("GetSchemaPrimSpec", (SdfPrimSpecHandle (*)(const TfType &))
             &This::GetSchemaPrimSpec,
             arg("primType"))
        .staticmethod("GetSchemaPrimSpec")

        .def("GetSchemaPropertySpec", &This::GetSchemaPropertySpec,
             (arg("primType"), arg("propName")))
        .staticmethod("GetSchemaPropertySpec")

        .def("GetSchemaAttributeSpec",
             &This::GetSchemaAttributeSpec,
             (arg("primType"), arg("attrName")))
        .staticmethod("GetSchemaAttributeSpec")

        .def("GetSchemaRelationshipSpec",
             &This::GetSchemaRelationshipSpec,
             (arg("primType"), arg("relName")))
        .staticmethod("GetSchemaRelationshipSpec")

        .def("GetDisallowedFields",
             &This::GetDisallowedFields,
             return_value_policy<TfPySequenceToList>())
        .staticmethod("GetDisallowedFields")

        .def("IsTyped",
             &This::IsTyped,
             (arg("primType")))
        .staticmethod("IsTyped")

        .def("IsConcrete",
             (bool (This::*)(const TfType &) const)
             &This::IsConcrete,
             (arg("primType")))
        .def("IsConcrete",
             (bool (This::*)(const TfToken &) const)
             &This::IsConcrete,
             (arg("primType")))

        .def("IsAppliedAPISchema", 
             (bool (This::*)(const TfType &) const)
             &This::IsAppliedAPISchema,
             (arg("apiSchemaType")))
        .def("IsAppliedAPISchema", 
             (bool (This::*)(const TfToken &) const)
             &This::IsAppliedAPISchema,
             (arg("apiSchemaType")))

        .def("IsMultipleApplyAPISchema", 
             (bool (This::*)(const TfType &) const)
             &This::IsMultipleApplyAPISchema,
             (arg("apiSchemaType")))
        .def("IsMultipleApplyAPISchema", 
             (bool (This::*)(const TfToken &) const)
             &This::IsMultipleApplyAPISchema,
             (arg("apiSchemaType")))

        .def("GetTypeFromName", &This::GetTypeFromName, 
            (arg("typeName")))
        .staticmethod("GetTypeFromName")

        .def("FindConcretePrimDefinition", 
             &This::FindConcretePrimDefinition,
             (arg("typeName")),
             return_internal_reference<>())

        .def("FindAppliedAPIPrimDefinition", 
             &This::FindAppliedAPIPrimDefinition,
             (arg("typeName")),
             return_internal_reference<>())

        .def("GetEmptyPrimDefinition", 
             &This::GetEmptyPrimDefinition,
             return_internal_reference<>())
        ;
}
