//
// Copyright 2023 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#ifndef PXR_USD_IMAGING_USD_IMAGING_DATA_SOURCE_USD_PRIM_INFO_H
#define PXR_USD_IMAGING_USD_IMAGING_DATA_SOURCE_USD_PRIM_INFO_H

#include "pxr/imaging/hd/dataSource.h"
#include "pxr/usd/usd/prim.h"

PXR_NAMESPACE_OPEN_SCOPE

/// \class UsdImagingDataSourceUsdPrimInfo
///
/// A container data source containing metadata such as
/// the specifier of a prim of native instancing information.
class UsdImagingDataSourceUsdPrimInfo : public HdContainerDataSource
{
public:
    HD_DECLARE_DATASOURCE(UsdImagingDataSourceUsdPrimInfo);
    
    TfTokenVector GetNames() override;
    HdDataSourceBaseHandle Get(const TfToken &name) override;

    ~UsdImagingDataSourceUsdPrimInfo();
    
private:
    UsdImagingDataSourceUsdPrimInfo(UsdPrim usdPrim);

private:
    UsdPrim _usdPrim;
};

HD_DECLARE_DATASOURCE_HANDLES(UsdImagingDataSourceUsdPrimInfo);

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_USD_IMAGING_USD_IMAGING_DATA_SOURCE_USD_H
