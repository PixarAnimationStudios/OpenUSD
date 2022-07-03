//
// Copyright 2020 Pixar
//
// Licensed under the Apache License, Version 2.0 (the "Apache License")
// with the following modification; you may not use this file except in
// compliance with the Apache License and the following modification to it:
// Section 6. Trademarks. is deleted and replaced with:
//
// 6. Trademarks. This License does not grant permission to use the trade
//    names, trademarks, service marks, or product names of the Licensor
//    and its affiliates, except as required to comply with Section 4(c) of
//    the License and to reproduce the content of the NOTICE file.
//
// You may obtain a copy of the Apache License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the Apache License with the above modification is
// distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
// KIND, either express or implied. See the Apache License for the specific
// language governing permissions and limitations under the Apache License.
//
#ifndef PXR_USD_IMAGING_USD_IMAGING_DATA_SOURCE_RELATIONSHIP_H
#define PXR_USD_IMAGING_USD_IMAGING_DATA_SOURCE_RELATIONSHIP_H

#include "pxr/usd/usd/relationship.h"
#include "pxr/usdImaging/usdImaging/dataSourceStageGlobals.h"
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
    VtValue GetValue(HdSampledDataSource::Time shutterOffset) override;

    /// Returns the extracted path array value of the attribute.
    /// \p shutterOffset is ignored.
    VtArray<SdfPath> GetTypedValue(
            HdSampledDataSource::Time shutterOffset) override;

    /// Returns \c false indicating USD relationhips cannot vary with time.
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
