//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/pxr.h"
#include "pxr/usd/usd/schemaRegistry.h"
#include "pxr/usd/usd/primDefinition.h"

#include "pxr/usd/sdf/attributeSpec.h"
#include "pxr/usd/sdf/propertySpec.h"
#include "pxr/usd/sdf/relationshipSpec.h"

#include "pxr/base/tf/pyResultConversions.h"
#include "pxr/base/tf/pySingleton.h"
#include "pxr/base/tf/pyUtils.h"
#include "pxr/base/tf/pyNanobindBridge.h"
#include "pxr/usd/usd/pyConversions.h"

#include <nanobind/nanobind.h>
#include <nanobind/stl/map.h>
#include <nanobind/stl/pair.h>
#include <nanobind/stl/string.h>
#include <nanobind/stl/vector.h>

namespace nb = nanobind;
using std::string;

PXR_NAMESPACE_USING_DIRECTIVE

static UsdPrimDefinition *
_WrapBuildComposedPrimDefinition(const UsdSchemaRegistry &self,
    const TfToken &primType, const TfTokenVector &appliedAPISchemas) 
{
    return self.BuildComposedPrimDefinition(primType, appliedAPISchemas).release();
}

void wrapUsdSchemaRegistry(nb::module_ &m)
{
    using SchemaInfoConstPtr = const UsdSchemaRegistry::SchemaInfo *;
    using SchemaInfoConstPtrVector = std::vector<SchemaInfoConstPtr>;

    using This = UsdSchemaRegistry;

    nb::class_<This> s(m, "SchemaRegistry",
                       nb::is_weak_referenceable(), nb::dynamic_attr());
    s
        .def(TfNbSingleton())
        .def_static("ParseSchemaFamilyAndVersionFromIdentifier",
             &This::ParseSchemaFamilyAndVersionFromIdentifier,
             nb::arg("schemaIdentifier"))
        .def_static("MakeSchemaIdentifierForFamilyAndVersion",
             &This::MakeSchemaIdentifierForFamilyAndVersion,
             nb::arg("schemaFamily"), nb::arg("schemaVersion"))

        .def_static("IsAllowedSchemaFamily", &This::IsAllowedSchemaFamily,
             nb::arg("schemaFamily"))
        .def_static("IsAllowedSchemaIdentifier", &This::IsAllowedSchemaIdentifier,
             nb::arg("schemaIdentifier"))

        .def_static("FindSchemaInfo", 
             (SchemaInfoConstPtr(*)(const TfType &)) &This::FindSchemaInfo,
             nb::arg("schemaType"),
             nb::rv_policy::reference)
        .def_static("FindSchemaInfo", 
             (SchemaInfoConstPtr(*)(const TfToken &)) &This::FindSchemaInfo,
             nb::arg("schemaIdentifier"),
             nb::rv_policy::reference)
        .def_static("FindSchemaInfo", 
             (SchemaInfoConstPtr(*)(const TfToken &, UsdSchemaVersion)) 
             &This::FindSchemaInfo,
             nb::arg("schemaFamily"), nb::arg("schemaVersion"),
             nb::rv_policy::reference)

        .def_static("FindSchemaInfosInFamily", 
             (const SchemaInfoConstPtrVector &(*)(const TfToken &))
             &This::FindSchemaInfosInFamily,
             nb::arg("schemaFamily"),
             nb::rv_policy::reference)
        .def_static("FindSchemaInfosInFamily",
             (SchemaInfoConstPtrVector(*)(
                 const TfToken &, UsdSchemaVersion, This::VersionPolicy))
             &This::FindSchemaInfosInFamily,
             nb::arg("schemaFamily"), nb::arg("schemaVersion"), nb::arg("versionPolicy"),
             nb::rv_policy::reference)

        .def_static("GetSchemaTypeName",
             (TfToken (*)(const TfType &)) &This::GetSchemaTypeName,
             nb::arg("schemaType"))
        .def_static("GetConcreteSchemaTypeName",
             (TfToken (*)(const TfType &)) &This::GetConcreteSchemaTypeName,
             nb::arg("schemaType"))
        .def_static("GetAPISchemaTypeName",
             (TfToken (*)(const TfType &)) &This::GetAPISchemaTypeName,
             nb::arg("schemaType"))

        .def_static("GetTypeFromSchemaTypeName", 
             &This::GetTypeFromSchemaTypeName, 
             nb::arg("typeName"))
        .def_static("GetConcreteTypeFromSchemaTypeName", 
             &This::GetConcreteTypeFromSchemaTypeName, 
             nb::arg("typeName"))
        .def_static("GetAPITypeFromSchemaTypeName", 
             &This::GetAPITypeFromSchemaTypeName, 
             nb::arg("typeName"))

        .def_static("IsDisallowedField",
             &This::IsDisallowedField,
             nb::arg("fieldName"))

        .def_static("IsTyped",
             &This::IsTyped,
             nb::arg("primType"))

        .def_static("GetSchemaKind",
             (UsdSchemaKind (*)(const TfType &)) &This::GetSchemaKind,
             nb::arg("primType"))
        .def_static("GetSchemaKind",
             (UsdSchemaKind (*)(const TfToken &)) &This::GetSchemaKind,
             nb::arg("primType"))

        .def_static("IsConcrete",
             (bool (*)(const TfType &)) &This::IsConcrete,
             nb::arg("primType"))
        .def_static("IsConcrete",
             (bool (*)(const TfToken &)) &This::IsConcrete,
             nb::arg("primType"))

        .def_static("IsAbstract",
             (bool (*)(const TfType &)) &This::IsAbstract,
             nb::arg("primType"))
        .def_static("IsAbstract",
             (bool (*)(const TfToken&)) &This::IsAbstract,
             nb::arg("primType"))

        .def_static("IsAppliedAPISchema", 
             (bool (*)(const TfType &)) &This::IsAppliedAPISchema,
             nb::arg("apiSchemaType"))
        .def_static("IsAppliedAPISchema", 
             (bool (*)(const TfToken &)) &This::IsAppliedAPISchema,
             nb::arg("apiSchemaType"))

        .def_static("IsMultipleApplyAPISchema", 
             (bool (*)(const TfType &)) &This::IsMultipleApplyAPISchema,
             nb::arg("apiSchemaType"))
        .def_static("IsMultipleApplyAPISchema", 
             (bool (*)(const TfToken &)) &This::IsMultipleApplyAPISchema,
             nb::arg("apiSchemaType"))

        .def_static("GetTypeFromName", &This::GetTypeFromName, 
             nb::arg("typeName"))

        .def_static("GetTypeNameAndInstance", &This::GetTypeNameAndInstance,
             nb::arg("typeName"))

        .def_static("IsAllowedAPISchemaInstanceName", 
             &This::IsAllowedAPISchemaInstanceName,
             nb::arg("apiSchemaName"), nb::arg("instanceName"))

        .def_static("GetAPISchemaCanOnlyApplyToTypeNames", 
             &This::GetAPISchemaCanOnlyApplyToTypeNames,
             nb::arg("apiSchemaName"), nb::arg("instanceName")=TfToken())

        .def_static("GetAutoApplyAPISchemas", &This::GetAutoApplyAPISchemas,
             nb::rv_policy::copy)

        .def_static("MakeMultipleApplyNameTemplate", 
             &This::MakeMultipleApplyNameTemplate,
             nb::arg("namespacePrefix"),
             nb::arg("baseName"))

        .def_static("MakeMultipleApplyNameInstance", 
             &This::MakeMultipleApplyNameInstance,
             nb::arg("nameTemplate"),
             nb::arg("instanceName"))

        .def_static("GetMultipleApplyNameTemplateBaseName", 
             &This::GetMultipleApplyNameTemplateBaseName,
             nb::arg("nameTemplate"))

        .def_static("IsMultipleApplyNameTemplate", 
             &This::IsMultipleApplyNameTemplate,
             nb::arg("nameTemplate"))

        .def("FindAbstractPrimDefinition", 
             &This::FindAbstractPrimDefinition,
             nb::arg("typeName"),
             nb::rv_policy::reference_internal)

        .def("FindConcretePrimDefinition", 
             &This::FindConcretePrimDefinition,
             nb::arg("typeName"),
             nb::rv_policy::reference_internal)

        .def("FindAppliedAPIPrimDefinition", 
             &This::FindAppliedAPIPrimDefinition,
             nb::arg("typeName"),
             nb::rv_policy::reference_internal)

        .def("GetEmptyPrimDefinition", 
             &This::GetEmptyPrimDefinition,
             nb::rv_policy::reference_internal)

        .def("BuildComposedPrimDefinition", 
             &_WrapBuildComposedPrimDefinition,
             nb::rv_policy::take_ownership)

        .def("GetFallbackPrimTypes", &This::GetFallbackPrimTypes, 
             nb::rv_policy::copy)
        ;

    // Need to convert TfToken properties of SchemaInfo to string
    // to wrap them to python
    auto wrapIdentifier = +[](const This::SchemaInfo &schemaInfo) {
        return schemaInfo.identifier.GetString();
    };
    auto wrapFamily = +[](const This::SchemaInfo &schemaInfo) {
        return schemaInfo.family.GetString();
    };

    nb::class_<This::SchemaInfo>(s, "SchemaInfo")
        .def("__init__", [](nb::args, nb::kwargs) {
            TfPyThrowRuntimeError("This class cannot be instantiated from Python");
        })
        .def_prop_ro("identifier", wrapIdentifier)
        .def_ro("type", &This::SchemaInfo::type)
        .def_prop_ro("family", wrapFamily)
        .def_ro("version", &This::SchemaInfo::version)
        .def_ro("kind", &This::SchemaInfo::kind)
        ;

    nb::enum_<UsdSchemaRegistry::VersionPolicy>(s, "VersionPolicy")
        .value("All", UsdSchemaRegistry::VersionPolicy::All)
        .value("GreaterThan", UsdSchemaRegistry::VersionPolicy::GreaterThan)
        .value("GreaterThanOrEqual", UsdSchemaRegistry::VersionPolicy::GreaterThanOrEqual)
        .value("LessThan", UsdSchemaRegistry::VersionPolicy::LessThan)
        .value("LessThanOrEqual", UsdSchemaRegistry::VersionPolicy::LessThanOrEqual)
    ;
}
