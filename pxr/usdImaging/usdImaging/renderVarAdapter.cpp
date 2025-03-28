//
// Copyright 2023 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/usdImaging/usdImaging/renderVarAdapter.h"
#include "pxr/usdImaging/usdImaging/dataSourceRenderPrims.h"
#include "pxr/usdImaging/usdImaging/delegate.h"
#include "pxr/usdImaging/usdImaging/indexProxy.h"
#include "pxr/usdImaging/usdImaging/tokens.h"

#include "pxr/imaging/hd/renderVarSchema.h"

#include "pxr/usd/usdRender/var.h"


PXR_NAMESPACE_OPEN_SCOPE


TF_REGISTRY_FUNCTION(TfType)
{
    typedef UsdImagingRenderVarAdapter Adapter;
    TfType t = TfType::Define<Adapter, TfType::Bases<Adapter::BaseAdapter> >();
    t.SetFactory< UsdImagingPrimAdapterFactory<Adapter> >();
}

UsdImagingRenderVarAdapter::~UsdImagingRenderVarAdapter() 
{
}

// -------------------------------------------------------------------------- //
// 2.0 Prim adapter API
// -------------------------------------------------------------------------- //

TfTokenVector
UsdImagingRenderVarAdapter::GetImagingSubprims(UsdPrim const& prim)
{
    return { TfToken() };
}

TfToken
UsdImagingRenderVarAdapter::GetImagingSubprimType(
    UsdPrim const& prim,
    TfToken const& subprim)
{
    if (subprim.IsEmpty()) {
        return HdRenderVarSchemaTokens->renderVar;
    }
    return TfToken();
}

HdContainerDataSourceHandle
UsdImagingRenderVarAdapter::GetImagingSubprimData(
    UsdPrim const& prim,
    TfToken const& subprim,
    const UsdImagingDataSourceStageGlobals &stageGlobals)
{
    if (subprim.IsEmpty()) {
        return UsdImagingDataSourceRenderVarPrim::New(
                    prim.GetPath(), prim, stageGlobals);
    }

    return nullptr;
}

HdDataSourceLocatorSet
UsdImagingRenderVarAdapter::InvalidateImagingSubprim(
    UsdPrim const& prim,
    TfToken const& subprim,
    TfTokenVector const& properties,
    const UsdImagingPropertyInvalidationType invalidationType)
{
    if (subprim.IsEmpty()) {
        return UsdImagingDataSourceRenderVarPrim::Invalidate(
            prim, subprim, properties, invalidationType);
    }

    return HdDataSourceLocatorSet();
}

// -------------------------------------------------------------------------- //
// 1.0 Prim adapter API
//
// \note No hydra prims are added/managed for UsdRenderVar prims.
//       UsdImagingRenderSettingsAdapter handles the flattening of
//       targeted products and vars.
// -------------------------------------------------------------------------- //

bool
UsdImagingRenderVarAdapter::IsSupported(
    UsdImagingIndexProxy const* index) const
{
    // Since we flatten products and vars into the targeting settings prim, 1.0
    // render delegates won't typically support render var prims as such.
    // Return true to supress warnings that the prim type isn't supported.
    return true;
}

SdfPath
UsdImagingRenderVarAdapter::Populate(
    UsdPrim const& prim, 
    UsdImagingIndexProxy* index,
    UsdImagingInstancerContext const* instancerContext)
{
    return SdfPath::EmptyPath();
}

void
UsdImagingRenderVarAdapter::_RemovePrim(
    SdfPath const& cachePath,
    UsdImagingIndexProxy* index)
{
}

void 
UsdImagingRenderVarAdapter::TrackVariability(
    UsdPrim const& prim,
    SdfPath const& cachePath,
    HdDirtyBits* timeVaryingBits,
    UsdImagingInstancerContext const* instancerContext) const
{
}

void 
UsdImagingRenderVarAdapter::UpdateForTime(
    UsdPrim const& prim,
    SdfPath const& cachePath, 
    UsdTimeCode time,
    HdDirtyBits requestedBits,
    UsdImagingInstancerContext const* 
    instancerContext) const
{
}

HdDirtyBits
UsdImagingRenderVarAdapter::ProcessPropertyChange(
    UsdPrim const& prim,
    SdfPath const& cachePath, 
    TfToken const& propertyName)
{
    return HdChangeTracker::Clean;
}

void
UsdImagingRenderVarAdapter::MarkDirty(
    UsdPrim const& prim,
    SdfPath const& cachePath,
    HdDirtyBits dirty,
    UsdImagingIndexProxy* index)
{
}

VtValue
UsdImagingRenderVarAdapter::Get(
    UsdPrim const& prim,
    SdfPath const& cachePath,
    TfToken const& key,
    UsdTimeCode time,
    VtIntArray *outIndices) const
{
    return VtValue();
}

PXR_NAMESPACE_CLOSE_SCOPE
