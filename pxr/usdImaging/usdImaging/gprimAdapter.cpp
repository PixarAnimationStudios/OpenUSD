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

#include "pxr/usdImaging/usdImaging/coordSysAdapter.h"
#include "pxr/usdImaging/usdImaging/debugCodes.h"
#include "pxr/usdImaging/usdImaging/delegate.h"
#include "pxr/usdImaging/usdImaging/indexProxy.h"
#include "pxr/usdImaging/usdImaging/instancerContext.h"
#include "pxr/usdImaging/usdImaging/tokens.h"

#include "pxr/imaging/hd/perfLog.h"
#include "pxr/imaging/hd/material.h"
#include "pxr/imaging/hd/renderDelegate.h"
#include "pxr/imaging/hd/sceneDelegate.h"

#include "pxr/usd/usdGeom/gprim.h"
#include "pxr/usd/usdGeom/motionAPI.h"
#include "pxr/usd/usdGeom/pointBased.h"
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

static TfTokenVector
_GetPrimvarsForMaterial(const VtValue & vtMaterial)
{
    TfTokenVector primvars;

    if (vtMaterial.IsHolding<HdMaterialNetworkMap>()) {

        HdMaterialNetworkMap const& networkMap = 
            vtMaterial.UncheckedGet<HdMaterialNetworkMap>();

        // To simplify the logic so we do not have to pick between different
        // networks (surface, displacement, volume), we merge all primvars.

        for (auto const& itMap : networkMap.map) {
            HdMaterialNetwork const& network = itMap.second;
            primvars.insert(primvars.end(), 
                network.primvars.begin(), network.primvars.end());
        }
    }

    return primvars;
}

UsdImagingGprimAdapter::~UsdImagingGprimAdapter() 
{
}

