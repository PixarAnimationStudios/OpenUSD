//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/usdImaging/usdRiPxrImaging/pxrCameraProjectionAdapter.h"

#include "pxr/imaging/hd/dataSource.h"
#include "pxr/imaging/hd/dataSourceLocator.h"

#include "pxr/usdImaging/usdImaging/primAdapter.h"
#include "pxr/usdImaging/usdImaging/types.h"
#include "pxr/usdImaging/usdRiPxrImaging/dataSourcePxrRenderTerminalPrims.h"
#include "pxr/usdImaging/usdRiPxrImaging/projectionSchema.h"
#include "pxr/usdImaging/usdRiPxrImaging/tokens.h"

#include "pxr/base/tf/registryManager.h"
#include "pxr/base/tf/staticTokens.h"
#include "pxr/base/tf/token.h"
#include "pxr/base/tf/type.h"

#include "pxr/pxr.h"

PXR_NAMESPACE_OPEN_SCOPE

TF_DEFINE_PRIVATE_TOKENS(
    _tokens,
    ((riProjectionShaderId, "ri:projection:shaderId"))
);

TF_REGISTRY_FUNCTION(TfType)
{
    using Adapter = UsdRiPxrImagingCameraProjectionAdapter;
    TfType t = TfType::Define<Adapter, TfType::Bases<Adapter::BaseAdapter>>();
    t.SetFactory<UsdImagingPrimAdapterFactory<Adapter>>();
}

UsdRiPxrImagingCameraProjectionAdapter::
UsdRiPxrImagingCameraProjectionAdapter() = default;

UsdRiPxrImagingCameraProjectionAdapter::
~UsdRiPxrImagingCameraProjectionAdapter() = default;

TfTokenVector
UsdRiPxrImagingCameraProjectionAdapter::GetImagingSubprims(
    const UsdPrim& /* prim */)
{
    return { TfToken() };
}

TfToken
UsdRiPxrImagingCameraProjectionAdapter::GetImagingSubprimType(
    const UsdPrim& /* prim */,
    const TfToken& subprim)
{
    if (subprim.IsEmpty()) {
        return UsdRiPxrImagingPrimTypeTokens->projection;
    }
    return TfToken();
}

HdContainerDataSourceHandle
UsdRiPxrImagingCameraProjectionAdapter::GetImagingSubprimData(
    const UsdPrim& prim,
    const TfToken& subprim,
    const UsdImagingDataSourceStageGlobals& stageGlobals)
{
    if (subprim.IsEmpty()) {
        return
            UsdRiPxrImaging_DataSourceRenderTerminalPrim
                <UsdRiPxrImagingProjectionSchema>::New(
                    prim.GetPath(), prim,
                    _tokens->riProjectionShaderId, stageGlobals);
    }
    return nullptr;
}

HdDataSourceLocatorSet
UsdRiPxrImagingCameraProjectionAdapter::InvalidateImagingSubprim(
    const UsdPrim& prim,
    const TfToken& subprim,
    const TfTokenVector& properties,
    const UsdImagingPropertyInvalidationType invalidationType)
{
    return 
        UsdRiPxrImaging_DataSourceRenderTerminalPrim
            <UsdRiPxrImagingProjectionSchema>::
                Invalidate( prim, subprim, properties, invalidationType);
}

PXR_NAMESPACE_CLOSE_SCOPE
