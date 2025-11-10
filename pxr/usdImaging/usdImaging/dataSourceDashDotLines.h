//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_USD_IMAGING_USD_IMAGING_DATA_SOURCE_DASHDOTLINES_H
#define PXR_USD_IMAGING_USD_IMAGING_DATA_SOURCE_DASHDOTLINES_H

#include "pxr/usdImaging/usdImaging/dataSourceGprim.h"
#include "pxr/usdImaging/usdImaging/dataSourceStageGlobals.h"

#include "pxr/usd/usdGeom/dashDotLines.h"

#include "pxr/imaging/hd/dataSource.h"

PXR_NAMESPACE_OPEN_SCOPE


/// \class UsdImagingDataSourceDashDotLinesTopology
///
/// A container data source representing DashDotLines topology information.
///
class ARCH_EXPORT_TYPE UsdImagingDataSourceDashDotLinesTopology : public HdContainerDataSource
{
public:
    HD_DECLARE_DATASOURCE(UsdImagingDataSourceDashDotLinesTopology);

    TfTokenVector GetNames() override;
    HdDataSourceBaseHandle Get(const TfToken & name) override;

private:
    UsdImagingDataSourceDashDotLinesTopology(
            const SdfPath &sceneIndexPath,
            UsdGeomDashDotLines usdDashDotLines,
            const UsdImagingDataSourceStageGlobals &stageGlobals);

private:
    const SdfPath _sceneIndexPath;
    UsdGeomDashDotLines _usdDashDotLines;
    const UsdImagingDataSourceStageGlobals &_stageGlobals;
};

HD_DECLARE_DATASOURCE_HANDLES(UsdImagingDataSourceDashDotLinesTopology);

// ----------------------------------------------------------------------------

/// \class UsdImagingDataSourceDashDotLines
///
/// A container data source representing data unique to DashDotLines
///
class ARCH_EXPORT_TYPE UsdImagingDataSourceDashDotLines : public HdContainerDataSource
{
public:
    HD_DECLARE_DATASOURCE(UsdImagingDataSourceDashDotLines);

    TfTokenVector GetNames() override;
    HdDataSourceBaseHandle Get(const TfToken &name) override;

private:
    UsdImagingDataSourceDashDotLines(
            const SdfPath &sceneIndexPath,
            UsdGeomDashDotLines usdDashDotLines,
            const UsdImagingDataSourceStageGlobals &stageGlobals);

private:
    const SdfPath _sceneIndexPath;
    UsdGeomDashDotLines _usdDashDotLines;
    const UsdImagingDataSourceStageGlobals & _stageGlobals;
};

HD_DECLARE_DATASOURCE_HANDLES(UsdImagingDataSourceDashDotLines);

// ----------------------------------------------------------------------------

/// \class UsdImagingDataSourceDashDotLinesPrim
///
/// A prim data source representing a UsdGeomDashDotLines prim. 
///
class ARCH_EXPORT_TYPE UsdImagingDataSourceDashDotLinesPrim : public UsdImagingDataSourceGprim
{
public:
    HD_DECLARE_DATASOURCE(UsdImagingDataSourceDashDotLinesPrim);

    TfTokenVector GetNames() override;
    HdDataSourceBaseHandle Get(const TfToken &name) override;

    USDIMAGING_API
    static HdDataSourceLocatorSet Invalidate(
            UsdPrim const& prim,
            const TfToken &subprim,
            const TfTokenVector &properties,
            UsdImagingPropertyInvalidationType invalidationType);

private:
    UsdImagingDataSourceDashDotLinesPrim(
        const SdfPath &sceneIndexPath,
        UsdPrim usdPrim,
        const UsdImagingDataSourceStageGlobals &stageGlobals);
};

HD_DECLARE_DATASOURCE_HANDLES(UsdImagingDataSourceDashDotLinesPrim);

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_USD_IMAGING_USD_IMAGING_DATA_SOURCE_DASHDOTLINES_H
