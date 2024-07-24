//
// Copyright 2022 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_USD_IMAGING_USD_IMAGING_DATA_SOURCE_BASISCURVES_H
#define PXR_USD_IMAGING_USD_IMAGING_DATA_SOURCE_BASISCURVES_H

#include "pxr/usdImaging/usdImaging/dataSourceGprim.h"
#include "pxr/usdImaging/usdImaging/dataSourceStageGlobals.h"

#include "pxr/usd/usdGeom/basisCurves.h"

#include "pxr/imaging/hd/dataSource.h"

PXR_NAMESPACE_OPEN_SCOPE


/// \class UsdImagingDataSourceBasisCurvesTopology
///
/// A container data source representing basis curves topology information.
///
class UsdImagingDataSourceBasisCurvesTopology : public HdContainerDataSource
{
public:
    HD_DECLARE_DATASOURCE(UsdImagingDataSourceBasisCurvesTopology);

    TfTokenVector GetNames() override;
    HdDataSourceBaseHandle Get(const TfToken & name) override;

private:
    UsdImagingDataSourceBasisCurvesTopology(
            const SdfPath &sceneIndexPath,
            UsdGeomBasisCurves usdBasisCurves,
            const UsdImagingDataSourceStageGlobals &stageGlobals);

private:
    const SdfPath _sceneIndexPath;
    UsdGeomBasisCurves _usdBasisCurves;
    const UsdImagingDataSourceStageGlobals &_stageGlobals;
};

HD_DECLARE_DATASOURCE_HANDLES(UsdImagingDataSourceBasisCurvesTopology);

// ----------------------------------------------------------------------------

/// \class UsdImagingDataSourceBasisCurves
///
/// A container data source representing data unique to basis curves
///
class UsdImagingDataSourceBasisCurves : public HdContainerDataSource
{
public:
    HD_DECLARE_DATASOURCE(UsdImagingDataSourceBasisCurves);

    TfTokenVector GetNames() override;
    HdDataSourceBaseHandle Get(const TfToken &name) override;

private:
    UsdImagingDataSourceBasisCurves(
            const SdfPath &sceneIndexPath,
            UsdGeomBasisCurves usdBasisCurves,
            const UsdImagingDataSourceStageGlobals &stageGlobals);

private:
    const SdfPath _sceneIndexPath;
    UsdGeomBasisCurves _usdBasisCurves;
    const UsdImagingDataSourceStageGlobals & _stageGlobals;
};

HD_DECLARE_DATASOURCE_HANDLES(UsdImagingDataSourceBasisCurves);

// ----------------------------------------------------------------------------

/// \class UsdImagingDataSourceBasisCurvesPrim
///
/// A prim data source representing a UsdGeomBasisCurves prim. 
///
class UsdImagingDataSourceBasisCurvesPrim : public UsdImagingDataSourceGprim
{
public:
    HD_DECLARE_DATASOURCE(UsdImagingDataSourceBasisCurvesPrim);

    TfTokenVector GetNames() override;
    HdDataSourceBaseHandle Get(const TfToken &name) override;

    USDIMAGING_API
    static HdDataSourceLocatorSet Invalidate(
            UsdPrim const& prim,
            const TfToken &subprim,
            const TfTokenVector &properties,
            UsdImagingPropertyInvalidationType invalidationType);

private:
    UsdImagingDataSourceBasisCurvesPrim(
        const SdfPath &sceneIndexPath,
        UsdPrim usdPrim,
        const UsdImagingDataSourceStageGlobals &stageGlobals);
};

HD_DECLARE_DATASOURCE_HANDLES(UsdImagingDataSourceBasisCurvesPrim);

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_USD_IMAGING_USD_IMAGING_DATA_SOURCE_BASISCURVES_H
