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

static UsdPrimDefinition *
_WrapBuildComposedPrimDefinition(const UsdSchemaRegistry &self,
    const TfToken &primType, const TfTokenVector &appliedAPISchemas) 
{
    return self.BuildComposedPrimDefinition(primType, appliedAPISchemas).release();
}

void wrapUsdSchemaRegistry()
{
    typedef UsdSchemaRegistry This;
    typedef TfWeakPtr<UsdSchemaRegistry> ThisPtr;

    class_<This, ThisPtr, boost::noncopyable>("SchemaRegistry", no_init)
        .def(TfPySingleton())

        .def("GetSchemaTypeName",
             (TfToken (*)(const TfType &)) &This::GetSchemaTypeName,
             (arg("schemaType")))
        .staticmethod("GetSchemaTypeName")
        .def("GetConcreteSchemaTypeName",
             (TfToken (*)(const TfType &)) &This::GetConcreteSchemaTypeName,
             (arg("schemaType")))
        .staticmethod("GetConcreteSchemaTypeName")
        .def("GetAPISchemaTypeName",
             (TfToken (*)(const TfType &)) &This::GetAPISchemaTypeName,
             (arg("schemaType")))
        .staticmethod("GetAPISchemaTypeName")

        .def("GetTypeFromSchemaTypeName", 
             &This::GetTypeFromSchemaTypeName, 
             (arg("typeName")))
        .staticmethod("GetTypeFromSchemaTypeName")
        .def("GetConcreteTypeFromSchemaTypeName", 
             &This::GetConcreteTypeFromSchemaTypeName, 
             (arg("typeName")))
        .staticmethod("GetConcreteTypeFromSchemaTypeName")
        .def("GetAPITypeFromSchemaTypeName", 
             &This::GetAPITypeFromSchemaTypeName, 
             (arg("typeName")))
        .staticmethod("GetAPITypeFromSchemaTypeName")

        .def("IsDisallowedField",
             &This::IsDisallowedField,
             (arg("fieldName")))
        .staticmethod("IsDisallowedField")

        .def("IsTyped",
             &This::IsTyped,
             (arg("primType")))
        .staticmethod("IsTyped")

        .def("GetSchemaKind",
             (UsdSchemaKind (*)(const TfType &)) &This::GetSchemaKind,
             (arg("primType")))
        .def("GetSchemaKind",
             (UsdSchemaKind (*)(const TfToken &)) &This::GetSchemaKind,
             (arg("primType")))
        .staticmethod("GetSchemaKind")

        .def("IsConcrete",
             (bool (*)(const TfType &)) &This::IsConcrete,
             (arg("primType")))
        .def("IsConcrete",
             (bool (*)(const TfToken &)) &This::IsConcrete,
             (arg("primType")))
        .staticmethod("IsConcrete")

        .def("IsAppliedAPISchema", 
             (bool (*)(const TfType &)) &This::IsAppliedAPISchema,
             (arg("apiSchemaType")))
        .def("IsAppliedAPISchema", 
             (bool (*)(const TfToken &)) &This::IsAppliedAPISchema,
             (arg("apiSchemaType")))
        .staticmethod("IsAppliedAPISchema")

        .def("IsMultipleApplyAPISchema", 
             (bool (*)(const TfType &)) &This::IsMultipleApplyAPISchema,
             (arg("apiSchemaType")))
        .def("IsMultipleApplyAPISchema", 
             (bool (*)(const TfToken &)) &This::IsMultipleApplyAPISchema,
             (arg("apiSchemaType")))
        .staticmethod("IsMultipleApplyAPISchema")

        .def("GetTypeFromName", &This::GetTypeFromName, 
            (arg("typeName")))
        .staticmethod("GetTypeFromName")

        .def("GetTypeAndInstance", &This::GetTypeAndInstance,
            (arg("typeName")), return_value_policy<TfPyPairToTuple>())
        .staticmethod("GetTypeAndInstance")

        .def("GetAutoApplyAPISchemas", &This::GetAutoApplyAPISchemas,
             return_value_policy<TfPyMapToDictionary>())
        .staticmethod("GetAutoApplyAPISchemas")

        .def("GetPropertyNamespacePrefix", &This::GetPropertyNamespacePrefix, 
            (arg("multiApplyAPISchemaName")))

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

        .def("BuildComposedPrimDefinition", 
             &_WrapBuildComposedPrimDefinition,
             return_value_policy<manage_new_object>())

        .def("GetFallbackPrimTypes", &This::GetFallbackPrimTypes, 
             return_value_policy<return_by_value>())
        ;
}
