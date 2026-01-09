//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "particleFieldAdapter.h"

#include "dataSourceParticleField.h"
#include <pxr/usdImaging/usdImaging/delegate.h>
#include <pxr/usdImaging/usdImaging/indexProxy.h>
#include <pxr/usdImaging/usdImaging/primvarUtils.h>
#include <pxr/usdImaging/usdImaging/tokens.h>

#include <pxr/imaging/hd/perfLog.h>
#include <pxr/imaging/hd/points.h>

#include <pxr/usd/usdGeom/points.h>
#include <pxr/usd/usdGeom/primvarsAPI.h>

#include <pxr/usd/usdLightField/particleField3DGaussianSplat.h>

#include <pxr/base/tf/type.h>

#include "../tokens.h"

PXR_NAMESPACE_OPEN_SCOPE

TF_REGISTRY_FUNCTION(TfType) {
    typedef UsdImaging_3DGaussianSplatAdapter Adapter;
    TfType adapterType = TfType::Define<Adapter, TfType::Bases<Adapter::BaseAdapter>>();
    adapterType.SetFactory<UsdImagingPrimAdapterFactory<Adapter>>();
}

UsdImaging_3DGaussianSplatAdapter::~UsdImaging_3DGaussianSplatAdapter() {}

TfTokenVector UsdImaging_3DGaussianSplatAdapter::GetImagingSubprims(UsdPrim const& prim) { return {TfToken()}; }

TfToken UsdImaging_3DGaussianSplatAdapter::GetImagingSubprimType(UsdPrim const& prim, TfToken const& subprim) {
    if (subprim.IsEmpty()) {
        return HdParticleFieldTokens->ParticleField3DGaussianSplat;
    }
    return TfToken();
}

HdContainerDataSourceHandle
UsdImaging_3DGaussianSplatAdapter::GetImagingSubprimData(UsdPrim const& prim, TfToken const& subprim,
                                                         const UsdImagingDataSourceStageGlobals& stageGlobals) {
    if (subprim.IsEmpty()) {
        return UsdImagingDataSource_3DGaussianSplatPrim::New(prim.GetPath(), prim, stageGlobals);
    }
    return nullptr;
}

HdDataSourceLocatorSet
UsdImaging_3DGaussianSplatAdapter::InvalidateImagingSubprim(UsdPrim const& prim, TfToken const& subprim,
                                                            TfTokenVector const& properties,
                                                            const UsdImagingPropertyInvalidationType invalidationType) {
    if (subprim.IsEmpty()) {
        return UsdImagingDataSource_3DGaussianSplatPrim::Invalidate(prim, subprim, properties, invalidationType);
    }

    return HdDataSourceLocatorSet();
}

bool UsdImaging_3DGaussianSplatAdapter::IsSupported(UsdImagingIndexProxy const* index) const {
    return index->IsRprimTypeSupported(HdParticleFieldTokens->ParticleField3DGaussianSplat);
}

SdfPath UsdImaging_3DGaussianSplatAdapter::Populate(UsdPrim const& prim, UsdImagingIndexProxy* index,
                                                    UsdImagingInstancerContext const* instancerContext) {
    return _AddRprim(HdParticleFieldTokens->ParticleField3DGaussianSplat, prim, index, GetMaterialUsdPath(prim),
                     instancerContext);
}

void UsdImaging_3DGaussianSplatAdapter::TrackVariability(UsdPrim const& prim, SdfPath const& cachePath,
                                                         HdDirtyBits* timeVaryingBits,
                                                         UsdImagingInstancerContext const* instancerContext) const {
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

void UsdImaging_3DGaussianSplatAdapter::UpdateForTime(UsdPrim const& prim, SdfPath const& cachePath, UsdTimeCode time,
                                                      HdDirtyBits requestedBits,
                                                      UsdImagingInstancerContext const* instancerContext) const {
    BaseAdapter::UpdateForTime(
        prim, cachePath, time, requestedBits, instancerContext);
}

HdDirtyBits UsdImaging_3DGaussianSplatAdapter::ProcessPropertyChange(UsdPrim const& prim, SdfPath const& cachePath,
                                                                     TfToken const& propertyName) {
    // Allow base class to handle change processing.
    return BaseAdapter::ProcessPropertyChange(prim, cachePath, propertyName);
}

/*virtual*/
VtValue UsdImaging_3DGaussianSplatAdapter::Get(UsdPrim const& prim, SdfPath const& cachePath, TfToken const& key,
                                               UsdTimeCode time, VtIntArray* outIndices) const {
    TRACE_FUNCTION();
    HF_MALLOC_TAG_FUNCTION();

    return BaseAdapter::Get(prim, cachePath, key, time, outIndices);
}

PXR_NAMESPACE_CLOSE_SCOPE
