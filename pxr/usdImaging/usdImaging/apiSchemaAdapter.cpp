//
// Copyright 2022 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/usdImaging/usdImaging/apiSchemaAdapter.h"

PXR_NAMESPACE_OPEN_SCOPE

UsdImagingAPISchemaAdapter::~UsdImagingAPISchemaAdapter() = default;

TfTokenVector
UsdImagingAPISchemaAdapter::GetImagingSubprims(
    UsdPrim const& prim,
    TfToken const& appliedInstanceName)
{
    return TfTokenVector();
}

TfToken
UsdImagingAPISchemaAdapter::GetImagingSubprimType(
    UsdPrim const& prim,
    TfToken const& subprim,
    TfToken const& appliedInstanceName)
{
    return TfToken();
}

HdContainerDataSourceHandle
UsdImagingAPISchemaAdapter::GetImagingSubprimData(
    UsdPrim const& prim,
    TfToken const& subprim,
    TfToken const& appliedInstanceName,
    const UsdImagingDataSourceStageGlobals &stageGlobals)
{
    return nullptr;
}

HdDataSourceLocatorSet
UsdImagingAPISchemaAdapter::InvalidateImagingSubprim(
    UsdPrim const& prim,
    TfToken const& subprim,
    TfToken const& appliedInstanceName,
    TfTokenVector const& properties,
    const UsdImagingPropertyInvalidationType invalidationType)
{
    return HdDataSourceLocatorSet();
}

PXR_NAMESPACE_CLOSE_SCOPE
