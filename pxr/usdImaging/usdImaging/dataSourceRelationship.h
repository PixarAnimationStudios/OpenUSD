//
// Copyright 2020 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_USD_IMAGING_USD_IMAGING_DATA_SOURCE_RELATIONSHIP_H
#define PXR_USD_IMAGING_USD_IMAGING_DATA_SOURCE_RELATIONSHIP_H

#include "pxr/usdImaging/usdImaging/api.h"
#include "pxr/usdImaging/usdImaging/dataSourceStageGlobals.h"
#include "pxr/usd/usd/relationship.h"
#include "pxr/imaging/hd/dataSource.h"
#include "pxr/imaging/hd/dataSourceTypeDefs.h"

PXR_NAMESPACE_OPEN_SCOPE

/// \class UsdImagingDataSourceRelationship
///
/// A data source that represents a USD relationship
///
class UsdImagingDataSourceRelationship : public HdPathArrayDataSource
{
public:
    HD_DECLARE_DATASOURCE(UsdImagingDataSourceRelationship);

    /// Returns the extracted path array value of the attribute, as a VtValue.
    /// \p shutterOffset is ignored.
    USDIMAGING_API
    VtValue GetValue(HdSampledDataSource::Time shutterOffset) override;

    /// Returns the extracted path array value of the attribute.
    /// \p shutterOffset is ignored.
    USDIMAGING_API
    VtArray<SdfPath> GetTypedValue(
            HdSampledDataSource::Time shutterOffset) override;

    /// Returns \c false indicating USD relationhips cannot vary with time.
    USDIMAGING_API
    bool GetContributingSampleTimesForInterval(
            HdSampledDataSource::Time startTime,
            HdSampledDataSource::Time endTime,
            std::vector<HdSampledDataSource::Time> *outSampleTimes) override;

private:
    /// Constructs a new UsdImagingDataSourceRelationship for the given
    /// \p usdRel.
    ///
    /// \p stageGlobals represents the context object for the UsdStage with
    /// which to evaluate this relationship.
    USDIMAGING_API
    UsdImagingDataSourceRelationship(
            const UsdRelationship &usdRel,
            const UsdImagingDataSourceStageGlobals &stageGlobals);

private:
    UsdRelationship _usdRel;
    const UsdImagingDataSourceStageGlobals & _stageGlobals;
};

HD_DECLARE_DATASOURCE_HANDLES(UsdImagingDataSourceRelationship);

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_USD_IMAGING_USD_IMAGING_DATA_SOURCE_RELATIONSHIP_H
