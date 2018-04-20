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
#include "pxr/usdImaging/usdImaging/gprimAdapter.h"

#include "pxr/usdImaging/usdImaging/debugCodes.h"
#include "pxr/usdImaging/usdImaging/delegate.h"
#include "pxr/usdImaging/usdImaging/indexProxy.h"
#include "pxr/usdImaging/usdImaging/instancerContext.h"
#include "pxr/usdImaging/usdImaging/tokens.h"

#include "pxr/imaging/hd/perfLog.h"
#include "pxr/imaging/hd/renderDelegate.h"
#include "pxr/imaging/hd/sceneDelegate.h"

#include "pxr/usd/usdGeom/gprim.h"
#include "pxr/usd/usdGeom/primvarsAPI.h"

#include "pxr/usd/usdShade/connectableAPI.h"
#include "pxr/usd/usdShade/material.h"
#include "pxr/usd/usdShade/shader.h"

#include "pxr/base/tf/type.h"

PXR_NAMESPACE_OPEN_SCOPE


TF_REGISTRY_FUNCTION(TfType)
{
    typedef UsdImagingGprimAdapter Adapter;
    TfType::Define<Adapter, TfType::Bases<Adapter::BaseAdapter> >();
    // No factory here, GprimAdapter is abstract.
}

static HdInterpolation
_UsdToHdInterpolation(TfToken const& usdInterp)
{
    if (usdInterp == UsdGeomTokens->uniform) {
        return HdInterpolationUniform;
    } else if (usdInterp == UsdGeomTokens->vertex) {
        return HdInterpolationVertex;
    } else if (usdInterp == UsdGeomTokens->varying) {
        return HdInterpolationVarying;
    } else if (usdInterp == UsdGeomTokens->faceVarying) {
        return HdInterpolationFaceVarying;
    } else if (usdInterp == UsdGeomTokens->constant) {
        return HdInterpolationConstant;
    }
    TF_CODING_ERROR("Unknown USD interpolation %s; treating as constant",
                    usdInterp.GetText());
    return HdInterpolationConstant;
}

static TfToken
_UsdToHdRole(TfToken const& usdRole)
{
    if (usdRole == SdfValueRoleNames->Point) {
        return HdPrimvarRoleTokens->point;
    } else if (usdRole == SdfValueRoleNames->Normal) {
        return HdPrimvarRoleTokens->normal;
    } else if (usdRole == SdfValueRoleNames->Vector) {
        return HdPrimvarRoleTokens->vector;
    } else if (usdRole == SdfValueRoleNames->Color) {
        return HdPrimvarRoleTokens->color;
    }
    // Empty token means no role specified
    return TfToken();
}

UsdImagingGprimAdapter::~UsdImagingGprimAdapter() 
{
}

/* static */
SdfPath
UsdImagingGprimAdapter::_ResolveCachePath(SdfPath const& primPath,
                                          UsdImagingInstancerContext const*
                                              instancerContext)
{
    SdfPath cachePath = primPath;

    // For non-instanced prims, cachePath and primPath will be the same, however
    // for instanced prims, cachePath will be something like:
    //
    // primPath: /__Master_1/cube
    // cachePath: /Models/cube_0.proto_cube_id0
    //
    // The name-mangling is so that multiple instancers/adapters can track the
    // same underlying UsdPrim.

    if (instancerContext != nullptr) {
        SdfPath const& instancer = instancerContext->instancerId;
        TfToken const& childName = instancerContext->childName;

        if (!instancer.IsEmpty()) {
            cachePath = instancer;
        }
        if (!childName.IsEmpty()) {
            cachePath = cachePath.AppendProperty(childName);
        }
    }
    return cachePath;
}

/* static */
SdfPath
UsdImagingGprimAdapter::_AddRprim(TfToken const& primType,
                                  UsdPrim const& usdPrim,
                                  UsdImagingIndexProxy* index,
                                  SdfPath const& materialId,
                                  UsdImagingInstancerContext const*
                                      instancerContext)
{
    SdfPath cachePath = _ResolveCachePath(usdPrim.GetPath(), instancerContext);
    SdfPath instancer = instancerContext ?
        instancerContext->instancerId : SdfPath();
    UsdPrim cachePrim = usdPrim.GetStage()->GetPrimAtPath(
        cachePath.GetAbsoluteRootOrPrimPath());

    index->InsertRprim(primType, cachePath, instancer, cachePrim,
        instancerContext ? instancerContext->instancerAdapter
            : UsdImagingPrimAdapterSharedPtr());
    HD_PERF_COUNTER_INCR(UsdImagingTokens->usdPopulatedPrimCount);

    // Populate shaders by reference from rprims.
    SdfPath materialPath = instancerContext ?
        instancerContext->instanceMaterialId : materialId;
    UsdPrim materialPrim = usdPrim.GetStage()->GetPrimAtPath(materialPath);

    if (materialPrim) {
       UsdImagingPrimAdapterSharedPtr materialAdapter =
           index->GetMaterialAdapter(materialPrim);
        if (materialAdapter) {
            materialAdapter->Populate(materialPrim, index, nullptr);
        }
    }

    return cachePath;
}

