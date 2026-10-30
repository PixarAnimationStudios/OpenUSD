//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/usd/usd/modelAPI.h"
#include "pxr/usd/usd/schemaBase.h"

#include "pxr/usd/sdf/primSpec.h"

#include "pxr/usd/usd/pyConversions.h"
#include "pxr/base/tf/pyContainerConversions.h"
#include "pxr/base/tf/pyResultConversions.h"
#include "pxr/base/tf/pyUtils.h"
#include "pxr/base/tf/pyNanobindBridge.h"
#include "pxr/base/tf/wrapTypeHelpers.h"
#include <nanobind/nanobind.h>
#include <nanobind/operators.h>
#include <nanobind/stl/string.h>
#include <nanobind/stl/vector.h>
#include <nanobind/stl/pair.h>
#include <nanobind/stl/map.h>
#include <nanobind/stl/tuple.h>

namespace nb = nanobind;

#include <string>

PXR_NAMESPACE_USING_DIRECTIVE

namespace {

#define WRAP_CUSTOM                                                     \
    template <class Cls> static void _CustomWrapCode(Cls &_class)

// fwd decl.
WRAP_CUSTOM;

static std::string
_Repr(const UsdModelAPI &self)
{
    std::string primRepr = TfPyRepr(self.GetPrim());
    return TfStringPrintf(
        "Usd.ModelAPI(%s)",
        primRepr.c_str());
}

} // anonymous namespace

void wrapUsdModelAPI(nb::module_ &m)
{
    typedef UsdModelAPI This;

    nb::class_<This, UsdAPISchemaBase> cls(m, "ModelAPI");

    cls
        .def(nb::init<UsdPrim>(), nb::arg("prim")=UsdPrim())
        .def(nb::init<UsdSchemaBase const&>(), nb::arg("schemaObj"))
        .def(TfTypePythonClass())

        .def_static("Get", &This::Get, nb::arg("stage"), nb::arg("path"))

        .def_static("GetSchemaAttributeNames",
             &This::GetSchemaAttributeNames,
             nb::arg("includeInherited")=true,
             nb::rv_policy::copy)

        .def_static("_GetStaticTfType", (TfType const &(*)()) TfType::Find<This>,
             nb::rv_policy::copy)

        .def("__bool__", [](const This &self) { return bool(self); })

        .def("__repr__", ::_Repr)
    ;

    _CustomWrapCode(cls);
}

// ===================================================================== //
// Feel free to add custom code below this line, it will be preserved by 
// the code generator.  The entry point for your custom code should look
// minimally like the following:
//
// WRAP_CUSTOM {
//     _class
//         .def("MyCustomMethod", ...)
//     ;
// }
//
// Of course any other ancillary or support code may be provided.
// 
// Just remember to wrap code in the appropriate delimiters:
// 'namespace {', '}'.
//
// ===================================================================== //
// --(BEGIN CUSTOM CODE)--

#include "pxr/base/tf/pyEnum.h"
#include "pxr/base/tf/pyStaticTokens.h"

namespace {

static TfToken _GetKind(const UsdModelAPI &self) {
    TfToken result;
    self.GetKind(&result);
    return result;
}

static SdfAssetPath _GetAssetIdentifier(const UsdModelAPI &self) {
    SdfAssetPath identifier;
    self.GetAssetIdentifier(&identifier);
    return identifier;
}

static std::string _GetAssetName(const UsdModelAPI &self) {
    std::string assetName;
    self.GetAssetName(&assetName);
    return assetName;
}

static std::string _GetAssetVersion(const UsdModelAPI &self) {
    std::string version;
    self.GetAssetVersion(&version);
    return version;
}

static VtArray<SdfAssetPath> _GetPayloadAssetDependencies(const UsdModelAPI &self) {
    VtArray<SdfAssetPath> assetDeps;
    self.GetPayloadAssetDependencies(&assetDeps);
    return assetDeps;
}

static VtDictionary _GetAssetInfo(const UsdModelAPI &self) {
    VtDictionary info;
    self.GetAssetInfo(&info);
    return info;
}

WRAP_CUSTOM {

    TF_PY_WRAP_PUBLIC_TOKENS("AssetInfoKeys", UsdModelAPIAssetInfoKeys,
                             USDMODEL_ASSET_INFO_KEYS);
    {
        // This must be defined before KindValidationModelHierarchy is used
        // as a default argument to validation.
        TfNbWrapEnum<UsdModelAPI::KindValidation>(_class);
        _class
            .def("GetKind", _GetKind)
            .def("SetKind", &UsdModelAPI::SetKind, nb::arg("value"))
            .def("IsKind", &UsdModelAPI::IsKind,
                 nb::arg("baseKind"),
                  nb::arg("validation")=UsdModelAPI::KindValidationModelHierarchy)
            .def("IsModel", &UsdModelAPI::IsModel)
            .def("IsGroup", &UsdModelAPI::IsGroup)

            .def("GetAssetIdentifier", _GetAssetIdentifier)
            .def("SetAssetIdentifier", &UsdModelAPI::SetAssetIdentifier)
            .def("GetAssetName", _GetAssetName)
            .def("SetAssetName", &UsdModelAPI::SetAssetName)
            .def("GetAssetVersion", _GetAssetVersion)
            .def("SetAssetVersion", &UsdModelAPI::SetAssetVersion)
            .def("GetPayloadAssetDependencies", _GetPayloadAssetDependencies)
            .def("SetPayloadAssetDependencies", 
                 &UsdModelAPI::SetPayloadAssetDependencies)
            .def("GetAssetInfo", _GetAssetInfo)
            .def("SetAssetInfo", &UsdModelAPI::SetAssetInfo)
            ;
    }
}

} // anonymous namespace
