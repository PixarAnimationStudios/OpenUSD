//
// Copyright 2020 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_USD_IMAGING_USD_IMAGING_DATA_SOURCE_POINT_INSTANCER_H
#define PXR_USD_IMAGING_USD_IMAGING_DATA_SOURCE_POINT_INSTANCER_H

#include "pxr/imaging/hd/dataSourceTypeDefs.h"

#include "pxr/usd/usdGeom/pointInstancer.h"

#include "pxr/usdImaging/usdImaging/dataSourcePrim.h"
#include "pxr/usdImaging/usdImaging/dataSourcePrimvars.h"

PXR_NAMESPACE_OPEN_SCOPE

// ----------------------------------------------------------------------------

/// \class UsdImagingDataSourcePointInstancerMask
///
/// A data source representing a point instancer's instance mask. It stores,
/// per instance, whether an instance is deactivated.  If it has zero length,
/// all instances are active.
///
class UsdImagingDataSourcePointInstancerMask : public HdBoolArrayDataSource
{
public:
    HD_DECLARE_DATASOURCE(UsdImagingDataSourcePointInstancerMask);

    VtValue GetValue(HdSampledDataSource::Time shutterOffset) override;

    VtBoolArray GetTypedValue(
        HdSampledDataSource::Time shutterOffset) override;

    bool GetContributingSampleTimesForInterval(
        HdSampledDataSource::Time startTime,
        HdSampledDataSource::Time endTime,
        std::vector<HdSampledDataSource::Time> * outSampleTimes) override;

private:
    UsdImagingDataSourcePointInstancerMask(
        const SdfPath &sceneIndexPath,
        const UsdGeomPointInstancer &usdPI,
        const UsdImagingDataSourceStageGlobals &stageGlobals);

private:
    UsdGeomPointInstancer _usdPI;
    const UsdImagingDataSourceStageGlobals &_stageGlobals;
};

HD_DECLARE_DATASOURCE_HANDLES(UsdImagingDataSourcePointInstancerMask);

// ----------------------------------------------------------------------------

/// \class UsdImagingDataSourcePointInstancerTopology
///
/// A data source representing a point instancer's instance topology. This
/// is made up of "prototypes", "protoIndices", and "mask", which is enough
/// to define the right number of instances with the right assigned prototype
/// and primvar index.
///
class UsdImagingDataSourcePointInstancerTopology : public HdContainerDataSource
{
public:
    HD_DECLARE_DATASOURCE(UsdImagingDataSourcePointInstancerTopology);

    TfTokenVector GetNames() override;
    HdDataSourceBaseHandle Get(const TfToken &name) override;

private:
    // Private constructor, use static New() instead.
    UsdImagingDataSourcePointInstancerTopology(
        const SdfPath &sceneIndexPath,
        UsdGeomPointInstancer usdPI,
        const UsdImagingDataSourceStageGlobals &stageGlobals);

private:
    const SdfPath _sceneIndexPath;
    UsdGeomPointInstancer _usdPI;
    const UsdImagingDataSourceStageGlobals &_stageGlobals;
};

HD_DECLARE_DATASOURCE_HANDLES(UsdImagingDataSourcePointInstancerTopology);

// ----------------------------------------------------------------------------

/// \class UsdImagingDataSourcePointInstancer
///
/// A data source representing the UsdGeom PointInstancer prim.
///
class UsdImagingDataSourcePointInstancerPrim
    : public UsdImagingDataSourcePrim
{
public:
    HD_DECLARE_DATASOURCE(UsdImagingDataSourcePointInstancerPrim);

    TfTokenVector GetNames() override;
    HdDataSourceBaseHandle Get(const TfToken &name) override;

    /// Returns the hydra attribute set we should invalidate if the value of
    /// the USD properties in \p properties change.
    USDIMAGING_API
    static HdDataSourceLocatorSet Invalidate(
        UsdPrim const &prim,
        const TfToken &subprim,
        const TfTokenVector &properties,
        UsdImagingPropertyInvalidationType invalidationType);

private:
    // Private constructor, use static New() instead.
    USDIMAGING_API
    UsdImagingDataSourcePointInstancerPrim(
        const SdfPath &sceneIndexPath,
        UsdPrim usdPrim,
        const UsdImagingDataSourceStageGlobals &stageGlobals);

};

HD_DECLARE_DATASOURCE_HANDLES(UsdImagingDataSourcePointInstancerPrim);

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_USD_IMAGING_USD_IMAGING_DATA_SOURCE_POINT_INSTANCER_H