void 
UsdImagingGprimAdapter::TrackVariability(UsdPrim const& prim,
                                         SdfPath const& cachePath,
                                         HdDirtyBits* timeVaryingBits,
                                         UsdImagingInstancerContext const* 
                                             instancerContext) const
{
    // WARNING: This method is executed from multiple threads, the value cache
    // has been carefully pre-populated to avoid mutating the underlying
    // container during update.
    
    // Why is this OK? 
    // Either the value is unvarying, in which case the time ordinate doesn't
    // matter; or the value is varying, in which case we will update it upon
    // first call to Delegate::SetTime(). 
    UsdTimeCode time(1.0);

    UsdImagingValueCache* valueCache = _GetValueCache();

    if (!_IsVarying(prim,
               UsdGeomTokens->primvarsDisplayColor,
               HdChangeTracker::DirtyPrimvar,
               UsdImagingTokens->usdVaryingPrimvar,
               timeVaryingBits,
               false)) {
        // Only do this second check if the displayColor isn't already known
        // to be varying.
        _IsVarying(prim,
               UsdGeomTokens->primvarsDisplayOpacity,
               HdChangeTracker::DirtyPrimvar,
               UsdImagingTokens->usdVaryingPrimvar,
               timeVaryingBits,
               false);
    }

    // Discover time-varying extent.
    _IsVarying(prim,
               UsdGeomTokens->extent,
               HdChangeTracker::DirtyExtent,
               UsdImagingTokens->usdVaryingExtent,
               timeVaryingBits,
               false);

    // Discover time-varying transforms.
    _IsTransformVarying(prim,
               HdChangeTracker::DirtyTransform,
               UsdImagingTokens->usdVaryingXform,
               timeVaryingBits);

    valueCache->GetVisible(cachePath) = GetVisible(prim, time);
    // Discover time-varying visibility.
    _IsVarying(prim,
               UsdGeomTokens->visibility,
               HdChangeTracker::DirtyVisibility,
               UsdImagingTokens->usdVaryingVisibility,
               timeVaryingBits,
               true);

    TfToken purpose = _GetPurpose(prim, time);
    // Empty purpose means there is no opinion, fall back to geom.
    if (purpose.IsEmpty())
        purpose = UsdGeomTokens->default_;
    valueCache->GetPurpose(cachePath) = purpose;
}

void
UsdImagingGprimAdapter::_RemovePrim(SdfPath const& cachePath,
                                    UsdImagingIndexProxy* index)
{
    index->RemoveRprim(cachePath);
}

void 
UsdImagingGprimAdapter::_ComputeAndMergePrimvar(
    UsdGeomGprim const& gprim,
    SdfPath const& cachePath,
    UsdGeomPrimvar const& primvar,
    UsdTimeCode time,
    UsdImagingValueCache* valueCache) const
{
    VtValue v;
    TfToken primvarName = primvar.GetPrimvarName();
    if (primvar.ComputeFlattened(&v, time)) {
        TF_DEBUG(USDIMAGING_SHADERS)
            .Msg("Found primvar %s\n", primvarName.GetText());
        valueCache->GetPrimvar(cachePath, primvarName) = v;
        _MergePrimvar(&valueCache->GetPrimvars(cachePath),
                      primvarName, 
                      _UsdToHdInterpolation(primvar.GetInterpolation()),
                      _UsdToHdRole(primvar.GetAttr().GetRoleName()));
    } else {
        TF_DEBUG(USDIMAGING_SHADERS)
            .Msg( "\t\t No primvar on <%s> named %s\n",
                  gprim.GetPath().GetText(), primvarName.GetText());
    }
}

