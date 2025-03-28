//
// Copyright 2017 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/usdImaging/usdImaging/distantLightAdapter.h"
#include "pxr/usdImaging/usdImaging/delegate.h"
#include "pxr/usdImaging/usdImaging/indexProxy.h"
#include "pxr/usdImaging/usdImaging/tokens.h"

#include "pxr/imaging/hd/tokens.h"

PXR_NAMESPACE_OPEN_SCOPE


TF_REGISTRY_FUNCTION(TfType)
{
    typedef UsdImagingDistantLightAdapter Adapter;
    TfType t = TfType::Define<Adapter, TfType::Bases<Adapter::BaseAdapter> >();
    t.SetFactory< UsdImagingPrimAdapterFactory<Adapter> >();
}

UsdImagingDistantLightAdapter::~UsdImagingDistantLightAdapter() 
{
}

TfTokenVector
UsdImagingDistantLightAdapter::GetImagingSubprims(UsdPrim const& prim)
{
    return { TfToken() };
}

TfToken
UsdImagingDistantLightAdapter::GetImagingSubprimType(
        UsdPrim const& prim, TfToken const& subprim)
{
    if (subprim.IsEmpty()) {
        return HdPrimTypeTokens->distantLight;
    }

    return TfToken();
}

bool
UsdImagingDistantLightAdapter::IsSupported(UsdImagingIndexProxy const* index) const
{
    return UsdImagingLightAdapter::IsEnabledSceneLights() &&
           index->IsSprimTypeSupported(HdPrimTypeTokens->distantLight);
}

SdfPath
UsdImagingDistantLightAdapter::Populate(UsdPrim const& prim, 
                            UsdImagingIndexProxy* index,
                            UsdImagingInstancerContext const* instancerContext)
{
    return _AddSprim(HdPrimTypeTokens->distantLight, prim, index, instancerContext);
}

void
UsdImagingDistantLightAdapter::_RemovePrim(SdfPath const& cachePath,
                                         UsdImagingIndexProxy* index)
{
    _RemoveSprim(HdPrimTypeTokens->distantLight, cachePath, index);
}


PXR_NAMESPACE_CLOSE_SCOPE
