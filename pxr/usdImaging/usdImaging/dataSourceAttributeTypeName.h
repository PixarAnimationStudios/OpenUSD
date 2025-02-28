//
// Copyright 2023 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_USD_IMAGING_USD_IMAGING_DATA_SOURCE_ATTRIBUTE_TYPENAME_H
#define PXR_USD_IMAGING_USD_IMAGING_DATA_SOURCE_ATTRIBUTE_TYPENAME_H

#include "pxr/usd/usd/attribute.h"
#include "pxr/usdImaging/usdImaging/api.h"
#include "pxr/imaging/hd/dataSource.h"
#include "pxr/imaging/hd/dataSourceTypeDefs.h"

PXR_NAMESPACE_OPEN_SCOPE

/// \class UsdImagingDataSourceAttributeTypeName
///
/// A data source that represents the type name on a USD Attribute
///
class UsdImagingDataSourceAttributeTypeName : public HdTokenDataSource
{
public:

    HD_DECLARE_DATASOURCE(UsdImagingDataSourceAttributeTypeName);

    /// Returns the VtValue of the type info for the attribute
    ///
    VtValue GetValue(HdSampledDataSource::Time shutterOffset) override
    {
        return VtValue(GetTypedValue(shutterOffset));
    }

    /// Returns the extracted TfToken value of the type info metadata 
    /// on the attribute.
    ///
    TfToken GetTypedValue(HdSampledDataSource::Time shutterOffset) override
    {
        TF_UNUSED(shutterOffset);
        // We are using the GetAsToken() here since we do not expect an alias 
        // other than the "official" type name.
        return _usdAttr.GetTypeName().GetAsToken();
    }

    /// Returns false since we do not expect the type info to vary over time.
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

    /// Constructs a new UsdImagingDataSourceAttributeTypeName for the given 
    /// \p usdAttr
    ///
    UsdImagingDataSourceAttributeTypeName(const UsdAttribute &usdAttr);

private:
    UsdAttribute _usdAttr;
};


PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_USD_IMAGING_USD_IMAGING_DATA_SOURCE_ATTRIBUTE_TYPENAME_H
