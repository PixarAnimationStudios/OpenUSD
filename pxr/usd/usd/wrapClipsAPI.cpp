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


PXR_NAMESPACE_USING_DIRECTIVE

namespace pxrUsdUsdWrapClipsAPI {

#define WRAP_CUSTOM                                                     \
    template <class Cls> static void _CustomWrapCode(Cls &_class)

// fwd decl.
WRAP_CUSTOM;


static std::string
_Repr(const UsdClipsAPI &self)
{
    std::string primRepr = TfPyRepr(self.GetPrim());
    return TfStringPrintf(
        "Usd.ClipsAPI(%s)",
        primRepr.c_str());
}

} // anonymous namespace

void wrapUsdClipsAPI()
{
    typedef UsdClipsAPI This;

    boost::python::class_<This, boost::python::bases<UsdAPISchemaBase> >
        cls("ClipsAPI");

    cls
        .def(boost::python::init<UsdPrim>(boost::python::arg("prim")))
        .def(boost::python::init<UsdSchemaBase const&>(boost::python::arg("schemaObj")))
        .def(TfTypePythonClass())

        .def("Get", &This::Get, (boost::python::arg("stage"), boost::python::arg("path")))
        .staticmethod("Get")

        .def("GetSchemaAttributeNames",
             &This::GetSchemaAttributeNames,
             boost::python::arg("includeInherited")=true,
             boost::python::return_value_policy<TfPySequenceToList>())
        .staticmethod("GetSchemaAttributeNames")

        .def("_GetStaticTfType", (TfType const &(*)()) TfType::Find<This>,
             boost::python::return_value_policy<boost::python::return_by_value>())
        .staticmethod("_GetStaticTfType")

        .def(!boost::python::self)


        .def("__repr__", pxrUsdUsdWrapClipsAPI::_Repr)
    ;

    pxrUsdUsdWrapClipsAPI::_CustomWrapCode(cls);
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

#include "pxr/base/tf/makePyConstructor.h"

namespace pxrUsdUsdWrapClipsAPI {

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
static bool _GetInterpolateMissingClipValues(const UsdClipsAPI &self, 
                                             const Args&... args) 
{
    bool interpolate = false;
    self.GetInterpolateMissingClipValues(&interpolate, args...);
    return interpolate;
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
             boost::python::arg("clips"))

        .def("GetClipSets", _GetClipSets)
        .def("SetClipSets", &UsdClipsAPI::SetClipSets,
             boost::python::arg("clipSets"))

        .def("GetClipAssetPaths", 
             (VtArray<SdfAssetPath>(*)(const UsdClipsAPI&))
                 (&_GetClipAssetPaths),
             boost::python::return_value_policy<TfPySequenceToList>())
        .def("GetClipAssetPaths", 
             (VtArray<SdfAssetPath>(*)(const UsdClipsAPI&, const std::string&))
                 (&_GetClipAssetPaths),
             boost::python::return_value_policy<TfPySequenceToList>(),
             boost::python::arg("clipSet"))
        .def("SetClipAssetPaths", 
             (void(*)(UsdClipsAPI&, TfPyObjWrapper))
                 (&_SetClipAssetPaths), 
             boost::python::arg("assetPaths"))
        .def("SetClipAssetPaths", 
             (void(*)(UsdClipsAPI&, TfPyObjWrapper, const std::string&))
                 (&_SetClipAssetPaths), 
             (boost::python::arg("assetPaths"), boost::python::arg("clipSet")))

        .def("ComputeClipAssetPaths",
            (VtArray<SdfAssetPath>(UsdClipsAPI::*)() const)
                (&UsdClipsAPI::ComputeClipAssetPaths),
             boost::python::return_value_policy<TfPySequenceToList>())
        .def("ComputeClipAssetPaths",
            (VtArray<SdfAssetPath>(UsdClipsAPI::*)(const std::string&) const)
                (&UsdClipsAPI::ComputeClipAssetPaths),
             boost::python::arg("clipSet"),
             boost::python::return_value_policy<TfPySequenceToList>())

        .def("GetClipPrimPath", 
             (std::string(*)(const UsdClipsAPI&))
                (&_GetClipPrimPath))
        .def("GetClipPrimPath", 
             (std::string(*)(const UsdClipsAPI&, const std::string&))
                 (&_GetClipPrimPath),
             boost::python::arg("clipSet"))
        .def("SetClipPrimPath", 
             (bool(UsdClipsAPI::*)(const std::string&))
                 (&UsdClipsAPI::SetClipPrimPath), 
             boost::python::arg("primPath"))
        .def("SetClipPrimPath", 
             (bool(UsdClipsAPI::*)(const std::string&, const std::string&))
                 (&UsdClipsAPI::SetClipPrimPath), 
             (boost::python::arg("primPath"), boost::python::arg("clipSet")))

        .def("GetClipActive", 
             (TfPyObjWrapper(*)(const UsdClipsAPI&))
                 (&_GetClipActive))
        .def("GetClipActive", 
             (TfPyObjWrapper(*)(const UsdClipsAPI&, const std::string&))
                 (&_GetClipActive),
             boost::python::arg("clipSet"))
        .def("SetClipActive", 
             (void(*)(UsdClipsAPI&, TfPyObjWrapper))
                 (&_SetClipActive), 
             boost::python::arg("activeClips"))
        .def("SetClipActive", 
             (void(*)(UsdClipsAPI&, TfPyObjWrapper, const std::string&))
                 (&_SetClipActive), 
             (boost::python::arg("activeClips"), boost::python::arg("clipSet")))

        .def("GetClipTimes", 
             (TfPyObjWrapper(*)(const UsdClipsAPI&))
                 (&_GetClipTimes))
        .def("GetClipTimes", 
             (TfPyObjWrapper(*)(const UsdClipsAPI&, const std::string&))
                 (&_GetClipTimes),
             boost::python::arg("clipSet"))
        .def("SetClipTimes", 
             (void(*)(UsdClipsAPI&, TfPyObjWrapper))
                 (&_SetClipTimes), 
             boost::python::arg("clipTimes"))
        .def("SetClipTimes", 
             (void(*)(UsdClipsAPI&, TfPyObjWrapper, const std::string&))
                 (&_SetClipTimes), 
             (boost::python::arg("clipTimes"), boost::python::arg("clipSet")))

        .def("GetClipManifestAssetPath", 
             (SdfAssetPath(*)(const UsdClipsAPI&))
                 (&_GetClipManifestAssetPath))
        .def("GetClipManifestAssetPath", 
             (SdfAssetPath(*)(const UsdClipsAPI&, const std::string&))
                 (&_GetClipManifestAssetPath),
             boost::python::arg("clipSet"))
        .def("SetClipManifestAssetPath", 
             (bool(UsdClipsAPI::*)(const SdfAssetPath&))
                 (&UsdClipsAPI::SetClipManifestAssetPath), 
             boost::python::arg("manifestAssetPath"))
        .def("SetClipManifestAssetPath", 
             (bool(UsdClipsAPI::*)(const SdfAssetPath&, const std::string&))
                 (&UsdClipsAPI::SetClipManifestAssetPath), 
             (boost::python::arg("manifestAssetPath"), boost::python::arg("clipSet")))

        .def("GenerateClipManifest", 
             (SdfLayerRefPtr(UsdClipsAPI::*)(bool) const)
                 (&UsdClipsAPI::GenerateClipManifest),
             boost::python::arg("writeBlocksForClipsWithMissingValues") = false,
             boost::python::return_value_policy<TfPyRefPtrFactory<> >())
        .def("GenerateClipManifest", 
             (SdfLayerRefPtr(UsdClipsAPI::*)(const std::string&, bool) const)
                 (&UsdClipsAPI::GenerateClipManifest),
             (boost::python::arg("clipSet"),
              boost::python::arg("writeBlocksForClipsWithMissingValues") = false),
             boost::python::return_value_policy<TfPyRefPtrFactory<> >())

        .def("GenerateClipManifestFromLayers", 
             &UsdClipsAPI::GenerateClipManifestFromLayers,
             (boost::python::arg("clipLayers"), boost::python::arg("clipPrimPath")),
             boost::python::return_value_policy<TfPyRefPtrFactory<> >())
        .staticmethod("GenerateClipManifestFromLayers")

        .def("GetInterpolateMissingClipValues", 
             (bool(*)(const UsdClipsAPI&))
                 (&_GetInterpolateMissingClipValues))
        .def("GetInterpolateMissingClipValues", 
             (bool(*)(const UsdClipsAPI&, const std::string&))
                 (&_GetInterpolateMissingClipValues),
             boost::python::arg("clipSet"))
        .def("SetInterpolateMissingClipValues", 
             (bool(UsdClipsAPI::*)(bool))
                 (&UsdClipsAPI::SetInterpolateMissingClipValues), 
             boost::python::arg("interpolate"))
        .def("SetInterpolateMissingClipValues", 
             (bool(UsdClipsAPI::*)(bool, const std::string&))
                 (&UsdClipsAPI::SetInterpolateMissingClipValues), 
             (boost::python::arg("interpolate"), boost::python::arg("clipSet")))

        .def("GetClipTemplateAssetPath", 
             (std::string(*)(const UsdClipsAPI&))
                (&_GetClipTemplateAssetPath))
        .def("GetClipTemplateAssetPath", 
             (std::string(*)(const UsdClipsAPI&, const std::string&))
                 (&_GetClipTemplateAssetPath),
             boost::python::arg("clipSet"))
        .def("SetClipTemplateAssetPath", 
             (void(*)(UsdClipsAPI&, TfPyObjWrapper))
                 (&_SetClipTemplateAssetPath), 
             boost::python::arg("clipTemplateAssetPath"))
        .def("SetClipTemplateAssetPath", 
             (void(*)(UsdClipsAPI&, TfPyObjWrapper, const std::string&))
                 (&_SetClipTemplateAssetPath), 
             (boost::python::arg("clipTemplateAssetPath"), boost::python::arg("clipSet")))

        .def("GetClipTemplateStride", 
             (double(*)(const UsdClipsAPI&))
                (&_GetClipTemplateStride))
        .def("GetClipTemplateStride", 
             (double(*)(const UsdClipsAPI&, const std::string&))
                 (&_GetClipTemplateStride),
             boost::python::arg("clipSet"))
        .def("SetClipTemplateStride", 
             (bool(UsdClipsAPI::*)(double))
                 (&UsdClipsAPI::SetClipTemplateStride),
             boost::python::arg("clipTemplateStride"))
        .def("SetClipTemplateStride", 
             (bool(UsdClipsAPI::*)(double, const std::string&))
                 (&UsdClipsAPI::SetClipTemplateStride),
             (boost::python::arg("clipTemplateStride"), boost::python::arg("clipSet")))

        .def("GetClipTemplateActiveOffset", 
             (double(*)(const UsdClipsAPI&))
                (&_GetClipTemplateActiveOffset))
        .def("GetClipTemplateActiveOffset", 
             (double(*)(const UsdClipsAPI&, const std::string&))
                 (&_GetClipTemplateActiveOffset),
             boost::python::arg("clipSet"))
        .def("SetClipTemplateActiveOffset", 
             (bool(UsdClipsAPI::*)(double))
                 (&UsdClipsAPI::SetClipTemplateActiveOffset),
             boost::python::arg("clipTemplateActiveOffset"))
        .def("SetClipTemplateActiveOffset", 
             (bool(UsdClipsAPI::*)(double, const std::string&))
                 (&UsdClipsAPI::SetClipTemplateActiveOffset),
             (boost::python::arg("clipTemplateActiveOffset"), boost::python::arg("clipSet")))

        .def("GetClipTemplateStartTime", 
             (double(*)(const UsdClipsAPI&))
                (&_GetClipTemplateStartTime))
        .def("GetClipTemplateStartTime", 
             (double(*)(const UsdClipsAPI&, const std::string&))
                 (&_GetClipTemplateStartTime),
             boost::python::arg("clipSet"))
        .def("SetClipTemplateStartTime", 
             (bool(UsdClipsAPI::*)(double))
                 (&UsdClipsAPI::SetClipTemplateStartTime),
             boost::python::arg("clipTemplateStartTime"))
        .def("SetClipTemplateStartTime", 
             (bool(UsdClipsAPI::*)(double, const std::string&))
                 (&UsdClipsAPI::SetClipTemplateStartTime),
             (boost::python::arg("clipTemplateStartTime"), boost::python::arg("clipSet")))

        .def("GetClipTemplateEndTime", 
             (double(*)(const UsdClipsAPI&))
                (&_GetClipTemplateEndTime))
        .def("GetClipTemplateEndTime", 
             (double(*)(const UsdClipsAPI&, const std::string&))
                 (&_GetClipTemplateEndTime),
             boost::python::arg("clipSet"))
        .def("SetClipTemplateEndTime", 
             (bool(UsdClipsAPI::*)(double))
                 (&UsdClipsAPI::SetClipTemplateEndTime),
             boost::python::arg("clipTemplateEndTime"))
        .def("SetClipTemplateEndTime", 
             (bool(UsdClipsAPI::*)(double, const std::string&))
                 (&UsdClipsAPI::SetClipTemplateEndTime),
             (boost::python::arg("clipTemplateEndTime"), boost::python::arg("clipSet")))
        ;
}

} // anonymous namespace
