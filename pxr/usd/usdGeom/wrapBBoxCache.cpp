//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/pxr.h"
#include "pxr/usd/usdGeom/bboxCache.h"

#include "pxr/base/tf/pyNanobindBridge.h"
#include "pxr/base/tf/pyResultConversions.h"

#include <nanobind/nanobind.h>
#include <nanobind/stl/vector.h>

namespace nb = nanobind;

PXR_NAMESPACE_USING_DIRECTIVE

static std::vector<int64_t>
_ExtractInstanceIds(const TfPyObjWrapper &instanceIds)
{
    std::vector<int64_t> ids;
    for (nb::handle h : nb::cast<nb::iterable>(instanceIds.Get())) {
        ids.push_back(nb::cast<int64_t>(h));
    }
    return ids;
}

static nb::object
_ComputePointInstanceWorldBounds(UsdGeomBBoxCache &self,
                                 const UsdGeomPointInstancer& instancer,
                                 TfPyObjWrapper instanceIds)
{
    std::vector<int64_t> ids = _ExtractInstanceIds(instanceIds);
    std::vector<GfBBox3d> boxes(ids.size());
    if (!self.ComputePointInstanceWorldBounds(
            instancer, ids.data(), ids.size(), boxes.data())) {
        return nb::none();
    }
    nb::list ret;
    for (auto const &elem: boxes)
        ret.append(nb::cast(elem));
    return ret;
}

static nb::object
_ComputePointInstanceRelativeBounds(UsdGeomBBoxCache &self,
                                    const UsdGeomPointInstancer& instancer,
                                    TfPyObjWrapper instanceIds,
                                    UsdPrim relativeToAncestorPrim)
{
    std::vector<int64_t> ids = _ExtractInstanceIds(instanceIds);
    std::vector<GfBBox3d> boxes(ids.size());
    if (!self.ComputePointInstanceRelativeBounds(
            instancer, ids.data(), ids.size(), relativeToAncestorPrim,
            boxes.data())) {
        return nb::none();
    }
    nb::list ret;
    for (auto const &elem: boxes)
        ret.append(nb::cast(elem));
    return ret;
}

static nb::object
_ComputePointInstanceLocalBounds(UsdGeomBBoxCache &self,
                                 const UsdGeomPointInstancer& instancer,
                                 TfPyObjWrapper instanceIds)
{
    std::vector<int64_t> ids = _ExtractInstanceIds(instanceIds);
    std::vector<GfBBox3d> boxes(ids.size());
    if (!self.ComputePointInstanceLocalBounds(
            instancer, ids.data(), ids.size(), boxes.data())) {
        return nb::none();
    }
    nb::list ret;
    for (auto const &elem: boxes)
        ret.append(nb::cast(elem));
    return ret;
}

static nb::object
_ComputePointInstanceUntransformedBounds(UsdGeomBBoxCache &self,
                                         const UsdGeomPointInstancer& instancer,
                                         TfPyObjWrapper instanceIds)
{
    std::vector<int64_t> ids = _ExtractInstanceIds(instanceIds);
    std::vector<GfBBox3d> boxes(ids.size());
    if (!self.ComputePointInstanceUntransformedBounds(
            instancer, ids.data(), ids.size(), boxes.data())) {
        return nb::none();
    }
    nb::list ret;
    for (auto const &elem: boxes)
        ret.append(nb::cast(elem));
    return ret;
}

static TfTokenVector
_IterableToTokenVector(nb::iterable it)
{
    TfTokenVector result;
    for (nb::handle h : it) {
        result.push_back(nb::cast<TfToken>(h));
    }
    return result;
}

