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
#include <boost/python.hpp>

#include "pxr/usd/usd/clipsAPI.h"

#include "pxr/usd/usd/schemaBase.h"
#include "pxr/usd/usd/conversions.h"

#include "pxr/usd/sdf/primSpec.h"

#include "pxr/base/tf/pyContainerConversions.h"
#include "pxr/base/tf/pyResultConversions.h"
#include "pxr/base/tf/pyUtils.h"
#include "pxr/base/tf/wrapTypeHelpers.h"

#include <string>

using namespace boost::python;

#define WRAP_CUSTOM                                                     \
    template <class Cls> static void _CustomWrapCode(Cls &_class)

// fwd decl.
WRAP_CUSTOM;


void wrapUsdClipsAPI()
{
    typedef UsdClipsAPI This;

    class_<This, bases<UsdSchemaBase> >
        cls("ClipsAPI");

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
// ===================================================================== //
// --(BEGIN CUSTOM CODE)--
static VtArray<SdfAssetPath> _GetClipAssetPaths(const UsdClipsAPI &self) {
    VtArray<SdfAssetPath> result;
    self.GetClipAssetPaths(&result);
    return result;
}

static void _SetClipAssetPaths(UsdClipsAPI &self, TfPyObjWrapper pyVal) {
    VtValue v = UsdPythonToSdfType(pyVal, SdfValueTypeNames->AssetArray);
    if (not v.IsHolding<VtArray<SdfAssetPath> >()) {
        TF_CODING_ERROR("Invalid value for 'clipAssetPaths' on %s",
                        UsdDescribe(self.GetPrim()).c_str());
        return;
    }

    self.SetClipAssetPaths(v.UncheckedGet<VtArray<SdfAssetPath> >());
}

static std::string _GetClipPrimPath(const UsdClipsAPI &self) {
    std::string result;
    self.GetClipPrimPath(&result);
    return result;
}

static TfPyObjWrapper _GetClipActive(const UsdClipsAPI &self) {
    VtVec2dArray result;
    self.GetClipActive(&result);
    return UsdVtValueToPython(VtValue(result));
}

static void _SetClipActive(UsdClipsAPI &self, TfPyObjWrapper pyVal) {
    VtValue v = UsdPythonToSdfType(pyVal, SdfValueTypeNames->Double2Array);
    if (not v.IsHolding<VtVec2dArray>()) {
        TF_CODING_ERROR("Invalid value for 'clipActive' on %s",
                        UsdDescribe(self.GetPrim()).c_str());
        return;
    }

    self.SetClipActive(v.UncheckedGet<VtVec2dArray>());
}

static TfPyObjWrapper _GetClipTimes(const UsdClipsAPI &self) {
    VtVec2dArray result;
    self.GetClipTimes(&result);
    return UsdVtValueToPython(VtValue(result));
}

static void _SetClipTimes(UsdClipsAPI &self, TfPyObjWrapper pyVal) {
    VtValue v = UsdPythonToSdfType(pyVal, SdfValueTypeNames->Double2Array);
    if (not v.IsHolding<VtVec2dArray>()) {
        TF_CODING_ERROR("Invalid value for 'clipTimes' on %s",
                        UsdDescribe(self.GetPrim()).c_str());
        return;
    }

    self.SetClipTimes(v.UncheckedGet<VtVec2dArray>());
}

static SdfAssetPath _GetClipManifestAssetPath(UsdClipsAPI &self) {
    SdfAssetPath manifestAssetPath;
    self.GetClipManifestAssetPath(&manifestAssetPath);
    return manifestAssetPath;
}


WRAP_CUSTOM {
    _class
        .def("GetClipAssetPaths", _GetClipAssetPaths,
             return_value_policy<TfPySequenceToList>())
        .def("SetClipAssetPaths", _SetClipAssetPaths, arg("assetPaths"))
        .def("GetClipPrimPath", _GetClipPrimPath)
        .def("SetClipPrimPath", &UsdClipsAPI::SetClipPrimPath, arg("primPath"))
        .def("GetClipActive", _GetClipActive)
        .def("SetClipActive", _SetClipActive, arg("activeClips"))
        .def("GetClipTimes", _GetClipTimes)
        .def("SetClipTimes", _SetClipTimes, arg("clipTimes"))
        .def("GetClipManifestAssetPath", _GetClipManifestAssetPath)
        .def("SetClipManifestAssetPath", 
             &UsdClipsAPI::SetClipManifestAssetPath, arg("manifestAssetPath"))
        ;
}
