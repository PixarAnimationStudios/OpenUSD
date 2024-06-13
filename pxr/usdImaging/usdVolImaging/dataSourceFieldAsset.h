//
// Copyright 2022 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#ifndef PXR_USD_IMAGING_USD_VOL_IMAGING_DATA_SOURCE_FIELD_ASSET_H
#define PXR_USD_IMAGING_USD_VOL_IMAGING_DATA_SOURCE_FIELD_ASSET_H

/// \file usdImaging/dataSourceFieldAsset.h

#include "pxr/pxr.h"
#include "pxr/usdImaging/usdVolImaging/api.h"

#include "pxr/usdImaging/usdImaging/dataSourcePrim.h"
#include "pxr/usdImaging/usdImaging/dataSourceStageGlobals.h"
#include "pxr/imaging/hd/dataSource.h"

PXR_NAMESPACE_OPEN_SCOPE


/// \class UsdImagingDataSourceFieldAsset
///
/// A container data source representing volumeField info
///
class UsdImagingDataSourceFieldAsset : public HdContainerDataSource
{
public:
    HD_DECLARE_DATASOURCE(UsdImagingDataSourceFieldAsset);

    TfTokenVector GetNames() override;
    HdDataSourceBaseHandle Get(const TfToken &name) override;

    USDVOLIMAGING_API
    ~UsdImagingDataSourceFieldAsset() override;

private:

    // Private constructor, use static New() instead.
    UsdImagingDataSourceFieldAsset(
            const SdfPath &sceneIndexPath,
            UsdPrim usdPrim,
            const UsdImagingDataSourceStageGlobals &stageGlobals);

private:
    const SdfPath _sceneIndexPath;
    UsdPrim _usdPrim;
    const UsdImagingDataSourceStageGlobals & _stageGlobals;
};

HD_DECLARE_DATASOURCE_HANDLES(UsdImagingDataSourceFieldAsset);

/// \class UsdImagingDataSourceFieldAssetPrim
///
/// A prim data source representing UsdVolOpenVDBAsset or UsdVolField3DAsset.
///
class UsdImagingDataSourceFieldAssetPrim : public UsdImagingDataSourcePrim
{
public:
    HD_DECLARE_DATASOURCE(UsdImagingDataSourceFieldAssetPrim);

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
    UsdImagingDataSourceFieldAssetPrim(
        const SdfPath &sceneIndexPath,
        UsdPrim usdPrim,
        const UsdImagingDataSourceStageGlobals &stageGlobals);
};

HD_DECLARE_DATASOURCE_HANDLES(UsdImagingDataSourceFieldAssetPrim);

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_USD_IMAGING_USD_VOL_IMAGING_DATA_SOURCE_FIELD_ASSET_H
