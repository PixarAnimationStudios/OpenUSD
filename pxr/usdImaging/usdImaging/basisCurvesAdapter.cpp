//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/usdImaging/usdImaging/basisCurvesAdapter.h"

#include "pxr/usdImaging/usdImaging/dataSourceBasisCurves.h"
#include "pxr/usdImaging/usdImaging/delegate.h"
#include "pxr/usdImaging/usdImaging/indexProxy.h"
#include "pxr/usdImaging/usdImaging/primvarUtils.h"
#include "pxr/usdImaging/usdImaging/tokens.h"

#include "pxr/imaging/hd/basisCurves.h"
#include "pxr/imaging/hd/perfLog.h"

#include "pxr/usd/usdGeom/basisCurves.h"
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
    typedef UsdImagingBasisCurvesAdapter Adapter;
    TfType t = TfType::Define<Adapter, TfType::Bases<Adapter::BaseAdapter> >();
    t.SetFactory< UsdImagingPrimAdapterFactory<Adapter> >();
}

UsdImagingBasisCurvesAdapter::~UsdImagingBasisCurvesAdapter() 
{
}

TfTokenVector
UsdImagingBasisCurvesAdapter::GetImagingSubprims(UsdPrim const& prim)
{
    return { TfToken() };
}

TfToken
UsdImagingBasisCurvesAdapter::GetImagingSubprimType(
        UsdPrim const& prim,
        TfToken const& subprim)
{
    if (subprim.IsEmpty()) {
        return HdPrimTypeTokens->basisCurves;
    }
    return TfToken();
}

HdContainerDataSourceHandle
UsdImagingBasisCurvesAdapter::GetImagingSubprimData(
        UsdPrim const& prim,
        TfToken const& subprim,
        const UsdImagingDataSourceStageGlobals &stageGlobals)
{
    if (subprim.IsEmpty()) {
        return UsdImagingDataSourceBasisCurvesPrim::New(
            prim.GetPath(),
            prim,
            stageGlobals);
    }
    return nullptr;
}

HdDataSourceLocatorSet
UsdImagingBasisCurvesAdapter::InvalidateImagingSubprim(
        UsdPrim const& prim,
        TfToken const& subprim,
        TfTokenVector const& properties,
        const UsdImagingPropertyInvalidationType invalidationType)
{
    if (subprim.IsEmpty()) {
        return UsdImagingDataSourceBasisCurvesPrim::Invalidate(
            prim, subprim, properties, invalidationType);
    }

    return HdDataSourceLocatorSet();
}

bool
UsdImagingBasisCurvesAdapter::IsSupported(
        UsdImagingIndexProxy const* index) const
{
    return index->IsRprimTypeSupported(HdPrimTypeTokens->basisCurves);
}

SdfPath
UsdImagingBasisCurvesAdapter::Populate(UsdPrim const& prim, 
                            UsdImagingIndexProxy* index,
                            UsdImagingInstancerContext const* instancerContext)
{
    return _AddRprim(HdPrimTypeTokens->basisCurves,
                     prim, index, GetMaterialUsdPath(prim), instancerContext);
}

void 
UsdImagingBasisCurvesAdapter::TrackVariability(UsdPrim const& prim,
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

    // Check for time-varying primvars:normals, and if that attribute
    // doesn't exist also check for time-varying normals.
    bool normalsExists = false;
    _IsVarying(prim,
               UsdImagingTokens->primvarsNormals,
               HdChangeTracker::DirtyNormals,
               UsdImagingTokens->usdVaryingNormals,
               timeVaryingBits,
               /*isInherited*/false,
               &normalsExists);
    if (!normalsExists) {
        UsdGeomPrimvar pv = _GetInheritedPrimvar(prim, HdTokens->normals);
        if (pv && pv.ValueMightBeTimeVarying()) {
            *timeVaryingBits |= HdChangeTracker::DirtyNormals;
            HD_PERF_COUNTER_INCR(UsdImagingTokens->usdVaryingNormals);
            normalsExists = true;
        }
    }
    if (!normalsExists) {
        _IsVarying(prim, UsdGeomTokens->normals,
                HdChangeTracker::DirtyNormals,
                UsdImagingTokens->usdVaryingNormals,
                timeVaryingBits,
                /*isInherited*/false);
    }
}

