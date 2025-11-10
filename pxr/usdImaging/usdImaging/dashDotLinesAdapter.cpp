//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/usdImaging/usdImaging/dashDotLinesAdapter.h"

#include "pxr/usdImaging/usdImaging/dataSourceDashDotLines.h"
#include "pxr/usdImaging/usdImaging/delegate.h"
#include "pxr/usdImaging/usdImaging/indexProxy.h"
#include "pxr/usdImaging/usdImaging/primvarUtils.h"
#include "pxr/usdImaging/usdImaging/tokens.h"

#include "pxr/imaging/hd/dashDotLines.h"
#include "pxr/imaging/hd/perfLog.h"

#include "pxr/usd/usdGeom/dashDotLines.h"
#include "pxr/usd/usdGeom/dashDotPatternAPI.h"
#include "pxr/usd/usdGeom/primvarsAPI.h"
#include "pxr/usd/usdGeom/xformCache.h"

#include "pxr/base/tf/type.h"

PXR_NAMESPACE_OPEN_SCOPE

// XXX: These primvar names are known here so that they may be exempted from 
// the filtering procedure that would normally exclude them.  This primvar
// filtering procedure is slated for removal in favor of the one in hdSt, 
// but in the mean time we must know these names here, despite them not yet
// being part of any formal schema and thus subject to change or deletion.
TF_DEFINE_PRIVATE_TOKENS(
    _rprimPrimvarNameTokens,
    (pointSizeScale)
    (screenSpaceWidths)
    (minScreenSpaceWidths)
);

TF_REGISTRY_FUNCTION(TfType)
{
    typedef UsdImagingDashDotLinesAdapter Adapter;
    TfType t = TfType::Define<Adapter, TfType::Bases<Adapter::BaseAdapter> >();
    t.SetFactory< UsdImagingPrimAdapterFactory<Adapter> >();
}

UsdImagingDashDotLinesAdapter::~UsdImagingDashDotLinesAdapter() 
{
}

TfTokenVector
UsdImagingDashDotLinesAdapter::GetImagingSubprims(UsdPrim const& prim)
{
    return { TfToken() };
}

TfToken
UsdImagingDashDotLinesAdapter::GetImagingSubprimType(
        UsdPrim const& prim,
        TfToken const& subprim)
{
    if (subprim.IsEmpty()) {
        return HdPrimTypeTokens->dashDotLines;
    }
    return TfToken();
}

HdContainerDataSourceHandle
UsdImagingDashDotLinesAdapter::GetImagingSubprimData(
        UsdPrim const& prim,
        TfToken const& subprim,
        const UsdImagingDataSourceStageGlobals &stageGlobals)
{
    if (subprim.IsEmpty()) {
        return UsdImagingDataSourceDashDotLinesPrim::New(
            prim.GetPath(),
            prim,
            stageGlobals);
    }
    return nullptr;
}

HdDataSourceLocatorSet
UsdImagingDashDotLinesAdapter::InvalidateImagingSubprim(
        UsdPrim const& prim,
        TfToken const& subprim,
        TfTokenVector const& properties,
        const UsdImagingPropertyInvalidationType invalidationType)
{
    if (subprim.IsEmpty()) {
        return UsdImagingDataSourceDashDotLinesPrim::Invalidate(
            prim, subprim, properties, invalidationType);
    }

    return HdDataSourceLocatorSet();
}

bool
UsdImagingDashDotLinesAdapter::IsSupported(
        UsdImagingIndexProxy const* index) const
{
    return index->IsRprimTypeSupported(HdPrimTypeTokens->dashDotLines);
}

SdfPath
UsdImagingDashDotLinesAdapter::Populate(UsdPrim const& prim, 
                            UsdImagingIndexProxy* index,
                            UsdImagingInstancerContext const* instancerContext)
{
    return _AddRprim(HdPrimTypeTokens->dashDotLines,
                     prim, index, GetMaterialUsdPath(prim), instancerContext);
}

