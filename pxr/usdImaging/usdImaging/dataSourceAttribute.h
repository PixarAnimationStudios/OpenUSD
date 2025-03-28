//
// Copyright 2020 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_USD_IMAGING_USD_IMAGING_DATA_SOURCE_ATTRIBUTE_H
#define PXR_USD_IMAGING_USD_IMAGING_DATA_SOURCE_ATTRIBUTE_H

#include "pxr/usd/usd/attribute.h"
#include "pxr/usd/usd/attributeQuery.h"
#include "pxr/usdImaging/usdImaging/api.h"
#include "pxr/usdImaging/usdImaging/dataSourceStageGlobals.h"
#include "pxr/imaging/hd/dataSource.h"
#include "pxr/imaging/hd/dataSourceTypeDefs.h"

#include <algorithm>

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
    
        // Start with the times that fall within the interval
        _usdAttrQuery.GetTimeSamplesInInterval(interval, &timeSamples);
        // Add bracketing sample times for the leading and trailing edges of the
        // interval.
        double first, ignore, last;
        bool hasFirst, hasLast;
        // If hasFirst/hasLast comes back false for an edge, or if both the left and
        // right bracketing times for the edge are the same, it means there's no
        // bracketing sample time anywhere beyond that edge, so we fall back to the
        // interval's edge.
        _usdAttrQuery.GetBracketingTimeSamples(interval.GetMin(), &first, &ignore, &hasFirst);
        if (!hasFirst || first == ignore) {
            first = interval.GetMin();
        }
        _usdAttrQuery.GetBracketingTimeSamples(interval.GetMax(), &ignore, &last, &hasLast);
        if (!hasLast || last == ignore ) {
            last = interval.GetMax();
        }
        // Add the bracketing sample times only if they actually fall outside the
        // interval. This maintains ordering and uniqueness.
        if (timeSamples.empty() || first < timeSamples.front()) {
            timeSamples.insert(timeSamples.begin(), first);    
        }
        if (last > timeSamples.back()) {
            timeSamples.insert(timeSamples.end(), last);
        }

        // We need to convert the time array because usd uses double and
        // hydra (and prman) use float :/.
        outSampleTimes->resize(timeSamples.size());
        for (size_t i = 0; i < timeSamples.size(); ++i) {
            (*outSampleTimes)[i] = timeSamples[i] - time.GetValue();
        }

        return outSampleTimes->size() > 1;
    }

protected:
    /// Constructors and member data are protected instead of private
    /// for use by a specialized subclass for accessing SdfAssetPath
    /// attributes which need special handling for UDIMs.

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
inline void
UsdImagingDataSourceAttribute_RecordObjectInStageGlobals(
    const UsdImagingDataSourceStageGlobals *stageGlobals,
    const SdfPath &objPath)
{
    // By default nothing to record.
}

template<>
inline void
UsdImagingDataSourceAttribute_RecordObjectInStageGlobals<SdfAssetPath>(
    const UsdImagingDataSourceStageGlobals *stageGlobals,
    const SdfPath &objPath)
{
    // Record asset path-valued attributes.
    stageGlobals->FlagAsAssetPathDependent(objPath);
}

template<typename T>
UsdImagingDataSourceAttribute<T>::UsdImagingDataSourceAttribute(
    const UsdAttribute &usdAttr,
    const UsdImagingDataSourceStageGlobals &stageGlobals,
    const SdfPath &sceneIndexPath,
    const HdDataSourceLocator &timeVaryingFlagLocator)
    : UsdImagingDataSourceAttribute(
        UsdAttributeQuery(usdAttr), stageGlobals,
        sceneIndexPath, timeVaryingFlagLocator)
{
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

    UsdImagingDataSourceAttribute_RecordObjectInStageGlobals<T>(
        &_stageGlobals, usdAttrQuery.GetAttribute().GetPath());
}

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_USD_IMAGING_USD_IMAGING_DATA_SOURCE_ATTRIBUTE_H
