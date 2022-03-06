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
/// \file wrapStitchClips.cpp

#include "pxr/pxr.h"
#include <boost/python/def.hpp>
#include <boost/python/extract.hpp>

#include "pxr/usd/usdUtils/stitchClips.h"
#include "pxr/base/tf/pyUtils.h"

#include <limits>


PXR_NAMESPACE_USING_DIRECTIVE

namespace pxrUsdUsdUtilsWrapStitchClips {

template <typename T>
T
_ConvertWithDefault(const boost::python::object obj, const T& def)
{
    if (!TfPyIsNone(obj)) {
        return boost::python::extract<T>(obj);
    } 
        
    return def;
}

bool
_ConvertStitchClips(const SdfLayerHandle& resultLayer,
                    const std::vector<std::string>& clipLayerFiles,
                    const SdfPath& clipPath,
                    const boost::python::object pyStartFrame,
                    const boost::python::object pyEndFrame,
                    const boost::python::object pyInterpolateMissingClipValues,
                    const boost::python::object pyClipSet)
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
                           const boost::python::object pyActiveOffset,
                           const boost::python::object pyInterpolateMissingClipValues,
                           const boost::python::object pyClipSet)
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
    boost::python::def("StitchClips",
        pxrUsdUsdUtilsWrapStitchClips::_ConvertStitchClips, 
        (boost::python::arg("resultLayer"), 
         boost::python::arg("clipLayerFiles"), 
         boost::python::arg("clipPath"), 
         boost::python::arg("startFrame")=boost::python::object(),
         boost::python::arg("endFrame")=boost::python::object(),
         boost::python::arg("interpolateMissingClipValues")=boost::python::object(),
         boost::python::arg("clipSet")=boost::python::object()));

    boost::python::def("StitchClipsTopology",
        pxrUsdUsdUtilsWrapStitchClips::_ConvertStitchClipsTopology,
        (boost::python::arg("topologyLayer"),
         boost::python::arg("clipLayerFiles")));

    boost::python::def("StitchClipsManifest",
        UsdUtilsStitchClipsManifest,
        (boost::python::arg("manifestLayer"), 
         boost::python::arg("topologyLayer"), 
         boost::python::arg("clipPath"),
         boost::python::arg("clipLayerFiles")));

    boost::python::def("StitchClipsTemplate",
        pxrUsdUsdUtilsWrapStitchClips::_ConvertStitchClipTemplate,
        (boost::python::arg("resultLayer"),
         boost::python::arg("topologyLayer"),
         boost::python::arg("manifestLayer"),
         boost::python::arg("clipPath"),
         boost::python::arg("templatePath"),
         boost::python::arg("startTimeCode"),
         boost::python::arg("endTimeCode"),
         boost::python::arg("stride"),
         boost::python::arg("activeOffset")=boost::python::object(),
         boost::python::arg("interpolateMissingClipValues")=boost::python::object(),
         boost::python::arg("clipSet")=boost::python::object()));

    boost::python::def("GenerateClipTopologyName",
        pxrUsdUsdUtilsWrapStitchClips::_ConvertGenerateClipTopologyName,
        (boost::python::arg("rootLayerName")));

    boost::python::def("GenerateClipManifestName",
        UsdUtilsGenerateClipManifestName,
        (boost::python::arg("rootLayerName")));
}
