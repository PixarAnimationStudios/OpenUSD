//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
/// \file wrapStitchClips.cpp

#include "pxr/pxr.h"
#include "pxr/external/boost/python/def.hpp"
#include "pxr/external/boost/python/extract.hpp"

#include "pxr/usd/usdUtils/stitchClips.h"
#include "pxr/base/tf/pyUtils.h"

#include <limits>

PXR_NAMESPACE_USING_DIRECTIVE

using namespace pxr_boost::python;

namespace {

template <typename T>
T
_ConvertWithDefault(const object obj, const T& def)
{
    if (!TfPyIsNone(obj)) {
        return extract<T>(obj);
    } 
        
    return def;
}

bool
_ConvertStitchClips(const SdfLayerHandle& resultLayer,
                    const std::vector<std::string>& clipLayerFiles,
                    const SdfPath& clipPath,
                    const object pyStartFrame,
                    const object pyEndFrame,
                    const object pyInterpolateMissingClipValues,
                    const object pyClipSet)
{
    const auto clipSet 
        = _ConvertWithDefault(pyClipSet, UsdClipsAPISetNames->default_);
    constexpr double dmax = std::numeric_limits<double>::max();
    return UsdUtilsStitchClips(
        resultLayer, clipLayerFiles, clipPath,
        _ConvertWithDefault(pyStartFrame, dmax),
        _ConvertWithDefault(pyEndFrame, dmax),
        _ConvertWithDefault(pyInterpolateMissingClipValues, false),
        clipSet);
}

bool
_ConvertStitchClipsTopology(const SdfLayerHandle& topologyLayer,
                           const std::vector<std::string>& clipLayerFiles)
{
    return UsdUtilsStitchClipsTopology(topologyLayer, clipLayerFiles);
}

std::string
_ConvertGenerateClipTopologyName(const std::string& resultLayerName) 
{
    return UsdUtilsGenerateClipTopologyName(resultLayerName);
}

bool
_ConvertStitchClipTemplate(const SdfLayerHandle& resultLayer,
                           const SdfLayerHandle& topologyLayer,
                           const SdfLayerHandle& manifestLayer,
                           const SdfPath& clipPath,
                           const std::string& templatePath,
                           const double startFrame,
                           const double endFrame,
                           const double stride,
                           const object pyActiveOffset,
                           const object pyInterpolateMissingClipValues,
                           const object pyClipSet)
{
    const auto clipSet 
        = _ConvertWithDefault(pyClipSet, UsdClipsAPISetNames->default_);
    const auto activeOffset 
        = _ConvertWithDefault(pyActiveOffset, 
                              std::numeric_limits<double>::max());
    const auto interpolateMissingClipValues
        = _ConvertWithDefault(pyInterpolateMissingClipValues, false);
    return UsdUtilsStitchClipsTemplate(
        resultLayer, topologyLayer, manifestLayer,
        clipPath, templatePath, startFrame,
        endFrame, stride, activeOffset, interpolateMissingClipValues, clipSet);
}

} // anonymous namespace 

void wrapStitchClips()
{
    def("StitchClips",
        _ConvertStitchClips, 
        (arg("resultLayer"), 
         arg("clipLayerFiles"), 
         arg("clipPath"), 
         arg("startFrame")=object(),
         arg("endFrame")=object(),
         arg("interpolateMissingClipValues")=object(),
         arg("clipSet")=object()));

    def("StitchClipsTopology",
        _ConvertStitchClipsTopology,
        (arg("topologyLayer"),
         arg("clipLayerFiles")));

    def("StitchClipsManifest",
        UsdUtilsStitchClipsManifest,
        (arg("manifestLayer"), 
         arg("topologyLayer"), 
         arg("clipPath"),
         arg("clipLayerFiles")));

    def("StitchClipsTemplate",
        _ConvertStitchClipTemplate,
        (arg("resultLayer"),
         arg("topologyLayer"),
         arg("manifestLayer"),
         arg("clipPath"),
         arg("templatePath"),
         arg("startTimeCode"),
         arg("endTimeCode"),
         arg("stride"),
         arg("activeOffset")=object(),
         arg("interpolateMissingClipValues")=object(),
         arg("clipSet")=object()));

    def("GenerateClipTopologyName",
        _ConvertGenerateClipTopologyName,
        (arg("rootLayerName")));

    def("GenerateClipManifestName",
        UsdUtilsGenerateClipManifestName,
        (arg("rootLayerName")));
}
