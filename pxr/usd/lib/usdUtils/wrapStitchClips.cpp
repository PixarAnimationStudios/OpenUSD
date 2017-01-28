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

PXR_NAMESPACE_OPEN_SCOPE


using namespace boost::python;

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
                    const object pyEndFrame)
{
    constexpr double dmax = std::numeric_limits<double>::max();
    return UsdUtilsStitchClips(resultLayer, clipLayerFiles, clipPath,
                               _ConvertWithDefault(pyStartFrame, dmax),
                               _ConvertWithDefault(pyEndFrame, dmax));
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
                           const SdfPath& clipPath,
                           const std::string& templatePath,
                           const double startFrame,
                           const double endFrame,
                           const double stride)
{
    return UsdUtilsStitchClipsTemplate(resultLayer, topologyLayer,
                                       clipPath, templatePath, startFrame,
                                       endFrame, stride);

}

void 
wrapStitchClips()
{
    def("StitchClips",
        _ConvertStitchClips, 
        (arg("resultLayer"), 
         arg("clipLayerFiles"), 
         arg("clipPath"), 
         arg("startFrame")=object(),
         arg("endFrame")=object()));

    def("StitchClipsTopology",
        _ConvertStitchClipsTopology,
        (arg("topologyLayer"),
         arg("clipLayerFiles")));

    def("StitchClipsTemplate",
        _ConvertStitchClipTemplate,
        (arg("resultLayer"),
         arg("topologyLayer"),
         arg("clipPath"),
         arg("templatePath"),
         arg("startTimeCode"),
         arg("endTimeCode"),
         arg("stride")));

    def("GenerateClipTopologyName",
        _ConvertGenerateClipTopologyName,
        (arg("rootLayerName")));
}

PXR_NAMESPACE_CLOSE_SCOPE

