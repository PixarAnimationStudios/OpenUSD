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
#ifndef PXR_USD_IMAGING_USD_IMAGING_DATA_SOURCE_ATTRIBUTE_H
#define PXR_USD_IMAGING_USD_IMAGING_DATA_SOURCE_ATTRIBUTE_H

#include "pxr/usd/usd/attribute.h"
#include "pxr/usd/usd/attributeQuery.h"
#include "pxr/usdImaging/usdImaging/api.h"
#include "pxr/usdImaging/usdImaging/dataSourceStageGlobals.h"
#include "pxr/imaging/hd/dataSource.h"
#include "pxr/imaging/hd/dataSourceTypeDefs.h"

PXR_NAMESPACE_OPEN_SCOPE

/// \class UsdImagingDataSourceAttribute<T>
///
/// A data source that represents a USD Attribute
///
template <typename T>
class UsdImagingDataSourceAttribute : public HdTypedSampledDataSource<T>
{
public:

    HD_DECLARE_DATASOURCE(UsdImagingDataSourceAttribute<T>);

    /// Returns the VtValue of this attribute at a given \p shutterOffset
    ///
    VtValue GetValue(HdSampledDataSource::Time shutterOffset) override
    {
        return VtValue(GetTypedValue(shutterOffset));
    }

    /// Returns the extracted T value of the attribute at \p shutterOffset
    ///
    T GetTypedValue(HdSampledDataSource::Time shutterOffset) override
    {
        // Zero-initialization for numerical types.
        T result{};
        UsdTimeCode time = _stageGlobals.GetTime();
        if (time.IsNumeric()) {
            time = UsdTimeCode(time.GetValue() + shutterOffset);
        }
        _usdAttrQuery.Get<T>(&result, time);
        return result;
    }

    /// Fills the \p outSampleTimes with the times between \p startTime and 
    /// \p endTime that have valid sample data and returns \c true.
    ///
    bool GetContributingSampleTimesForInterval(
            HdSampledDataSource::Time startTime,
            HdSampledDataSource::Time endTime,
            std::vector<HdSampledDataSource::Time> *outSampleTimes) override
    {
        UsdTimeCode time = _stageGlobals.GetTime();
        if (!_usdAttrQuery.ValueMightBeTimeVarying() ||
            !time.IsNumeric()) {
            return false;
        }

        GfInterval interval(
            time.GetValue() + startTime,
            time.GetValue() + endTime);
        std::vector<double> timeSamples;
        _usdAttrQuery.GetTimeSamplesInInterval(interval, &timeSamples);

        // Add boundary timesamples, if necessary.
        if (timeSamples.empty() || timeSamples[0] > interval.GetMin()) {
            timeSamples.insert(timeSamples.begin(), interval.GetMin());
        }
        if (timeSamples.back() < interval.GetMax()) {
            timeSamples.push_back(interval.GetMax());
        }

        // We need to convert the time array because usd uses double and
        // hydra (and prman) use float :/.
        outSampleTimes->resize(timeSamples.size());
        for (size_t i = 0; i < timeSamples.size(); ++i) {
            (*outSampleTimes)[i] = timeSamples[i] - time.GetValue();
        }

        return true;
    }

private:

    /// Constructs a new UsdImagingDataSourceAttribute for the given \p usdAttr
    ///
    /// \p stageGlobals represents the context object for the UsdStage with
    /// which to evaluate this attribute data source.
    ///
    /// \p timeVaryingFlagLocator represents the locator that should be dirtied
    /// when time changes, if this attribute is time varying. An empty locator
    /// means that this attribute isn't tracked for time varyingness. This is
    /// distinct from the attribute name, say, because the attribute name
    /// may not correspond to a meaningful Hydra dirty locator. It's the
    /// responsibility of whoever is instantiating this data source to know the
    /// meaning of this attribute to Hydra.
    ///
    /// Note: client code is calling the constructor via static New.
    UsdImagingDataSourceAttribute(
            const UsdAttribute &usdAttr,
            const UsdImagingDataSourceStageGlobals &stageGlobals,
            const SdfPath &sceneIndexPath = SdfPath::EmptyPath(),
            const HdDataSourceLocator &timeVaryingFlagLocator =
                    HdDataSourceLocator::EmptyLocator());

    /// Constructor override taking an attribute query.
    UsdImagingDataSourceAttribute(
            const UsdAttributeQuery &usdAttrQuery,
            const UsdImagingDataSourceStageGlobals &stageGlobals,
            const SdfPath &sceneIndexPath = SdfPath::EmptyPath(),
            const HdDataSourceLocator &timeVaryingFlagLocator =
                    HdDataSourceLocator::EmptyLocator());

private:
    UsdAttributeQuery _usdAttrQuery;
    const UsdImagingDataSourceStageGlobals &_stageGlobals;
};

// ----------------------------------------------------------------------------


/// Returns an instance of UsdImagingDataSourceAttribute with a given T 
/// inferred from the usd attribute's sdf type
///
USDIMAGING_API
HdSampledDataSourceHandle
UsdImagingDataSourceAttributeNew(
        const UsdAttribute &usdAttr,
        const UsdImagingDataSourceStageGlobals &stageGlobals,
        const SdfPath &sceneIndexPath = SdfPath::EmptyPath(),
        const HdDataSourceLocator &timeVaryingFlagLocator =
                HdDataSourceLocator::EmptyLocator());

/// Override taking an attribute query
USDIMAGING_API
HdSampledDataSourceHandle
UsdImagingDataSourceAttributeNew(
        const UsdAttributeQuery &usdAttrQuery,
        const UsdImagingDataSourceStageGlobals &stageGlobals,
        const SdfPath &sceneIndexPath = SdfPath::EmptyPath(),
        const HdDataSourceLocator &timeVaryingFlagLocator =
                HdDataSourceLocator::EmptyLocator());

// ----------------------------------------------------------------------------

template<typename T>
UsdImagingDataSourceAttribute<T>::UsdImagingDataSourceAttribute(
    const UsdAttribute &usdAttr,
    const UsdImagingDataSourceStageGlobals &stageGlobals,
    const SdfPath &sceneIndexPath,
    const HdDataSourceLocator &timeVaryingFlagLocator)
    : _usdAttrQuery(usdAttr)
    , _stageGlobals(stageGlobals) 
{
    if (!timeVaryingFlagLocator.IsEmpty()) {
        if (_usdAttrQuery.ValueMightBeTimeVarying()) {
            _stageGlobals.FlagAsTimeVarying(
                    sceneIndexPath, timeVaryingFlagLocator);
        }
    }
}

template<typename T>
UsdImagingDataSourceAttribute<T>::UsdImagingDataSourceAttribute(
    const UsdAttributeQuery &usdAttrQuery,
    const UsdImagingDataSourceStageGlobals &stageGlobals,
    const SdfPath &sceneIndexPath,
    const HdDataSourceLocator &timeVaryingFlagLocator)
    : _usdAttrQuery(usdAttrQuery)
    , _stageGlobals(stageGlobals) 
{
    if (!timeVaryingFlagLocator.IsEmpty()) {
        if (_usdAttrQuery.ValueMightBeTimeVarying()) {
            _stageGlobals.FlagAsTimeVarying(
                    sceneIndexPath, timeVaryingFlagLocator);
        }
    }
}

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_USD_IMAGING_USD_IMAGING_DATA_SOURCE_ATTRIBUTE_H
