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
#include "pxr/usd/usd/modelAPI.h"
#include "pxr/usd/usd/schemaBase.h"
#include "pxr/usd/usd/conversions.h"

#include "pxr/usd/sdf/primSpec.h"

#include "pxr/base/tf/pyContainerConversions.h"
#include "pxr/base/tf/pyResultConversions.h"
#include "pxr/base/tf/pyUtils.h"
#include "pxr/base/tf/wrapTypeHelpers.h"

#include <boost/python.hpp>

#include <string>

using namespace boost::python;

PXR_NAMESPACE_OPEN_SCOPE

#define WRAP_CUSTOM                                                     \
    template <class Cls> static void _CustomWrapCode(Cls &_class)

// fwd decl.
WRAP_CUSTOM;


void wrapUsdModelAPI()
{
    typedef UsdModelAPI This;

    class_<This, bases<UsdSchemaBase> >
        cls("ModelAPI");

    cls
        .def(init<UsdPrim>(arg("prim")))
        .def(init<UsdSchemaBase const&>(arg("schemaObj")))
        .def(TfTypePythonClass())

        .def("Get", &This::Get, (arg("stage"), arg("path")))
        .staticmethod("Get")


        .def("GetSchemaAttributeNames",
             &This::GetSchemaAttributeNames,
             arg("includeInherited")=true,
             return_value_policy<TfPySequenceToList>())
        .staticmethod("GetSchemaAttributeNames")

        .def("_GetStaticTfType", (TfType const &(*)()) TfType::Find<This>,
             return_value_policy<return_by_value>())
        .staticmethod("_GetStaticTfType")

        .def(!self)


    ;

    _CustomWrapCode(cls);
}

PXR_NAMESPACE_CLOSE_SCOPE

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
// 'PXR_NAMESPACE_OPEN_SCOPE', 'PXR_NAMESPACE_CLOSE_SCOPE'.
//
// ===================================================================== //
// --(BEGIN CUSTOM CODE)--

#include "pxr/base/tf/pyStaticTokens.h"

PXR_NAMESPACE_OPEN_SCOPE

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

    _class
        .def("GetKind", _GetKind)
        .def("SetKind", &UsdModelAPI::SetKind, arg("value"))
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

PXR_NAMESPACE_CLOSE_SCOPE
