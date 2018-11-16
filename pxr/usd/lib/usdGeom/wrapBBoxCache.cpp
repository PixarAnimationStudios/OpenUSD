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
#include "pxr/pxr.h"
#include "pxr/usd/usdGeom/bboxCache.h"

#include "pxr/base/tf/pyResultConversions.h"

#include <boost/python/class.hpp>
#include <boost/python/stl_iterator.hpp>

using namespace boost::python;

PXR_NAMESPACE_USING_DIRECTIVE

static object
_ComputePointInstanceWorldBounds(UsdGeomBBoxCache &self,
                                 const UsdGeomPointInstancer& instancer,
                                 object instanceIds)
{
    boost::python::stl_input_iterator<int64_t> begin(instanceIds), end;
    std::vector<int64_t> ids(begin, end);
    std::vector<GfBBox3d> boxes(ids.size());
    if (!self.ComputePointInstanceWorldBounds(
            instancer, ids.data(), ids.size(), boxes.data())) {
        return object();
    }
    boost::python::list ret;
    for (auto const &elem: boxes)
        ret.append(elem);
    return ret;
}

static object
_ComputePointInstanceRelativeBounds(UsdGeomBBoxCache &self,
                                    const UsdGeomPointInstancer& instancer,
                                    object instanceIds,
                                    UsdPrim relativeToAncestorPrim)
{
    boost::python::stl_input_iterator<int64_t> begin(instanceIds), end;
    std::vector<int64_t> ids(begin, end);
    std::vector<GfBBox3d> boxes(ids.size());
    if (!self.ComputePointInstanceRelativeBounds(
            instancer, ids.data(), ids.size(), relativeToAncestorPrim,
            boxes.data())) {
        return object();
    }
    boost::python::list ret;
    for (auto const &elem: boxes)
        ret.append(elem);
    return ret;
}

static object
_ComputePointInstanceLocalBounds(UsdGeomBBoxCache &self,
                                 const UsdGeomPointInstancer& instancer,
                                 object instanceIds)
{
    boost::python::stl_input_iterator<int64_t> begin(instanceIds), end;
    std::vector<int64_t> ids(begin, end);
    std::vector<GfBBox3d> boxes(ids.size());
    if (!self.ComputePointInstanceLocalBounds(
            instancer, ids.data(), ids.size(), boxes.data())) {
        return object();
    }
    boost::python::list ret;
    for (auto const &elem: boxes)
        ret.append(elem);
    return ret;
}

static object
_ComputePointInstanceUntransformedBounds(UsdGeomBBoxCache &self,
                                         const UsdGeomPointInstancer& instancer,
                                         object instanceIds)
{
    boost::python::stl_input_iterator<int64_t> begin(instanceIds), end;
    std::vector<int64_t> ids(begin, end);
    std::vector<GfBBox3d> boxes(ids.size());
    if (!self.ComputePointInstanceUntransformedBounds(
            instancer, ids.data(), ids.size(), boxes.data())) {
        return object();
    }
    boost::python::list ret;
    for (auto const &elem: boxes)
        ret.append(elem);
    return ret;
}

void wrapUsdGeomBBoxCache()
{
    typedef UsdGeomBBoxCache BBoxCache;

    GfBBox3d (BBoxCache::*ComputeUntransformedBound_1)(const UsdPrim &) = 
        &BBoxCache::ComputeUntransformedBound;
    GfBBox3d (BBoxCache::*ComputeUntransformedBound_2)(
        const UsdPrim &,
        const SdfPathSet &,
        const TfHashMap<SdfPath, GfMatrix4d, SdfPath::Hash> &) = 
        &BBoxCache::ComputeUntransformedBound;

    class_<BBoxCache>(
        "BBoxCache",
        init<UsdTimeCode, TfTokenVector, optional<bool, bool> >(
            (arg("time"), arg("includedPurposes"),
             arg("useExtentsHint"), arg("ignoreVisibility"))))
        .def("ComputeWorldBound", &BBoxCache::ComputeWorldBound, arg("prim"))
        .def("ComputeLocalBound", &BBoxCache::ComputeLocalBound, arg("prim"))
        .def("ComputeRelativeBound", &BBoxCache::ComputeRelativeBound, 
            (arg("prim"), arg("relativeRootPrim")))
        .def("ComputeUntransformedBound", ComputeUntransformedBound_1, 
            arg("prim"))
        .def("ComputeUntransformedBound", ComputeUntransformedBound_2, 
             (arg("prim"),
              arg("pathsToSkip"),
              arg("ctmOverrides")))
        .def("ComputePointInstanceWorldBounds",
             _ComputePointInstanceWorldBounds,
             (arg("instancer"), arg("instanceIds")))
        .def("ComputePointInstanceWorldBound",
             &BBoxCache::ComputePointInstanceWorldBound,
             (arg("instancer"), arg("instanceId")))
        .def("ComputePointInstanceRelativeBounds",
             _ComputePointInstanceRelativeBounds,
             (arg("instancer"), arg("instanceIds"),
              arg("relativeToAncestorPrim")))
        .def("ComputePointInstanceRelativeBound",
             &BBoxCache::ComputePointInstanceRelativeBound,
             (arg("instancer"), arg("instanceId"),
              arg("relativeToAncestorPrim")))
        .def("ComputePointInstanceLocalBounds",
             _ComputePointInstanceLocalBounds,
             (arg("instancer"), arg("instanceIds")))
        .def("ComputePointInstanceLocalBound",
             &BBoxCache::ComputePointInstanceLocalBound,
             (arg("instancer"), arg("instanceId")))
        .def("ComputePointInstanceUntransformedBounds",
             _ComputePointInstanceUntransformedBounds,
             (arg("instancer"), arg("instanceIds")))
        .def("ComputePointInstanceUntransformedBound",
             &BBoxCache::ComputePointInstanceUntransformedBound,
             (arg("instancer"), arg("instanceId")))
        
        .def("Clear", &BBoxCache::Clear)
        .def("SetIncludedPurposes", &BBoxCache::SetIncludedPurposes,
             arg("includedPurposes"))
        .def("GetIncludedPurposes", &BBoxCache::GetIncludedPurposes,
             return_value_policy<TfPySequenceToList>())
        .def("SetTime", &BBoxCache::SetTime, arg("time"))
        .def("GetTime", &BBoxCache::GetTime)
        .def("SetBaseTime", &BBoxCache::SetBaseTime, arg("time"))
        .def("GetBaseTime", &BBoxCache::GetBaseTime)
        .def("HasBaseTime", &BBoxCache::HasBaseTime)
        .def("ClearBaseTime", &BBoxCache::ClearBaseTime)
        .def("GetUseExtentsHint", &BBoxCache::GetUseExtentsHint)
        ;
}
