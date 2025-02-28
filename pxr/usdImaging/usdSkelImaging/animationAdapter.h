//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_USD_IMAGING_USD_SKEL_IMAGING_ANIMATION_ADAPTER_H
#define PXR_USD_IMAGING_USD_SKEL_IMAGING_ANIMATION_ADAPTER_H

#include "pxr/pxr.h"
#include "pxr/usdImaging/usdSkelImaging/api.h"

#include "pxr/usdImaging/usdImaging/sceneIndexPrimAdapter.h"

PXR_NAMESPACE_OPEN_SCOPE


/// \class UsdSkelImagingAnimationAdapter
///
/// Prim adapter for UsdSkel's SkelAnimation.
///
class UsdSkelImagingAnimationAdapter
    : public UsdImagingSceneIndexPrimAdapter
{
public:
    using BaseAdapter = UsdImagingSceneIndexPrimAdapter;

    USDSKELIMAGING_API
    UsdSkelImagingAnimationAdapter();

    USDSKELIMAGING_API
    ~UsdSkelImagingAnimationAdapter() override;

    USDSKELIMAGING_API
    TfTokenVector GetImagingSubprims(UsdPrim const &prim) override;

    USDSKELIMAGING_API
    TfToken GetImagingSubprimType(
            UsdPrim const &prim,
            TfToken const &subprim) override;

    USDSKELIMAGING_API
    HdContainerDataSourceHandle GetImagingSubprimData(
            UsdPrim const& prim,
            TfToken const& subprim,
            const UsdImagingDataSourceStageGlobals &stageGlobals) override;

    USDSKELIMAGING_API
    HdDataSourceLocatorSet InvalidateImagingSubprim(
            UsdPrim const& prim,
            TfToken const& subprim,
            TfTokenVector const& properties,
            UsdImagingPropertyInvalidationType invalidationType) override;
};


PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_USD_IMAGING_USD_SKEL_IMAGING_ANIMATION_ADAPTER_H
