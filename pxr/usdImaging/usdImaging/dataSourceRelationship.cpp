//
// Copyright 2020 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/usdImaging/usdImaging/dataSourceRelationship.h"

PXR_NAMESPACE_OPEN_SCOPE

UsdImagingDataSourceRelationship::UsdImagingDataSourceRelationship(
        const UsdRelationship &usdRel,
        const UsdImagingDataSourceStageGlobals &stageGlobals)
: _usdRel(usdRel)
, _stageGlobals(stageGlobals)
{}

VtValue
UsdImagingDataSourceRelationship::GetValue(
        HdSampledDataSource::Time shutterOffset)
{
    return VtValue(GetTypedValue(shutterOffset));
}

VtArray<SdfPath>
UsdImagingDataSourceRelationship::GetTypedValue(
        HdSampledDataSource::Time shutterOffset)
{
    SdfPathVector paths;
    _usdRel.GetForwardedTargets(&paths);
    VtArray<SdfPath> vtPaths(paths.begin(), paths.end());
    return vtPaths;
}

bool
UsdImagingDataSourceRelationship::GetContributingSampleTimesForInterval(
        HdSampledDataSource::Time startTime,
        HdSampledDataSource::Time endTime,
        std::vector<HdSampledDataSource::Time> *outSampleTimes)
{
    // Relationships are constant across time in USD.
    return false;
}

PXR_NAMESPACE_CLOSE_SCOPE
