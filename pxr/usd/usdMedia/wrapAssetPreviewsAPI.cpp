//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/usd/usdMedia/assetPreviewsAPI.h"
#include "pxr/usd/usd/schemaBase.h"

#include "pxr/usd/sdf/primSpec.h"

#include "pxr/usd/usd/pyConversions.h"
#include "pxr/base/tf/pyAnnotatedBoolResult.h"
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
_Repr(const UsdMediaAssetPreviewsAPI &self)
{
    std::string primRepr = TfPyRepr(self.GetPrim());
    return TfStringPrintf(
        "UsdMedia.AssetPreviewsAPI(%s)",
        primRepr.c_str());
}

struct UsdMediaAssetPreviewsAPI_CanApplyResult : 
    public TfPyAnnotatedBoolResult<std::string>
{
    UsdMediaAssetPreviewsAPI_CanApplyResult(bool val, std::string const &msg) :
        TfPyAnnotatedBoolResult<std::string>(val, msg) {}
};

static UsdMediaAssetPreviewsAPI_CanApplyResult
_WrapCanApply(const UsdPrim& prim)
{
    std::string whyNot;
    bool result = UsdMediaAssetPreviewsAPI::CanApply(prim, &whyNot);
    return UsdMediaAssetPreviewsAPI_CanApplyResult(result, whyNot);
}

} // anonymous namespace

void wrapUsdMediaAssetPreviewsAPI(nb::module_ &m)
{
    typedef UsdMediaAssetPreviewsAPI This;

    UsdMediaAssetPreviewsAPI_CanApplyResult::WrapNb<UsdMediaAssetPreviewsAPI_CanApplyResult>(m, "_CanApplyResult", "whyNot");

    nb::class_<This, UsdAPISchemaBase> cls(m, "AssetPreviewsAPI");

    cls
        .def(nb::init<UsdPrim>(), nb::arg("prim")=UsdPrim())
        .def(nb::init<UsdSchemaBase const&>(), nb::arg("schemaObj"))
        .def(TfTypePythonClass())

        .def_static("Get", &This::Get, nb::arg("stage"), nb::arg("path"))

        .def_static("CanApply", &_WrapCanApply, nb::arg("prim"))

        .def_static("Apply", &This::Apply, nb::arg("prim"))

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

namespace {
static 
nb::object _WrapGetDefaultThumbnails(const UsdMediaAssetPreviewsAPI &self)
{
    UsdMediaAssetPreviewsAPI::Thumbnails  t;

    if (self.GetDefaultThumbnails(&t)){
        return nb::cast(t);
    }
    else {
        return nb::none();
    }
}

static 
std::string _ThumbnailsRepr(const UsdMediaAssetPreviewsAPI::Thumbnails &self)
{
    std::vector<std::string> initList;

    // append all members
    initList.push_back(TfStringPrintf("defaultImage=%s",
                                      TfPyRepr(self.defaultImage).c_str()));

    return TF_PY_REPR_PREFIX + "AssetPreviewsAPI.Thumbnails(" + 
        TfStringJoin(initList) + ")";
}

WRAP_CUSTOM {
    using This = UsdMediaAssetPreviewsAPI;

    _class
        .def("GetDefaultThumbnails", &_WrapGetDefaultThumbnails,
             nb::rv_policy::copy)
        .def("SetDefaultThumbnails", &This::SetDefaultThumbnails,
             nb::arg("thumbnails"))
        .def("ClearDefaultThumbnails", &This::ClearDefaultThumbnails)
        .def_static("GetAssetDefaultPreviews", 
             (This (*)(const std::string &layerPath))
             &This::GetAssetDefaultPreviews,
             nb::arg("layerPath"))
        .def_static("GetAssetDefaultPreviews", 
             (This (*)(const SdfLayerHandle &layer))
             &This::GetAssetDefaultPreviews,
             nb::arg("layer"))
        ;

    // wrap UsdMediaAssetPreviewsAPI::Thumbnails struct, available as
    // UsdMedia.AssetPreviewsAPI.Thumbnails
    nb::class_<This::Thumbnails>(_class, "Thumbnails")
        .def(nb::init<SdfAssetPath>(),
             nb::arg("defaultImage")=SdfAssetPath())
        .def_rw("defaultImage", &This::Thumbnails::defaultImage)
        .def("__repr__", _ThumbnailsRepr)
        ;
}

}
