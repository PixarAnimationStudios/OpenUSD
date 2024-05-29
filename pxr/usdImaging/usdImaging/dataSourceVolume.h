//
// Copyright 2020 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#ifndef PXR_USD_IMAGING_USD_IMAGING_DATA_SOURCE_VOLUME_H
#define PXR_USD_IMAGING_USD_IMAGING_DATA_SOURCE_VOLUME_H

#include "pxr/usdImaging/usdImaging/dataSourceGprim.h"
#include "pxr/usdImaging/usdImaging/dataSourceStageGlobals.h"

#include "pxr/usd/usdVol/volume.h"


PXR_NAMESPACE_OPEN_SCOPE

/// \class UsdImagingDataSourceVolumeFieldBindings
///
/// A container data source representing volume field binding information.
///
class UsdImagingDataSourceVolumeFieldBindings : public HdContainerDataSource
{
public:
    HD_DECLARE_DATASOURCE(UsdImagingDataSourceVolumeFieldBindings);

    TfTokenVector GetNames() override;
    HdDataSourceBaseHandle Get(const TfToken & name) override;

private:

    // Private constructor, use static New() instead.
    UsdImagingDataSourceVolumeFieldBindings(
            UsdVolVolume usdVolume,
            const UsdImagingDataSourceStageGlobals &stageGlobals);


private:
    UsdVolVolume _usdVolume;
    const UsdImagingDataSourceStageGlobals &_stageGlobals;
};

HD_DECLARE_DATASOURCE_HANDLES(UsdImagingDataSourceVolumeFieldBindings);

// ----------------------------------------------------------------------------

/// \class UsdImagingDataSourceVolumePrim
///
/// A prim data source representing a UsdVolVolume prim. 
///
class UsdImagingDataSourceVolumePrim : public UsdImagingDataSourceGprim
{
public:
    HD_DECLARE_DATASOURCE(UsdImagingDataSourceVolumePrim);

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
    UsdImagingDataSourceVolumePrim(
        const SdfPath &sceneIndexPath,
        UsdPrim usdPrim,
        const UsdImagingDataSourceStageGlobals &stageGlobals);
};

HD_DECLARE_DATASOURCE_HANDLES(UsdImagingDataSourceVolumePrim);

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_USD_IMAGING_USD_IMAGING_DATA_SOURCE_VOLUME_H
