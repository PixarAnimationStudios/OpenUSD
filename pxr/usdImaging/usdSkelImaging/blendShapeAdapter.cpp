//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/usdImaging/usdSkelImaging/blendShapeAdapter.h"

#include "pxr/usdImaging/usdSkelImaging/dataSourceBlendShapePrim.h"
#include "pxr/usdImaging/usdSkelImaging/tokens.h"

PXR_NAMESPACE_OPEN_SCOPE

TF_REGISTRY_FUNCTION(TfType)
{
    using Adapter = UsdSkelImagingBlendShapeAdapter;
    TfType t = TfType::Define<Adapter, TfType::Bases<Adapter::BaseAdapter> >();
    t.SetFactory< UsdImagingPrimAdapterFactory<Adapter> >();
}

UsdSkelImagingBlendShapeAdapter::
UsdSkelImagingBlendShapeAdapter() = default;

UsdSkelImagingBlendShapeAdapter::
~UsdSkelImagingBlendShapeAdapter() = default;

TfTokenVector
UsdSkelImagingBlendShapeAdapter::GetImagingSubprims(UsdPrim const& prim)
{
    return { TfToken() };
}

TfToken
UsdSkelImagingBlendShapeAdapter::GetImagingSubprimType(
        UsdPrim const& prim,
        TfToken const& subprim)
{
    if (!subprim.IsEmpty()) {
        return {};
    }

    return UsdSkelImagingPrimTypeTokens->skelBlendShape;
}

HdContainerDataSourceHandle
UsdSkelImagingBlendShapeAdapter::GetImagingSubprimData(
        UsdPrim const& prim,
        TfToken const& subprim,
        const UsdImagingDataSourceStageGlobals &stageGlobals)
{
    if (!subprim.IsEmpty()) {
        return nullptr;
    }

    return UsdSkelImagingDataSourceBlendShapePrim::New(
        prim.GetPath(), prim, stageGlobals);
}

HdDataSourceLocatorSet
UsdSkelImagingBlendShapeAdapter::InvalidateImagingSubprim(
        UsdPrim const& prim,
        TfToken const& subprim,
        TfTokenVector const& properties,
        UsdImagingPropertyInvalidationType invalidationType)
{
    if (!subprim.IsEmpty()) {
        return HdDataSourceLocatorSet();
    }

    return UsdSkelImagingDataSourceBlendShapePrim::Invalidate(
        prim, subprim,properties, invalidationType);
}

PXR_NAMESPACE_CLOSE_SCOPE
