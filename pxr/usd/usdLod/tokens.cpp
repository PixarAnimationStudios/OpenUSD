//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/usd/usdLod/tokens.h"

PXR_NAMESPACE_OPEN_SCOPE

UsdLodTokensType::UsdLodTokensType() :
    allLOD("allLOD", TfToken::Immortal),
    audio("audio", TfToken::Immortal),
    blendThresholds("blendThresholds", TfToken::Immortal),
    boundingVolume("boundingVolume", TfToken::Immortal),
    center("center", TfToken::Immortal),
    extent("extent", TfToken::Immortal),
    imaging("imaging", TfToken::Immortal),
    indexedLOD("indexedLOD", TfToken::Immortal),
    inherited("inherited", TfToken::Immortal),
    lodDefaultIndex("lod:default:index", TfToken::Immortal),
    lodDomain("lod:domain", TfToken::Immortal),
    lodHeuristics("lod:heuristics", TfToken::Immortal),
    lodOverrideIndex("lod:override:index", TfToken::Immortal),
    lodOverrideMode("lod:override:mode", TfToken::Immortal),
    noLOD("noLOD", TfToken::Immortal),
    noOverride("noOverride", TfToken::Immortal),
    physics("physics", TfToken::Immortal),
    projectedExtent("projectedExtent", TfToken::Immortal),
    projectedSphere("projectedSphere", TfToken::Immortal),
    projectionMethod("projectionMethod", TfToken::Immortal),
    thresholds("thresholds", TfToken::Immortal),
    LODDistanceHeuristic("LODDistanceHeuristic", TfToken::Immortal),
    LODHeuristic("LODHeuristic", TfToken::Immortal),
    LODOverrideAPI("LODOverrideAPI", TfToken::Immortal),
    LODRootAPI("LODRootAPI", TfToken::Immortal),
    LODScreenSizeHeuristic("LODScreenSizeHeuristic", TfToken::Immortal),
    allTokens({
        allLOD,
        audio,
        blendThresholds,
        boundingVolume,
        center,
        extent,
        imaging,
        indexedLOD,
        inherited,
        lodDefaultIndex,
        lodDomain,
        lodHeuristics,
        lodOverrideIndex,
        lodOverrideMode,
        noLOD,
        noOverride,
        physics,
        projectedExtent,
        projectedSphere,
        projectionMethod,
        thresholds,
        LODDistanceHeuristic,
        LODHeuristic,
        LODOverrideAPI,
        LODRootAPI,
        LODScreenSizeHeuristic
    })
{
}

TfStaticData<UsdLodTokensType> UsdLodTokens;

PXR_NAMESPACE_CLOSE_SCOPE
