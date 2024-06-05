//
// Copyright 2023 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#ifndef PXR_USD_IMAGING_USD_IMAGING_DATA_SOURCE_NURBS_PATCH_H
#define PXR_USD_IMAGING_USD_IMAGING_DATA_SOURCE_NURBS_PATCH_H

#include "pxr/usdImaging/usdImaging/dataSourceGprim.h"

PXR_NAMESPACE_OPEN_SCOPE

/// \class UsdImagingDataSourceNurbsPatchPrim
///
/// A prim data source representing UsdNurbsPatch.
///
class UsdImagingDataSourceNurbsPatchPrim : public UsdImagingDataSourceGprim
{
public:
    HD_DECLARE_DATASOURCE(UsdImagingDataSourceNurbsPatchPrim);

    TfTokenVector GetNames() override;
    HdDataSourceBaseHandle Get(const TfToken &name) override;

    static
    HdDataSourceLocatorSet
    Invalidate(
        UsdPrim const& prim,
        const TfToken &subprim,
        const TfTokenVector &properties,
        UsdImagingPropertyInvalidationType invalidationType);

private:
    // Private constructor, use static New() instead.
    UsdImagingDataSourceNurbsPatchPrim(
        const SdfPath &sceneIndexPath,
        UsdPrim usdPrim,
        const UsdImagingDataSourceStageGlobals &stageGlobals);
};

HD_DECLARE_DATASOURCE_HANDLES(UsdImagingDataSourceNurbsPatchPrim);

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_USD_IMAGING_USD_IMAGING_DATA_SOURCE_NURBS_PATCH_H
