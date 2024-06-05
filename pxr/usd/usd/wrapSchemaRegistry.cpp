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

#include <boost/python.hpp>
#include <boost/python/enum.hpp>

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
    using SchemaInfoConstPtr = const UsdSchemaRegistry::SchemaInfo *;
    using SchemaInfoConstPtrVector = std::vector<SchemaInfoConstPtr>;

    using This = UsdSchemaRegistry;
    using ThisPtr = TfWeakPtr<UsdSchemaRegistry>;

    scope s = class_<This, ThisPtr, boost::noncopyable>("SchemaRegistry", no_init)
        .def(TfPySingleton())
        .def("ParseSchemaFamilyAndVersionFromIdentifier",
             &This::ParseSchemaFamilyAndVersionFromIdentifier,
             (arg("schemaIdentifier")),
             return_value_policy<TfPyPairToTuple>())
        .staticmethod("ParseSchemaFamilyAndVersionFromIdentifier")
        .def("MakeSchemaIdentifierForFamilyAndVersion",
             &This::MakeSchemaIdentifierForFamilyAndVersion,
             (arg("schemaFamily"), arg("schemaVersion")))
        .staticmethod("MakeSchemaIdentifierForFamilyAndVersion")

        .def("IsAllowedSchemaFamily", &This::IsAllowedSchemaFamily,
             arg("schemaFamily"))
        .staticmethod("IsAllowedSchemaFamily")
        .def("IsAllowedSchemaIdentifier", &This::IsAllowedSchemaIdentifier,
             arg("schemaIdentifier"))
        .staticmethod("IsAllowedSchemaIdentifier")

        .def("FindSchemaInfo", 
             (SchemaInfoConstPtr(*)(const TfType &)) &This::FindSchemaInfo,
             (arg("schemaType")),
             return_internal_reference<>())
        .def("FindSchemaInfo", 
             (SchemaInfoConstPtr(*)(const TfToken &)) &This::FindSchemaInfo,
             (arg("schemaIdentifier")),
             return_internal_reference<>())
        .def("FindSchemaInfo", 
             (SchemaInfoConstPtr(*)(const TfToken &, UsdSchemaVersion)) 
             &This::FindSchemaInfo,
             (arg("schemaFamily"), arg("schemaVersion")),
             return_internal_reference<>())
        .staticmethod("FindSchemaInfo")

        .def("FindSchemaInfosInFamily", 
             (const SchemaInfoConstPtrVector &(*)(const TfToken &))
             &This::FindSchemaInfosInFamily,
             arg("schemaFamily"),
             return_value_policy<TfPySequenceToList>())
        .def("FindSchemaInfosInFamily",
             (SchemaInfoConstPtrVector(*)(
                 const TfToken &, UsdSchemaVersion, This::VersionPolicy))
             &This::FindSchemaInfosInFamily,
             (arg("schemaFamily"), arg("schemaVersion"), arg("versionPolicy")),
             return_value_policy<TfPySequenceToList>())
        .staticmethod("FindSchemaInfosInFamily")

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

        .def("IsAbstract",
             (bool (*)(const TfType &)) &This::IsAbstract,
             (arg("primType")))
        .def("IsAbstract",
             (bool (*)(const TfToken&)) &This::IsAbstract,
             (arg("primType")))
        .staticmethod("IsAbstract")

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

        .def("GetTypeNameAndInstance", &This::GetTypeNameAndInstance,
            (arg("typeName")), return_value_policy<TfPyPairToTuple>())
        .staticmethod("GetTypeNameAndInstance")

        .def("IsAllowedAPISchemaInstanceName", 
             &This::IsAllowedAPISchemaInstanceName,
             (arg("apiSchemaName"), arg("instanceName")))
        .staticmethod("IsAllowedAPISchemaInstanceName")

        .def("GetAPISchemaCanOnlyApplyToTypeNames", 
             &This::GetAPISchemaCanOnlyApplyToTypeNames,
             (arg("apiSchemaName"), arg("instanceName")=TfToken()),
             return_value_policy<TfPySequenceToList>())
        .staticmethod("GetAPISchemaCanOnlyApplyToTypeNames")

        .def("GetAutoApplyAPISchemas", &This::GetAutoApplyAPISchemas,
             return_value_policy<TfPyMapToDictionary>())
        .staticmethod("GetAutoApplyAPISchemas")

        .def("MakeMultipleApplyNameTemplate", 
             &This::MakeMultipleApplyNameTemplate,
             arg("namespacePrefix"),
             arg("baseName"))
        .staticmethod("MakeMultipleApplyNameTemplate")

        .def("MakeMultipleApplyNameInstance", 
             &This::MakeMultipleApplyNameInstance,
             arg("nameTemplate"),
             arg("instanceName"))
        .staticmethod("MakeMultipleApplyNameInstance")

        .def("GetMultipleApplyNameTemplateBaseName", 
             &This::GetMultipleApplyNameTemplateBaseName,
             arg("nameTemplate"))
        .staticmethod("GetMultipleApplyNameTemplateBaseName")

        .def("IsMultipleApplyNameTemplate", 
             &This::IsMultipleApplyNameTemplate,
             arg("nameTemplate"))
        .staticmethod("IsMultipleApplyNameTemplate")

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

    // Need to convert TfToken properties of SchemaInfo to string
    // to wrap them to python
    auto wrapIdentifier = +[](const This::SchemaInfo &schemaInfo) {
        return schemaInfo.identifier.GetString();
    };
    auto wrapFamily = +[](const This::SchemaInfo &schemaInfo) {
        return schemaInfo.family.GetString();
    };

    class_<This::SchemaInfo>("SchemaInfo", no_init)
        .add_property("identifier", wrapIdentifier)
        .def_readonly("type", &This::SchemaInfo::type)
        .add_property("family", wrapFamily)
        .def_readonly("version", &This::SchemaInfo::version)
        .def_readonly("kind", &This::SchemaInfo::kind)
        ;

    enum_<UsdSchemaRegistry::VersionPolicy>("VersionPolicy")
        .value("All", UsdSchemaRegistry::VersionPolicy::All)
        .value("GreaterThan", UsdSchemaRegistry::VersionPolicy::GreaterThan)
        .value("GreaterThanOrEqual", UsdSchemaRegistry::VersionPolicy::GreaterThanOrEqual)
        .value("LessThan", UsdSchemaRegistry::VersionPolicy::LessThan)
        .value("LessThanOrEqual", UsdSchemaRegistry::VersionPolicy::LessThanOrEqual)
    ;    
}