bool
UsdImagingBasisCurvesAdapter::_IsBuiltinPrimvar(TfToken const& primvarName) const
{
    return (primvarName == HdTokens->normals ||
            primvarName == HdTokens->widths) ||
        UsdImagingGprimAdapter::_IsBuiltinPrimvar(primvarName);
}

void 
UsdImagingBasisCurvesAdapter::UpdateForTime(
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
            UsdGeomBasisCurves curves(prim);
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

    if (requestedBits & HdChangeTracker::DirtyNormals) {
        // First check for "primvars:normals"
        UsdGeomPrimvarsAPI primvarsApi(prim);
        UsdGeomPrimvar pv = primvarsApi.GetPrimvar(
            UsdImagingTokens->primvarsNormals);
        if (!pv) {
            // If it's not found locally, see if it's inherited
            pv = _GetInheritedPrimvar(prim, HdTokens->normals);
        }

        if (pv) {
            _ComputeAndMergePrimvar(prim, pv, time, &primvars);
        } else {
            UsdGeomBasisCurves curves(prim);
            VtVec3fArray normals;
            if (curves.GetNormalsAttr().Get(&normals, time)) {
                _MergePrimvar(&primvars,
                    UsdGeomTokens->normals,
                    UsdImagingUsdToHdInterpolation(curves.GetNormalsInterpolation()),
                    HdPrimvarRoleTokens->normal);
            } else {
                _RemovePrimvar(&primvars, UsdGeomTokens->normals);
            }
        }
    }
}

HdDirtyBits
UsdImagingBasisCurvesAdapter::ProcessPropertyChange(UsdPrim const& prim,
                                             SdfPath const& cachePath,
                                             TfToken const& propertyName)
{
    // Even though points is treated as a primvar, it is special and is always
    // treated as a vertex primvar.
    if (propertyName == UsdGeomTokens->points) {
        return HdChangeTracker::DirtyPoints;
    
    } else if (propertyName == UsdGeomTokens->curveVertexCounts ||
             propertyName == UsdGeomTokens->basis ||
             propertyName == UsdGeomTokens->type ||
             propertyName == UsdGeomTokens->wrap) {
        return HdChangeTracker::DirtyTopology;

    // Handle attributes that are treated as "built-in" primvars.
    } else if (propertyName == UsdGeomTokens->widths) {
        UsdGeomCurves curves(prim);
        return UsdImagingPrimAdapter::_ProcessNonPrefixedPrimvarPropertyChange(
            prim, cachePath, propertyName, HdTokens->widths,
            UsdImagingUsdToHdInterpolation(curves.GetWidthsInterpolation()),
            HdChangeTracker::DirtyWidths);
    
    } else if (propertyName == UsdGeomTokens->normals) {
        UsdGeomPointBased pb(prim);
        return UsdImagingPrimAdapter::_ProcessNonPrefixedPrimvarPropertyChange(
            prim, cachePath, propertyName, HdTokens->normals,
            UsdImagingUsdToHdInterpolation(pb.GetNormalsInterpolation()),
            HdChangeTracker::DirtyNormals);
    }
    // Handle prefixed primvars that use special dirty bits.
    else if (propertyName == UsdImagingTokens->primvarsWidths) {
        return UsdImagingPrimAdapter::_ProcessPrefixedPrimvarPropertyChange(
            prim, cachePath, propertyName, HdChangeTracker::DirtyWidths);
    
    } else if (propertyName == UsdImagingTokens->primvarsNormals) {
        return UsdImagingPrimAdapter::_ProcessPrefixedPrimvarPropertyChange(
                prim, cachePath, propertyName, HdChangeTracker::DirtyNormals);
    }

    // Allow base class to handle change processing.
    return BaseAdapter::ProcessPropertyChange(prim, cachePath, propertyName);
}