void 
UsdImagingDashDotLinesAdapter::TrackVariability(UsdPrim const& prim,
                                               SdfPath const& cachePath,
                                               HdDirtyBits* timeVaryingBits,
                                               UsdImagingInstancerContext const*
                                                   instancerContext) const
{
    BaseAdapter::TrackVariability(
        prim, cachePath, timeVaryingBits, instancerContext);

    // Discover time-varying points.
    _IsVarying(prim,
               UsdGeomTokens->points,
               HdChangeTracker::DirtyPoints,
               UsdImagingTokens->usdVaryingPrimvar,
               timeVaryingBits,
               /*isInherited*/false);

    // Discover time-varying topology.
    _IsVarying(prim, UsdGeomTokens->curveVertexCounts,
                       HdChangeTracker::DirtyTopology,
                       UsdImagingTokens->usdVaryingTopology,
                       timeVaryingBits,
                       /*isInherited*/false);

    // Check for time-varying primvars:widths, and if that attribute
    // doesn't exist also check for time-varying widths.
    bool widthsExists = false;
    _IsVarying(prim,
               UsdImagingTokens->primvarsWidths,
               HdChangeTracker::DirtyWidths,
               UsdImagingTokens->usdVaryingWidths,
               timeVaryingBits,
               /*isInherited*/false,
               &widthsExists);
    if (!widthsExists) {
        UsdGeomPrimvar pv = _GetInheritedPrimvar(prim, HdTokens->widths);
        if (pv && pv.ValueMightBeTimeVarying()) {
            *timeVaryingBits |= HdChangeTracker::DirtyWidths;
            HD_PERF_COUNTER_INCR(UsdImagingTokens->usdVaryingWidths);
            widthsExists = true;
        }
    }
    if (!widthsExists) {
        _IsVarying(prim, UsdGeomTokens->widths,
                HdChangeTracker::DirtyWidths,
                UsdImagingTokens->usdVaryingWidths,
                timeVaryingBits,
                /*isInherited*/false);
    }
}

bool
UsdImagingDashDotLinesAdapter::_IsBuiltinPrimvar(TfToken const& primvarName) const
{
    return (primvarName == HdTokens->widths) ||
            primvarName == HdTokens->pattern ||
            primvarName == HdTokens->patternPeriod ||
            primvarName == HdTokens->patternScale ||
            primvarName == HdTokens->startCapType ||
            primvarName == HdTokens->endCapType ||
        UsdImagingGprimAdapter::_IsBuiltinPrimvar(primvarName);
}

void 
UsdImagingDashDotLinesAdapter::UpdateForTime(
    UsdPrim const& prim,
    SdfPath const& cachePath, 
    UsdTimeCode time,
    HdDirtyBits requestedBits,
    UsdImagingInstancerContext const* instancerContext) const
{
    BaseAdapter::UpdateForTime(
        prim, cachePath, time, requestedBits, instancerContext);

    UsdImagingPrimvarDescCache* primvarDescCache = _GetPrimvarDescCache();
    HdPrimvarDescriptorVector& primvars = 
        primvarDescCache->GetPrimvars(cachePath);

    if (requestedBits & HdChangeTracker::DirtyWidths) {
        // First check for "primvars:widths"
        UsdGeomPrimvarsAPI primvarsApi(prim);
        UsdGeomPrimvar pv = primvarsApi.GetPrimvar(
            UsdImagingTokens->primvarsWidths);
        if (!pv) {
            // If it's not found locally, see if it's inherited
            pv = _GetInheritedPrimvar(prim, HdTokens->widths);
        }

        if (pv) {
            _ComputeAndMergePrimvar(prim, pv, time, &primvars);
        } else {
            UsdGeomDashDotLines curves(prim);
            HdInterpolation interpolation;
            VtFloatArray widths;
            if (curves.GetWidthsAttr().Get(&widths, time)) {
                interpolation = UsdImagingUsdToHdInterpolation(
                    curves.GetWidthsInterpolation());
            } else {
                interpolation = HdInterpolationConstant;
            }
            _MergePrimvar(&primvars, UsdGeomTokens->widths, interpolation);
        }
    }

    if (requestedBits & HdChangeTracker::DirtyPrimvar) {
        if (_Get<TfToken>(prim, UsdGeomTokens->shapeDetail, UsdTimeCode::Default())
            != UsdGeomTokens->noCapJoint)
        {
            _MergePrimvar(&primvars, HdTokens->startCapType, HdInterpolationConstant);
            _MergePrimvar(&primvars, HdTokens->endCapType, HdInterpolationConstant);
        }
        _MergePrimvar(&primvars, HdTokens->pattern, HdInterpolationConstant);
        _MergePrimvar(&primvars, HdTokens->patternPeriod, HdInterpolationConstant);
        _MergePrimvar(&primvars, HdTokens->patternScale, HdInterpolationConstant);
    }
}