void 
UsdImagingGprimAdapter::UpdateForTime(UsdPrim const& prim,
                               SdfPath const& cachePath, 
                               UsdTimeCode time,
                               HdDirtyBits requestedBits,
                               UsdImagingInstancerContext const* 
                                   instancerContext) const
{
    UsdImagingValueCache* valueCache = _GetValueCache();

    SdfPath usdMaterialPath;
    if (requestedBits & (HdChangeTracker::DirtyPrimvar |
                         HdChangeTracker::DirtyMaterialId)) {
        usdMaterialPath = GetMaterialId(prim);

        // If we're processing this gprim on behalf of an instancer,
        // use the material binding specified by the instancer if we
        // aren't able to find a material binding for this prim itself.
        if (instancerContext && usdMaterialPath.IsEmpty()) {
            usdMaterialPath = instancerContext->instanceMaterialId;
        }
    }

    if (requestedBits & HdChangeTracker::DirtyPrimvar) {
        // XXX: need to validate gprim schema
        UsdGeomGprim gprim(prim);
        TfToken interpToken;
        valueCache->GetColor(cachePath) =
            GetColorAndOpacity(prim, time, &interpToken);
        _MergePrimvar(
            &valueCache->GetPrimvars(cachePath),
            HdTokens->color,
            _UsdToHdInterpolation(interpToken),
            HdPrimvarRoleTokens->color);

        if (!usdMaterialPath.IsEmpty()) {
            // XXX:HACK: Currently GetMaterialPrimvars() does not return
            // correct results, so in the meantime let's just ask USD
            // for the list of primvars.
            UsdGeomPrimvarsAPI primvars(gprim);
            if (_CanComputeMaterialNetworks()) {
                // Local (non-inherited) primvars
                for (auto const &pv: primvars.GetPrimvars()) {
                    _ComputeAndMergePrimvar(
                        gprim, cachePath, pv, time, valueCache);
                }
                // Inherited primvars
                for (auto const &pv: primvars.FindInheritedPrimvars()) {
                    _ComputeAndMergePrimvar(
                        gprim, cachePath, pv, time, valueCache);
                }
            } else {
                // Obtain the primvars used in the material bound to this prim
                // and check if they are in this prim, if so, add them to the 
                // primvars descriptions.
                TfTokenVector matPrimvarNames;
                VtValue vtMaterialPrimvars;
                if (valueCache->FindMaterialPrimvars(usdMaterialPath, 
                        &vtMaterialPrimvars)) {
                    if (vtMaterialPrimvars.IsHolding<TfTokenVector>()) {
                        matPrimvarNames = 
                            vtMaterialPrimvars.Get<TfTokenVector>();
                    }
                }

                for (auto const &pvName : matPrimvarNames) {
                    UsdGeomPrimvar pv = primvars.GetPrimvar(pvName);
                    if (!pv) {
                        // If not found, try as inherited primvar.
                        pv = primvars.FindInheritedPrimvar(pvName);
                    }
                    if (pv) {
                        _ComputeAndMergePrimvar(
                            gprim, cachePath, pv, time, valueCache);
                    }
                }
            }
        }
    }

    if (requestedBits & HdChangeTracker::DirtyDoubleSided){
        valueCache->GetDoubleSided(cachePath) = _GetDoubleSided(prim);
    }

    if (requestedBits & HdChangeTracker::DirtyTransform) {
        valueCache->GetTransform(cachePath) = GetTransform(prim, time);
    }

    if (requestedBits & HdChangeTracker::DirtyExtent) {
        valueCache->GetExtent(cachePath) = _GetExtent(prim, time);
    }

    if (requestedBits & HdChangeTracker::DirtyVisibility){
        valueCache->GetVisible(cachePath) = GetVisible(prim, time);
    }

    if (requestedBits & HdChangeTracker::DirtyMaterialId){
        valueCache->GetMaterialId(cachePath) = usdMaterialPath;

        TF_DEBUG(USDIMAGING_SHADERS).Msg("Shader for <%s> is <%s>\n",
                prim.GetPath().GetText(), usdMaterialPath.GetText());

    }
}

HdDirtyBits
UsdImagingGprimAdapter::ProcessPropertyChange(UsdPrim const& prim,
                                      SdfPath const& cachePath, 
                                      TfToken const& propertyName)
{
    if(propertyName == UsdGeomTokens->visibility 
          || propertyName == UsdGeomTokens->purpose)
        return HdChangeTracker::DirtyVisibility;

    else if (UsdGeomXformable::IsTransformationAffectedByAttrNamed(propertyName))
        return HdChangeTracker::DirtyTransform;

    else if (propertyName == UsdGeomTokens->extent) 
        return HdChangeTracker::DirtyExtent;

    else if (propertyName == UsdGeomTokens->doubleSided) 
        return HdChangeTracker::DirtyDoubleSided;
    
    // TODO: support sparse displayColor updates

    return HdChangeTracker::AllDirty;
}

