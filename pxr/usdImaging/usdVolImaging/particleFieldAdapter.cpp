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

#include "pxr/usd/usdVol/particleField3DGaussianSplat.h"
#include "pxr/usd/usdVol/tokens.h"

#include "pxr/base/tf/type.h"

PXR_NAMESPACE_OPEN_SCOPE

TF_REGISTRY_FUNCTION(TfType)
{
    using Adapter = UsdImagingParticleFieldAdapter;
    TfType t = TfType::Define<Adapter, TfType::Bases<Adapter::BaseAdapter> >();
    t.SetFactory<UsdImagingPrimAdapterFactory<Adapter> >();
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
        return HdPrimTypeTokens->particleField;
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
    return index->IsRprimTypeSupported(HdPrimTypeTokens->particleField);
}

SdfPath
UsdImagingParticleFieldAdapter::Populate(
    UsdPrim const& prim, UsdImagingIndexProxy* index,
    UsdImagingInstancerContext const* instancerContext)
{
    return _AddRprim(HdPrimTypeTokens->particleField, prim, index,
        GetMaterialUsdPath(prim), instancerContext);
}

void UsdImagingParticleFieldAdapter::TrackVariability(
    UsdPrim const& prim, SdfPath const& cachePath,
    HdDirtyBits* timeVaryingBits,
    UsdImagingInstancerContext const* instancerContext) const
{
    _IsVarying(prim, UsdVolTokens->positionsh,
        HdChangeTracker::DirtyPrimvar,
        UsdImagingTokens->usdVaryingPrimvar,
        timeVaryingBits, false);
    _IsVarying(prim, UsdVolTokens->positions,
        HdChangeTracker::DirtyPrimvar,
        UsdImagingTokens->usdVaryingPrimvar,
        timeVaryingBits, false);

    _IsVarying(prim, UsdVolTokens->orientationsh,
        HdChangeTracker::DirtyPrimvar,
        UsdImagingTokens->usdVaryingPrimvar,
        timeVaryingBits, false);
    _IsVarying(prim, UsdVolTokens->orientations,
        HdChangeTracker::DirtyPrimvar,
        UsdImagingTokens->usdVaryingPrimvar,
        timeVaryingBits, false);

    _IsVarying(prim, UsdVolTokens->scalesh,
        HdChangeTracker::DirtyPrimvar,
        UsdImagingTokens->usdVaryingPrimvar,
        timeVaryingBits, false);
    _IsVarying(prim, UsdVolTokens->scales,
        HdChangeTracker::DirtyPrimvar,
        UsdImagingTokens->usdVaryingPrimvar,
        timeVaryingBits, false);

    _IsVarying(prim, UsdVolTokens->opacitiesh,
        HdChangeTracker::DirtyPrimvar,
        UsdImagingTokens->usdVaryingPrimvar,
        timeVaryingBits, false);
    _IsVarying(prim, UsdVolTokens->opacities,
        HdChangeTracker::DirtyPrimvar,
        UsdImagingTokens->usdVaryingPrimvar,
        timeVaryingBits, false);

    _IsVarying(prim, UsdVolTokens->radianceSphericalHarmonicsCoefficientsh,
        HdChangeTracker::DirtyPrimvar,
        UsdImagingTokens->usdVaryingPrimvar,
        timeVaryingBits, false);
    _IsVarying(prim, UsdVolTokens->radianceSphericalHarmonicsCoefficients,
        HdChangeTracker::DirtyPrimvar,
        UsdImagingTokens->usdVaryingPrimvar,
        timeVaryingBits, false);

    _IsVarying(prim, UsdVolTokens->radianceSphericalHarmonicsDegree,
        HdChangeTracker::DirtyPrimvar,
        UsdImagingTokens->usdVaryingPrimvar,
        timeVaryingBits, false);

    BaseAdapter::TrackVariability(
        prim, cachePath, timeVaryingBits, instancerContext);
}

