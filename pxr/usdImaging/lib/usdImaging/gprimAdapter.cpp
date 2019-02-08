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
        if (materialPrim.IsA<UsdShadeMaterial>()) {
            UsdImagingPrimAdapterSharedPtr materialAdapter =
                index->GetMaterialAdapter(materialPrim);
            if (materialAdapter) {
                materialAdapter->Populate(materialPrim, index, nullptr);
            }
        } else {
            TF_WARN("Gprim <%s> has illegal material reference to "
                    "prim <%s> of type (%s)", usdPrim.GetPath().GetText(),
                    materialPrim.GetPath().GetText(),
                    materialPrim.GetTypeName().GetText());
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

bool
UsdImagingGprimAdapter::_IsBuiltinPrimvar(TfToken const& primvarName) const
{
    return (primvarName == HdTokens->displayColor ||
            primvarName == HdTokens->displayOpacity);
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
    HdPrimvarDescriptorVector& primvars = valueCache->GetPrimvars(cachePath);

    if (requestedBits & HdChangeTracker::DirtyPoints) {
        VtValue& points = valueCache->GetPoints(cachePath);
        points = GetPoints(prim, cachePath, time);

        // Expose points as a primvar.
        _MergePrimvar(
            &primvars,
            HdTokens->points,
            HdInterpolationVertex,
            HdPrimvarRoleTokens->point);
    }

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

        TfToken colorInterp;
        VtValue color;
        if (GetColor(prim, time, &colorInterp, &color)) {
            valueCache->GetColor(cachePath) = color;
            _MergePrimvar(
                &primvars,
                HdTokens->displayColor,
                _UsdToHdInterpolation(colorInterp),
                HdPrimvarRoleTokens->color);
        }

        TfToken opacityInterp;
        VtValue opacity;
        if (GetOpacity(prim, time, &opacityInterp, &opacity)) {
            valueCache->GetOpacity(cachePath) = opacity;
            _MergePrimvar(
                &primvars,
                HdTokens->displayOpacity,
                _UsdToHdInterpolation(opacityInterp));
        }

        if (_GetMaterialBindingPurpose() == HdTokens->full) {
            // XXX:HACK: Currently GetMaterialPrimvars() does not return
            // correct results, so in the meantime let's just ask USD
            // for the list of primvars.  The inherited primvars from parent
            // should really be cached and shared...
            
            // All primvars returned by plural Find* methods have already
            // been verified to have some authored value
            UsdGeomPrimvarsAPI primvars(prim);
            for (auto const &pv: primvars.FindPrimvarsWithInheritance()) {
                if (_IsBuiltinPrimvar(pv.GetPrimvarName())) {
                    continue;
                }
                _ComputeAndMergePrimvar(
                    prim, cachePath, pv, time, valueCache);
            }
        } else {

            if (!usdMaterialPath.IsEmpty()) {
                // Obtain the primvars used in the material bound to this prim
                // and check if they are in this prim, if so, add them to the 
                // primvars descriptions.
                TfTokenVector matPrimvarNames;
                valueCache->FindMaterialPrimvars(usdMaterialPath, 
                                                 &matPrimvarNames);

                UsdGeomPrimvarsAPI primvars(gprim);
                for (auto const &pvName : matPrimvarNames) {
                    if (_IsBuiltinPrimvar(pvName)) {
                        continue;
                    }
                    // XXX If we can cache inheritable primvars at each 
                    // non-leaf prim, then we can use the overload that keeps
                    // us from needing to search up ancestors.
                    UsdGeomPrimvar pv = primvars.FindPrimvarWithInheritance(pvName);
                    if (pv.HasValue()) {
                        _ComputeAndMergePrimvar(
                            prim, cachePath, pv, time, valueCache);
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

    else if (TfStringStartsWith(propertyName.GetString(),
                               UsdShadeTokens->materialBinding.GetString()) ||
             TfStringStartsWith(propertyName.GetString(),
                                UsdTokens->collection.GetString())) {
        return HdChangeTracker::DirtyMaterialId;
    }
    
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
    index->MarkRprimDirty(cachePath, HdChangeTracker::DirtyDisplayStyle);
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

void
UsdImagingGprimAdapter::MarkMaterialDirty(UsdPrim const& prim,
                                          SdfPath const& cachePath,
                                          UsdImagingIndexProxy* index)
{
    // If the Usd material changed, it could mean the primvar set also changed
    // Hydra doesn't currently manage detection and propagation of these
    // changes, so we must mark the rprim dirty.
    index->MarkRprimDirty(cachePath, HdChangeTracker::DirtyMaterialId);
}

/*virtual*/
VtValue
UsdImagingGprimAdapter::GetPoints(UsdPrim const& prim,
                                  SdfPath const& cachePath,
                                  UsdTimeCode time) const
{
    TF_UNUSED(cachePath);
    HD_TRACE_FUNCTION();
    HF_MALLOC_TAG_FUNCTION();

    VtVec3fArray points;
    if (!prim.GetAttribute(UsdGeomTokens->points).Get(&points, time)) {
        TF_WARN("Points could not be read from prim: <%s>",
                prim.GetPath().GetText());
        points = VtVec3fArray();
    }

    return VtValue(points);
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
bool
UsdImagingGprimAdapter::GetColor(UsdPrim const& prim,
                                 UsdTimeCode time,
                                 TfToken* interpolation,
                                 VtValue* color)
{
    HD_TRACE_FUNCTION();
    HF_MALLOC_TAG_FUNCTION();

    VtVec3fArray result(1, GfVec3f(0.5f));
    TfToken colorInterp;

    // for a prim's color we use the following precedence:
    // material rel >  local prim var(s)
    {
        // -- Material --        
        // XXX: Primvar values that come from shaders should not be part of
        // the Rprim data, it should live as part of the shader so it can be 
        // shared, though that poses some interesting questions for vertex & 
        // varying rate shader provided primvars.
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

                if (matPrim &&
                    matPrim.GetAttribute(HdTokens->displayColor)
                        .Get(&result[0], time)) {
                    colorInterp = UsdGeomTokens->constant;
                }
            }
        }
    }

    {
        // -- Prim local prim var --
        if (colorInterp.IsEmpty()) { // did not get color from material
            UsdGeomGprim gprimSchema(prim);
            const UsdGeomPrimvar& primvar = 
                gprimSchema.GetDisplayColorPrimvar();
            if (primvar.ComputeFlattened(&result, time)) {
                colorInterp = primvar.GetInterpolation();
                if (colorInterp == UsdGeomTokens->constant &&
                    result.size() > 1) {
                    TF_WARN("Prim %s has %lu element(s) for %s even "
                            "though it is marked constant.",
                            prim.GetPath().GetText(), result.size(),
                            primvar.GetName().GetText());
                    result.resize(1);
                }
            }
        }
    }

    if (colorInterp.IsEmpty()) {
        // No color defined for this prim
        return false;
    }

    if (interpolation) {
        *interpolation = colorInterp;
    }
    if (color) {
        *color = VtValue(result);
    }
    return true;
}

/* static */
bool
UsdImagingGprimAdapter::GetOpacity(UsdPrim const& prim,
                                   UsdTimeCode time,
                                   TfToken* interpolation,
                                   VtValue* opacity)
{
    HD_TRACE_FUNCTION();
    HF_MALLOC_TAG_FUNCTION();

    VtFloatArray result(1, 1.0f);
    TfToken opacityInterp;

    // for a prim's opacity, we use the following precedence:
    // material rel >  local prim var(s)
    {
        // -- Material --        
        // XXX: Primvar values that come from shaders should not be part of
        // the Rprim data, it should live as part of the shader so it can be 
        // shared, though that poses some interesting questions for vertex & 
        // varying rate shader provided primvars.
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

                if (matPrim &&
                    matPrim.GetAttribute(HdTokens->displayOpacity)
                        .Get(&result[0], time)) {
                    opacityInterp = UsdGeomTokens->constant;
                }
            }
        }
    }

    {
        // -- Prim local prim var --
        if (opacityInterp.IsEmpty()) { // did not get opacity from material
            UsdGeomGprim gprimSchema(prim);
            const UsdGeomPrimvar& primvar = 
                gprimSchema.GetDisplayOpacityPrimvar();
            if (primvar.ComputeFlattened(&result, time)) {
                opacityInterp = primvar.GetInterpolation();
                if (opacityInterp == UsdGeomTokens->constant &&
                    result.size() > 1) {
                    TF_WARN("Prim %s has %lu element(s) for %s even "
                            "though it is marked constant.",
                            prim.GetPath().GetText(), result.size(),
                            primvar.GetName().GetText());
                    result.resize(1);
                }
            }
        }
    }

    if (opacityInterp.IsEmpty()) {
        return false;
    }

    if (interpolation) {
        *interpolation = opacityInterp;
    }
    if (opacity) {
        *opacity = VtValue(result);
    }
    return true;
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

