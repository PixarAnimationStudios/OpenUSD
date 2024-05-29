//
// Copyright 2018 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/usdImaging/usdImaging/fieldAdapter.h"
#include "pxr/usdImaging/usdImaging/delegate.h"
#include "pxr/usdImaging/usdImaging/indexProxy.h"
#include "pxr/usdImaging/usdImaging/tokens.h"

#include "pxr/imaging/hd/field.h"

PXR_NAMESPACE_OPEN_SCOPE

TF_DEFINE_PRIVATE_TOKENS(
    _tokens,
    (textureMemory)
);


TF_REGISTRY_FUNCTION(TfType)
{
    typedef UsdImagingFieldAdapter Adapter;
    TfType::Define<Adapter, TfType::Bases<Adapter::BaseAdapter> >();
    // No factory here, UsdImagingFieldAdapter is abstract.
}

UsdImagingFieldAdapter::~UsdImagingFieldAdapter() = default;

bool
UsdImagingFieldAdapter::IsSupported(UsdImagingIndexProxy const* index) const
{
    return index->IsBprimTypeSupported(GetPrimTypeToken());
}

SdfPath
UsdImagingFieldAdapter::Populate(UsdPrim const& prim, 
                            UsdImagingIndexProxy* index,
                            UsdImagingInstancerContext const* instancerContext)
{
    index->InsertBprim(GetPrimTypeToken(), prim.GetPath(), prim);
    HD_PERF_COUNTER_INCR(UsdImagingTokens->usdPopulatedPrimCount);

    return prim.GetPath();
}

void
UsdImagingFieldAdapter::_RemovePrim(SdfPath const& cachePath,
                                         UsdImagingIndexProxy* index)
{
    index->RemoveBprim(GetPrimTypeToken(), cachePath);
}

void 
UsdImagingFieldAdapter::TrackVariability(UsdPrim const& prim,
                                        SdfPath const& cachePath,
                                        HdDirtyBits* timeVaryingBits,
                                        UsdImagingInstancerContext const* 
                                            instancerContext) const
{
    // Discover time-varying transforms.
    _IsTransformVarying(prim,
        HdField::DirtyBits::DirtyTransform,
        UsdImagingTokens->usdVaryingXform,
        timeVaryingBits);

    // If any of the field attributes is time varying 
    // we will assume all field params are time-varying.
    const std::vector<UsdAttribute> &attrs = prim.GetAttributes();
    TF_FOR_ALL(attrIter, attrs) {
        const UsdAttribute& attr = *attrIter;
        if (attr.GetNumTimeSamples()>1){
            *timeVaryingBits |= HdField::DirtyBits::DirtyParams;
        }
    }
}

// Thread safe.
//  * Populate dirty bits for the given \p time.
void 
UsdImagingFieldAdapter::UpdateForTime(UsdPrim const& prim,
                               SdfPath const& cachePath, 
                               UsdTimeCode time,
                               HdDirtyBits requestedBits,
                               UsdImagingInstancerContext const* 
                                   instancerContext) const
{
}

HdDirtyBits
UsdImagingFieldAdapter::ProcessPropertyChange(UsdPrim const& prim,
                                      SdfPath const& cachePath, 
                                      TfToken const& propertyName)
{
    return HdChangeTracker::AllDirty;
}

void
UsdImagingFieldAdapter::MarkDirty(UsdPrim const& prim,
                                  SdfPath const& cachePath,
                                  HdDirtyBits dirty,
                                  UsdImagingIndexProxy* index)
{
    index->MarkBprimDirty(cachePath, dirty);
}

void
UsdImagingFieldAdapter::MarkTransformDirty(UsdPrim const& prim,
                                           SdfPath const& cachePath,
                                           UsdImagingIndexProxy* index)
{
    static const HdDirtyBits transformDirty = HdField::DirtyTransform;
    index->MarkBprimDirty(cachePath, transformDirty);
}

void
UsdImagingFieldAdapter::MarkVisibilityDirty(UsdPrim const& prim,
                                            SdfPath const& cachePath,
                                            UsdImagingIndexProxy* index)
{
    // TBD
}

VtValue
UsdImagingFieldAdapter::Get(UsdPrim const& prim,
                            SdfPath const& cachePath,
                            TfToken const& key,
                            UsdTimeCode time,
                            VtIntArray *outIndices) const
{
    if (key == _tokens->textureMemory) {
        UsdAttribute const &attr = prim.GetAttribute(key);
        VtValue value;
        if (attr && attr.Get(&value, time)) {
            return value;
        }
        
        // Fallback to 0.0;
        return VtValue(0.0f);
    }
        
    TF_CODING_ERROR(
        "Property %s not supported for fields by UsdImaging, path: %s",
        key.GetText(), cachePath.GetText());
    return VtValue();
}

PXR_NAMESPACE_CLOSE_SCOPE
