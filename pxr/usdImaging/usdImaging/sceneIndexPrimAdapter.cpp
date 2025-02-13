//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/usdImaging/usdImaging/sceneIndexPrimAdapter.h"

#include "pxr/base/tf/type.h"

PXR_NAMESPACE_OPEN_SCOPE

TF_REGISTRY_FUNCTION(TfType)
{
    using Adapter = UsdImagingSceneIndexPrimAdapter;
    TfType::Define<Adapter, TfType::Bases<Adapter::BaseAdapter>>();
    // No factory here, SceneIndexPrimAdapter is abstract.
}

UsdImagingSceneIndexPrimAdapter::UsdImagingSceneIndexPrimAdapter() = default;
UsdImagingSceneIndexPrimAdapter::~UsdImagingSceneIndexPrimAdapter() = default;

SdfPath
UsdImagingSceneIndexPrimAdapter::Populate(UsdPrim const& prim,
        UsdImagingIndexProxy* index,
        UsdImagingInstancerContext const* instancerContext)
{
    // We may run across imageable prims in UsdImagingDelegate, for which it
    // will dispatch to this adapter.  Make sure we don't do any work for them.
    TF_WARN("UsdImagingStageSceneIndex adapter invoked for UsdImagingDelegate "
            "for prim <%s>, skipping...", prim.GetPath().GetText());
    return SdfPath();
}

void
UsdImagingSceneIndexPrimAdapter::TrackVariability(UsdPrim const& prim,
        SdfPath const& cachePath,
        HdDirtyBits* timeVaryingBits,
        UsdImagingInstancerContext const* instancerContext) const
{
    TF_CODING_ERROR("This function should never be called.");
}

void
UsdImagingSceneIndexPrimAdapter::UpdateForTime(UsdPrim const& prim,
        SdfPath const& cachePath, 
        UsdTimeCode time,
        HdDirtyBits requestedBits,
        UsdImagingInstancerContext const* instancerContext) const
{
    TF_CODING_ERROR("This function should never be called.");
}

HdDirtyBits
UsdImagingSceneIndexPrimAdapter::ProcessPropertyChange(UsdPrim const& prim,
        SdfPath const& cachePath,
        TfToken const& propertyName)
{
    TF_CODING_ERROR("This function should never be called.");
    return HdChangeTracker::Clean;
}

void
UsdImagingSceneIndexPrimAdapter::MarkDirty(UsdPrim const& prim,
        SdfPath const& cachePath,
        HdDirtyBits dirty,
        UsdImagingIndexProxy* index)
{
    TF_CODING_ERROR("This function should never be called.");
}

void
UsdImagingSceneIndexPrimAdapter::_RemovePrim(SdfPath const& cachePath,
        UsdImagingIndexProxy* index)
{
    TF_CODING_ERROR("This function should never be called.");
}

PXR_NAMESPACE_CLOSE_SCOPE
