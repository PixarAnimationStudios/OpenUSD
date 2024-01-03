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
#include "pxr/usdImaging/usdRiPxrImaging/pxrDisplayFilterAdapter.h"
#include "pxr/usdImaging/usdRiPxrImaging/pxrRenderTerminalHelper.h"
#include "pxr/usdImaging/usdRiPxrImaging/dataSourcePxrRenderTerminalPrims.h"
#include "pxr/usdImaging/usdImaging/delegate.h"
#include "pxr/usdImaging/usdImaging/indexProxy.h"
#include "pxr/usdImaging/usdImaging/tokens.h"

#include "pxr/imaging/hd/displayFilterSchema.h"
#include "pxr/imaging/hd/material.h"
#include "pxr/imaging/hd/tokens.h"
#include "pxr/base/gf/vec4f.h"

PXR_NAMESPACE_OPEN_SCOPE

TF_DEFINE_PRIVATE_TOKENS(
    _tokens,
    ((riDisplayFilterShaderId, "ri:displayFilter:shaderId"))
);


TF_REGISTRY_FUNCTION(TfType)
{
    using Adapter = UsdRiPxrImagingDisplayFilterAdapter;
    TfType t = TfType::Define<Adapter, TfType::Bases<Adapter::BaseAdapter> >();
    t.SetFactory< UsdImagingPrimAdapterFactory<Adapter> >();
}

UsdRiPxrImagingDisplayFilterAdapter::
~UsdRiPxrImagingDisplayFilterAdapter() = default;


// -------------------------------------------------------------------------- //
// 2.0 Prim adapter API
// -------------------------------------------------------------------------- //

TfTokenVector
UsdRiPxrImagingDisplayFilterAdapter::GetImagingSubprims(UsdPrim const& prim)
{
    return { TfToken() };
}

TfToken
UsdRiPxrImagingDisplayFilterAdapter::GetImagingSubprimType(
    UsdPrim const& prim,
    TfToken const& subprim)
{
    if (subprim.IsEmpty()) {
        return HdPrimTypeTokens->displayFilter;
    }
    return TfToken();
}

HdContainerDataSourceHandle
UsdRiPxrImagingDisplayFilterAdapter::GetImagingSubprimData(
    UsdPrim const& prim,
    TfToken const& subprim,
    const UsdImagingDataSourceStageGlobals &stageGlobals)
{
    if (subprim.IsEmpty()) {
        return 
            UsdRiPxrImaging_DataSourceRenderTerminalPrim<HdDisplayFilterSchema>::
                New(prim.GetPath(), prim,
                    _tokens->riDisplayFilterShaderId, stageGlobals);
    }

    return nullptr;
}

HdDataSourceLocatorSet
UsdRiPxrImagingDisplayFilterAdapter::InvalidateImagingSubprim(
    UsdPrim const& prim,
    TfToken const& subprim,
    TfTokenVector const& properties,
    const UsdImagingPropertyInvalidationType invalidationType)
{
    if (subprim.IsEmpty()) {
        return
            UsdRiPxrImaging_DataSourceRenderTerminalPrim<HdDisplayFilterSchema>::
            Invalidate(
                prim, subprim, properties, invalidationType);
    }

    return HdDataSourceLocatorSet();
}

// -------------------------------------------------------------------------- //
// 1.0 Prim adapter API
// -------------------------------------------------------------------------- //

bool
UsdRiPxrImagingDisplayFilterAdapter::IsSupported(
    UsdImagingIndexProxy const* index) const
{
    bool supported = index->IsSprimTypeSupported(HdPrimTypeTokens->displayFilter);
    return supported;
}

SdfPath
UsdRiPxrImagingDisplayFilterAdapter::Populate(
    UsdPrim const& prim, 
    UsdImagingIndexProxy* index,
    UsdImagingInstancerContext const* instancerContext)
{
    SdfPath cachePath = prim.GetPath();
    if (index->IsPopulated(cachePath)) {
        return cachePath;
    }

    index->InsertSprim(HdPrimTypeTokens->displayFilter, cachePath, prim);
    HD_PERF_COUNTER_INCR(UsdImagingTokens->usdPopulatedPrimCount);

    return cachePath;
}

void
UsdRiPxrImagingDisplayFilterAdapter::_RemovePrim(
    SdfPath const& cachePath,
    UsdImagingIndexProxy* index)
{
    index->RemoveSprim(HdPrimTypeTokens->displayFilter, cachePath);
}

void 
UsdRiPxrImagingDisplayFilterAdapter::TrackVariability(
    UsdPrim const& prim,
    SdfPath const& cachePath,
    HdDirtyBits* timeVaryingBits,
    UsdImagingInstancerContext const* instancerContext) const
{
    // If any of the DisplayFilter attributes are time varying 
    // we will assume all DisplayFilter params are time-varying.
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
UsdRiPxrImagingDisplayFilterAdapter::UpdateForTime(
    UsdPrim const& prim,
    SdfPath const& cachePath, 
    UsdTimeCode time,
    HdDirtyBits requestedBits,
    UsdImagingInstancerContext const* 
    instancerContext) const
{
}

HdDirtyBits
UsdRiPxrImagingDisplayFilterAdapter::ProcessPropertyChange(
    UsdPrim const& prim,
    SdfPath const& cachePath, 
    TfToken const& propertyName)
{
    return HdChangeTracker::AllDirty;
}

void
UsdRiPxrImagingDisplayFilterAdapter::MarkDirty(
    UsdPrim const& prim,
    SdfPath const& cachePath,
    HdDirtyBits dirty,
    UsdImagingIndexProxy* index)
{
    index->MarkSprimDirty(cachePath, dirty);
}

VtValue
UsdRiPxrImagingDisplayFilterAdapter::Get(
    UsdPrim const& prim,
    SdfPath const& cachePath,
    TfToken const& key,
    UsdTimeCode time,
    VtIntArray *outIndices) const
{
    if (key == HdDisplayFilterSchemaTokens->resource) {
        return VtValue(
            UsdRiPxrImagingRenderTerminalHelper::CreateHdMaterialNode2(
                prim,
                _tokens->riDisplayFilterShaderId,
                HdPrimTypeTokens->displayFilter));
    }

    TF_CODING_ERROR(
        "Property %s not supported for DisplayFilter by UsdImaging, path: %s",
        key.GetText(), cachePath.GetText());
    return VtValue();
}

PXR_NAMESPACE_CLOSE_SCOPE
