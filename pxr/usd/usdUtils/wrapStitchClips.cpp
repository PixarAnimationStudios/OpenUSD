//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
/// \file wrapStitchClips.cpp

#include "pxr/pxr.h"
#include "pxr/base/tf/pyNanobindBridge.h"
#include "pxr/base/vt/valueFromPython.h"

#include <nanobind/nanobind.h>
#include <nanobind/stl/string.h>
#include <nanobind/stl/vector.h>

#include "pxr/usd/usdUtils/stitchClips.h"
#include "pxr/base/tf/pyUtils.h"

#include <limits>

PXR_NAMESPACE_USING_DIRECTIVE

namespace nb = nanobind;

namespace {

template <typename T>
T
_ConvertWithDefault(const nb::object obj, const T& def)
{
    if (!obj.is_none()) {
        return nb::cast<T>(obj);
    } 
        
    return def;
}

bool
_ConvertStitchClips(const SdfLayerHandle& resultLayer,
                    const std::vector<std::string>& clipLayerFiles,
                    const SdfPath& clipPath,
                    const nb::object pyStartFrame,
                    const nb::object pyEndFrame,
                    const nb::object pyInterpolateMissingClipValues,
                    const nb::object pyClipSet)
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
                           const nb::object pyActiveOffset,
                           const nb::object pyInterpolateMissingClipValues,
                           const nb::object pyClipSet)
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

void wrapStitchClips(nanobind::module_ &m)
{
    m.def("StitchClips",
        _ConvertStitchClips, 
        nb::arg("resultLayer"), 
        nb::arg("clipLayerFiles"), 
        nb::arg("clipPath"), 
        nb::arg("startFrame").none() = nb::none(),
        nb::arg("endFrame").none() = nb::none(),
        nb::arg("interpolateMissingClipValues").none() = nb::none(),
        nb::arg("clipSet").none() = nb::none());

    m.def("StitchClipsTopology",
        _ConvertStitchClipsTopology,
        nb::arg("topologyLayer"),
        nb::arg("clipLayerFiles"));

    m.def("StitchClipsManifest",
        UsdUtilsStitchClipsManifest,
        nb::arg("manifestLayer"), 
        nb::arg("topologyLayer"), 
        nb::arg("clipPath"),
        nb::arg("clipLayerFiles"));

    m.def("StitchClipsTemplate",
        _ConvertStitchClipTemplate,
        nb::arg("resultLayer"),
        nb::arg("topologyLayer"),
        nb::arg("manifestLayer"),
        nb::arg("clipPath"),
        nb::arg("templatePath"),
        nb::arg("startTimeCode"),
        nb::arg("endTimeCode"),
        nb::arg("stride"),
        nb::arg("activeOffset").none() = nb::none(),
        nb::arg("interpolateMissingClipValues").none() = nb::none(),
        nb::arg("clipSet").none() = nb::none());

    m.def("GenerateClipTopologyName",
        _ConvertGenerateClipTopologyName,
        nb::arg("rootLayerName"));

    m.def("GenerateClipManifestName",
        UsdUtilsGenerateClipManifestName,
        nb::arg("rootLayerName"));
}
