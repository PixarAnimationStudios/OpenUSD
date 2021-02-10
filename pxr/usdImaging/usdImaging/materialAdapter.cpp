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
#include "pxr/usdImaging/usdImaging/materialAdapter.h"
#include "pxr/usdImaging/usdImaging/delegate.h"
#include "pxr/usdImaging/usdImaging/indexProxy.h"
#include "pxr/usdImaging/usdImaging/tokens.h"
#include "pxr/usdImaging/usdImaging/materialParamUtils.h"

#include "pxr/imaging/hd/material.h"
#include "pxr/imaging/hd/perfLog.h"

#include "pxr/usd/usdShade/material.h"
#include "pxr/usd/usdShade/shader.h"

#include "pxr/usd/ar/resolverScopedCache.h"
#include "pxr/usd/ar/resolverContextBinder.h"

PXR_NAMESPACE_OPEN_SCOPE

TF_REGISTRY_FUNCTION(TfType)
{
    typedef UsdImagingMaterialAdapter Adapter;
    TfType t = TfType::Define<Adapter, TfType::Bases<Adapter::BaseAdapter> >();
    t.SetFactory< UsdImagingPrimAdapterFactory<Adapter> >();
}

UsdImagingMaterialAdapter::~UsdImagingMaterialAdapter()
{
}

bool
UsdImagingMaterialAdapter::IsSupported(UsdImagingIndexProxy const* index) const
{
    return index->IsSprimTypeSupported(HdPrimTypeTokens->material);
}

SdfPath
UsdImagingMaterialAdapter::Populate(
    UsdPrim const& prim,
    UsdImagingIndexProxy* index,
    UsdImagingInstancerContext const* instancerContext)
{
    HD_TRACE_FUNCTION();
    HF_MALLOC_TAG_FUNCTION();
    // Since material are populated by reference, they need to take care not to
    // be populated multiple times.
    SdfPath cachePath = prim.GetPath();
    if (index->IsPopulated(cachePath)) {
        return cachePath;
    }

    UsdShadeMaterial material(prim);
    if (!material) return SdfPath::EmptyPath();

    // Skip materials that do not match renderDelegate supported types.
    // XXX We can further improve filtering by combining the below descendants
    // gather and validate the Sdr node types are supported by render delegate.
    const TfToken context = _GetMaterialNetworkSelector();
    UsdShadeShader surface = material.ComputeSurfaceSource(context);
    UsdShadeShader volume = material.ComputeVolumeSource(context);
    if (!surface && !volume) return SdfPath::EmptyPath();

    index->InsertSprim(HdPrimTypeTokens->material,
                       cachePath,
                       prim, shared_from_this());
    HD_PERF_COUNTER_INCR(UsdImagingTokens->usdPopulatedPrimCount);

    // Also register dependencies on behalf of any descendent
    // UsdShadeShader prims, since they are consumed to
    // create the material network.
    for (UsdPrim const& child: prim.GetDescendants()) {
        if (child.IsA<UsdShadeShader>()) {
            index->AddDependency(cachePath, child);
        }
    }

    return prim.GetPath();
}

/* virtual */
void
UsdImagingMaterialAdapter::TrackVariability(
    UsdPrim const& prim,
    SdfPath const& cachePath,
    HdDirtyBits* timeVaryingBits,
    UsdImagingInstancerContext const*
    instancerContext) const
{
    UsdShadeMaterial material(prim);
    if (!material) {
        TF_RUNTIME_ERROR("Expected material prim at <%s> to be of type "
                         "'UsdShadeMaterial', not type '%s'; ignoring",
                         prim.GetPath().GetText(),
                         prim.GetTypeName().GetText());
        return;
    }

    const TfToken context = _GetMaterialNetworkSelector();

    if (UsdShadeShader s = material.ComputeSurfaceSource(context)) {
        if (UsdImaging_IsHdMaterialNetworkTimeVarying(s.GetPrim())) {
            *timeVaryingBits |= HdMaterial::DirtyResource;
            return;
        }
    }

    if (UsdShadeShader d = material.ComputeDisplacementSource(context)) {
        if (UsdImaging_IsHdMaterialNetworkTimeVarying(d.GetPrim())) {
            *timeVaryingBits |= HdMaterial::DirtyResource;
            return;
        }
    }

    if (UsdShadeShader v = material.ComputeVolumeSource(context)) {
        if (UsdImaging_IsHdMaterialNetworkTimeVarying(v.GetPrim())) {
            *timeVaryingBits |= HdMaterial::DirtyResource;
            return;
        }
    }
}

