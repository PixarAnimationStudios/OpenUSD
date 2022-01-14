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
