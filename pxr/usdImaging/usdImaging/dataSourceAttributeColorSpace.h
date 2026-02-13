//
// Copyright 2023 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_USD_IMAGING_USD_IMAGING_DATA_SOURCE_ATTRIBUTE_COLORSPACE_H
#define PXR_USD_IMAGING_USD_IMAGING_DATA_SOURCE_ATTRIBUTE_COLORSPACE_H

#include "pxr/usd/usd/attribute.h"
#include "pxr/usdImaging/usdImaging/api.h"
#include "pxr/imaging/hd/dataSource.h"
#include "pxr/imaging/hd/dataSourceTypeDefs.h"
#include "pxr/usd/usd/colorSpaceAPI.h"

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

    /// Returns the extracted TfToken value of the color space for 
    /// the attribute.
    ///
    TfToken GetTypedValue(HdSampledDataSource::Time shutterOffset) override
    {
        if (!_usdAttr) {
            return {};
        }

        TF_UNUSED(shutterOffset);

        // Special Case handling of UsdUVTexture Nodes which has an
        // 'inputs:sourceColorSpace' that we want to consolidate on the 
        // 'inputs:file' parameter
        TfToken sourceColorSpaceInput = _GetSourceColorSpaceInput();
        if (!sourceColorSpaceInput.IsEmpty()) {
            return sourceColorSpaceInput;
        }

        // If there is no inputs:sourceColorSpace value authored, use the
        // ColorspaceAPI to find the resolved color space for the attribute.
        return UsdColorSpaceAPI::ComputeColorSpaceName(_usdAttr);
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


    // Helper to get the TfToken authored to the 'inputs:sourceColorSpace' 
    // for the 'inputs:file' attribute
    TfToken _GetSourceColorSpaceInput() {
        static TfToken inputSourceColorSpace("inputs:sourceColorSpace");

        // Only add add the source color space to the file attribute
        if (_usdAttr.GetName() != "inputs:file") {
            return TfToken();
        }

        TfToken sourceColorSpace;
        UsdPrim prim = _usdAttr.GetPrim();
        if (prim.HasAttribute(inputSourceColorSpace)) {
            VtValue sourceCSValue;
            prim.GetAttribute(inputSourceColorSpace).Get(&sourceCSValue);
            if (sourceCSValue.IsHolding<TfToken>()) {
                sourceColorSpace = sourceCSValue.UncheckedGet<TfToken>();
            }
        }
        return sourceColorSpace;
    }

private:
    UsdAttribute _usdAttr;
};


PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_USD_IMAGING_USD_IMAGING_DATA_SOURCE_ATTRIBUTE_COLORSPACE_H