/* static */
SdfPath
UsdImagingGprimAdapter::_ResolveCachePath(SdfPath const& usdPath,
                                          UsdImagingInstancerContext const*
                                              instancerContext)
{
    SdfPath cachePath = usdPath;

    // For non-instanced prims, cachePath and usdPath will be the same, however
    // for instanced prims, cachePath will be something like:
    //
    // primPath: /__Prototype_1/cube
    // cachePath: /Models/cube_0.proto_cube_id0
    //
    // The name-mangling is so that multiple instancers/adapters can track the
    // same underlying UsdPrim.

    if (instancerContext != nullptr) {
        SdfPath const& instancer = instancerContext->instancerCachePath;
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

SdfPath
UsdImagingGprimAdapter::_AddRprim(TfToken const& primType,
                                  UsdPrim const& usdPrim,
                                  UsdImagingIndexProxy* index,
                                  SdfPath const& materialUsdPath,
                                  UsdImagingInstancerContext const*
                                      instancerContext)
{
    SdfPath cachePath = _ResolveCachePath(usdPrim.GetPath(), instancerContext);

    // For an instanced gprim, this is the instancer prim.
    // For a non-instanced gprim, this is just the gprim.
    UsdPrim proxyPrim = usdPrim.GetStage()->GetPrimAtPath(
        cachePath.GetAbsoluteRootOrPrimPath());

    index->InsertRprim(primType, cachePath, proxyPrim,
        instancerContext ? instancerContext->instancerAdapter
            : UsdImagingPrimAdapterSharedPtr());
    HD_PERF_COUNTER_INCR(UsdImagingTokens->usdPopulatedPrimCount);

    // As long as we're passing the proxyPrim in here, we need to add a
    // manual dependency on usdPrim so that usd editing works correctly;
    // also, get rid of the proxyPrim dependency.
    // XXX: We should get rid of proxyPrim entirely.
    if (instancerContext != nullptr) {
        index->RemovePrimInfoDependency(cachePath);
        index->AddDependency(cachePath, usdPrim);
    }

    // If there's no local material path, fall back to the instancer material.
    SdfPath resolvedUsdMaterialPath = materialUsdPath;
    if (materialUsdPath.IsEmpty() && instancerContext != nullptr) {
        resolvedUsdMaterialPath = instancerContext->instancerMaterialUsdPath;
    }
    UsdPrim materialPrim =
        usdPrim.GetStage()->GetPrimAtPath(resolvedUsdMaterialPath);

    if (materialPrim) {
        if (materialPrim.IsA<UsdShadeMaterial>()) {
            UsdImagingPrimAdapterSharedPtr materialAdapter =
                index->GetMaterialAdapter(materialPrim);
            if (materialAdapter) {
                materialAdapter->Populate(materialPrim, index, nullptr);
                // We need to register a dependency on the material prim so
                // that geometry is updated when the material is
                // (specifically, DirtyMaterialId).
                // XXX: Eventually, it would be great to push this into hydra.
                index->AddDependency(cachePath, materialPrim);
            }
        } else {
            TF_WARN("Gprim <%s> has illegal material reference to "
                    "prim <%s> of type (%s)", usdPrim.GetPath().GetText(),
                    materialPrim.GetPath().GetText(),
                    materialPrim.GetTypeName().GetText());
        }
    }

    // Populate coordinate system sprims bound to rprims.
    if (_DoesDelegateSupportCoordSys()) {
        if (UsdImagingPrimAdapterSharedPtr coordSysAdapter =
            _GetAdapter(HdPrimTypeTokens->coordSys)) {
            coordSysAdapter->Populate(usdPrim, index, instancerContext);
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
    // See if any of the inherited primvars are time-dependent.
    UsdImaging_InheritedPrimvarStrategy::value_type inheritedPrimvarRecord =
        _GetInheritedPrimvars(prim.GetParent());
    if (inheritedPrimvarRecord && inheritedPrimvarRecord->variable) {
        *timeVaryingBits |= HdChangeTracker::DirtyPrimvar;
        HD_PERF_COUNTER_INCR(UsdImagingTokens->usdVaryingPrimvar);
    }
    if (!(*timeVaryingBits & HdChangeTracker::DirtyPrimvar)) {
        // See if any local primvars are time-dependent.
        UsdGeomPrimvarsAPI primvarsAPI(prim);
        std::vector<UsdGeomPrimvar> primvars =
            primvarsAPI.GetPrimvarsWithValues();
        for (UsdGeomPrimvar const& pv : primvars) {
            if (pv.ValueMightBeTimeVarying()) {
                *timeVaryingBits |= HdChangeTracker::DirtyPrimvar;
                HD_PERF_COUNTER_INCR(UsdImagingTokens->usdVaryingPrimvar);
                break;
            }
        }
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

    // Discover time-varying visibility.
    _IsVarying(prim,
               UsdGeomTokens->visibility,
               HdChangeTracker::DirtyVisibility,
               UsdImagingTokens->usdVaryingVisibility,
               timeVaryingBits,
               true);

    // Discover time-varying points.
    _IsVarying(prim,
               UsdGeomTokens->velocities,
               HdChangeTracker::DirtyPoints,
               UsdImagingTokens->usdVaryingPrimvar,
               timeVaryingBits,
               false) ||
        _IsVarying(prim,
                   UsdGeomTokens->accelerations,
                   HdChangeTracker::DirtyPoints,
                   UsdImagingTokens->usdVaryingPrimvar,
                   timeVaryingBits,
                   false) ||
            _IsVarying(prim,
                       UsdGeomTokens->motionNonlinearSampleCount,
                       HdChangeTracker::DirtyPoints,
                       UsdImagingTokens->usdVaryingPrimvar,
                       timeVaryingBits,
                       true) ||
                _IsVarying(prim,
                           UsdGeomTokens->motionBlurScale,
                           HdChangeTracker::DirtyPoints,
                           UsdImagingTokens->usdVaryingPrimvar,
                           timeVaryingBits,
                           true);

    // XXX: "points" is handled by derived classes.

    // Discover time-varying double-sidedness.
    _IsVarying(prim,
               UsdGeomTokens->doubleSided,
               HdChangeTracker::DirtyDoubleSided,
               UsdImagingTokens->usdVaryingTopology,
               timeVaryingBits,
               false);
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
    UsdImagingPrimvarDescCache* primvarDescCache = _GetPrimvarDescCache();
    HdPrimvarDescriptorVector& vPrimvars = 
        primvarDescCache->GetPrimvars(cachePath);

    if (requestedBits & HdChangeTracker::DirtyPoints) {

        // Expose points as a primvar.
        _MergePrimvar(
            &vPrimvars,
            HdTokens->points,
            HdInterpolationVertex,
            HdPrimvarRoleTokens->point);

        // Velocity information is expected to be authored at the same sample
        // rate as points data, so use the points dirty bit to let us know when
        // to publish velocities.
        UsdGeomPointBased pointBased(prim);
        VtVec3fArray velocities;
        if (pointBased.GetVelocitiesAttr() &&
            pointBased.GetVelocitiesAttr().Get(&velocities, time)) {
            // Expose velocities as a primvar.
            _MergePrimvar(
                &vPrimvars,
                HdTokens->velocities,
                HdInterpolationVertex,
                HdPrimvarRoleTokens->vector);
        }

        // Acceleration information is expected to be authored at the same sample
        // rate as points data, so use the points dirty bit to let us know when
        // to publish accelerations.
        VtVec3fArray accelerations;
        if (pointBased.GetAccelerationsAttr() &&
            pointBased.GetAccelerationsAttr().Get(&accelerations, time)) {
            // Expose accelerations as a primvar.
            _MergePrimvar(
                &vPrimvars,
                HdTokens->accelerations,
                HdInterpolationVertex,
                HdPrimvarRoleTokens->vector);
        }

        // Since nonlinearSampleCount is tied to the calculation
        // of the motion-blurred points, we also use the points dirty
        // bit here to know when to publish its value. Since it is
        // inherited, we go through the corresponding resolved attribute
        // cache.
        UsdImaging_NonlinearSampleCountCache *nonlinearSampleCountCache =
            _GetNonlinearSampleCountCache();
        // Check that it has any opinions.
        if (nonlinearSampleCountCache->GetValue(prim) !=
                UsdImaging_NonlinearSampleCountStrategy::invalidValue) {
            _MergePrimvar(
                &vPrimvars,
                HdTokens->nonlinearSampleCount,
                HdInterpolationConstant,
                HdPrimvarRoleTokens->none);
        }

        // Comment similar to above nonlinear sample count cache applies to
        // blur scale.
        UsdImaging_BlurScaleCache *blurScaleCache = _GetBlurScaleCache();
        // Check that it has any opinions.
        if (blurScaleCache->GetValue(prim).has_value) {
            _MergePrimvar(
                &vPrimvars,
                HdTokens->blurScale,
                HdInterpolationConstant,
                HdPrimvarRoleTokens->none);
        }
    }

    SdfPathVector materialUsdPaths;
    if (requestedBits & (HdChangeTracker::DirtyPrimvar |
                         HdChangeTracker::DirtyMaterialId)) {
        SdfPath materialUsdPath = GetMaterialUsdPath(prim);
        if (!materialUsdPath.IsEmpty()) {
            materialUsdPaths.push_back(materialUsdPath);
        } else if (instancerContext) {
            // If we're processing this gprim on behalf of an instancer,
            // use the material binding specified by the instancer if we
            // aren't able to find a material binding for this prim itself.
            materialUsdPaths.push_back(instancerContext->instancerMaterialUsdPath);
        }
    }

    if (requestedBits & HdChangeTracker::DirtyPrimvar) {
        if (UsdGeomImageable imageable = UsdGeomImageable(prim)) {
            for (const UsdGeomSubset &subset: UsdGeomSubset::GetAllGeomSubsets(imageable)) {
                SdfPath materialUsdPath = GetMaterialUsdPath(subset.GetPrim());
                if (!materialUsdPath.IsEmpty()) {
                    materialUsdPaths.push_back(materialUsdPath);
                }
            }
        }
    }

    if (requestedBits & HdChangeTracker::DirtyPrimvar) {
        // Handle color/opacity specially, since they can be shadowed by
        // material parameters.  If we don't find them, check inherited
        // primvars.
        TfToken colorInterp;
        VtValue color;
        VtIntArray colorIndices(0);
        if (GetColor(prim, time, &colorInterp, &color, &colorIndices)) {
            _MergePrimvar(
                &vPrimvars,
                HdTokens->displayColor,
                _UsdToHdInterpolation(colorInterp),
                HdPrimvarRoleTokens->color,
                !colorIndices.empty());
        } else {
            UsdGeomPrimvar pv =
                _GetInheritedPrimvar(prim, HdTokens->displayColor);
            if (pv) {
                _ComputeAndMergePrimvar(prim, pv, time, &vPrimvars);
            }
        }

        TfToken opacityInterp;
        VtValue opacity;
        VtIntArray opacityIndices(0);
        if (GetOpacity(prim, time, &opacityInterp, &opacity, &opacityIndices)) {
            _MergePrimvar(
                &vPrimvars,
                HdTokens->displayOpacity,
                _UsdToHdInterpolation(opacityInterp),
                TfToken(),
                !opacityIndices.empty());
        } else {
            UsdGeomPrimvar pv =
                _GetInheritedPrimvar(prim, HdTokens->displayOpacity);
            if (pv) {
                _ComputeAndMergePrimvar(prim, pv, time, &vPrimvars);
            }
        }

        // Compile a list of primvars to check.
        std::vector<UsdGeomPrimvar> primvars;
        UsdImaging_InheritedPrimvarStrategy::value_type inheritedPrimvarRecord =
            _GetInheritedPrimvars(prim.GetParent());
        if (inheritedPrimvarRecord) {
            primvars = inheritedPrimvarRecord->primvars;
        }

        UsdGeomPrimvarsAPI primvarsAPI(prim);
        std::vector<UsdGeomPrimvar> local = primvarsAPI.GetPrimvarsWithValues();
        primvars.insert(primvars.end(), local.begin(), local.end());

        // Some backends may not want to load all primvars due to memory limits.
        // We filter the list of primvars, removing any the materials and rprims
        // don't expect.
        TfTokenVector rprimPrimvarNames;
        TfTokenVector matPrimvarNames;
        if (_IsPrimvarFilteringNeeded()) {
            rprimPrimvarNames = _GetRprimPrimvarNames();
            if (!materialUsdPaths.empty()) {
                matPrimvarNames = _CollectMaterialPrimvars(materialUsdPaths,
                                                           time);
            }
        }

        for (auto const &pv : primvars) {
            if (_IsBuiltinPrimvar(pv.GetPrimvarName())) {
                // This primvar has been handled explicitly above already.
                continue;
            }
            if (_IsPrimvarFilteringNeeded() &&
                std::find(rprimPrimvarNames.begin(),
                          rprimPrimvarNames.end(),
                          pv.GetPrimvarName()) == rprimPrimvarNames.end() &&
                std::find(matPrimvarNames.begin(),
                          matPrimvarNames.end(),
                          pv.GetPrimvarName()) == matPrimvarNames.end()) {
                // No material or rprim expects this primvar, so it doesn't
                // pass filtering, so skip it.
                continue;
            }

            _ComputeAndMergePrimvar(prim, pv, time, &vPrimvars);
        }
    }
}

HdDirtyBits
UsdImagingGprimAdapter::ProcessPropertyChange(UsdPrim const& prim,
                                      SdfPath const& cachePath, 
                                      TfToken const& propertyName)
{
    if (propertyName == UsdGeomTokens->visibility)
        return HdChangeTracker::DirtyVisibility;

    if (propertyName == UsdGeomTokens->purpose)
        return HdChangeTracker::DirtyRenderTag;

    if (UsdGeomXformable::IsTransformationAffectedByAttrNamed(propertyName))
        return HdChangeTracker::DirtyTransform;

    if (propertyName == UsdGeomTokens->extent) 
        return HdChangeTracker::DirtyExtent;

    if (propertyName == UsdGeomTokens->doubleSided) 
        return HdChangeTracker::DirtyDoubleSided;

    if (propertyName == UsdGeomTokens->velocities ||
             propertyName == UsdGeomTokens->accelerations ||
             propertyName == UsdGeomTokens->motionNonlinearSampleCount ||
             propertyName == UsdGeomTokens->motionBlurScale)
        // XXX: "points" is handled by derived classes.
        return HdChangeTracker::DirtyPoints;

    if (UsdShadeMaterialBindingAPI::CanContainPropertyName(propertyName) ||
            UsdCollectionAPI::CanContainPropertyName(propertyName)) {
        return HdChangeTracker::DirtyMaterialId |
               HdChangeTracker::DirtyPrimvar;
    }
    
    // Note: This doesn't handle "built-in" attributes that are treated as
    // primvars. That responsibility falls on the child adapter.
    if (UsdGeomPrimvarsAPI::CanContainPropertyName(propertyName)) {
        return UsdImagingPrimAdapter::_ProcessPrefixedPrimvarPropertyChange(
                prim, cachePath, propertyName);
    }

    return HdChangeTracker::Clean;
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
UsdImagingGprimAdapter::MarkRenderTagDirty(UsdPrim const& prim,
                                           SdfPath const& cachePath,
                                           UsdImagingIndexProxy* index)
{
    index->MarkRprimDirty(cachePath, HdChangeTracker::DirtyRenderTag);
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
                                  UsdTimeCode time) const
{
    HD_TRACE_FUNCTION();
    HF_MALLOC_TAG_FUNCTION();

    // Previously, we issued a warning if the points attribute wasn't
    // authored, which resulted in a lot of logging.
    // Handle it silently instead by returning an empty array.
    return VtValue(_Get<VtVec3fArray>(prim, UsdGeomTokens->points, time));
}

/*virtual*/
GfRange3d 
UsdImagingGprimAdapter::GetExtent(UsdPrim const& prim, 
                                  SdfPath const& cachePath, 
                                  UsdTimeCode time) const
{
    HD_TRACE_FUNCTION();
    HF_MALLOC_TAG_FUNCTION();
    UsdGeomGprim gprim(prim);

    if (!TF_VERIFY(gprim)) {
        return GfRange3d();
    }

    VtVec3fArray extent;
    if (gprim.GetExtentAttr().Get(&extent, time) && extent.size() == 2) {
        // Note:
        // Usd stores extent as 2 float vecs. We do an implicit 
        // conversion to doubles
        return GfRange3d(extent[0], extent[1]);
    } else {
        // Return empty range if no value was found, or the wrong number of 
        // extent values were provided.        
        // Note: The default empty is [FLT_MAX,-FLT_MAX].
        // TODO: Should this compute the extent based on the points instead?
        return GfRange3d();
    }
}

/*virtual*/
bool
UsdImagingGprimAdapter::GetDoubleSided(UsdPrim const& prim, 
                                       SdfPath const& cachePath, 
                                       UsdTimeCode time) const
{
    HD_TRACE_FUNCTION();
    HF_MALLOC_TAG_FUNCTION();
    UsdGeomGprim gprim(prim);

    if (!TF_VERIFY(gprim)) {
        return false;
    }

    bool doubleSided = false;
    gprim.GetDoubleSidedAttr().Get(&doubleSided, time);
    return doubleSided;
}

/*virtual*/
SdfPath
UsdImagingGprimAdapter::GetMaterialId(UsdPrim const& prim, 
                                      SdfPath const& cachePath, 
                                      UsdTimeCode time) const
{
    return GetMaterialUsdPath(prim);
}

/*virtual*/
VtValue
UsdImagingGprimAdapter::Get(UsdPrim const& prim,
                            SdfPath const& cachePath,
                            TfToken const& key,
                            UsdTimeCode time,
                            VtIntArray *outIndices) const
{
    TRACE_FUNCTION();
    HF_MALLOC_TAG_FUNCTION();

    VtValue value;
    UsdGeomGprim gprim(prim);
    if (!TF_VERIFY(gprim)) {
        return value;
    }

    if (key == HdTokens->displayColor) {
        // First we try to obtain color from the prim, 
        // if not present, we try to get if through inheritance,
        // and lastly, we use a fallback value.
        TfToken interp;
        if (GetColor(prim, time, &interp, &value, outIndices)) {
            return value;
        } 

        // Inheritance
        UsdGeomPrimvar pv = _GetInheritedPrimvar(prim, HdTokens->displayColor);
        if (outIndices) {
            if (pv && pv.Get(&value, time)) {
                pv.GetIndices(outIndices, time);
                return value;
            }
        } else if (pv && pv.ComputeFlattened(&value, time)) {
            return value;
        }

        // Fallback
        VtVec3fArray vec(1, GfVec3f(.5,.5,.5));
        value = vec;
        return value;
    } else if (key == HdTokens->displayOpacity) {
        // First we try to obtain color from the prim, 
        // if not present, we try to get if through inheritance,
        // and lastly, we use a fallback value.
        TfToken interp;
        if (GetOpacity(prim, time, &interp, &value, outIndices)) {
            return value;
        }

        // Inheritance
        UsdGeomPrimvar pv = _GetInheritedPrimvar(prim, HdTokens->displayOpacity);
        if (outIndices) {
            if (pv && pv.Get(&value, time)) {
                pv.GetIndices(outIndices, time);
                return value;
            }
        } else if (pv && pv.ComputeFlattened(&value, time)) {
            return value;
        }

        // Fallback
        VtFloatArray vec(1, 1.0f);
        value = VtValue(vec);
        return value;

    } else if (key == HdTokens->normals) {
        // Fallback
        VtVec3fArray vec(1, GfVec3f(0,0,0));
        value = VtValue(vec);
        return value;

    } else if (key == HdTokens->widths) {
        // Fallback
        VtFloatArray vec(1, 1.0f);
        value = VtValue(vec);
        return value;

    } else if (key == HdTokens->points) {
        return GetPoints(prim, time);

    } else if (key == HdTokens->velocities) {
        UsdGeomPointBased pointBased(prim);
        VtVec3fArray velocities;
        if (pointBased.GetVelocitiesAttr() &&
            pointBased.GetVelocitiesAttr().Get(&velocities, time)) {
            return VtValue(velocities);
        }

    } else if (key == HdTokens->accelerations) {
        // Acceleration information is expected to be authored @ the same sample
        // rate as points data, so use the points dirty bit to let us know when
        // to publish accelerations.
        UsdGeomPointBased pointBased(prim);
        VtVec3fArray accelerations;
        if (pointBased.GetAccelerationsAttr() &&
            pointBased.GetAccelerationsAttr().Get(&accelerations, time)) {
            return VtValue(accelerations);
        }
    } else if (key == HdTokens->nonlinearSampleCount) {
        UsdImaging_NonlinearSampleCountCache *cache =
            _GetNonlinearSampleCountCache();
        const int value =
            cache->GetTime() == time
                ? cache->GetValue(prim)
                : UsdImaging_NonlinearSampleCountStrategy::
                      ComputeNonlinearSampleCount(prim,time);
        if (value !=
                UsdImaging_NonlinearSampleCountStrategy::invalidValue) {
            return VtValue(value);
        } else {
            // Default value from UsdGeom's
            // MotionAPI.motion:nonlinearSampleCount
            constexpr int defaultValue = 3;
            return VtValue(defaultValue);
        }
    } else if (key == HdTokens->blurScale) {
        UsdImaging_BlurScaleCache *cache = _GetBlurScaleCache();
        const UsdImaging_BlurScaleStrategy::value_type value =
            cache->GetTime() == time
                ? cache->GetValue(prim)
                : UsdImaging_BlurScaleStrategy::ComputeBlurScale(prim,time);
        if (value.has_value) {
            return VtValue(value.value);
        } else {
            // Default value from UsdGeom's
            // MotionAPI.motion:blurScale
            constexpr float defaultValue = 1.0f;
            return VtValue(defaultValue);
        }
    } else if (UsdGeomPrimvar pv = gprim.GetPrimvar(key)) {
        if (outIndices) {
            if (pv && pv.Get(&value, time)) {
                pv.GetIndices(outIndices, time);
                return value;
            }
        } else if (pv && pv.ComputeFlattened(&value, time)) {
            return value;
        }
    } else if (UsdGeomPrimvar pv = _GetInheritedPrimvar(prim, key)) {
        if (outIndices) {
            if (pv && pv.Get(&value, time)) {
                pv.GetIndices(outIndices, time);
                return value;
            }
        } else if (pv && pv.ComputeFlattened(&value, time)) {
            return value;
        }
    }

    return BaseAdapter::Get(prim, cachePath, key, time, outIndices);
}

// -------------------------------------------------------------------------- //

/* static */
bool
UsdImagingGprimAdapter::GetColor(UsdPrim const& prim,
                                 UsdTimeCode time,
                                 TfToken* interpolation,
                                 VtValue* color,
                                 VtIntArray *indices)
{
    TRACE_FUNCTION();
    HF_MALLOC_TAG_FUNCTION();

    VtVec3fArray result(1, GfVec3f(0.5f));
    VtIntArray colorIndices(0);
    TfToken colorInterp;
    bool hasAuthoredColor = false;

    // for a prim's color we use the following precedence:
    // material rel >  local prim var(s)
    {
        // -- Material --        
        // XXX: Primvar values that come from shaders should not be part of
        // the Rprim data, it should live as part of the shader so it can be 
        // shared, though that poses some interesting questions for vertex & 
        // varying rate shader provided primvars.
        UsdRelationship mat = 
            UsdShadeMaterialBindingAPI(prim).GetDirectBindingRel();
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
                    hasAuthoredColor = true;
                }
            }
        }
    }

    {
        // -- Prim local prim var --
        if (!hasAuthoredColor) { // did not get color from material
            UsdGeomGprim gprimSchema(prim);
            const UsdGeomPrimvar& primvar = 
                gprimSchema.GetDisplayColorPrimvar();
            colorInterp = primvar.GetInterpolation();

            if (indices) {
                if (primvar.Get(&result, time)) {
                    hasAuthoredColor = true;
                    primvar.GetIndices(&colorIndices, time);

                    if (colorInterp == UsdGeomTokens->constant &&
                        result.size() > 1) {
                        TF_WARN("Prim %s has %lu element(s) for %s even "
                                "though it is marked constant.",
                                prim.GetPath().GetText(), result.size(),
                                primvar.GetName().GetText());
                        result.resize(1);
                        colorIndices = VtIntArray(1, 0);
                    } 
                }
            } else if (primvar.ComputeFlattened(&result, time)) {
                hasAuthoredColor = true;

                if (colorInterp == UsdGeomTokens->constant &&
                    result.size() > 1) {
                    TF_WARN("Prim %s has %lu element(s) for %s even "
                            "though it is marked constant.",
                            prim.GetPath().GetText(), result.size(),
                            primvar.GetName().GetText());
                    result.resize(1);
                } 
            } else if (primvar.HasAuthoredValue()) {
                // If the primvar exists and ComputeFlattened returns false, 
                // the value authored is None, in which case, we return an empty
                // array.
                hasAuthoredColor = true;
                result = VtVec3fArray();
            } else {
                // All UsdGeomPointBased prims have the displayColor primvar
                // by default. Suppress unauthored ones from being
                // published to the backend.
            }
        }
    }

    if (!hasAuthoredColor) {
        return false;
    }

    if (interpolation) {
        *interpolation = colorInterp;
    }
    if (indices) {
        *indices = colorIndices;
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
                                   VtValue* opacity,
                                   VtIntArray *indices)
{
    HD_TRACE_FUNCTION();
    HF_MALLOC_TAG_FUNCTION();

    VtFloatArray result(1, 1.0f);
    VtIntArray opacityIndices(0);
    TfToken opacityInterp;
    bool hasAuthoredOpacity = false;

    // for a prim's opacity, we use the following precedence:
    // material rel >  local prim var(s)
    {
        // -- Material --        
        // XXX: Primvar values that come from shaders should not be part of
        // the Rprim data, it should live as part of the shader so it can be 
        // shared, though that poses some interesting questions for vertex & 
        // varying rate shader provided primvars.
        UsdRelationship mat = 
            UsdShadeMaterialBindingAPI(prim).GetDirectBindingRel();
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
                    hasAuthoredOpacity = true;
                }
            }
        }
    }

    {
        // -- Prim local prim var --
        if (!hasAuthoredOpacity) { // did not get opacity from material
            UsdGeomGprim gprimSchema(prim);
            const UsdGeomPrimvar& primvar = 
                gprimSchema.GetDisplayOpacityPrimvar();
            opacityInterp = primvar.GetInterpolation();
            
            if (indices) {
                if (primvar.Get(&result, time)) {
                    hasAuthoredOpacity = true;
                    primvar.GetIndices(&opacityIndices, time);

                    if (opacityInterp == UsdGeomTokens->constant &&
                        result.size() > 1) {
                        TF_WARN("Prim %s has %lu element(s) for %s even "
                                "though it is marked constant.",
                                prim.GetPath().GetText(), result.size(),
                                primvar.GetName().GetText());
                        result.resize(1);
                        opacityIndices = VtIntArray(1, 0);
                    } 
                }
            } else if (primvar.ComputeFlattened(&result, time)) {
                hasAuthoredOpacity = true;

                if (opacityInterp == UsdGeomTokens->constant &&
                    result.size() > 1) {
                    TF_WARN("Prim %s has %lu element(s) for %s even "
                            "though it is marked constant.",
                            prim.GetPath().GetText(), result.size(),
                            primvar.GetName().GetText());
                    result.resize(1);
                } 
            } else if (primvar.HasAuthoredValue()) {
                // If the primvar exists and ComputeFlattened returns false, 
                // the value authored is None, in which case, we return an empty
                // array,
                hasAuthoredOpacity = true;
                result = VtFloatArray();
            } else {
                // All UsdGeomPointBased prims have the displayOpacity primvar
                // by default. Suppress unauthored ones from being
                // published to the backend.
            }
        }
    }

    if (!hasAuthoredOpacity) {
        return false;
    }

    if (interpolation) {
        *interpolation = opacityInterp;
    }
    if (indices) {
        *indices = opacityIndices;
    }
    if (opacity) {
        *opacity = VtValue(result);
    }
    return true;
}

TfTokenVector
UsdImagingGprimAdapter::_CollectMaterialPrimvars(
    SdfPathVector const& materialUsdPaths, 
    UsdTimeCode time) const
{
    TfTokenVector primvars;

    for (SdfPath const& materialUsdPath : materialUsdPaths) {
        if (UsdPrim matPrim = _GetPrim(materialUsdPath)) {
            // NOTE: We need to directly access the registered instance
            //       of UsdImagingMaterialAdapter in order to query its
            //       material resource. Those are registered to match
            //       the USD prim type name.
            if (UsdImagingPrimAdapterSharedPtr materialAdapter
                    =_GetAdapter(matPrim.GetTypeName())) {
                VtValue vtMaterial = materialAdapter->GetMaterialResource(
                    matPrim, matPrim.GetPath(), time);
                TfTokenVector pvNames = _GetPrimvarsForMaterial(vtMaterial);
                primvars.insert(primvars.end(), pvNames.begin(), 
                    pvNames.end());
            }
         }
    }

    std::sort(primvars.begin(), primvars.end());
    primvars.erase(std::unique(primvars.begin(), primvars.end()),
        primvars.end());

    return primvars;
}

TfTokenVector const&
UsdImagingGprimAdapter::_GetRprimPrimvarNames() const
{
    static TfTokenVector primvarNames;
    return primvarNames;
}


PXR_NAMESPACE_CLOSE_SCOPE

