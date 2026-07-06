//
// Copyright 2024 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "usdRiPxrImaging/pxrEnergyFilterAdapter.h"

#include "usdRiPxrImaging/dataSourcePxrRenderTerminalPrims.h"
#include "usdRiPxrImaging/pxrRenderTerminalHelper.h"

#include "pxr/imaging/hd/energyFilterSchema.h"
#include "pxr/imaging/hd/overlayContainerDataSource.h"
#include "pxr/imaging/hd/retainedDataSource.h"
#include "pxr/imaging/hd/tokens.h"

#include "pxr/usdImaging/usdImaging/delegate.h"
#include "pxr/usdImaging/usdImaging/indexProxy.h"
#include "pxr/usdImaging/usdImaging/tokens.h"

#include "pxr/base/gf/vec4f.h"

PXR_NAMESPACE_OPEN_SCOPE

TF_DEFINE_PRIVATE_TOKENS(
    _tokens,
    ((riEnergyFilterShaderId,  "ri:energyFilter:shaderId"))
    ((riEnergyFilterEnabled,   "ri:energyFilter:enabled"))
    ((riEnergyFilterLpe,       "ri:energyFilter:lpe"))
    ((riEnergyFilterOrder,     "ri:energyFilter:order"))
);


TF_REGISTRY_FUNCTION(TfType)
{
    using Adapter = UsdRiPxrImagingEnergyFilterAdapter;
    TfType t = TfType::Define<Adapter, TfType::Bases<Adapter::BaseAdapter> >();
    t.SetFactory< UsdImagingPrimAdapterFactory<Adapter> >();
}

UsdRiPxrImagingEnergyFilterAdapter::
~UsdRiPxrImagingEnergyFilterAdapter() = default;


// -------------------------------------------------------------------------- //
// 2.0 Prim adapter API
// -------------------------------------------------------------------------- //

TfTokenVector
UsdRiPxrImagingEnergyFilterAdapter::GetImagingSubprims(UsdPrim const& prim)
{
    return { TfToken() };
}

TfToken
UsdRiPxrImagingEnergyFilterAdapter::GetImagingSubprimType(
    UsdPrim const& prim,
    TfToken const& subprim)
{
    if (subprim.IsEmpty()) {
        return HdPrimTypeTokens->energyFilter;
    }
    return TfToken();
}

HdContainerDataSourceHandle
UsdRiPxrImagingEnergyFilterAdapter::GetImagingSubprimData(
    UsdPrim const& prim,
    TfToken const& subprim,
    const UsdImagingDataSourceStageGlobals &stageGlobals)
{
    if (subprim.IsEmpty()) {
        HdContainerDataSourceHandle terminalDS =
            UsdRiPxrImaging_DataSourceRenderTerminalPrim<HdEnergyFilterSchema>::
                New(prim.GetPath(), prim,
                    _tokens->riEnergyFilterShaderId, stageGlobals);

        // Overlay the base-class properties (enabled, lpe, order) as flat
        // attributes so that downstream Hydra consumers can read them via
        // sceneDelegate->Get(path, token).
        const TfToken flatAttrs[] = {
            _tokens->riEnergyFilterEnabled,
            _tokens->riEnergyFilterLpe,
            _tokens->riEnergyFilterOrder
        };
        std::vector<TfToken> names;
        std::vector<HdDataSourceBaseHandle> values;
        for (const TfToken &attrName : flatAttrs) {
            UsdAttribute attr = prim.GetAttribute(attrName);
            VtValue val;
            if (attr && attr.HasAuthoredValue() && attr.Get(&val)) {
                names.push_back(attrName);
                values.push_back(HdRetainedSampledDataSource::New(val));
            }
        }
        if (!names.empty()) {
            return HdOverlayContainerDataSource::New(
                HdRetainedContainerDataSource::New(
                    names.size(), names.data(), values.data()),
                terminalDS);
        }
        return terminalDS;
    }

    return nullptr;
}

