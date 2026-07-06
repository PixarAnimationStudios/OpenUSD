//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
// GENERATED FILE.  DO NOT EDIT.
#include "pxr/external/boost/python/class.hpp"
#include "pxr/usd/usdLod/tokens.h"

PXR_NAMESPACE_USING_DIRECTIVE

#define _ADD_TOKEN(cls, name) \
    cls.add_static_property(#name, +[]() { return UsdLodTokens->name.GetString(); });

void wrapUsdLodTokens()
{
    pxr_boost::python::class_<UsdLodTokensType, pxr_boost::python::noncopyable>
        cls("Tokens", pxr_boost::python::no_init);
    _ADD_TOKEN(cls, allLOD);
    _ADD_TOKEN(cls, audio);
    _ADD_TOKEN(cls, blendThresholds);
    _ADD_TOKEN(cls, boundingVolume);
    _ADD_TOKEN(cls, center);
    _ADD_TOKEN(cls, extent);
    _ADD_TOKEN(cls, imaging);
    _ADD_TOKEN(cls, indexedLOD);
    _ADD_TOKEN(cls, inherited);
    _ADD_TOKEN(cls, lodDefaultIndex);
    _ADD_TOKEN(cls, lodDomain);
    _ADD_TOKEN(cls, lodHeuristics);
    _ADD_TOKEN(cls, lodOverrideIndex);
    _ADD_TOKEN(cls, lodOverrideMode);
    _ADD_TOKEN(cls, noLOD);
    _ADD_TOKEN(cls, noOverride);
    _ADD_TOKEN(cls, physics);
    _ADD_TOKEN(cls, projectedExtent);
    _ADD_TOKEN(cls, projectedSphere);
    _ADD_TOKEN(cls, projectionMethod);
    _ADD_TOKEN(cls, thresholds);
    _ADD_TOKEN(cls, LODDistanceHeuristic);
    _ADD_TOKEN(cls, LODHeuristic);
    _ADD_TOKEN(cls, LODOverrideAPI);
    _ADD_TOKEN(cls, LODRootAPI);
    _ADD_TOKEN(cls, LODScreenSizeHeuristic);
}