void
UsdImagingGprimAdapter::MarkDirty(UsdPrim const& prim,
                                  SdfPath const& cachePath,
                                  HdDirtyBits dirty,
                                  UsdImagingIndexProxy* index)
{
    index->MarkRprimDirty(cachePath, dirty);
}

void
UsdImagingGprimAdapter::MarkRefineLevelDirty(UsdPrim const& prim,
                                             SdfPath const& cachePath,
                                             UsdImagingIndexProxy* index)
{
    index->MarkRprimDirty(cachePath, HdChangeTracker::DirtyRefineLevel);
}

void
UsdImagingGprimAdapter::MarkReprDirty(UsdPrim const& prim,
                                      SdfPath const& cachePath,
                                      UsdImagingIndexProxy* index)
{
    index->MarkRprimDirty(cachePath, HdChangeTracker::DirtyRepr);
}

void
UsdImagingGprimAdapter::MarkCullStyleDirty(UsdPrim const& prim,
                                           SdfPath const& cachePath,
                                           UsdImagingIndexProxy* index)
{
    index->MarkRprimDirty(cachePath, HdChangeTracker::DirtyCullStyle);
}

void
UsdImagingGprimAdapter::MarkTransformDirty(UsdPrim const& prim,
                                           SdfPath const& cachePath,
                                           UsdImagingIndexProxy* index)
{
    index->MarkRprimDirty(cachePath, HdChangeTracker::DirtyTransform);
}

void
UsdImagingGprimAdapter::MarkVisibilityDirty(UsdPrim const& prim,
                                            SdfPath const& cachePath,
                                            UsdImagingIndexProxy* index)
{
    index->MarkRprimDirty(cachePath, HdChangeTracker::DirtyVisibility);
}

// -------------------------------------------------------------------------- //

GfRange3d 
UsdImagingGprimAdapter::_GetExtent(UsdPrim const& prim, UsdTimeCode time) const
{
    HD_TRACE_FUNCTION();
    HF_MALLOC_TAG_FUNCTION();
    UsdGeomGprim gprim(prim);
    VtVec3fArray extent;
    if (gprim.GetExtentAttr().Get(&extent, time)) {
        // Note:
        // Usd stores extent as 2 float vecs. We do an implicit 
        // conversion to doubles
        return GfRange3d(extent[0], extent[1]);
    } else {
        // Return empty range if no value was found.
        // TODO: Should this compute the extent based on the points instead?
        return GfRange3d();
    }
}