HdDataSourceLocatorSet
UsdRiPxrImagingEnergyFilterAdapter::InvalidateImagingSubprim(
    UsdPrim const& prim,
    TfToken const& subprim,
    TfTokenVector const& properties,
    const UsdImagingPropertyInvalidationType invalidationType)
{
    if (subprim.IsEmpty()) {
        return
            UsdRiPxrImaging_DataSourceRenderTerminalPrim<HdEnergyFilterSchema>::
            Invalidate(
                prim, subprim, properties, invalidationType);
    }

    return HdDataSourceLocatorSet();
}

// -------------------------------------------------------------------------- //
// 1.0 Prim adapter API
// -------------------------------------------------------------------------- //

bool
UsdRiPxrImagingEnergyFilterAdapter::IsSupported(
    UsdImagingIndexProxy const* index) const
{
    return index->IsSprimTypeSupported(HdPrimTypeTokens->energyFilter);
}

SdfPath
UsdRiPxrImagingEnergyFilterAdapter::Populate(
    UsdPrim const& prim,
    UsdImagingIndexProxy* index,
    UsdImagingInstancerContext const* instancerContext)
{
    SdfPath cachePath = prim.GetPath();
    if (index->IsPopulated(cachePath)) {
        return cachePath;
    }

    index->InsertSprim(HdPrimTypeTokens->energyFilter, cachePath, prim);
    HD_PERF_COUNTER_INCR(UsdImagingTokens->usdPopulatedPrimCount);

    return cachePath;
}

void
UsdRiPxrImagingEnergyFilterAdapter::_RemovePrim(
    SdfPath const& cachePath,
    UsdImagingIndexProxy* index)
{
    index->RemoveSprim(HdPrimTypeTokens->energyFilter, cachePath);
}

void
UsdRiPxrImagingEnergyFilterAdapter::TrackVariability(
    UsdPrim const& prim,
    SdfPath const& cachePath,
    HdDirtyBits* timeVaryingBits,
    UsdImagingInstancerContext const* instancerContext) const
{
    const std::vector<UsdAttribute> &attrs = prim.GetAttributes();
    TF_FOR_ALL(attrIter, attrs) {
        const UsdAttribute& attr = *attrIter;
        if (attr.ValueMightBeTimeVarying()) {
            *timeVaryingBits |= HdChangeTracker::DirtyParams;
        }
    }
}

void
UsdRiPxrImagingEnergyFilterAdapter::UpdateForTime(
    UsdPrim const& prim,
    SdfPath const& cachePath,
    UsdTimeCode time,
    HdDirtyBits requestedBits,
    UsdImagingInstancerContext const* instancerContext) const
{
}

HdDirtyBits
UsdRiPxrImagingEnergyFilterAdapter::ProcessPropertyChange(
    UsdPrim const& prim,
    SdfPath const& cachePath,
    TfToken const& propertyName)
{
    return HdChangeTracker::AllDirty;
}

void
UsdRiPxrImagingEnergyFilterAdapter::MarkDirty(
    UsdPrim const& prim,
    SdfPath const& cachePath,
    HdDirtyBits dirty,
    UsdImagingIndexProxy* index)
{
    index->MarkSprimDirty(cachePath, dirty);
}

VtValue
UsdRiPxrImagingEnergyFilterAdapter::Get(
    UsdPrim const& prim,
    SdfPath const& cachePath,
    TfToken const& key,
    UsdTimeCode time,
    VtIntArray *outIndices) const
{
    if (key == HdEnergyFilterSchemaTokens->resource) {
        return VtValue(
            UsdRiPxrImagingRenderTerminalHelper::CreateHdMaterialNode2(
                prim,
                _tokens->riEnergyFilterShaderId,
                HdPrimTypeTokens->energyFilter));
    }

    // Base-class properties (enabled, lpe, order): only return authored values
    // to avoid surfacing schema defaults (e.g. the default lpe expression).
    UsdAttribute attr = prim.GetAttribute(key);
    VtValue val;
    if (attr && attr.HasAuthoredValue() && attr.Get(&val)) {
        return val;
    }

    TF_CODING_ERROR(
        "Property %s not supported for EnergyFilter by UsdImaging, path: %s",
        key.GetText(), cachePath.GetText());
    return VtValue();
}

PXR_NAMESPACE_CLOSE_SCOPE