HdDirtyBits
UsdImagingDashDotLinesAdapter::ProcessPropertyChange(UsdPrim const& prim,
                                             SdfPath const& cachePath,
                                             TfToken const& propertyName)
{
    // Even though points is treated as a primvar, it is special and is always
    // treated as a vertex primvar.
    if (propertyName == UsdGeomTokens->points) {
        return HdChangeTracker::DirtyPoints;
    
    } else if (propertyName == UsdGeomTokens->curveVertexCounts ||
             propertyName == UsdGeomTokens->screenSpacePattern) {
        return HdChangeTracker::DirtyTopology;

    // Handle attributes that are treated as "built-in" primvars.
    } else if (propertyName == UsdGeomTokens->widths) {
        UsdGeomCurves curves(prim);
        return UsdImagingPrimAdapter::_ProcessNonPrefixedPrimvarPropertyChange(
            prim, cachePath, propertyName, HdTokens->widths,
            UsdImagingUsdToHdInterpolation(curves.GetWidthsInterpolation()),
            HdChangeTracker::DirtyWidths);

    // Handle prefixed primvars that use special dirty bits.
    } else if (propertyName == UsdImagingTokens->primvarsWidths) {
        return UsdImagingPrimAdapter::_ProcessPrefixedPrimvarPropertyChange(
            prim, cachePath, propertyName, HdChangeTracker::DirtyWidths);
    
    }

    // Allow base class to handle change processing.
    return BaseAdapter::ProcessPropertyChange(prim, cachePath, propertyName);
}

/*virtual*/
VtValue
UsdImagingDashDotLinesAdapter::GetTopology(UsdPrim const& prim, 
                                          SdfPath const& cachePath,
                                          UsdTimeCode time) const
{
    TRACE_FUNCTION();
    HF_MALLOC_TAG_FUNCTION();

    // These are uniform attributes and can't vary over time.
    UsdTimeCode unvarying = UsdTimeCode::Default();

    // Get the implementation type.
    TfToken shapeDetail = _Get<TfToken>(prim, UsdGeomTokens->shapeDetail, unvarying);

    // Get if the pattern is screen spaced.
    bool isScreenSpacePattern = _Get<bool>(prim, UsdGeomTokens->screenSpacePattern, unvarying);

    HdDashDotLinesTopology topology(
        shapeDetail, isScreenSpacePattern,
        _Get<VtIntArray>(prim, UsdGeomTokens->curveVertexCounts, time),
        VtIntArray());
    return VtValue(topology);
}

/*virtual*/
VtValue
UsdImagingDashDotLinesAdapter::Get(UsdPrim const& prim,
                                  SdfPath const& cachePath,
                                  TfToken const& key,
                                  UsdTimeCode time,
                                  VtIntArray *outIndices) const
{
    TRACE_FUNCTION();
    HF_MALLOC_TAG_FUNCTION();

    if (key == HdTokens->widths) {
        // First check for "primvars:widths"
        UsdGeomPrimvarsAPI primvarsApi(prim);
        UsdGeomPrimvar pv = primvarsApi.GetPrimvar(
            UsdImagingTokens->primvarsWidths);
        if (!pv) {
            // If it's not found locally, see if it's inherited
            pv = _GetInheritedPrimvar(prim, HdTokens->widths);
        }

        VtValue value;

        if (outIndices) {
            if (pv && pv.Get(&value, time)) {
                pv.GetIndices(outIndices, time);
                return value;
            }
        } else if (pv && pv.ComputeFlattened(&value, time)) {
            return value;
        }
        
        // Try to get widths directly from the curves
        UsdGeomDashDotLines curves(prim);
        VtFloatArray widths;
        if (curves && curves.GetWidthsAttr().Get(&widths, time)) {
            value = widths;
            return value;
        }
    }
    else if (key == HdTokens->pattern) {
        UsdGeomDashDotLines curves(prim);
        VtVec2fArray pattern;
        if (curves) {
            curves.GetPrim().GetAttribute(HdTokens->pattern).Get(&pattern, time);
        }
        return VtValue(pattern);
    }
    else if (key == HdTokens->patternPeriod) {
        UsdGeomDashDotLines curves(prim);
        float period = 1.0f;
        if (curves) {
            curves.GetPrim().GetAttribute(HdTokens->patternPeriod).Get(&period, time);;
        }
        return VtValue(period);
    }
    else if (key == HdTokens->patternScale) {
        UsdGeomDashDotLines curves(prim);
        float scale = 1.0f;
        if (curves) {
            curves.GetPatternScaleAttr().Get(&scale, time);
        }
        return VtValue(scale);
    }
    else if (key == HdTokens->startCapType) {
        UsdGeomDashDotLines curves(prim);
        TfToken startCapType = UsdGeomTokens->round;
        if (curves) {
            curves.GetStartCapTypeAttr().Get(&startCapType, time);
        }
        return VtValue(startCapType);
    }
    else if (key == HdTokens->endCapType) {
        UsdGeomDashDotLines curves(prim);
        TfToken endCapType = UsdGeomTokens->round;
        if (curves) {
            curves.GetEndCapTypeAttr().Get(&endCapType, time);
        }
        return VtValue(endCapType);
    }
    return BaseAdapter::Get(prim, cachePath, key, time, outIndices);
}

PXR_NAMESPACE_CLOSE_SCOPE
