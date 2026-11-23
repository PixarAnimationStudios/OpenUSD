//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "rmanUsdImaging/envDayLightAdapter.h"

#if PXR_VERSION >= 2502

#include "pxr/usdImaging/usdImaging/indexProxy.h"
#include "pxr/imaging/hd/tokens.h"

#include <iostream>

PXR_NAMESPACE_OPEN_SCOPE

TF_REGISTRY_FUNCTION(TfType)
{
    using Adapter = HdPrman_EnvDayLightAdapter;
    TfType t = TfType::Define<Adapter, TfType::Bases<Adapter::BaseAdapter>>();
    t.SetFactory<UsdImagingPrimAdapterFactory<Adapter>>();
}

HdPrman_EnvDayLightAdapter::~HdPrman_EnvDayLightAdapter() = default;

TfTokenVector
HdPrman_EnvDayLightAdapter::GetImagingSubprims(UsdPrim const& prim)
{
    return { TfToken() };
}

TfToken
HdPrman_EnvDayLightAdapter::GetImagingSubprimType(UsdPrim const& prim, TfToken const& subprim)
{
    if (subprim.IsEmpty()) {
        return HdPrimTypeTokens->light;
    }

    return TfToken();
}

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_VERSION >= 2502