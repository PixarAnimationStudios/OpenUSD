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
#include "pxr/usd/usdMedia/assetPreviewsAPI.h"
#include "pxr/usd/usd/schemaBase.h"

#include "pxr/usd/sdf/primSpec.h"

#include "pxr/usd/usd/pyConversions.h"
#include "pxr/base/tf/pyAnnotatedBoolResult.h"
#include "pxr/base/tf/pyContainerConversions.h"
#include "pxr/base/tf/pyResultConversions.h"
#include "pxr/base/tf/pyUtils.h"
#include "pxr/base/tf/wrapTypeHelpers.h"

#include <boost/python.hpp>

#include <string>

using namespace boost::python;

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

void wrapUsdMediaAssetPreviewsAPI()
{
    typedef UsdMediaAssetPreviewsAPI This;

    UsdMediaAssetPreviewsAPI_CanApplyResult::Wrap<UsdMediaAssetPreviewsAPI_CanApplyResult>(
        "_CanApplyResult", "whyNot");

    class_<This, bases<UsdAPISchemaBase> >
        cls("AssetPreviewsAPI");

    cls
        .def(init<UsdPrim>(arg("prim")))
        .def(init<UsdSchemaBase const&>(arg("schemaObj")))
        .def(TfTypePythonClass())

        .def("Get", &This::Get, (arg("stage"), arg("path")))
        .staticmethod("Get")

        .def("CanApply", &_WrapCanApply, (arg("prim")))
        .staticmethod("CanApply")

        .def("Apply", &This::Apply, (arg("prim")))
        .staticmethod("Apply")

        .def("GetSchemaAttributeNames",
             &This::GetSchemaAttributeNames,
             arg("includeInherited")=true,
             return_value_policy<TfPySequenceToList>())
        .staticmethod("GetSchemaAttributeNames")

        .def("_GetStaticTfType", (TfType const &(*)()) TfType::Find<This>,
             return_value_policy<return_by_value>())
        .staticmethod("_GetStaticTfType")

        .def(!self)


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
object _WrapGetDefaultThumbnails(const UsdMediaAssetPreviewsAPI &self)
{
    UsdMediaAssetPreviewsAPI::Thumbnails  t;

    if (self.GetDefaultThumbnails(&t)){
        return object(t);
    }
    else {
        return object();
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
             return_value_policy<return_by_value>())
        .def("SetDefaultThumbnails", &This::SetDefaultThumbnails,
             (arg("thumbnails")))
        .def("ClearDefaultThumbnails", &This::ClearDefaultThumbnails)
        .def("GetAssetDefaultPreviews", 
             (This (*)(const std::string &layerPath))
             &This::GetAssetDefaultPreviews,
             (arg("layerPath")))
        .def("GetAssetDefaultPreviews", 
             (This (*)(const SdfLayerHandle &layer))
             &This::GetAssetDefaultPreviews,
             (arg("layer")))
            .staticmethod("GetAssetDefaultPreviews")
        ;

    // Create a root scope so that Thumbnails is scoped under 
    // UsdMedia.AssetPreviewsAPI
    scope assetPreviewsAPI = _class;

    // wrap UsdMediaAssetPreviewsAPI::Thumbnails struct, available as
    // UsdMedia.AssetPreviewsAPI.Thumbnails
    class_<This::Thumbnails>("Thumbnails")
        .def(init<SdfAssetPath>(
                 (arg("defaultImage")=SdfAssetPath())))
        .def_readwrite("defaultImage", &This::Thumbnails::defaultImage)
        .def("__repr__", _ThumbnailsRepr)
        ;

}

}