void wrapUsdGeomBBoxCache(nb::module_ &m)
{
    typedef UsdGeomBBoxCache BBoxCache;

    GfBBox3d (BBoxCache::*ComputeUntransformedBound_1)(const UsdPrim &) = 
        &BBoxCache::ComputeUntransformedBound;

    nb::class_<BBoxCache>(m, "BBoxCache")
        .def(nb::init<UsdTimeCode, TfTokenVector, bool, bool>(),
             nb::arg("time"), nb::arg("includedPurposes"),
             nb::arg("useExtentsHint") = false,
             nb::arg("ignoreVisibility") = false)
        .def("__init__", [](BBoxCache *self, UsdTimeCode time, nb::iterable includedPurposes,
                            bool useExtentsHint, bool ignoreVisibility) {
            new (self) BBoxCache(time, _IterableToTokenVector(includedPurposes),
                                 useExtentsHint, ignoreVisibility);
        }, nb::arg("time"), nb::arg("includedPurposes"),
           nb::arg("useExtentsHint") = false,
           nb::arg("ignoreVisibility") = false)
        .def("ComputeWorldBound", &BBoxCache::ComputeWorldBound, nb::arg("prim"))
        .def("ComputeWorldBoundWithOverrides",
             [](BBoxCache &self,
                const UsdPrim &prim,
                const SdfPathSet &pathsToSkip,
                const GfMatrix4d &primOverride,
                const std::unordered_map<SdfPath, GfMatrix4d, SdfPath::Hash> &ctmOverrides) {
                 TfHashMap<SdfPath, GfMatrix4d, SdfPath::Hash> map(
                     ctmOverrides.begin(), ctmOverrides.end());
                 return self.ComputeWorldBoundWithOverrides(
                     prim, pathsToSkip, primOverride, map);
             },
             nb::arg("prim"),
             nb::arg("pathsToSkip"),
             nb::arg("primOverride"),
             nb::arg("ctmOverrides"))              
        .def("ComputeLocalBound", &BBoxCache::ComputeLocalBound, nb::arg("prim"))
        .def("ComputeRelativeBound", &BBoxCache::ComputeRelativeBound, 
             nb::arg("prim"), nb::arg("relativeRootPrim"))
        .def("ComputeUntransformedBound", ComputeUntransformedBound_1, 
             nb::arg("prim"))
        .def("ComputeUntransformedBound",
             [](BBoxCache &self,
                const UsdPrim &prim,
                const SdfPathSet &pathsToSkip,
                const std::unordered_map<SdfPath, GfMatrix4d, SdfPath::Hash> &ctmOverrides) {
                 TfHashMap<SdfPath, GfMatrix4d, SdfPath::Hash> map(
                     ctmOverrides.begin(), ctmOverrides.end());
                 return self.ComputeUntransformedBound(prim, pathsToSkip, map);
             },
             nb::arg("prim"),
             nb::arg("pathsToSkip"),
             nb::arg("ctmOverrides"))
        .def("ComputePointInstanceWorldBounds",
             _ComputePointInstanceWorldBounds,
             nb::arg("instancer"), nb::arg("instanceIds"))
        .def("ComputePointInstanceWorldBound",
             &BBoxCache::ComputePointInstanceWorldBound,
             nb::arg("instancer"), nb::arg("instanceId"))
        .def("ComputePointInstanceRelativeBounds",
             _ComputePointInstanceRelativeBounds,
             nb::arg("instancer"), nb::arg("instanceIds"),
             nb::arg("relativeToAncestorPrim"))
        .def("ComputePointInstanceRelativeBound",
             &BBoxCache::ComputePointInstanceRelativeBound,
             nb::arg("instancer"), nb::arg("instanceId"),
             nb::arg("relativeToAncestorPrim"))
        .def("ComputePointInstanceLocalBounds",
             _ComputePointInstanceLocalBounds,
             nb::arg("instancer"), nb::arg("instanceIds"))
        .def("ComputePointInstanceLocalBound",
             &BBoxCache::ComputePointInstanceLocalBound,
             nb::arg("instancer"), nb::arg("instanceId"))
        .def("ComputePointInstanceUntransformedBounds",
             _ComputePointInstanceUntransformedBounds,
             nb::arg("instancer"), nb::arg("instanceIds"))
        .def("ComputePointInstanceUntransformedBound",
             &BBoxCache::ComputePointInstanceUntransformedBound,
             nb::arg("instancer"), nb::arg("instanceId"))
        
        .def("Clear", &BBoxCache::Clear)
        .def("SetIncludedPurposes", &BBoxCache::SetIncludedPurposes,
             nb::arg("includedPurposes"))
        .def("SetIncludedPurposes", [](BBoxCache &self, nb::iterable includedPurposes) {
            self.SetIncludedPurposes(_IterableToTokenVector(includedPurposes));
        }, nb::arg("includedPurposes"))
        .def("GetIncludedPurposes", &BBoxCache::GetIncludedPurposes,
             nb::rv_policy::copy)
        .def("SetTime", &BBoxCache::SetTime, nb::arg("time"))
        .def("GetTime", &BBoxCache::GetTime)
        .def("SetBaseTime", &BBoxCache::SetBaseTime, nb::arg("time"))
        .def("GetBaseTime", &BBoxCache::GetBaseTime)
        .def("HasBaseTime", &BBoxCache::HasBaseTime)
        .def("ClearBaseTime", &BBoxCache::ClearBaseTime)
        .def("GetUseExtentsHint", &BBoxCache::GetUseExtentsHint)
        ;

}