/*virtual*/
VtValue
UsdImagingBasisCurvesAdapter::GetTopology(UsdPrim const& prim, 
                                          SdfPath const& cachePath,
                                          UsdTimeCode time) const
{
    TRACE_FUNCTION();
    HF_MALLOC_TAG_FUNCTION();

    // These are uniform attributes and can't vary over time.
    UsdTimeCode unvarying = UsdTimeCode::Default();
    TfToken topoCurveBasis, topoCurveType, topoCurveWrap;
    TfToken curveBasis = _Get<TfToken>(prim, UsdGeomTokens->basis, unvarying);
    TfToken curveType = _Get<TfToken>(prim, UsdGeomTokens->type, unvarying);
    TfToken curveWrap = _Get<TfToken>(prim, UsdGeomTokens->wrap, unvarying);

    if(curveBasis == UsdGeomTokens->bezier) {
        topoCurveBasis = HdTokens->bezier;
    }
    else if(curveBasis == UsdGeomTokens->bspline) {
        topoCurveBasis = HdTokens->bspline;
    }
    else if(curveBasis == UsdGeomTokens->catmullRom) {
        topoCurveBasis = HdTokens->catmullRom;
    }
    else {
        topoCurveBasis = HdTokens->bezier;
        if (!curveBasis.IsEmpty()) {
            TF_WARN("Unknown curve basis '%s', using '%s'", 
                curveBasis.GetText(), topoCurveBasis.GetText());
        }
    }

    if(curveType == UsdGeomTokens->linear) {
        topoCurveType = HdTokens->linear;
    }
    else if(curveType == UsdGeomTokens->cubic) {
        topoCurveType = HdTokens->cubic;
    }
    else {
        topoCurveType = HdTokens->cubic;
        if (!curveType.IsEmpty()) {
            TF_WARN("Unknown curve type '%s', using '%s'", 
                curveType.GetText(), topoCurveType.GetText());
        }
    }

    if(curveWrap == UsdGeomTokens->periodic) {
        topoCurveWrap = HdTokens->periodic;
    }
    else if(curveWrap == UsdGeomTokens->nonperiodic) {
        topoCurveWrap = HdTokens->nonperiodic;
    }
    else if(curveWrap == UsdGeomTokens->pinned) {
        topoCurveWrap = HdTokens->pinned;
    }
    else {
        topoCurveWrap = HdTokens->nonperiodic;
        if (!curveWrap.IsEmpty()) {
            TF_WARN("Unknown curve wrap '%s', using '%s'", 
                curveWrap.GetText(), topoCurveWrap.GetText());
        }
    }

    HdBasisCurvesTopology topology(
        topoCurveType, topoCurveBasis, topoCurveWrap,
        _Get<VtIntArray>(prim, UsdGeomTokens->curveVertexCounts, time),
        VtIntArray());
    return VtValue(topology);
}

/*virtual*/
VtValue
UsdImagingBasisCurvesAdapter::Get(UsdPrim const& prim,
                                  SdfPath const& cachePath,
                                  TfToken const& key,
                                  UsdTimeCode time,
                                  VtIntArray *outIndices) const
{
    TRACE_FUNCTION();
    HF_MALLOC_TAG_FUNCTION();

    if (key == HdTokens->normals) {
        // First check for "primvars:normals"
        UsdGeomPrimvarsAPI primvarsApi(prim);
        UsdGeomPrimvar pv = primvarsApi.GetPrimvar(
            UsdImagingTokens->primvarsNormals);
        if (!pv) {
            // If it's not found locally, see if it's inherited
            pv = _GetInheritedPrimvar(prim, HdTokens->normals);
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

        // If there's no "primvars:normals",
        // fall back to UsdGeomBasisCurves' "normals" attribute. 
        UsdGeomBasisCurves curves(prim);
        VtVec3fArray normals;
        if (curves && curves.GetNormalsAttr().Get(&normals, time)) {
            value = normals;
            return value;
        }

    } else if (key == HdTokens->widths) {
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
        UsdGeomBasisCurves curves(prim);
        VtFloatArray widths;
        if (curves && curves.GetWidthsAttr().Get(&widths, time)) {
            value = widths;
            return value;
        }
    }

    return BaseAdapter::Get(prim, cachePath, key, time, outIndices);
}

/*override*/
TfTokenVector const&
UsdImagingBasisCurvesAdapter::_GetRprimPrimvarNames() const
{
    // This result should match the GetBuiltinPrimvarNames result from
    // HdStBasisCurves, which we're not allowed to call here. Points, normals
    // and widths are already handled explicitly in GprimAdapter, so there's no
    // need to except them from filtering by claiming them here.
    //
    // See comment on _rprimPrimvarNameTokens warning regarding using these 
    // primvars.
    static TfTokenVector primvarNames{
        _rprimPrimvarNameTokens->pointSizeScale,
        _rprimPrimvarNameTokens->screenSpaceWidths,
        _rprimPrimvarNameTokens->minScreenSpaceWidths
    };
    return primvarNames;
}

PXR_NAMESPACE_CLOSE_SCOPE
