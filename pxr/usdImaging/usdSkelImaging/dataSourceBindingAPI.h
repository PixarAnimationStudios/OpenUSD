//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#ifndef PXR_USD_IMAGING_USD_SKEL_IMAGING_DATA_SOURCE_BINDING_API_H
#define PXR_USD_IMAGING_USD_SKEL_IMAGING_DATA_SOURCE_BINDING_API_H

#include "pxr/usdImaging/usdSkelImaging/api.h"

#include "pxr/usdImaging/usdImaging/dataSourceStageGlobals.h"
#include "pxr/usdImaging/usdImaging/types.h"
#include "pxr/usd/usd/prim.h"

#include "pxr/imaging/hd/dataSource.h"

PXR_NAMESPACE_OPEN_SCOPE

/// \class UsdSkelImagingDataSourceBindingAPI
///
/// A prim data source for UsdSkel's SkelBindingAPI.
///
class UsdSkelImagingDataSourceBindingAPI : public HdContainerDataSource
{
public:
    HD_DECLARE_DATASOURCE(UsdSkelImagingDataSourceBindingAPI);

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
    UsdSkelImagingDataSourceBindingAPI(
        const SdfPath &sceneIndexPath,
        UsdPrim usdPrim,
        const UsdImagingDataSourceStageGlobals &stageGlobals);

    const SdfPath _sceneIndexPath;
    const UsdPrim _usdPrim;
    const UsdImagingDataSourceStageGlobals &_stageGlobals;
};

HD_DECLARE_DATASOURCE_HANDLES(UsdSkelImagingDataSourceBindingAPI);

PXR_NAMESPACE_CLOSE_SCOPE

#endif
