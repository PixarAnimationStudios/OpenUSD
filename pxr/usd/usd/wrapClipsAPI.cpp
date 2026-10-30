//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/usd/usd/clipsAPI.h"
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
_Repr(const UsdClipsAPI &self)
{
    std::string primRepr = TfPyRepr(self.GetPrim());
    return TfStringPrintf(
        "Usd.ClipsAPI(%s)",
        primRepr.c_str());
}

} // anonymous namespace

void wrapUsdClipsAPI(nb::module_ &m)
{
    typedef UsdClipsAPI This;

    nb::class_<This, UsdAPISchemaBase> cls(m, "ClipsAPI");

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

#include "pxr/base/tf/makePyConstructor.h"

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
             nb::arg("clips"))

        .def("GetClipSets", _GetClipSets)
        .def("SetClipSets", &UsdClipsAPI::SetClipSets,
             nb::arg("clipSets"))

        .def("GetClipAssetPaths", 
             (VtArray<SdfAssetPath>(*)(const UsdClipsAPI&))
                 (&_GetClipAssetPaths),
             nb::rv_policy::copy)
        .def("GetClipAssetPaths", 
             (VtArray<SdfAssetPath>(*)(const UsdClipsAPI&, const std::string&))
                 (&_GetClipAssetPaths),
             nb::rv_policy::copy,
             nb::arg("clipSet"))
        .def("SetClipAssetPaths", 
             (void(*)(UsdClipsAPI&, TfPyObjWrapper))
                 (&_SetClipAssetPaths), 
             nb::arg("assetPaths"))
        .def("SetClipAssetPaths", 
             (void(*)(UsdClipsAPI&, TfPyObjWrapper, const std::string&))
                 (&_SetClipAssetPaths), 
             nb::arg("assetPaths"), nb::arg("clipSet"))

        .def("ComputeClipAssetPaths",
            (VtArray<SdfAssetPath>(UsdClipsAPI::*)() const)
                (&UsdClipsAPI::ComputeClipAssetPaths),
             nb::rv_policy::copy)
        .def("ComputeClipAssetPaths",
            (VtArray<SdfAssetPath>(UsdClipsAPI::*)(const std::string&) const)
                (&UsdClipsAPI::ComputeClipAssetPaths),
             nb::arg("clipSet"),
             nb::rv_policy::copy)

        .def("GetClipPrimPath", 
             (std::string(*)(const UsdClipsAPI&))
                (&_GetClipPrimPath))
        .def("GetClipPrimPath", 
             (std::string(*)(const UsdClipsAPI&, const std::string&))
                 (&_GetClipPrimPath),
             nb::arg("clipSet"))
        .def("SetClipPrimPath", 
             (bool(UsdClipsAPI::*)(const std::string&))
                 (&UsdClipsAPI::SetClipPrimPath), 
             nb::arg("primPath"))
        .def("SetClipPrimPath", 
             (bool(UsdClipsAPI::*)(const std::string&, const std::string&))
                 (&UsdClipsAPI::SetClipPrimPath), 
             nb::arg("primPath"), nb::arg("clipSet"))

        .def("GetClipActive", 
             (TfPyObjWrapper(*)(const UsdClipsAPI&))
                 (&_GetClipActive))
        .def("GetClipActive", 
             (TfPyObjWrapper(*)(const UsdClipsAPI&, const std::string&))
                 (&_GetClipActive),
             nb::arg("clipSet"))
        .def("SetClipActive", 
             (void(*)(UsdClipsAPI&, TfPyObjWrapper))
                 (&_SetClipActive), 
             nb::arg("activeClips"))
        .def("SetClipActive", 
             (void(*)(UsdClipsAPI&, TfPyObjWrapper, const std::string&))
                 (&_SetClipActive), 
             nb::arg("activeClips"), nb::arg("clipSet"))

        .def("GetClipTimes", 
             (TfPyObjWrapper(*)(const UsdClipsAPI&))
                 (&_GetClipTimes))
        .def("GetClipTimes", 
             (TfPyObjWrapper(*)(const UsdClipsAPI&, const std::string&))
                 (&_GetClipTimes),
             nb::arg("clipSet"))
        .def("SetClipTimes", 
             (void(*)(UsdClipsAPI&, TfPyObjWrapper))
                 (&_SetClipTimes), 
             nb::arg("clipTimes"))
        .def("SetClipTimes", 
             (void(*)(UsdClipsAPI&, TfPyObjWrapper, const std::string&))
                 (&_SetClipTimes), 
             nb::arg("clipTimes"), nb::arg("clipSet"))

        .def("GetClipManifestAssetPath", 
             (SdfAssetPath(*)(const UsdClipsAPI&))
                 (&_GetClipManifestAssetPath))
        .def("GetClipManifestAssetPath", 
             (SdfAssetPath(*)(const UsdClipsAPI&, const std::string&))
                 (&_GetClipManifestAssetPath),
             nb::arg("clipSet"))
        .def("SetClipManifestAssetPath", 
             (bool(UsdClipsAPI::*)(const SdfAssetPath&))
                 (&UsdClipsAPI::SetClipManifestAssetPath), 
             nb::arg("manifestAssetPath"))
        .def("SetClipManifestAssetPath", 
             (bool(UsdClipsAPI::*)(const SdfAssetPath&, const std::string&))
                 (&UsdClipsAPI::SetClipManifestAssetPath), 
             nb::arg("manifestAssetPath"), nb::arg("clipSet"))

        .def("GenerateClipManifest", 
             (SdfLayerRefPtr(UsdClipsAPI::*)(bool) const)
                 (&UsdClipsAPI::GenerateClipManifest),
             nb::arg("writeBlocksForClipsWithMissingValues") = false)
        .def("GenerateClipManifest", 
             (SdfLayerRefPtr(UsdClipsAPI::*)(const std::string&, bool) const)
                 (&UsdClipsAPI::GenerateClipManifest),
             nb::arg("clipSet"),
              nb::arg("writeBlocksForClipsWithMissingValues") = false)

        .def_static("GenerateClipManifestFromLayers", 
             &UsdClipsAPI::GenerateClipManifestFromLayers,
             nb::arg("clipLayers"), nb::arg("clipPrimPath"))

        .def("GetInterpolateMissingClipValues", 
             (bool(*)(const UsdClipsAPI&))
                 (&_GetInterpolateMissingClipValues))
        .def("GetInterpolateMissingClipValues", 
             (bool(*)(const UsdClipsAPI&, const std::string&))
                 (&_GetInterpolateMissingClipValues),
             nb::arg("clipSet"))
        .def("SetInterpolateMissingClipValues", 
             (bool(UsdClipsAPI::*)(bool))
                 (&UsdClipsAPI::SetInterpolateMissingClipValues), 
             nb::arg("interpolate"))
        .def("SetInterpolateMissingClipValues", 
             (bool(UsdClipsAPI::*)(bool, const std::string&))
                 (&UsdClipsAPI::SetInterpolateMissingClipValues), 
             nb::arg("interpolate"), nb::arg("clipSet"))

        .def("GetClipTemplateAssetPath", 
             (std::string(*)(const UsdClipsAPI&))
                (&_GetClipTemplateAssetPath))
        .def("GetClipTemplateAssetPath", 
             (std::string(*)(const UsdClipsAPI&, const std::string&))
                 (&_GetClipTemplateAssetPath),
             nb::arg("clipSet"))
        .def("SetClipTemplateAssetPath", 
             (void(*)(UsdClipsAPI&, TfPyObjWrapper))
                 (&_SetClipTemplateAssetPath), 
             nb::arg("clipTemplateAssetPath"))
        .def("SetClipTemplateAssetPath", 
             (void(*)(UsdClipsAPI&, TfPyObjWrapper, const std::string&))
                 (&_SetClipTemplateAssetPath), 
             nb::arg("clipTemplateAssetPath"), nb::arg("clipSet"))

        .def("GetClipTemplateStride", 
             (double(*)(const UsdClipsAPI&))
                (&_GetClipTemplateStride))
        .def("GetClipTemplateStride", 
             (double(*)(const UsdClipsAPI&, const std::string&))
                 (&_GetClipTemplateStride),
             nb::arg("clipSet"))
        .def("SetClipTemplateStride", 
             (bool(UsdClipsAPI::*)(double))
                 (&UsdClipsAPI::SetClipTemplateStride),
             nb::arg("clipTemplateStride"))
        .def("SetClipTemplateStride", 
             (bool(UsdClipsAPI::*)(double, const std::string&))
                 (&UsdClipsAPI::SetClipTemplateStride),
             nb::arg("clipTemplateStride"), nb::arg("clipSet"))

        .def("GetClipTemplateActiveOffset", 
             (double(*)(const UsdClipsAPI&))
                (&_GetClipTemplateActiveOffset))
        .def("GetClipTemplateActiveOffset", 
             (double(*)(const UsdClipsAPI&, const std::string&))
                 (&_GetClipTemplateActiveOffset),
             nb::arg("clipSet"))
        .def("SetClipTemplateActiveOffset", 
             (bool(UsdClipsAPI::*)(double))
                 (&UsdClipsAPI::SetClipTemplateActiveOffset),
             nb::arg("clipTemplateActiveOffset"))
        .def("SetClipTemplateActiveOffset", 
             (bool(UsdClipsAPI::*)(double, const std::string&))
                 (&UsdClipsAPI::SetClipTemplateActiveOffset),
             nb::arg("clipTemplateActiveOffset"), nb::arg("clipSet"))

        .def("GetClipTemplateStartTime", 
             (double(*)(const UsdClipsAPI&))
                (&_GetClipTemplateStartTime))
        .def("GetClipTemplateStartTime", 
             (double(*)(const UsdClipsAPI&, const std::string&))
                 (&_GetClipTemplateStartTime),
             nb::arg("clipSet"))
        .def("SetClipTemplateStartTime", 
             (bool(UsdClipsAPI::*)(double))
                 (&UsdClipsAPI::SetClipTemplateStartTime),
             nb::arg("clipTemplateStartTime"))
        .def("SetClipTemplateStartTime", 
             (bool(UsdClipsAPI::*)(double, const std::string&))
                 (&UsdClipsAPI::SetClipTemplateStartTime),
             nb::arg("clipTemplateStartTime"), nb::arg("clipSet"))

        .def("GetClipTemplateEndTime", 
             (double(*)(const UsdClipsAPI&))
                (&_GetClipTemplateEndTime))
        .def("GetClipTemplateEndTime", 
             (double(*)(const UsdClipsAPI&, const std::string&))
                 (&_GetClipTemplateEndTime),
             nb::arg("clipSet"))
        .def("SetClipTemplateEndTime", 
             (bool(UsdClipsAPI::*)(double))
                 (&UsdClipsAPI::SetClipTemplateEndTime),
             nb::arg("clipTemplateEndTime"))
        .def("SetClipTemplateEndTime", 
             (bool(UsdClipsAPI::*)(double, const std::string&))
                 (&UsdClipsAPI::SetClipTemplateEndTime),
             nb::arg("clipTemplateEndTime"), nb::arg("clipSet"))
        ;
}

} // anonymous namespace
