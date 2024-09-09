//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/pxr.h"
#include "pxr/usd/usdGeom/bboxCache.h"

#include "pxr/base/tf/pyResultConversions.h"

#include "pxr/external/boost/python/class.hpp"
#include "pxr/external/boost/python/stl_iterator.hpp"

PXR_NAMESPACE_USING_DIRECTIVE

using namespace pxr_boost::python;

static object
_ComputePointInstanceWorldBounds(UsdGeomBBoxCache &self,
                                 const UsdGeomPointInstancer& instancer,
                                 object instanceIds)
{
    pxr_boost::python::stl_input_iterator<int64_t> begin(instanceIds), end;
    std::vector<int64_t> ids(begin, end);
    std::vector<GfBBox3d> boxes(ids.size());
    if (!self.ComputePointInstanceWorldBounds(
            instancer, ids.data(), ids.size(), boxes.data())) {
        return object();
    }
    pxr_boost::python::list ret;
    for (auto const &elem: boxes)
        ret.append(elem);
    return std::move(ret);
}

static object
_ComputePointInstanceRelativeBounds(UsdGeomBBoxCache &self,
                                    const UsdGeomPointInstancer& instancer,
                                    object instanceIds,
                                    UsdPrim relativeToAncestorPrim)
{
    pxr_boost::python::stl_input_iterator<int64_t> begin(instanceIds), end;
    std::vector<int64_t> ids(begin, end);
    std::vector<GfBBox3d> boxes(ids.size());
    if (!self.ComputePointInstanceRelativeBounds(
            instancer, ids.data(), ids.size(), relativeToAncestorPrim,
            boxes.data())) {
        return object();
    }
    pxr_boost::python::list ret;
    for (auto const &elem: boxes)
        ret.append(elem);
    return std::move(ret);
}

static object
_ComputePointInstanceLocalBounds(UsdGeomBBoxCache &self,
                                 const UsdGeomPointInstancer& instancer,
                                 object instanceIds)
{
    pxr_boost::python::stl_input_iterator<int64_t> begin(instanceIds), end;
    std::vector<int64_t> ids(begin, end);
    std::vector<GfBBox3d> boxes(ids.size());
    if (!self.ComputePointInstanceLocalBounds(
            instancer, ids.data(), ids.size(), boxes.data())) {
        return object();
    }
    pxr_boost::python::list ret;
    for (auto const &elem: boxes)
        ret.append(elem);
    return std::move(ret);
}

static object
_ComputePointInstanceUntransformedBounds(UsdGeomBBoxCache &self,
                                         const UsdGeomPointInstancer& instancer,
                                         object instanceIds)
{
    pxr_boost::python::stl_input_iterator<int64_t> begin(instanceIds), end;
    std::vector<int64_t> ids(begin, end);
    std::vector<GfBBox3d> boxes(ids.size());
    if (!self.ComputePointInstanceUntransformedBounds(
            instancer, ids.data(), ids.size(), boxes.data())) {
        return object();
    }
    pxr_boost::python::list ret;
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

    class_<BBoxCache>(
        "BBoxCache",
        init<UsdTimeCode, TfTokenVector, optional<bool, bool> >(
            (arg("time"), arg("includedPurposes"),
             arg("useExtentsHint"), arg("ignoreVisibility"))))
        .def("ComputeWorldBound", &BBoxCache::ComputeWorldBound, arg("prim"))
        .def("ComputeWorldBoundWithOverrides",
             &BBoxCache::ComputeWorldBoundWithOverrides,
             (arg("prim"),
              arg("pathsToSkip"),
              arg("primOverride"),
              arg("ctmOverrides")))              
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
