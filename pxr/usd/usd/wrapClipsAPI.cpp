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
#include "pxr/base/tf/wrapTypeHelpers.h"

#include "pxr/external/boost/python.hpp"

#include <string>

PXR_NAMESPACE_USING_DIRECTIVE

using namespace pxr_boost::python;

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

        .def("ComputeClipAssetPaths",
            (VtArray<SdfAssetPath>(UsdClipsAPI::*)() const)
                (&UsdClipsAPI::ComputeClipAssetPaths),
             return_value_policy<TfPySequenceToList>())
        .def("ComputeClipAssetPaths",
            (VtArray<SdfAssetPath>(UsdClipsAPI::*)(const std::string&) const)
                (&UsdClipsAPI::ComputeClipAssetPaths),
             arg("clipSet"),
             return_value_policy<TfPySequenceToList>())

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

        .def("GenerateClipManifest", 
             (SdfLayerRefPtr(UsdClipsAPI::*)(bool) const)
                 (&UsdClipsAPI::GenerateClipManifest),
             arg("writeBlocksForClipsWithMissingValues") = false,
             return_value_policy<TfPyRefPtrFactory<> >())
        .def("GenerateClipManifest", 
             (SdfLayerRefPtr(UsdClipsAPI::*)(const std::string&, bool) const)
                 (&UsdClipsAPI::GenerateClipManifest),
             (arg("clipSet"),
              arg("writeBlocksForClipsWithMissingValues") = false),
             return_value_policy<TfPyRefPtrFactory<> >())

        .def("GenerateClipManifestFromLayers", 
             &UsdClipsAPI::GenerateClipManifestFromLayers,
             (arg("clipLayers"), arg("clipPrimPath")),
             return_value_policy<TfPyRefPtrFactory<> >())
        .staticmethod("GenerateClipManifestFromLayers")

        .def("GetInterpolateMissingClipValues", 
             (bool(*)(const UsdClipsAPI&))
                 (&_GetInterpolateMissingClipValues))
        .def("GetInterpolateMissingClipValues", 
             (bool(*)(const UsdClipsAPI&, const std::string&))
                 (&_GetInterpolateMissingClipValues),
             arg("clipSet"))
        .def("SetInterpolateMissingClipValues", 
             (bool(UsdClipsAPI::*)(bool))
                 (&UsdClipsAPI::SetInterpolateMissingClipValues), 
             arg("interpolate"))
        .def("SetInterpolateMissingClipValues", 
             (bool(UsdClipsAPI::*)(bool, const std::string&))
                 (&UsdClipsAPI::SetInterpolateMissingClipValues), 
             (arg("interpolate"), arg("clipSet")))

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
