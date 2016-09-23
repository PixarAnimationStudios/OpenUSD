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

#include <boost/python/def.hpp>
#include <boost/python/extract.hpp>

#include "pxr/usd/usdUtils/stitchClips.h"
#include "pxr/base/tf/pyUtils.h"

#include <limits>

using namespace boost::python;

template <typename T>
T
_ConvertWithDefault(const boost::python::object obj, const T& def)
{
    if (not TfPyIsNone(obj)) {
        return extract<T>(obj);
    } 
        
    return def;
}

void
_ConvertStitchClips(const SdfLayerHandle& resultLayer,
                    const std::vector<std::string>& clipLayerFiles,
                    const SdfPath& clipPath,
                    const boost::python::object reuseExistingTopology,
                    const boost::python::object pyStartFrame,
                    const boost::python::object pyEndFrame)
{
    constexpr double dmax = std::numeric_limits<double>::max();
    UsdUtilsStitchClips(resultLayer, clipLayerFiles, clipPath,
                        _ConvertWithDefault(reuseExistingTopology, true),
                        _ConvertWithDefault(pyStartFrame, dmax),
                        _ConvertWithDefault(pyEndFrame, dmax));
}

void _ConvertStitchClipsToplogy(const SdfLayerHandle& topologyLayer,
                                const std::vector<std::string>& clipLayerFiles)
{
    UsdUtilsStitchClipsTopology(topologyLayer, clipLayerFiles);
}

void 
wrapStitchClips()
{
    def("StitchClips",
        _ConvertStitchClips, 
        (arg("resultLayer"), 
         arg("clipLayerFiles"), 
         arg("clipPath"), 
         arg("reuseExistingTopology")=boost::python::object(),
         arg("startFrame")=boost::python::object(),
         arg("endFrame")=boost::python::object()));

    def("StitchClipsTopology",
        _ConvertStitchClipsToplogy,
        (arg("topologyLayer"),
         arg("clipLayerFiles")));
}