/* static */
VtValue
UsdImagingGprimAdapter::GetColorAndOpacity(UsdPrim const& prim,
                                           UsdTimeCode time,
                                           TfToken* interpolation)
{
    HD_TRACE_FUNCTION();
    HF_MALLOC_TAG_FUNCTION();
    const GfVec3f defaultColor(0.5f);
    const float defaultOpacity = 1.0f;
    VtVec4fArray result(1);
    result[0] = GfVec4f(defaultColor[0],
                        defaultColor[1],
                        defaultColor[2],
                        defaultOpacity);

    size_t numColors = 1, numOpacities = 1;
    TfToken colorInterp, opacityInterp;
    TfToken colorPrimvarName, opacityPrimvarName;

    // for a prim's color & opacity, we use the following precedence:
    // material rel >  local prim var(s)
    {
        // -- Material --        
        // XXX: Primvar values that come from shaders should not be part of
        // the Rprim data, it should live as part of the shader so it can be 
        // shared, though that poses some interesting questions for vertex & 
        // varying rate shader provided primvars.
        static TfToken displayColorToken("displayColor");
        static TfToken displayOpacityToken("displayOpacity");

        UsdRelationship mat = UsdShadeMaterial::GetBindingRel(prim);
        SdfPathVector matTargets;
        if (mat.GetForwardedTargets(&matTargets)) {
            if (!matTargets.empty()) {
                if (matTargets.size() > 1) {
                    TF_WARN("<%s> has more than one material target; "\
                            "using first one found: <%s>",
                            prim.GetPath().GetText(),
                            matTargets.front().GetText());
                }
                UsdPrim matPrim(
                    prim.GetStage()->GetPrimAtPath(matTargets.front()));

                GfVec3f matColor;
                if (matPrim &&
                    matPrim.GetAttribute(displayColorToken)
                        .Get(&matColor, time)) {
                    colorInterp = UsdGeomTokens->constant; 
                    colorPrimvarName = displayColorToken;
                    result[0][0] = matColor[0];
                    result[0][1] = matColor[1];
                    result[0][2] = matColor[2];
                }

                float matOpacity;
                if (matPrim &&
                    matPrim.GetAttribute(displayOpacityToken)
                        .Get(&matOpacity, time)) {
                    opacityInterp = UsdGeomTokens->constant;
                    opacityPrimvarName = displayOpacityToken;
                    result[0][3] = matOpacity;
                }
            }
        }
    }

    {
        // -- Prim local prim var --
        UsdGeomGprim gprimSchema(prim);
 
        if (colorInterp.IsEmpty()) { // did not get color from material
            VtArray<GfVec3f> colorArray;
                const UsdGeomPrimvar& primvar = 
                    gprimSchema.GetDisplayColorPrimvar();
            if (primvar.ComputeFlattened(&colorArray, time)) {
                colorInterp = primvar.GetInterpolation();
                colorPrimvarName = primvar.GetName();
                numColors = colorArray.size();
                result.resize(numColors);

                if (colorInterp == UsdGeomTokens->constant) {
                    if (numColors > 0) {
                        result[0][0] = colorArray[0][0];
                        result[0][1] = colorArray[0][1];
                        result[0][2] = colorArray[0][2];
                    }

                    if (numColors != 1) {
                        // warn and copy default color for remaining elements
                         TF_WARN("Prim %s has %lu element(s) for %s even "
                                 "though it is marked constant.",
                                 prim.GetPath().GetText(), numColors,
                                 colorPrimvarName.GetText());
                        
                        for (size_t ii = 1; ii < numColors; ii++) {
                            result[ii][0] = defaultColor[0];
                            result[ii][1] = defaultColor[1];
                            result[ii][2] = defaultColor[2];
                        }
                    }
                } else {
                    for (size_t ii = 0; ii < numColors; ii++) {
                        result[ii][0] = colorArray[ii][0];
                        result[ii][1] = colorArray[ii][1];
                        result[ii][2] = colorArray[ii][2];
                    }
                }
            } else {
                // displayColor is treated as a special primvar -- if it isn't
                // authored by the user, the schema defaults it to constant 
                // interp. 
                // if authored with no data (allowed for non-constant interp),
                // we should return an empty result.
                colorInterp = primvar.GetInterpolation();
                if (colorInterp != UsdGeomTokens->constant) {
                    numColors = 0;
                }

                static TfToken 
                    defaultDisplayColorToken("displayColor(default)");
                colorPrimvarName = defaultDisplayColorToken;
            }
        }

        // Guaranteed to have set either material/local/default color interp.
        TF_VERIFY(!colorInterp.IsEmpty());

        if (opacityInterp.IsEmpty()) { // did not get opacity from material
            VtArray<float> opacityArray;
                const UsdGeomPrimvar& primvar = 
                    gprimSchema.GetDisplayOpacityPrimvar();
            if (primvar.ComputeFlattened(&opacityArray, time)) {
                opacityInterp = primvar.GetInterpolation();
                opacityPrimvarName = primvar.GetName();
                numOpacities = opacityArray.size();
                if (numOpacities > result.size()) {
                    result.resize(numOpacities);
                }

                // copy just the opacities; color is populated in the 
                // consolidation step
                if (opacityInterp == UsdGeomTokens->constant) {
                    if (numOpacities > 0) {
                        result[0][3] = opacityArray[0];
                    }

                    if (numOpacities != 1) {
                        // warn and copy default opacity for remaining elements
                         TF_WARN("Prim %s has %lu element(s) for %s even "
                                 "though it is marked constant.",
                                 prim.GetPath().GetText(), numOpacities,
                                 opacityPrimvarName.GetText());
                        
                        for (size_t ii = 1; ii < numOpacities; ii++) {
                            result[ii][3] = defaultOpacity;
                        }
                    }
                } else {
                    for (size_t ii = 0; ii < numOpacities; ii++) {
                        result[ii][3] = opacityArray[ii];
                    }
                }
            } else {
                // displayOpacity is treated as a special primvar -- if it isn't
                // authored by the user, the schema defaults it to constant 
                // interp. 
                // if authored with no data (allowed for non-constant interp),
                // we should return an empty result.
                opacityInterp = primvar.GetInterpolation();
                if (opacityInterp != UsdGeomTokens->constant) {
                    numOpacities = 0;
                }

                static TfToken
                    defaultDisplayOpacityToken("displayOpacity(default)");
                opacityPrimvarName = defaultDisplayOpacityToken;
            }
        }
        // Guaranteed to have set either material/local/default opacity interp
        TF_VERIFY(!opacityInterp.IsEmpty());
    }

    {
        // --  Cases where we can surely issue warnings --
        if (colorInterp == opacityInterp &&
            numColors != numOpacities &&
            (numColors > 0 && numOpacities > 0)) {
            // interp modes same but (non-zero) lengths different for primvars 
            // is surely an input error
            TF_WARN("Prim %s has %lu elements for %s and %lu "
                    "elements for %s even though they have the "
                    "same interpolation mode %s", prim.GetPath().GetText(), 
                    numColors, colorPrimvarName.GetText(),
                    numOpacities, opacityPrimvarName.GetText(),
                    colorInterp.GetText());

        } else if (colorInterp != opacityInterp && 
                   (colorInterp != UsdGeomTokens->constant &&
                    opacityInterp != UsdGeomTokens->constant)) {
            // we can sensibly handle the case of different interp modes with
            // one of them being constant by splatting it across. 
            // for everything else, issue a warning.
            TF_WARN("Prim %s has %s interpolation for %s and %s "
                    "interpolation for %s; this combination is not "
                    "supported by UsdImaging", prim.GetPath().GetText(),
                    colorInterp.GetText(), colorPrimvarName.GetText(),
                    opacityInterp.GetText(), opacityPrimvarName.GetText());
        }
    }

    {
        // -- Consolidate missing color or opacity values in result --
        if (numColors == 0 || numOpacities == 0) {
            // remove default value that was filled in
            result.resize(0);
            // override the (default) color interp mode if opacity was authored
            // and empty
            if (numOpacities == 0) {
                colorInterp = opacityInterp;
            }
        } else {
            const size_t resultSize = result.size();
            if (numColors < numOpacities) {
                GfVec3f splatColor = defaultColor;
                if (colorInterp == UsdGeomTokens->constant) {
                    // override color interp mode and splat first color
                    colorInterp = opacityInterp;
                    splatColor = GfVec3f(
                        result[0][0], result[0][1], result[0][2]);
                }
                for(size_t ii = numColors; ii < resultSize; ii++) {
                    result[ii][0] = splatColor[0];
                    result[ii][1] = splatColor[1];
                    result[ii][2] = splatColor[2];
                }
            }
            else {
                float splatOpacity = defaultOpacity;
                // resultSize may be 0 (if empty color primvar array), so don't 
                // splat 
                if (opacityInterp == UsdGeomTokens->constant) {
                    // splat first opacity
                    splatOpacity = result[0][3];
                }
                for(size_t ii = numOpacities; ii < resultSize; ii++) {
                    result[ii][3] = splatOpacity;
                }
            }
        }
    }

    // If the interpolation we're passing back is constant, truncate the array
    // if necessary so that we don't have an array-valued color in the shader.
    // We will have already warned above about one or both of the primvars
    // having constant interpolation but multiple values.
    if (colorInterp == UsdGeomTokens->constant && result.size() > 1) {
        result.resize(1);
    }
    if (interpolation) {
        *interpolation = colorInterp;
    }
    return VtValue(result);
}

TfToken
UsdImagingGprimAdapter::_GetPurpose(UsdPrim const& prim, UsdTimeCode time) const
{
    HD_TRACE_FUNCTION();
    // PERFORMANCE: Make this more efficient, see http://bug/90497
    return UsdGeomImageable(prim).ComputePurpose();
}

bool 
UsdImagingGprimAdapter::_GetDoubleSided(UsdPrim const& prim) const
{
    HD_TRACE_FUNCTION();
    HF_MALLOC_TAG_FUNCTION();

    if(!TF_VERIFY(prim.IsA<UsdGeomGprim>(), "%s\n",
                prim.GetPath().GetText()))
        return false;

    return _Get<bool>(prim, UsdGeomTokens->doubleSided, UsdTimeCode::Default());
}

PXR_NAMESPACE_CLOSE_SCOPE

