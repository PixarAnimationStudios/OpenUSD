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
#include "pxr/usd/usd/clipsAPI.h"
#include "pxr/usd/usd/schemaBase.h"

#include "pxr/usd/sdf/primSpec.h"

#include "pxr/usd/usd/pyConversions.h"
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


} // anonymous namespace

void wrapUsdClipsAPI()
{
    typedef UsdClipsAPI This;

    class_<This, bases<UsdAPISchemaBase> >
        cls("ClipsAPI");

    cls
        .def(init<UsdPrim>(arg("prim")))
        .def(init<UsdSchemaBase const&>(arg("schemaObj")))
        .def(TfTypePythonClass())

        .def("Get", &This::Get, (arg("stage"), arg("path")))
        .staticmethod("Get")

        .def("IsConcrete",
            static_cast<bool (*)(void)>( [](){ return This::IsConcrete; }))
        .staticmethod("IsConcrete")

        .def("IsTyped",
            static_cast<bool (*)(void)>( [](){ return This::IsTyped; } ))
        .staticmethod("IsTyped")

        .def("IsApplied", 
            static_cast<bool (*)(void)>( [](){ return This::IsApplied; } ))
        .staticmethod("IsApplied")

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
// 
// Just remember to wrap code in the appropriate delimiters:
// 'namespace {', '}'.
//
// ===================================================================== //
// --(BEGIN CUSTOM CODE)--

namespace {

static VtDictionary _GetClips(const UsdClipsAPI &self)
{
    VtDictionary result;
    self.GetClips(&result);
    return result;
}

static SdfStringListOp _GetClipSets(const UsdClipsAPI &self)
{
    SdfStringListOp result;
    self.GetClipSets(&result);
    return result;
}

template <class... Args>
static VtArray<SdfAssetPath> _GetClipAssetPaths(const UsdClipsAPI &self,
                                                const Args&... args)
{
    VtArray<SdfAssetPath> result;
    self.GetClipAssetPaths(&result, args...);
    return result;
}

template <class... Args>
static void _SetClipAssetPaths(UsdClipsAPI &self, TfPyObjWrapper pyVal, 
                               const Args&... args) 
{
    VtValue v = UsdPythonToSdfType(pyVal, SdfValueTypeNames->AssetArray);
    if (!v.IsHolding<VtArray<SdfAssetPath> >()) {
        TF_CODING_ERROR("Invalid value for 'clipAssetPaths' on %s",
                        UsdDescribe(self.GetPrim()).c_str());
        return;
    }

    self.SetClipAssetPaths(v.UncheckedGet<VtArray<SdfAssetPath> >(), args...);
}

template <class... Args>
static std::string _GetClipPrimPath(const UsdClipsAPI &self, 
                                    const Args&... args) 
{
    std::string result;
    self.GetClipPrimPath(&result, args...);
    return result;
}

template <class... Args>
static TfPyObjWrapper _GetClipActive(const UsdClipsAPI &self, 
                                     const Args&... args) 
{
    VtVec2dArray result;
    self.GetClipActive(&result, args...);
    return UsdVtValueToPython(VtValue(result));
}

template <class... Args>
static void _SetClipActive(UsdClipsAPI &self, TfPyObjWrapper pyVal, 
                           const Args&... args) 
{
    VtValue v = UsdPythonToSdfType(pyVal, SdfValueTypeNames->Double2Array);
    if (!v.IsHolding<VtVec2dArray>()) {
        TF_CODING_ERROR("Invalid value for 'clipActive' on %s",
                        UsdDescribe(self.GetPrim()).c_str());
        return;
    }

    self.SetClipActive(v.UncheckedGet<VtVec2dArray>(), args...);
}

template <class... Args>
static TfPyObjWrapper _GetClipTimes(const UsdClipsAPI &self, 
                                    const Args&... args) 
{
    VtVec2dArray result;
    self.GetClipTimes(&result, args...);
    return UsdVtValueToPython(VtValue(result));
}

template <class... Args>
static void _SetClipTimes(UsdClipsAPI &self, TfPyObjWrapper pyVal, 
                          const Args&... args) 
{
    VtValue v = UsdPythonToSdfType(pyVal, SdfValueTypeNames->Double2Array);
    if (!v.IsHolding<VtVec2dArray>()) {
        TF_CODING_ERROR("Invalid value for 'clipTimes' on %s",
                        UsdDescribe(self.GetPrim()).c_str());
        return;
    }

    self.SetClipTimes(v.UncheckedGet<VtVec2dArray>(), args...);
}

template <class... Args>
static SdfAssetPath _GetClipManifestAssetPath(const UsdClipsAPI &self, 
                                              const Args&... args) 
{
    SdfAssetPath manifestAssetPath;
    self.GetClipManifestAssetPath(&manifestAssetPath, args...);
    return manifestAssetPath;
}

template <class... Args>
static void _SetClipTemplateAssetPath(UsdClipsAPI& self, TfPyObjWrapper pyVal,
                                      const Args&... args) 
{
    VtValue v = UsdPythonToSdfType(pyVal, SdfValueTypeNames->String);
    if (!v.IsHolding<std::string>()) {
        TF_CODING_ERROR("Invalid value for 'clipTemplateAssetPath' on %s",
                        UsdDescribe(self.GetPrim()).c_str());
        return;
    }

    self.SetClipTemplateAssetPath(v.UncheckedGet<std::string>(), args...);
}

template <class... Args>
static std::string _GetClipTemplateAssetPath(const UsdClipsAPI &self,
                                             const Args&... args) 
{
    std::string clipTemplateAssetPath;
    self.GetClipTemplateAssetPath(&clipTemplateAssetPath, args...);
    return clipTemplateAssetPath;
}

template <class... Args>
static double _GetClipTemplateStride(const UsdClipsAPI &self, 
                                     const Args&... args) 
{
    double clipTemplateStride;
    self.GetClipTemplateStride(&clipTemplateStride, args...);
    return clipTemplateStride;
}

template <class... Args>
static double _GetClipTemplateActiveOffset(const UsdClipsAPI &self, 
                                     const Args&... args) 
{
    double clipTemplateActiveOffset;
    self.GetClipTemplateActiveOffset(&clipTemplateActiveOffset, args...);
    return clipTemplateActiveOffset;
}

template <class... Args>
static double _GetClipTemplateStartTime(const UsdClipsAPI &self, 
                                        const Args&... args) 
{
    double clipTemplateStartTime;
    self.GetClipTemplateStartTime(&clipTemplateStartTime, args...);
    return clipTemplateStartTime;
}

template <class... Args>
static double _GetClipTemplateEndTime(const UsdClipsAPI &self, 
                                      const Args&... args) 
{
    double clipTemplateEndTime;
    self.GetClipTemplateEndTime(&clipTemplateEndTime, args...);
    return clipTemplateEndTime;
}

WRAP_CUSTOM {
    _class
        .def("GetClips", _GetClips)
        .def("SetClips", &UsdClipsAPI::SetClips,
             arg("clips"))

        .def("GetClipSets", _GetClipSets)
        .def("SetClipSets", &UsdClipsAPI::SetClipSets,
             arg("clipSets"))

        .def("GetClipAssetPaths", 
             (VtArray<SdfAssetPath>(*)(const UsdClipsAPI&))
                 (&_GetClipAssetPaths),
             return_value_policy<TfPySequenceToList>())
        .def("GetClipAssetPaths", 
             (VtArray<SdfAssetPath>(*)(const UsdClipsAPI&, const std::string&))
                 (&_GetClipAssetPaths),
             return_value_policy<TfPySequenceToList>(),
             arg("clipSet"))
        .def("SetClipAssetPaths", 
             (void(*)(UsdClipsAPI&, TfPyObjWrapper))
                 (&_SetClipAssetPaths), 
             arg("assetPaths"))
        .def("SetClipAssetPaths", 
             (void(*)(UsdClipsAPI&, TfPyObjWrapper, const std::string&))
                 (&_SetClipAssetPaths), 
             (arg("assetPaths"), arg("clipSet")))

        .def("GetClipPrimPath", 
             (std::string(*)(const UsdClipsAPI&))
                (&_GetClipPrimPath))
        .def("GetClipPrimPath", 
             (std::string(*)(const UsdClipsAPI&, const std::string&))
                 (&_GetClipPrimPath),
             arg("clipSet"))
        .def("SetClipPrimPath", 
             (bool(UsdClipsAPI::*)(const std::string&))
                 (&UsdClipsAPI::SetClipPrimPath), 
             arg("primPath"))
        .def("SetClipPrimPath", 
             (bool(UsdClipsAPI::*)(const std::string&, const std::string&))
                 (&UsdClipsAPI::SetClipPrimPath), 
             (arg("primPath"), arg("clipSet")))

        .def("GetClipActive", 
             (TfPyObjWrapper(*)(const UsdClipsAPI&))
                 (&_GetClipActive))
        .def("GetClipActive", 
             (TfPyObjWrapper(*)(const UsdClipsAPI&, const std::string&))
                 (&_GetClipActive),
             arg("clipSet"))
        .def("SetClipActive", 
             (void(*)(UsdClipsAPI&, TfPyObjWrapper))
                 (&_SetClipActive), 
             arg("activeClips"))
        .def("SetClipActive", 
             (void(*)(UsdClipsAPI&, TfPyObjWrapper, const std::string&))
                 (&_SetClipActive), 
             (arg("activeClips"), arg("clipSet")))

        .def("GetClipTimes", 
             (TfPyObjWrapper(*)(const UsdClipsAPI&))
                 (&_GetClipTimes))
        .def("GetClipTimes", 
             (TfPyObjWrapper(*)(const UsdClipsAPI&, const std::string&))
                 (&_GetClipTimes),
             arg("clipSet"))
        .def("SetClipTimes", 
             (void(*)(UsdClipsAPI&, TfPyObjWrapper))
                 (&_SetClipTimes), 
             arg("clipTimes"))
        .def("SetClipTimes", 
             (void(*)(UsdClipsAPI&, TfPyObjWrapper, const std::string&))
                 (&_SetClipTimes), 
             (arg("clipTimes"), arg("clipSet")))

        .def("GetClipManifestAssetPath", 
             (SdfAssetPath(*)(const UsdClipsAPI&))
                 (&_GetClipManifestAssetPath))
        .def("GetClipManifestAssetPath", 
             (SdfAssetPath(*)(const UsdClipsAPI&, const std::string&))
                 (&_GetClipManifestAssetPath),
             arg("clipSet"))
        .def("SetClipManifestAssetPath", 
             (bool(UsdClipsAPI::*)(const SdfAssetPath&))
                 (&UsdClipsAPI::SetClipManifestAssetPath), 
             arg("manifestAssetPath"))
        .def("SetClipManifestAssetPath", 
             (bool(UsdClipsAPI::*)(const SdfAssetPath&, const std::string&))
                 (&UsdClipsAPI::SetClipManifestAssetPath), 
             (arg("manifestAssetPath"), arg("clipSet")))


        .def("GetClipTemplateAssetPath", 
             (std::string(*)(const UsdClipsAPI&))
                (&_GetClipTemplateAssetPath))
        .def("GetClipTemplateAssetPath", 
             (std::string(*)(const UsdClipsAPI&, const std::string&))
                 (&_GetClipTemplateAssetPath),
             arg("clipSet"))
        .def("SetClipTemplateAssetPath", 
             (void(*)(UsdClipsAPI&, TfPyObjWrapper))
                 (&_SetClipTemplateAssetPath), 
             arg("clipTemplateAssetPath"))
        .def("SetClipTemplateAssetPath", 
             (void(*)(UsdClipsAPI&, TfPyObjWrapper, const std::string&))
                 (&_SetClipTemplateAssetPath), 
             (arg("clipTemplateAssetPath"), arg("clipSet")))

        .def("GetClipTemplateStride", 
             (double(*)(const UsdClipsAPI&))
                (&_GetClipTemplateStride))
        .def("GetClipTemplateStride", 
             (double(*)(const UsdClipsAPI&, const std::string&))
                 (&_GetClipTemplateStride),
             arg("clipSet"))
        .def("SetClipTemplateStride", 
             (bool(UsdClipsAPI::*)(double))
                 (&UsdClipsAPI::SetClipTemplateStride),
             arg("clipTemplateStride"))
        .def("SetClipTemplateStride", 
             (bool(UsdClipsAPI::*)(double, const std::string&))
                 (&UsdClipsAPI::SetClipTemplateStride),
             (arg("clipTemplateStride"), arg("clipSet")))

        .def("GetClipTemplateActiveOffset", 
             (double(*)(const UsdClipsAPI&))
                (&_GetClipTemplateActiveOffset))
        .def("GetClipTemplateActiveOffset", 
             (double(*)(const UsdClipsAPI&, const std::string&))
                 (&_GetClipTemplateActiveOffset),
             arg("clipSet"))
        .def("SetClipTemplateActiveOffset", 
             (bool(UsdClipsAPI::*)(double))
                 (&UsdClipsAPI::SetClipTemplateActiveOffset),
             arg("clipTemplateActiveOffset"))
        .def("SetClipTemplateActiveOffset", 
             (bool(UsdClipsAPI::*)(double, const std::string&))
                 (&UsdClipsAPI::SetClipTemplateActiveOffset),
             (arg("clipTemplateActiveOffset"), arg("clipSet")))

        .def("GetClipTemplateStartTime", 
             (double(*)(const UsdClipsAPI&))
                (&_GetClipTemplateStartTime))
        .def("GetClipTemplateStartTime", 
             (double(*)(const UsdClipsAPI&, const std::string&))
                 (&_GetClipTemplateStartTime),
             arg("clipSet"))
        .def("SetClipTemplateStartTime", 
             (bool(UsdClipsAPI::*)(double))
                 (&UsdClipsAPI::SetClipTemplateStartTime),
             arg("clipTemplateStartTime"))
        .def("SetClipTemplateStartTime", 
             (bool(UsdClipsAPI::*)(double, const std::string&))
                 (&UsdClipsAPI::SetClipTemplateStartTime),
             (arg("clipTemplateStartTime"), arg("clipSet")))

        .def("GetClipTemplateEndTime", 
             (double(*)(const UsdClipsAPI&))
                (&_GetClipTemplateEndTime))
        .def("GetClipTemplateEndTime", 
             (double(*)(const UsdClipsAPI&, const std::string&))
                 (&_GetClipTemplateEndTime),
             arg("clipSet"))
        .def("SetClipTemplateEndTime", 
             (bool(UsdClipsAPI::*)(double))
                 (&UsdClipsAPI::SetClipTemplateEndTime),
             arg("clipTemplateEndTime"))
        .def("SetClipTemplateEndTime", 
             (bool(UsdClipsAPI::*)(double, const std::string&))
                 (&UsdClipsAPI::SetClipTemplateEndTime),
             (arg("clipTemplateEndTime"), arg("clipSet")))
        ;
}

} // anonymous namespace
