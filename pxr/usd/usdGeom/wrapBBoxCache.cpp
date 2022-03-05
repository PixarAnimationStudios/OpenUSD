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


PXR_NAMESPACE_USING_DIRECTIVE

static boost::python::object
_ComputePointInstanceWorldBounds(UsdGeomBBoxCache &self,
                                 const UsdGeomPointInstancer& instancer,
                                 boost::python::object instanceIds)
{
    boost::python::stl_input_iterator<int64_t> begin(instanceIds), end;
    std::vector<int64_t> ids(begin, end);
    std::vector<GfBBox3d> boxes(ids.size());
    if (!self.ComputePointInstanceWorldBounds(
            instancer, ids.data(), ids.size(), boxes.data())) {
        return boost::python::object();
    }
    boost::python::list ret;
    for (auto const &elem: boxes)
        ret.append(elem);
    return std::move(ret);
}

static boost::python::object
_ComputePointInstanceRelativeBounds(UsdGeomBBoxCache &self,
                                    const UsdGeomPointInstancer& instancer,
                                    boost::python::object instanceIds,
                                    UsdPrim relativeToAncestorPrim)
{
    boost::python::stl_input_iterator<int64_t> begin(instanceIds), end;
    std::vector<int64_t> ids(begin, end);
    std::vector<GfBBox3d> boxes(ids.size());
    if (!self.ComputePointInstanceRelativeBounds(
            instancer, ids.data(), ids.size(), relativeToAncestorPrim,
            boxes.data())) {
        return boost::python::object();
    }
    boost::python::list ret;
    for (auto const &elem: boxes)
        ret.append(elem);
    return std::move(ret);
}

static boost::python::object
_ComputePointInstanceLocalBounds(UsdGeomBBoxCache &self,
                                 const UsdGeomPointInstancer& instancer,
                                 boost::python::object instanceIds)
{
    boost::python::stl_input_iterator<int64_t> begin(instanceIds), end;
    std::vector<int64_t> ids(begin, end);
    std::vector<GfBBox3d> boxes(ids.size());
    if (!self.ComputePointInstanceLocalBounds(
            instancer, ids.data(), ids.size(), boxes.data())) {
        return boost::python::object();
    }
    boost::python::list ret;
    for (auto const &elem: boxes)
        ret.append(elem);
    return std::move(ret);
}

static boost::python::object
_ComputePointInstanceUntransformedBounds(UsdGeomBBoxCache &self,
                                         const UsdGeomPointInstancer& instancer,
                                         boost::python::object instanceIds)
{
    boost::python::stl_input_iterator<int64_t> begin(instanceIds), end;
    std::vector<int64_t> ids(begin, end);
    std::vector<GfBBox3d> boxes(ids.size());
    if (!self.ComputePointInstanceUntransformedBounds(
            instancer, ids.data(), ids.size(), boxes.data())) {
        return boost::python::object();
    }
    boost::python::list ret;
    for (auto const &elem: boxes)
        ret.append(elem);
    return std::move(ret);
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

    boost::python::class_<BBoxCache>(
        "BBoxCache",
        boost::python::init<UsdTimeCode, TfTokenVector, boost::python::optional<bool, bool> >(
            (boost::python::arg("time"), boost::python::arg("includedPurposes"),
             boost::python::arg("useExtentsHint"), boost::python::arg("ignoreVisibility"))))
        .def("ComputeWorldBound", &BBoxCache::ComputeWorldBound, boost::python::arg("prim"))
        .def("ComputeLocalBound", &BBoxCache::ComputeLocalBound, boost::python::arg("prim"))
        .def("ComputeRelativeBound", &BBoxCache::ComputeRelativeBound, 
            (boost::python::arg("prim"), boost::python::arg("relativeRootPrim")))
        .def("ComputeUntransformedBound", ComputeUntransformedBound_1, 
            boost::python::arg("prim"))
        .def("ComputeUntransformedBound", ComputeUntransformedBound_2, 
             (boost::python::arg("prim"),
              boost::python::arg("pathsToSkip"),
              boost::python::arg("ctmOverrides")))
        .def("ComputePointInstanceWorldBounds",
             _ComputePointInstanceWorldBounds,
             (boost::python::arg("instancer"), boost::python::arg("instanceIds")))
        .def("ComputePointInstanceWorldBound",
             &BBoxCache::ComputePointInstanceWorldBound,
             (boost::python::arg("instancer"), boost::python::arg("instanceId")))
        .def("ComputePointInstanceRelativeBounds",
             _ComputePointInstanceRelativeBounds,
             (boost::python::arg("instancer"), boost::python::arg("instanceIds"),
              boost::python::arg("relativeToAncestorPrim")))
        .def("ComputePointInstanceRelativeBound",
             &BBoxCache::ComputePointInstanceRelativeBound,
             (boost::python::arg("instancer"), boost::python::arg("instanceId"),
              boost::python::arg("relativeToAncestorPrim")))
        .def("ComputePointInstanceLocalBounds",
             _ComputePointInstanceLocalBounds,
             (boost::python::arg("instancer"), boost::python::arg("instanceIds")))
        .def("ComputePointInstanceLocalBound",
             &BBoxCache::ComputePointInstanceLocalBound,
             (boost::python::arg("instancer"), boost::python::arg("instanceId")))
        .def("ComputePointInstanceUntransformedBounds",
             _ComputePointInstanceUntransformedBounds,
             (boost::python::arg("instancer"), boost::python::arg("instanceIds")))
        .def("ComputePointInstanceUntransformedBound",
             &BBoxCache::ComputePointInstanceUntransformedBound,
             (boost::python::arg("instancer"), boost::python::arg("instanceId")))
        
        .def("Clear", &BBoxCache::Clear)
        .def("SetIncludedPurposes", &BBoxCache::SetIncludedPurposes,
             boost::python::arg("includedPurposes"))
        .def("GetIncludedPurposes", &BBoxCache::GetIncludedPurposes,
             boost::python::return_value_policy<TfPySequenceToList>())
        .def("SetTime", &BBoxCache::SetTime, boost::python::arg("time"))
        .def("GetTime", &BBoxCache::GetTime)
        .def("SetBaseTime", &BBoxCache::SetBaseTime, boost::python::arg("time"))
        .def("GetBaseTime", &BBoxCache::GetBaseTime)
        .def("HasBaseTime", &BBoxCache::HasBaseTime)
        .def("ClearBaseTime", &BBoxCache::ClearBaseTime)
        .def("GetUseExtentsHint", &BBoxCache::GetUseExtentsHint)
        ;
}
