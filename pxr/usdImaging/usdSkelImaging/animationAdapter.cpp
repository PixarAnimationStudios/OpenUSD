//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/usdImaging/usdSkelImaging/animationAdapter.h"

#include "pxr/usdImaging/usdSkelImaging/dataSourceAnimationPrim.h"
#include "pxr/usdImaging/usdSkelImaging/tokens.h"

PXR_NAMESPACE_OPEN_SCOPE

TF_REGISTRY_FUNCTION(TfType)
{
    using Adapter = UsdSkelImagingAnimationAdapter;
    TfType t = TfType::Define<Adapter, TfType::Bases<Adapter::BaseAdapter> >();
    t.SetFactory< UsdImagingPrimAdapterFactory<Adapter> >();
}

UsdSkelImagingAnimationAdapter::
UsdSkelImagingAnimationAdapter() = default;

UsdSkelImagingAnimationAdapter::
~UsdSkelImagingAnimationAdapter() = default;

TfTokenVector
UsdSkelImagingAnimationAdapter::GetImagingSubprims(UsdPrim const& prim)
{
    return { TfToken() };
}

TfToken
UsdSkelImagingAnimationAdapter::GetImagingSubprimType(
        UsdPrim const& prim,
        TfToken const& subprim)
{
    if (!subprim.IsEmpty()) {
        return {};
    }

    return UsdSkelImagingPrimTypeTokens->skelAnimation;
}

HdContainerDataSourceHandle
UsdSkelImagingAnimationAdapter::GetImagingSubprimData(
        UsdPrim const& prim,
        TfToken const& subprim,
        const UsdImagingDataSourceStageGlobals &stageGlobals)
{
    if (!subprim.IsEmpty()) {
        return nullptr;
    }

    return UsdSkelImagingDataSourceAnimationPrim::New(
        prim.GetPath(), prim, stageGlobals);
}

HdDataSourceLocatorSet
UsdSkelImagingAnimationAdapter::InvalidateImagingSubprim(
        UsdPrim const& prim,
        TfToken const& subprim,
        TfTokenVector const& properties,
        UsdImagingPropertyInvalidationType invalidationType)
{
    if (!subprim.IsEmpty()) {
        return HdDataSourceLocatorSet();
    }

    return UsdSkelImagingDataSourceAnimationPrim::Invalidate(
        prim, subprim,properties, invalidationType);
}

PXR_NAMESPACE_CLOSE_SCOPE