void UsdImagingParticleFieldAdapter::UpdateForTime(
    UsdPrim const& prim, SdfPath const& cachePath, UsdTimeCode time,
    HdDirtyBits requestedBits,
    UsdImagingInstancerContext const* instancerContext) const
{
    UsdImagingPrimvarDescCache* primvarDescCache = _GetPrimvarDescCache();

    if (requestedBits & HdChangeTracker::DirtyPrimvar) {
        // XXX: in the future, if we decompose this into API schema accesses,
        // we can reuse this code for other types of particle fields...
        UsdVolParticleField3DGaussianSplat gs(prim);
        HdPrimvarDescriptorVector& vPrimvars =
            primvarDescCache->GetPrimvars(cachePath);

        UsdAttribute attr;

        if (gs.UsesFloatPositions(&attr)) {
            VtVec3fArray val;
            if (attr && attr.Get(&val, time)) {
                _MergePrimvar(&vPrimvars, UsdVolTokens->positions,
                    HdInterpolationVertex, HdPrimvarRoleTokens->vector);
            }
        } else {
            VtVec3hArray val;
            if (attr && attr.Get(&val, time)) {
                _MergePrimvar(&vPrimvars, UsdVolTokens->positions,
                    HdInterpolationVertex, HdPrimvarRoleTokens->vector);
            }
        }

        if (gs.UsesFloatOrientations(&attr)) {
            VtQuatfArray val;
            if (attr && attr.Get(&val, time)) {
                _MergePrimvar(&vPrimvars, UsdVolTokens->orientations,
                    HdInterpolationVertex);
            }
        } else {
            VtQuathArray val;
            if (attr && attr.Get(&val, time)) {
                _MergePrimvar(&vPrimvars, UsdVolTokens->orientations,
                    HdInterpolationVertex);
            }
        }

        if (gs.UsesFloatScales(&attr)) {
            VtVec3fArray val;
            if (attr && attr.Get(&val, time)) {
                _MergePrimvar(&vPrimvars, UsdVolTokens->scales,
                    HdInterpolationVertex);
            }
        } else {
            VtVec3hArray val;
            if (attr && attr.Get(&val, time)) {
                _MergePrimvar(&vPrimvars, UsdVolTokens->scales,
                    HdInterpolationVertex);
            }
        }

        if (gs.UsesFloatOpacities(&attr)) {
            VtFloatArray val;
            if (attr && attr.Get(&val, time)) {
                _MergePrimvar(&vPrimvars, UsdVolTokens->opacities,
                    HdInterpolationVertex);
            }
        } else {
            VtHalfArray val;
            if (attr && attr.Get(&val, time)) {
                _MergePrimvar(&vPrimvars, UsdVolTokens->opacities,
                    HdInterpolationVertex);
            }
        }

        // Note that the element size of the coefficients is a function of the
        // SH degree, specifically: pow(degree+1, 2).
        if (gs.UsesFloatRadianceCoefficients(&attr)) {
            VtVec3fArray val;
            if (attr && attr.Get(&val, time)) {
                _MergePrimvar(&vPrimvars,
                    UsdVolTokens->radianceSphericalHarmonicsCoefficients,
                    HdInterpolationVertex);
            }
        } else {
            VtVec3hArray val;
            if (attr && attr.Get(&val, time)) {
                _MergePrimvar(&vPrimvars,
                    UsdVolTokens->radianceSphericalHarmonicsCoefficients,
                    HdInterpolationVertex);
            }
        }

        int val;
        if (gs.GetRadianceSphericalHarmonicsDegreeAttr().Get(&val, time)) {
            _MergePrimvar(&vPrimvars,
                UsdVolTokens->radianceSphericalHarmonicsDegree,
                HdInterpolationConstant);
        }
    }
    BaseAdapter::UpdateForTime(
        prim, cachePath, time, requestedBits, instancerContext);
}

HdDirtyBits
UsdImagingParticleFieldAdapter::ProcessPropertyChange(
    UsdPrim const& prim, SdfPath const& cachePath,
    TfToken const& propertyName)
{
    if (propertyName == UsdVolTokens->positions ||
        propertyName == UsdVolTokens->positionsh ||
        propertyName == UsdVolTokens->orientations ||
        propertyName == UsdVolTokens->orientationsh ||
        propertyName == UsdVolTokens->scales ||
        propertyName == UsdVolTokens->scalesh ||
        propertyName == UsdVolTokens->opacities ||
        propertyName == UsdVolTokens->opacitiesh ||
        propertyName == UsdVolTokens->radianceSphericalHarmonicsDegree ||
        propertyName == UsdVolTokens->radianceSphericalHarmonicsCoefficients ||
        propertyName == UsdVolTokens->radianceSphericalHarmonicsCoefficientsh) {
        return HdChangeTracker::DirtyPrimvar;
    }

    return BaseAdapter::ProcessPropertyChange(prim, cachePath, propertyName);
}

VtValue
UsdImagingParticleFieldAdapter::Get(
    UsdPrim const& prim, SdfPath const& cachePath, TfToken const& key,
    UsdTimeCode time, VtIntArray* outIndices) const {

    TfToken forwardedKey;
    // XXX: in the future, if we decompose this into API schema accesses,
    // we can reuse this code for other types of particle fields...
    UsdVolParticleField3DGaussianSplat gs(prim);

    // For attributes which have both a float/half implementation, the hydra
    // name "foo" will pull from the USD name "foo"/"fooh" as applicable.
    if (key == UsdVolTokens->positions) {
        gs.UsesFloatPositions(&forwardedKey);
    } else if (key == UsdVolTokens->orientations) {
        gs.UsesFloatOrientations(&forwardedKey);
    } else if (key == UsdVolTokens->scales) {
        gs.UsesFloatScales(&forwardedKey);
    } else if (key == UsdVolTokens->opacities) {
        gs.UsesFloatOpacities(&forwardedKey);
    } else if (key == UsdVolTokens->radianceSphericalHarmonicsCoefficients) {
        gs.UsesFloatRadianceCoefficients(&forwardedKey);
    } else {
        forwardedKey = key;
    }

    return BaseAdapter::Get(prim, cachePath, forwardedKey, time, outIndices);
}

PXR_NAMESPACE_CLOSE_SCOPE
