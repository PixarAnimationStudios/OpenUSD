//
// Copyright 2022 Pixar
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
#include "pxr/usdImaging/usdRiImaging/pxrSampleFilterAdapter.h"
#include "pxr/usdImaging/usdRiImaging/pxrRenderTerminalHelper.h"
#include "pxr/usdImaging/usdRiImaging/dataSourcePxrRenderTerminalPrims.h"
#include "pxr/usdImaging/usdImaging/delegate.h"
#include "pxr/usdImaging/usdImaging/indexProxy.h"
#include "pxr/usdImaging/usdImaging/tokens.h"

#include "pxr/imaging/hd/material.h"
#include "pxr/imaging/hd/sampleFilterSchema.h"
#include "pxr/imaging/hd/tokens.h"
#include "pxr/base/gf/vec4f.h"

PXR_NAMESPACE_OPEN_SCOPE

TF_DEFINE_PRIVATE_TOKENS(
    _tokens,
    ((riSampleFilterShaderId, "ri:sampleFilter:shaderId"))
);


TF_REGISTRY_FUNCTION(TfType)
{
    using Adapter = UsdRiImagingPxrSampleFilterAdapter;
    TfType t = TfType::Define<Adapter, TfType::Bases<Adapter::BaseAdapter> >();
    t.SetFactory< UsdImagingPrimAdapterFactory<Adapter> >();
}

UsdRiImagingPxrSampleFilterAdapter::
~UsdRiImagingPxrSampleFilterAdapter() = default;


// -------------------------------------------------------------------------- //
// 2.0 Prim adapter API
// -------------------------------------------------------------------------- //

TfTokenVector
UsdRiImagingPxrSampleFilterAdapter::GetImagingSubprims(UsdPrim const& prim)
{
    return { TfToken() };
}

TfToken
UsdRiImagingPxrSampleFilterAdapter::GetImagingSubprimType(
    UsdPrim const& prim,
    TfToken const& subprim)
{
    if (subprim.IsEmpty()) {
        return HdPrimTypeTokens->sampleFilter;
    }
    return TfToken();
}

HdContainerDataSourceHandle
UsdRiImagingPxrSampleFilterAdapter::GetImagingSubprimData(
    UsdPrim const& prim,
    TfToken const& subprim,
    const UsdImagingDataSourceStageGlobals &stageGlobals)
{
    if (subprim.IsEmpty()) {
        return 
            UsdRiImaging_DataSourceRenderTerminalPrim<HdSampleFilterSchema>::
                New(prim.GetPath(), prim,
                    _tokens->riSampleFilterShaderId, stageGlobals);
    }

    return nullptr;
}

HdDataSourceLocatorSet
UsdRiImagingPxrSampleFilterAdapter::InvalidateImagingSubprim(
    UsdPrim const& prim,
    TfToken const& subprim,
    TfTokenVector const& properties,
    const UsdImagingPropertyInvalidationType invalidationType)
{
    if (subprim.IsEmpty()) {
        return 
            UsdRiImaging_DataSourceRenderTerminalPrim<HdSampleFilterSchema>::
            Invalidate(
                prim, subprim, properties, invalidationType);
    }

    return HdDataSourceLocatorSet();
}

// -------------------------------------------------------------------------- //
// 1.0 Prim adapter API
// -------------------------------------------------------------------------- //

bool
UsdRiImagingPxrSampleFilterAdapter::IsSupported(
    UsdImagingIndexProxy const* index) const
{
    bool supported = index->IsSprimTypeSupported(HdPrimTypeTokens->sampleFilter);
    return supported;
}

SdfPath
UsdRiImagingPxrSampleFilterAdapter::Populate(
    UsdPrim const& prim, 
    UsdImagingIndexProxy* index,
    UsdImagingInstancerContext const* instancerContext)
{
    SdfPath cachePath = prim.GetPath();
    if (index->IsPopulated(cachePath)) {
        return cachePath;
    }

    index->InsertSprim(HdPrimTypeTokens->sampleFilter, cachePath, prim);
    HD_PERF_COUNTER_INCR(UsdImagingTokens->usdPopulatedPrimCount);

    return cachePath;
}

void
UsdRiImagingPxrSampleFilterAdapter::_RemovePrim(
    SdfPath const& cachePath,
    UsdImagingIndexProxy* index)
{
    index->RemoveSprim(HdPrimTypeTokens->sampleFilter, cachePath);
}

void 
UsdRiImagingPxrSampleFilterAdapter::TrackVariability(
    UsdPrim const& prim,
    SdfPath const& cachePath,
    HdDirtyBits* timeVaryingBits,
    UsdImagingInstancerContext const* instancerContext) const
{
    // If any of the SampleFilter attributes are time varying 
    // we will assume all SampleFilter params are time-varying.
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
UsdRiImagingPxrSampleFilterAdapter::UpdateForTime(
    UsdPrim const& prim,
    SdfPath const& cachePath, 
    UsdTimeCode time,
    HdDirtyBits requestedBits,
    UsdImagingInstancerContext const* 
    instancerContext) const
{
}

HdDirtyBits
UsdRiImagingPxrSampleFilterAdapter::ProcessPropertyChange(
    UsdPrim const& prim,
    SdfPath const& cachePath, 
    TfToken const& propertyName)
{
    return HdChangeTracker::AllDirty;
}

void
UsdRiImagingPxrSampleFilterAdapter::MarkDirty(
    UsdPrim const& prim,
    SdfPath const& cachePath,
    HdDirtyBits dirty,
    UsdImagingIndexProxy* index)
{
    index->MarkSprimDirty(cachePath, dirty);
}

VtValue
UsdRiImagingPxrSampleFilterAdapter::Get(
    UsdPrim const& prim,
    SdfPath const& cachePath,
    TfToken const& key,
    UsdTimeCode time,
    VtIntArray *outIndices) const
{
    if (key == HdSampleFilterSchemaTokens->resource) {
        return VtValue(
            UsdRiImagingPxrRenderTerminalHelper::CreateHdMaterialNode2(
                prim,
                _tokens->riSampleFilterShaderId,
                HdPrimTypeTokens->sampleFilter));
    }

    TF_CODING_ERROR(
        "Property %s not supported for SampleFilter by UsdImaging, path: %s",
        key.GetText(), cachePath.GetText());
    return VtValue();
}

PXR_NAMESPACE_CLOSE_SCOPE
