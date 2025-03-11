//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#ifndef PXR_USD_IMAGING_USD_SKEL_IMAGING_DATA_SOURCE_BLEND_SHAPE_PRIM_H
#define PXR_USD_IMAGING_USD_SKEL_IMAGING_DATA_SOURCE_BLEND_SHAPE_PRIM_H

#include "pxr/usdImaging/usdSkelImaging/api.h"

#include "pxr/usdImaging/usdImaging/dataSourceGprim.h"

PXR_NAMESPACE_OPEN_SCOPE

/// \class UsdSkelImagingDataSourceBlendShapePrim
///
/// A prim data source for UsdSkel's BlendShape.
///
class UsdSkelImagingDataSourceBlendShapePrim : public UsdImagingDataSourcePrim
{
public:
    HD_DECLARE_DATASOURCE(UsdSkelImagingDataSourceBlendShapePrim);

    USDSKELIMAGING_API
    TfTokenVector GetNames() override;

    USDSKELIMAGING_API
    HdDataSourceBaseHandle Get(const TfToken &name) override;

    USDSKELIMAGING_API
    static
    HdDataSourceLocatorSet
    Invalidate(
        UsdPrim const& prim,
        const TfToken &subprim,
        const TfTokenVector &properties,
        UsdImagingPropertyInvalidationType invalidationType);

private:
    USDSKELIMAGING_API
    UsdSkelImagingDataSourceBlendShapePrim(
        const SdfPath &sceneIndexPath,
        UsdPrim usdPrim,
        const UsdImagingDataSourceStageGlobals &stageGlobals);
};

HD_DECLARE_DATASOURCE_HANDLES(UsdSkelImagingDataSourceBlendShapePrim);

PXR_NAMESPACE_CLOSE_SCOPE

#endif