/* virtual */
void
UsdImagingMaterialAdapter::UpdateForTime(
    UsdPrim const& prim,
    SdfPath const& cachePath,
    UsdTimeCode time,
    HdDirtyBits requestedBits,
    UsdImagingInstancerContext const*
    instancerContext) const
{
}

/* virtual */
HdDirtyBits
UsdImagingMaterialAdapter::ProcessPropertyChange(
    UsdPrim const& prim,
    SdfPath const& cachePath,
    TfToken const& propertyName)
{
    if (propertyName == UsdGeomTokens->visibility) {
        // Materials aren't affected by visibility
        return HdChangeTracker::Clean;
    }

    // The only meaningful change is to dirty the computed resource,
    // an HdMaterialNetwork.
    return HdMaterial::DirtyResource;
}

/* virtual */
void
UsdImagingMaterialAdapter::MarkDirty(
    UsdPrim const& prim,
    SdfPath const& cachePath,
    HdDirtyBits dirty,
    UsdImagingIndexProxy* index)
{
    // If this is invoked on behalf of a Shader prim underneath a
    // Material prim, walk up to the enclosing Material.
    SdfPath materialCachePath = cachePath;
    UsdPrim materialPrim = prim;
    while (materialPrim && !materialPrim.IsA<UsdShadeMaterial>()) {
        materialPrim = materialPrim.GetParent();
        materialCachePath = materialCachePath.GetParentPath();
    }
    if (!TF_VERIFY(materialPrim)) {
        return;
    }

    index->MarkSprimDirty(materialCachePath, dirty);
}


/* virtual */
void
UsdImagingMaterialAdapter::MarkMaterialDirty(
    UsdPrim const& prim,
    SdfPath const& cachePath,
    UsdImagingIndexProxy* index)
{
    MarkDirty(prim, cachePath, HdMaterial::DirtyResource, index);
}

/* virtual */
void
UsdImagingMaterialAdapter::_RemovePrim(
    SdfPath const& cachePath,
    UsdImagingIndexProxy* index)
{
    index->RemoveSprim(HdPrimTypeTokens->material, cachePath);
}

VtValue 
UsdImagingMaterialAdapter::GetMaterialResource(UsdPrim const &prim,
                                               SdfPath const& cachePath, 
                                               UsdTimeCode time) const
{
    UsdShadeMaterial material(prim);
    if (!material) {
        TF_RUNTIME_ERROR("Expected material prim at <%s> to be of type "
                         "'UsdShadeMaterial', not type '%s'; ignoring",
                         prim.GetPath().GetText(),
                         prim.GetTypeName().GetText());
        return VtValue();
    }

    // Bind the usd stage's resolver context for correct asset resolution.
    ArResolverContextBinder binder(prim.GetStage()->GetPathResolverContext());
    ArResolverScopedCache resolverCache;

    HdMaterialNetworkMap networkMap;

    const TfToken context = _GetMaterialNetworkSelector();
    TfTokenVector shaderSourceTypes = _GetShaderSourceTypes();

    if (UsdShadeShader s = material.ComputeSurfaceSource(context)) {
        UsdImaging_BuildHdMaterialNetworkFromTerminal(
            s.GetPrim(), 
            HdMaterialTerminalTokens->surface,
            shaderSourceTypes,
            &networkMap,
            time);
    }

    if (UsdShadeShader d = material.ComputeDisplacementSource(context)) {
        UsdImaging_BuildHdMaterialNetworkFromTerminal(
            d.GetPrim(),
            HdMaterialTerminalTokens->displacement,
            shaderSourceTypes,
            &networkMap,
            time);
    }

    if (UsdShadeShader v = material.ComputeVolumeSource(context)) {
        UsdImaging_BuildHdMaterialNetworkFromTerminal(
            v.GetPrim(),
            HdMaterialTerminalTokens->volume,
            shaderSourceTypes,
            &networkMap,
            time);
    }

    return VtValue(networkMap);
}


PXR_NAMESPACE_CLOSE_SCOPE
