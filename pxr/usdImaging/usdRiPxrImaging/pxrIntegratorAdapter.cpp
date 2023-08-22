//
// Copyright 2023 Pixar
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
#include "pxr/usdImaging/usdRiPxrImaging/pxrIntegratorAdapter.h"
#include "pxr/usdImaging/usdRiPxrImaging/pxrRenderTerminalHelper.h"
#include "pxr/usdImaging/usdRiPxrImaging/dataSourcePxrRenderTerminalPrims.h"
#include "pxr/usdImaging/usdImaging/delegate.h"
#include "pxr/usdImaging/usdImaging/indexProxy.h"
#include "pxr/usdImaging/usdImaging/tokens.h"

#include "pxr/imaging/hd/integratorSchema.h"
#include "pxr/imaging/hd/material.h"
#include "pxr/imaging/hd/tokens.h"
#include "pxr/base/gf/vec4f.h"

PXR_NAMESPACE_OPEN_SCOPE

TF_DEFINE_PRIVATE_TOKENS(
    _tokens,
    ((riIntegratorShaderId, "ri:integrator:shaderId"))
);


TF_REGISTRY_FUNCTION(TfType)
{
    using Adapter = UsdRiPxrImagingIntegratorAdapter;
    TfType t = TfType::Define<Adapter, TfType::Bases<Adapter::BaseAdapter> >();
    t.SetFactory< UsdImagingPrimAdapterFactory<Adapter> >();
}

UsdRiPxrImagingIntegratorAdapter::~UsdRiPxrImagingIntegratorAdapter() = default;

// -------------------------------------------------------------------------- //
// 2.0 Prim adapter API
// -------------------------------------------------------------------------- //

TfTokenVector
UsdRiPxrImagingIntegratorAdapter::GetImagingSubprims(UsdPrim const& prim)
{
    return { TfToken() };
}

TfToken
UsdRiPxrImagingIntegratorAdapter::GetImagingSubprimType(
    UsdPrim const& prim,
    TfToken const& subprim)
{
    if (subprim.IsEmpty()) {
        return HdPrimTypeTokens->integrator;
    }
    return TfToken();
}

HdContainerDataSourceHandle
UsdRiPxrImagingIntegratorAdapter::GetImagingSubprimData(
    UsdPrim const& prim,
    TfToken const& subprim,
    const UsdImagingDataSourceStageGlobals &stageGlobals)
{
    if (subprim.IsEmpty()) {
        return 
            UsdRiPxrImaging_DataSourceRenderTerminalPrim<HdIntegratorSchema>::New(
                prim.GetPath(), prim,
                _tokens->riIntegratorShaderId, stageGlobals);
    }

    return nullptr;
}

HdDataSourceLocatorSet
UsdRiPxrImagingIntegratorAdapter::InvalidateImagingSubprim(
    UsdPrim const& prim,
    TfToken const& subprim,
    TfTokenVector const& properties,
    const UsdImagingPropertyInvalidationType invalidationType)
{
    if (subprim.IsEmpty()) {
        return 
            UsdRiPxrImaging_DataSourceRenderTerminalPrim<HdIntegratorSchema>::
            Invalidate(
                prim, subprim, properties, invalidationType);
    }

    return HdDataSourceLocatorSet();
}

// -------------------------------------------------------------------------- //
// 1.0 Prim adapter API
// -------------------------------------------------------------------------- //

bool
UsdRiPxrImagingIntegratorAdapter::IsSupported(
    UsdImagingIndexProxy const* index) const
{
    bool supported = index->IsSprimTypeSupported(HdPrimTypeTokens->integrator);
    return supported;
}

SdfPath
UsdRiPxrImagingIntegratorAdapter::Populate(
    UsdPrim const& prim, 
    UsdImagingIndexProxy* index,
    UsdImagingInstancerContext const* instancerContext)
{
    SdfPath cachePath = prim.GetPath();
    if (index->IsPopulated(cachePath)) {
        return cachePath;
    }

    index->InsertSprim(HdPrimTypeTokens->integrator, cachePath, prim);
    HD_PERF_COUNTER_INCR(UsdImagingTokens->usdPopulatedPrimCount);

    return cachePath;
}

void
UsdRiPxrImagingIntegratorAdapter::_RemovePrim(
    SdfPath const& cachePath,
    UsdImagingIndexProxy* index)
{
    index->RemoveSprim(HdPrimTypeTokens->integrator, cachePath);
}

void 
UsdRiPxrImagingIntegratorAdapter::TrackVariability(
    UsdPrim const& prim,
    SdfPath const& cachePath,
    HdDirtyBits* timeVaryingBits,
    UsdImagingInstancerContext const* instancerContext) const
{
    // If any of the Integrator attributes are time varying 
    // we will assume all Integrator params are time-varying.
    const std::vector<UsdAttribute> &attrs = prim.GetAttributes();
    TF_FOR_ALL(attrIter, attrs) {
        const UsdAttribute& attr = *attrIter;
        if (attr.ValueMightBeTimeVarying()) {
            *timeVaryingBits |= HdChangeTracker::DirtyParams;
        }
    }
}

// Thread safe.
//  * Populate dirty bits for the given \p time.
void 
UsdRiPxrImagingIntegratorAdapter::UpdateForTime(
    UsdPrim const& prim,
    SdfPath const& cachePath, 
    UsdTimeCode time,
    HdDirtyBits requestedBits,
    UsdImagingInstancerContext const* 
    instancerContext) const
{
}

HdDirtyBits
UsdRiPxrImagingIntegratorAdapter::ProcessPropertyChange(
    UsdPrim const& prim,
    SdfPath const& cachePath, 
    TfToken const& propertyName)
{
    return HdChangeTracker::AllDirty;
}

void
UsdRiPxrImagingIntegratorAdapter::MarkDirty(
    UsdPrim const& prim,
    SdfPath const& cachePath,
    HdDirtyBits dirty,
    UsdImagingIndexProxy* index)
{
    index->MarkSprimDirty(cachePath, dirty);
}


VtValue
UsdRiPxrImagingIntegratorAdapter::Get(
    UsdPrim const& prim,
    SdfPath const& cachePath,
    TfToken const& key,
    UsdTimeCode time,
    VtIntArray *outIndices) const
{
    if (key == HdIntegratorSchemaTokens->resource) {
        return VtValue(
            UsdRiPxrImagingRenderTerminalHelper::CreateHdMaterialNode2(
                prim,
                _tokens->riIntegratorShaderId,
                HdPrimTypeTokens->integrator));
    }

    TF_CODING_ERROR(
        "Property %s not supported for Integrator by UsdImaging, path: %s",
        key.GetText(), cachePath.GetText());
    return VtValue();
}

PXR_NAMESPACE_CLOSE_SCOPE
