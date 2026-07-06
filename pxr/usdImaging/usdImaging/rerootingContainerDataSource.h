//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_USD_IMAGING_USD_IMAGING_REROOTING_CONTAINER_DATA_SOURCE_H
#define PXR_USD_IMAGING_USD_IMAGING_REROOTING_CONTAINER_DATA_SOURCE_H

#include "pxr/usdImaging/usdImaging/api.h"

#include "pxr/imaging/hd/dataSource.h"
#include "pxr/usd/pcp/mapFunction.h"
#include "pxr/usd/sdf/path.h"


PXR_NAMESPACE_OPEN_SCOPE

/// \class UsdImagingRerootingContainerDataSource
///
/// Applies the given PcpMapFunction to data source in the given
/// container data source.
///
class UsdImagingRerootingContainerDataSource : public HdContainerDataSource
{
public:
    HD_DECLARE_DATASOURCE(UsdImagingRerootingContainerDataSource)

    USDIMAGING_API
    ~UsdImagingRerootingContainerDataSource();

    USDIMAGING_API
    TfTokenVector GetNames() override;

    USDIMAGING_API
    HdDataSourceBaseHandle Get(const TfToken& name) override;

private:
    USDIMAGING_API
    UsdImagingRerootingContainerDataSource(
        HdContainerDataSourceHandle inputDataSource,
        PcpMapFunction const& mapFn);

    USDIMAGING_API
    UsdImagingRerootingContainerDataSource(
        HdContainerDataSourceHandle inputDataSource,
        const SdfPath &srcPrefix,
        const SdfPath &dstPrefix);

    const HdContainerDataSourceHandle _inputDataSource;
    const PcpMapFunction _mapFn;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif
