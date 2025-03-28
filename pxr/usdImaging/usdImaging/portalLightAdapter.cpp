//
// Copyright 2017 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/usdImaging/usdImaging/portalLightAdapter.h"
#include "pxr/usdImaging/usdImaging/delegate.h"
#include "pxr/usdImaging/usdImaging/indexProxy.h"
#include "pxr/usdImaging/usdImaging/tokens.h"

#include "pxr/imaging/hd/tokens.h"

PXR_NAMESPACE_OPEN_SCOPE



TF_REGISTRY_FUNCTION(TfType)
{
    typedef UsdImagingPortalLightAdapter Adapter;
    TfType t = TfType::Define<Adapter, TfType::Bases<Adapter::BaseAdapter> >();
    t.SetFactory< UsdImagingPrimAdapterFactory<Adapter> >();
}

UsdImagingPortalLightAdapter::~UsdImagingPortalLightAdapter() 
{
}

bool
UsdImagingPortalLightAdapter::IsSupported(UsdImagingIndexProxy const* index) const
{
    return false;
}

TfTokenVector
UsdImagingPortalLightAdapter::GetImagingSubprims(UsdPrim const& prim)
{
    return { TfToken() };
}

TfToken
UsdImagingPortalLightAdapter::GetImagingSubprimType(
    UsdPrim const& prim, TfToken const& subprim)
{
    if (subprim.IsEmpty()) {
        return HdPrimTypeTokens->light;
    }
    return TfToken();
}

SdfPath
UsdImagingPortalLightAdapter::Populate(UsdPrim const& prim, 
                            UsdImagingIndexProxy* index,
                            UsdImagingInstancerContext const* instancerContext)
{
    TF_CODING_ERROR("Portal lights are not yet supported in USD imaging");
    return prim.GetPath();
}

void
UsdImagingPortalLightAdapter::_RemovePrim(SdfPath const& cachePath,
                                            UsdImagingIndexProxy* index)
{
    TF_CODING_ERROR("Portal lights are not yet supported in USD imaging");
}


PXR_NAMESPACE_CLOSE_SCOPE
