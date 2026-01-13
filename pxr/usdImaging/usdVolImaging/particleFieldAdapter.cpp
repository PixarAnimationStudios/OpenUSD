//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/usdImaging/usdVolImaging/particleFieldAdapter.h"

#include "pxr/usdImaging/usdVolImaging/dataSourceParticleField.h"

#include "pxr/usdImaging/usdImaging/delegate.h"
#include "pxr/usdImaging/usdImaging/indexProxy.h"
#include "pxr/usdImaging/usdImaging/primvarUtils.h"
#include "pxr/usdImaging/usdImaging/tokens.h"

#include "pxr/imaging/hd/perfLog.h"
#include "pxr/imaging/hd/points.h"

#include "pxr/usd/usdVol/particleField.h"

#include "pxr/base/tf/type.h"

PXR_NAMESPACE_OPEN_SCOPE

TF_REGISTRY_FUNCTION(TfType) {
    typedef UsdImagingParticleFieldAdapter Adapter;
    TfType adapterType =
        TfType::Define<Adapter, TfType::Bases<Adapter::BaseAdapter>>();
    adapterType.SetFactory<UsdImagingPrimAdapterFactory<Adapter>>();
}

UsdImagingParticleFieldAdapter::~UsdImagingParticleFieldAdapter()
{
}

TfTokenVector
UsdImagingParticleFieldAdapter::GetImagingSubprims(UsdPrim const& prim)
{
    return { TfToken() };
}

TfToken
UsdImagingParticleFieldAdapter::GetImagingSubprimType(
    UsdPrim const& prim, TfToken const& subprim)
{
    if (subprim.IsEmpty()) {
        return HdParticleFieldTokens->ParticleField3DGaussianSplat;
    }
    return TfToken();
}

HdContainerDataSourceHandle
UsdImagingParticleFieldAdapter::GetImagingSubprimData(
    UsdPrim const& prim, TfToken const& subprim,
    const UsdImagingDataSourceStageGlobals& stageGlobals)
{
    if (subprim.IsEmpty()) {
        return UsdImagingDataSourceParticleFieldPrim::New(
            prim.GetPath(), prim, stageGlobals);
    }
    return nullptr;
}

HdDataSourceLocatorSet
UsdImagingParticleFieldAdapter::InvalidateImagingSubprim(
    UsdPrim const& prim, TfToken const& subprim,
    TfTokenVector const& properties,
    const UsdImagingPropertyInvalidationType invalidationType)
{
    if (subprim.IsEmpty()) {
        return UsdImagingDataSourceParticleFieldPrim::Invalidate(
            prim, subprim, properties, invalidationType);
    }

    return HdDataSourceLocatorSet();
}

bool
UsdImagingParticleFieldAdapter::IsSupported(
    UsdImagingIndexProxy const* index) const
{
    return index->IsRprimTypeSupported(
        HdParticleFieldTokens->ParticleField3DGaussianSplat);
}

SdfPath
UsdImagingParticleFieldAdapter::Populate(
    UsdPrim const& prim, UsdImagingIndexProxy* index,
    UsdImagingInstancerContext const* instancerContext)
{
    return _AddRprim(HdParticleFieldTokens->ParticleField3DGaussianSplat,
        prim, index, GetMaterialUsdPath(prim), instancerContext);
}

void UsdImagingParticleFieldAdapter::TrackVariability(
    UsdPrim const& prim, SdfPath const& cachePath,
    HdDirtyBits* timeVaryingBits,
    UsdImagingInstancerContext const* instancerContext) const
{
    BaseAdapter::TrackVariability(prim, cachePath, timeVaryingBits, instancerContext);
//
//     // Discover time-varying points.
//     _IsVarying(prim,
//     UsdLightFieldTokens->positions,
//                HdChangeTracker::DirtyPoints,
//                UsdImagingTokens->usdVaryingPrimvar,
//                timeVaryingBits,
//                /*isInherited*/false);
//     _IsVarying(prim,
// UsdLightFieldTokens->positionsh,
//            HdChangeTracker::DirtyPoints,
//            UsdImagingTokens->usdVaryingPrimvar,
//            timeVaryingBits,
//            /*isInherited*/false);
}

void UsdImagingParticleFieldAdapter::UpdateForTime(
    UsdPrim const& prim, SdfPath const& cachePath, UsdTimeCode time,
    HdDirtyBits requestedBits,
    UsdImagingInstancerContext const* instancerContext) const
{
    BaseAdapter::UpdateForTime(
        prim, cachePath, time, requestedBits, instancerContext);
}

HdDirtyBits
UsdImagingParticleFieldAdapter::ProcessPropertyChange(
    UsdPrim const& prim, SdfPath const& cachePath,
    TfToken const& propertyName)
{
    return BaseAdapter::ProcessPropertyChange(prim, cachePath, propertyName);
}

VtValue
UsdImagingParticleFieldAdapter::Get(
    UsdPrim const& prim, SdfPath const& cachePath, TfToken const& key,
    UsdTimeCode time, VtIntArray* outIndices) const {

    return BaseAdapter::Get(prim, cachePath, key, time, outIndices);
}

PXR_NAMESPACE_CLOSE_SCOPE
