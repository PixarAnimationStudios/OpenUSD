//
// Copyright 2024 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
////////////////////////////////////////////////////////////////////////

#ifndef EXT_RMANPKG_PLUGIN_RENDERMAN__HD_PRMAN_RILEY_CONVERSION_SCENE_INDEX__H
#define EXT_RMANPKG_PLUGIN_RENDERMAN__HD_PRMAN_RILEY_CONVERSION_SCENE_INDEX__H

#include "pxr/pxr.h"
#include "pxr/imaging/hd/filteringSceneIndex.h"
#include "pxr/base/tf/declarePtrs.h"
#include "hdPrman/api.h"

PXR_NAMESPACE_OPEN_SCOPE

class HdPrman_RileyConversionSceneIndex;
TF_DECLARE_REF_PTRS(HdPrman_RileyConversionSceneIndex);

/// \class HdPrman_RileyConversionSceneIndex
///
/// Converts hydra prims to riley prims.
///
/// This scene index is part of the HdPrman 2.0 pathway and instantiated by
/// HdPrman_Renderer2SceneIndexPlugin.
///
/// The implementation is incomplete and only active
/// HD_PRMAN_EXPERIMENTAL_RILEY_SCENE_INDEX_OBSERVER is set to true.
///
/// The limitations are as follows:
/// - it only converts spheres
/// - it always uses the fallback material
/// - it ignores instancers
///
/// An example: Given a sphere /Sphere, the conversion results in:
///
/// /Sphere, type: riley:geometryPrototype
/// /Sphere/RileyConversionGeometryInstance, type: riley:geometryInstance
///
class HdPrman_RileyConversionSceneIndex
    : public HdSingleInputFilteringSceneIndexBase
{
public:
    static HdPrman_RileyConversionSceneIndexRefPtr
    New(const HdSceneIndexBaseRefPtr& inputSceneIndex);

    HdSceneIndexPrim GetPrim(const SdfPath &primPath) const override;
    SdfPathVector GetChildPrimPaths(const SdfPath &primPath) const override;

protected:
    HdPrman_RileyConversionSceneIndex(
        const HdSceneIndexBaseRefPtr& inputSceneIndex);
    ~HdPrman_RileyConversionSceneIndex();

    void _PrimsAdded(
        const HdSceneIndexBase &sender,
        const HdSceneIndexObserver::AddedPrimEntries &entries) override;
    void _PrimsRemoved(
        const HdSceneIndexBase &sender,
        const HdSceneIndexObserver::RemovedPrimEntries &entries) override;
    void _PrimsDirtied(
        const HdSceneIndexBase &sender,
        const HdSceneIndexObserver::DirtiedPrimEntries &entries) override;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // EXT_RMANPKG_PLUGIN_RENDERMAN__HD_PRMAN_RILEY_CONVERSION_SCENE_INDEX__H

