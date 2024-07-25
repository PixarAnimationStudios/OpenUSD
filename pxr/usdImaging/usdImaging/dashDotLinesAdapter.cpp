//
// Copyright 2016 Pixar
//
// Licensed under the Apache License, Version 2.0 (the "Apache License")
// with the following modification; you may not use this file except in
// compliance with the Apache License and the following modification to it:
// Section 6. Trademarks. is deleted and replaced with:
//
// 6. Trademarks. This License does not grant permission to use the trade
//    names, trademarks, service marks, or product names of the Licensor
//    and its affiliates, except as required to comply with Section 4(c) of
//    the License and to reproduce the content of the NOTICE file.
//
// You may obtain a copy of the Apache License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the Apache License with the above modification is
// distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
// KIND, either express or implied. See the Apache License for the specific
// language governing permissions and limitations under the Apache License.
//
#include "pxr/usdImaging/usdImaging/dashDotLinesAdapter.h"

#include "pxr/usdImaging/usdImaging/dataSourceBasisCurves.h"
#include "pxr/usdImaging/usdImaging/delegate.h"
#include "pxr/usdImaging/usdImaging/indexProxy.h"
#include "pxr/usdImaging/usdImaging/primvarUtils.h"
#include "pxr/usdImaging/usdImaging/tokens.h"

#include "pxr/imaging/hd/basisCurves.h"
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
    // The DashDotLines primitive uses the basisCurves rprim.
    if (subprim.IsEmpty()) {
        return HdPrimTypeTokens->basisCurves;
    }
    return TfToken();
}

HdContainerDataSourceHandle
UsdImagingDashDotLinesAdapter::GetImagingSubprimData(
        UsdPrim const& prim,
        TfToken const& subprim,
        const UsdImagingDataSourceStageGlobals &stageGlobals)
{
    // The DashDotLines primitive uses the basisCurves rprim.
    if (subprim.IsEmpty()) {
        return UsdImagingDataSourceBasisCurvesPrim::New(
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
    // The DashDotLines primitive uses the basisCurves rprim.
    if (subprim.IsEmpty()) {
        return UsdImagingDataSourceBasisCurvesPrim::Invalidate(
            prim, subprim, properties, invalidationType);
    }

    return HdDataSourceLocatorSet();
}

bool
UsdImagingDashDotLinesAdapter::IsSupported(
        UsdImagingIndexProxy const* index) const
{
    // The DashDotLines primitive uses the basisCurves rprim.
    return index->IsRprimTypeSupported(HdPrimTypeTokens->basisCurves);
}

SdfPath
UsdImagingDashDotLinesAdapter::Populate(UsdPrim const& prim, 
                            UsdImagingIndexProxy* index,
                            UsdImagingInstancerContext const* instancerContext)
{
    // The DashDotLines primitive uses the basisCurves rprim.
    return _AddRprim(HdPrimTypeTokens->basisCurves,
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
    //
    // Note that basis, wrap and type are all uniform attributes, so they can't
    // vary over time.
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
            primvarName == HdTokens->patternPartCount ||
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
        _MergePrimvar(&primvars, HdTokens->pattern, HdInterpolationConstant);
        _MergePrimvar(&primvars, HdTokens->patternPartCount, HdInterpolationConstant);
        _MergePrimvar(&primvars, HdTokens->patternPeriod, HdInterpolationConstant);
        _MergePrimvar(&primvars, HdTokens->patternScale, HdInterpolationConstant);
        _MergePrimvar(&primvars, HdTokens->startCapType, HdInterpolationConstant);
        _MergePrimvar(&primvars, HdTokens->endCapType, HdInterpolationConstant);
        _MergePrimvar(&primvars, HdTokens->adjPoints1, HdInterpolationVertex);
        _MergePrimvar(&primvars, HdTokens->adjPoints2, HdInterpolationVertex);
        _MergePrimvar(&primvars, HdTokens->adjPoints3, HdInterpolationVertex);
        _MergePrimvar(&primvars, HdTokens->accumulatedLength, HdInterpolationVertex);
        _MergePrimvar(&primvars, HdTokens->extrude, HdInterpolationVertex);
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
    TfToken topoCurveStyle;
    // Get if the pattern is screen spaced.
    bool isScreenSpacePattern = _Get<bool>(prim, UsdGeomTokens->screenSpacePattern, unvarying);

    if (isScreenSpacePattern) {
        topoCurveStyle = HdTokens->screenSpaceDashDot;
    }
    else {
        topoCurveStyle = HdTokens->dashDot;
    }

    // We use the basisCurves rprim, so here we need to create the basisCurves topology.
    HdBasisCurvesTopology topology(
        HdTokens->linear, HdTokens->bezier, HdTokens->nonperiodic, topoCurveStyle,
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
    else if (key == HdTokens->patternPartCount) {
        UsdGeomDashDotLines curves(prim);
        VtVec2fArray pattern;
        if (curves) {
            curves.GetPrim().GetAttribute(HdTokens->pattern).Get(&pattern, time);
        }
        int count = 0;
        count = pattern.size();
        return VtValue(count);
    }
    else if (key == HdTokens->pattern) {
        UsdGeomDashDotLines curves(prim);
        VtVec2fArray pattern;
        if (curves) {
            curves.GetPrim().GetAttribute(HdTokens->pattern).Get(&pattern, time);
        }
        if (pattern.size() == 0)
        {
            pattern.emplace_back(0.0, 0.0);
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
        TfToken startCapType = HdTokens->round;
        if (curves) {
            curves.GetStartCapTypeAttr().Get(&startCapType, time);
        }
        if(startCapType == HdTokens->square)
            return VtValue(1);
        else if(startCapType == HdTokens->triangle)
            return VtValue(2);
        else
            return VtValue(0);
    }
    else if (key == HdTokens->endCapType) {
        UsdGeomDashDotLines curves(prim);
        TfToken endCapType = HdTokens->round;
        if (curves) {
            curves.GetEndCapTypeAttr().Get(&endCapType, time);
        }
        if (endCapType == HdTokens->square)
            return VtValue(1);
        else if (endCapType == HdTokens->triangle)
            return VtValue(2);
        else
            return VtValue(0);
    }
    return BaseAdapter::Get(prim, cachePath, key, time, outIndices);
}

PXR_NAMESPACE_CLOSE_SCOPE
