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

    boost::python::class_<This, ThisPtr, boost::noncopyable>("SchemaRegistry", boost::python::no_init)
        .def(TfPySingleton())

        .def("GetSchemaTypeName",
             (TfToken (*)(const TfType &)) &This::GetSchemaTypeName,
             (boost::python::arg("schemaType")))
        .staticmethod("GetSchemaTypeName")
        .def("GetConcreteSchemaTypeName",
             (TfToken (*)(const TfType &)) &This::GetConcreteSchemaTypeName,
             (boost::python::arg("schemaType")))
        .staticmethod("GetConcreteSchemaTypeName")
        .def("GetAPISchemaTypeName",
             (TfToken (*)(const TfType &)) &This::GetAPISchemaTypeName,
             (boost::python::arg("schemaType")))
        .staticmethod("GetAPISchemaTypeName")

        .def("GetTypeFromSchemaTypeName", 
             &This::GetTypeFromSchemaTypeName, 
             (boost::python::arg("typeName")))
        .staticmethod("GetTypeFromSchemaTypeName")
        .def("GetConcreteTypeFromSchemaTypeName", 
             &This::GetConcreteTypeFromSchemaTypeName, 
             (boost::python::arg("typeName")))
        .staticmethod("GetConcreteTypeFromSchemaTypeName")
        .def("GetAPITypeFromSchemaTypeName", 
             &This::GetAPITypeFromSchemaTypeName, 
             (boost::python::arg("typeName")))
        .staticmethod("GetAPITypeFromSchemaTypeName")

        .def("IsDisallowedField",
             &This::IsDisallowedField,
             (boost::python::arg("fieldName")))
        .staticmethod("IsDisallowedField")

        .def("IsTyped",
             &This::IsTyped,
             (boost::python::arg("primType")))
        .staticmethod("IsTyped")

        .def("GetSchemaKind",
             (UsdSchemaKind (*)(const TfType &)) &This::GetSchemaKind,
             (boost::python::arg("primType")))
        .def("GetSchemaKind",
             (UsdSchemaKind (*)(const TfToken &)) &This::GetSchemaKind,
             (boost::python::arg("primType")))
        .staticmethod("GetSchemaKind")

        .def("IsConcrete",
             (bool (*)(const TfType &)) &This::IsConcrete,
             (boost::python::arg("primType")))
        .def("IsConcrete",
             (bool (*)(const TfToken &)) &This::IsConcrete,
             (boost::python::arg("primType")))
        .staticmethod("IsConcrete")

        .def("IsAppliedAPISchema", 
             (bool (*)(const TfType &)) &This::IsAppliedAPISchema,
             (boost::python::arg("apiSchemaType")))
        .def("IsAppliedAPISchema", 
             (bool (*)(const TfToken &)) &This::IsAppliedAPISchema,
             (boost::python::arg("apiSchemaType")))
        .staticmethod("IsAppliedAPISchema")

        .def("IsMultipleApplyAPISchema", 
             (bool (*)(const TfType &)) &This::IsMultipleApplyAPISchema,
             (boost::python::arg("apiSchemaType")))
        .def("IsMultipleApplyAPISchema", 
             (bool (*)(const TfToken &)) &This::IsMultipleApplyAPISchema,
             (boost::python::arg("apiSchemaType")))
        .staticmethod("IsMultipleApplyAPISchema")

        .def("GetTypeFromName", &This::GetTypeFromName, 
            (boost::python::arg("typeName")))
        .staticmethod("GetTypeFromName")

        .def("GetTypeNameAndInstance", &This::GetTypeNameAndInstance,
            (boost::python::arg("typeName")), boost::python::return_value_policy<TfPyPairToTuple>())
        .staticmethod("GetTypeNameAndInstance")

        .def("IsAllowedAPISchemaInstanceName", 
             &This::IsAllowedAPISchemaInstanceName,
             (boost::python::arg("apiSchemaName"), boost::python::arg("instanceName")))
        .staticmethod("IsAllowedAPISchemaInstanceName")

        .def("GetAPISchemaCanOnlyApplyToTypeNames", 
             &This::GetAPISchemaCanOnlyApplyToTypeNames,
             (boost::python::arg("apiSchemaName"), boost::python::arg("instanceName")=TfToken()),
             boost::python::return_value_policy<TfPySequenceToList>())
        .staticmethod("GetAPISchemaCanOnlyApplyToTypeNames")

        .def("GetAutoApplyAPISchemas", &This::GetAutoApplyAPISchemas,
             boost::python::return_value_policy<TfPyMapToDictionary>())
        .staticmethod("GetAutoApplyAPISchemas")

        .def("MakeMultipleApplyNameTemplate", 
             &This::MakeMultipleApplyNameTemplate,
             boost::python::arg("namespacePrefix"),
             boost::python::arg("baseName"))
        .staticmethod("MakeMultipleApplyNameTemplate")

        .def("MakeMultipleApplyNameInstance", 
             &This::MakeMultipleApplyNameInstance,
             boost::python::arg("nameTemplate"),
             boost::python::arg("instanceName"))
        .staticmethod("MakeMultipleApplyNameInstance")

        .def("GetMultipleApplyNameTemplateBaseName", 
             &This::GetMultipleApplyNameTemplateBaseName,
             boost::python::arg("nameTemplate"))
        .staticmethod("GetMultipleApplyNameTemplateBaseName")

        .def("IsMultipleApplyNameTemplate", 
             &This::IsMultipleApplyNameTemplate,
             boost::python::arg("nameTemplate"))
        .staticmethod("IsMultipleApplyNameTemplate")

        .def("FindConcretePrimDefinition", 
             &This::FindConcretePrimDefinition,
             (boost::python::arg("typeName")),
             boost::python::return_internal_reference<>())

        .def("FindAppliedAPIPrimDefinition", 
             &This::FindAppliedAPIPrimDefinition,
             (boost::python::arg("typeName")),
             boost::python::return_internal_reference<>())

        .def("GetEmptyPrimDefinition", 
             &This::GetEmptyPrimDefinition,
             boost::python::return_internal_reference<>())

        .def("BuildComposedPrimDefinition", 
             &_WrapBuildComposedPrimDefinition,
             boost::python::return_value_policy<boost::python::manage_new_object>())

        .def("GetFallbackPrimTypes", &This::GetFallbackPrimTypes, 
             boost::python::return_value_policy<boost::python::return_by_value>())
        ;
}
