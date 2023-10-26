//
// Copyright 2023 Pixar
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
#ifndef PXR_USD_IMAGING_USD_IMAGING_DATA_SOURCE_ATTRIBUTE_COLORSPACE_H
#define PXR_USD_IMAGING_USD_IMAGING_DATA_SOURCE_ATTRIBUTE_COLORSPACE_H

#include "pxr/usd/usd/attribute.h"
#include "pxr/usdImaging/usdImaging/api.h"
#include "pxr/imaging/hd/dataSource.h"
#include "pxr/imaging/hd/dataSourceTypeDefs.h"

PXR_NAMESPACE_OPEN_SCOPE

/// \class UsdImagingDataSourceAttributeColorSpace
///
/// A data source that represents the metadata on a USD Attribute
///
class UsdImagingDataSourceAttributeColorSpace : public HdTokenDataSource
{
public:

    HD_DECLARE_DATASOURCE(UsdImagingDataSourceAttributeColorSpace);

    /// Returns the VtValue of the colorspace for the attribute
    ///
    VtValue GetValue(HdSampledDataSource::Time shutterOffset) override
    {
        return VtValue(GetTypedValue(shutterOffset));
    }

    /// Returns the extracted TfToken value of the color space metadata 
    /// on the attribute.
    ///
    TfToken GetTypedValue(HdSampledDataSource::Time shutterOffset) override
    {
        TF_UNUSED(shutterOffset);
        return _usdAttr.GetColorSpace();
    }

    /// Returns false since we do not expect the color space value to vary
    /// over time.
    ///
    bool GetContributingSampleTimesForInterval(
        HdSampledDataSource::Time startTime,
        HdSampledDataSource::Time endTime,
        std::vector<HdSampledDataSource::Time> *outSampleTimes) override
    {
        TF_UNUSED(startTime);
        TF_UNUSED(endTime);
        return false;
    }

private:

    /// Constructs a new UsdImagingDataSourceAttributeColorSpace for the given 
    /// \p usdAttr
    ///
    UsdImagingDataSourceAttributeColorSpace(const UsdAttribute &usdAttr);

private:
    UsdAttribute _usdAttr;
};


PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_USD_IMAGING_USD_IMAGING_DATA_SOURCE_ATTRIBUTE_COLORSPACE_H
