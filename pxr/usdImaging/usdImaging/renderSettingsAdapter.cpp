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
#include "pxr/usdImaging/usdImaging/renderSettingsAdapter.h"
#include "pxr/usdImaging/usdImaging/delegate.h"
#include "pxr/usdImaging/usdImaging/indexProxy.h"
#include "pxr/usdImaging/usdImaging/tokens.h"

#include "pxr/imaging/hd/tokens.h"
#include "pxr/base/gf/vec4f.h"

PXR_NAMESPACE_OPEN_SCOPE

TF_DEFINE_PRIVATE_TOKENS(
    _tokens,
    ((outputsRiSampleFilters, "outputs:ri:sampleFilters"))
);


TF_REGISTRY_FUNCTION(TfType)
{
    typedef UsdImagingRenderSettingsAdapter Adapter;
    TfType t = TfType::Define<Adapter, TfType::Bases<Adapter::BaseAdapter> >();
    t.SetFactory< UsdImagingPrimAdapterFactory<Adapter> >();
}

UsdImagingRenderSettingsAdapter::~UsdImagingRenderSettingsAdapter() 
{
}

bool
UsdImagingRenderSettingsAdapter::IsSupported(
    UsdImagingIndexProxy const* index) const
{
    bool supported = index->IsBprimTypeSupported(HdPrimTypeTokens->renderSettings);
    return supported;
}

SdfPath
UsdImagingRenderSettingsAdapter::Populate(
    UsdPrim const& prim, 
    UsdImagingIndexProxy* index,
    UsdImagingInstancerContext const* instancerContext)
{
    index->InsertBprim(HdPrimTypeTokens->renderSettings, prim.GetPath(), prim);
    HD_PERF_COUNTER_INCR(UsdImagingTokens->usdPopulatedPrimCount);

    // Check for Sample Filter Connections
    SdfPathVector connections;
    prim.GetAttribute(_tokens->outputsRiSampleFilters)
        .GetConnections(&connections);
    for (auto const& connPath : connections) {
        const UsdPrim &connPrim = prim.GetStage()->GetPrimAtPath(
            connPath.GetPrimPath());
        UsdImagingPrimAdapterSharedPtr adapter = _GetPrimAdapter(connPrim);
        if (adapter) {
            index->AddDependency(prim.GetPath(), connPrim);
            adapter->Populate(connPrim, index, nullptr);
        }
    }

    return prim.GetPath();
}

void
UsdImagingRenderSettingsAdapter::_RemovePrim(
    SdfPath const& cachePath,
    UsdImagingIndexProxy* index)
{
    index->RemoveBprim(HdPrimTypeTokens->renderSettings, cachePath);
}

void 
UsdImagingRenderSettingsAdapter::TrackVariability(
    UsdPrim const& prim,
    SdfPath const& cachePath,
    HdDirtyBits* timeVaryingBits,
    UsdImagingInstancerContext const* instancerContext) const
{
    // If any of the RenderSettings attributes are time varying 
    // we will assume all RenderSetting params are time-varying.
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
UsdImagingRenderSettingsAdapter::UpdateForTime(
    UsdPrim const& prim,
    SdfPath const& cachePath, 
    UsdTimeCode time,
    HdDirtyBits requestedBits,
    UsdImagingInstancerContext const* 
    instancerContext) const
{
}

HdDirtyBits
UsdImagingRenderSettingsAdapter::ProcessPropertyChange(
    UsdPrim const& prim,
    SdfPath const& cachePath, 
    TfToken const& propertyName)
{
    return HdChangeTracker::AllDirty;
}

void
UsdImagingRenderSettingsAdapter::MarkDirty(
    UsdPrim const& prim,
    SdfPath const& cachePath,
    HdDirtyBits dirty,
    UsdImagingIndexProxy* index)
{
    index->MarkBprimDirty(cachePath, dirty);
}

VtValue
UsdImagingRenderSettingsAdapter::Get(
    UsdPrim const& prim,
    SdfPath const& cachePath,
    TfToken const& key,
    UsdTimeCode time,
    VtIntArray *outIndices) const
{
    if (prim.HasAttribute(key)) {
        UsdAttribute const &attr = prim.GetAttribute(key);

        // Only return authored attribute values and UsdShadeConnectableAPI
        // connections
        VtValue value;
        if (attr.HasAuthoredValue() && attr.Get(&value, time)) {
            return value;
        }
        if (UsdShadeOutput::IsOutput(attr)) {
            UsdShadeAttributeVector targets =
                UsdShadeUtils::GetValueProducingAttributes(
                    UsdShadeOutput(attr));
            SdfPathVector outputs;
            for (auto const& output : targets) {
                outputs.push_back(output.GetPrimPath());
            }
            return VtValue(outputs);
        }
        return value;
    }

    TF_CODING_ERROR(
        "Property %s not supported for RenderSettings by UsdImaging, path: %s",
        key.GetText(), cachePath.GetText());
    return VtValue();
}

PXR_NAMESPACE_CLOSE_SCOPE
