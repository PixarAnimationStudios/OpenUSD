//
// Copyright 2026 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_USD_IMAGING_USD_EXEC_IMAGING_COMPUTED_DATA_SOURCE_H
#define PXR_USD_IMAGING_USD_EXEC_IMAGING_COMPUTED_DATA_SOURCE_H

/// \file

#include "pxr/pxr.h"

#include "pxr/usdImaging/usdExecImaging/api.h"
#include "pxr/usdImaging/usdExecImaging/requestAccessorInterface.h"
#include "pxr/usdImaging/usdExecImaging/valueKey.h"

#include "pxr/base/vt/value.h"
#include "pxr/imaging/hd/dataSource.h"

#include <utility>

PXR_NAMESPACE_OPEN_SCOPE


/// Common implementation for data sources that produce exec-computed values.
///
/// This class is pseudo-private. Clients should not instantiate it directly.
/// This class is declared in a public header, because it is requred by the
/// template class UsdExecImagingComputedTypedSampledDataSource.
///
class UsdExecImaging_ComputedDataSourceImpl
{
public:
    using Time = HdSampledDataSource::Time;

    USDEXECIMAGING_API
    UsdExecImaging_ComputedDataSourceImpl(
        UsdExecImagingRequestAccessorInterfaceSharedPtr requestAccessor,
        UsdExecImagingValueKey valueKey);

    USDEXECIMAGING_API
    VtValue GetValue(Time shutterOffset) const;

    USDEXECIMAGING_API
    bool GetContributingSampleTimesForInterval(
        Time startTime, 
        Time endTime,
        std::vector<Time> * outSampleTimes) const;

private:
    UsdExecImagingRequestAccessorInterfaceSharedPtr _requestAccessor;
    UsdExecImagingValueKey _valueKey;
};

/// An HdSampledDataSource that produces exec-computed values.
///
/// The data source produces a computed value by reading it from a
/// UsdExecImagingRequestAccessorInterface specified in the constructor.
///
class UsdExecImagingComputedSampledDataSource : public HdSampledDataSource
{
public:
    HD_DECLARE_DATASOURCE(UsdExecImagingComputedSampledDataSource)

    USDEXECIMAGING_API
    ~UsdExecImagingComputedSampledDataSource() override;

    VtValue GetValue(Time shutterOffset) override {
        return _impl.GetValue(shutterOffset);
    }

    bool GetContributingSampleTimesForInterval(
        Time startTime, 
        Time endTime,
        std::vector<Time> *outSampleTimes) override {
        return _impl.GetContributingSampleTimesForInterval(
            startTime, endTime, outSampleTimes);
    }

private:
    UsdExecImagingComputedSampledDataSource(
        UsdExecImagingRequestAccessorInterfaceSharedPtr requestAccessor,
        UsdExecImagingValueKey valueKey)
        : _impl(std::move(requestAccessor), std::move(valueKey))
    {}

private:
    UsdExecImaging_ComputedDataSourceImpl _impl;
};

/// An HdTypedSampledDataSource that produces exec-computed values.
///
/// The data source produces a computed value by reading it from a
/// UsdExecImagingRequestAccessorInterface specified in the constructor.
///
template <class ValueType>
class UsdExecImagingComputedTypedSampledDataSource
    : public HdTypedSampledDataSource<ValueType>
{
public:
    HD_DECLARE_DATASOURCE(
        UsdExecImagingComputedTypedSampledDataSource<ValueType>);

    using Time = HdSampledDataSource::Time;

    VtValue GetValue(Time shutterOffset) override {
        return _impl.GetValue(shutterOffset);
    }

    ValueType GetTypedValue(Time shutterOffset) override {
        return _impl.GetValue(shutterOffset).template Get<ValueType>();
    }

    bool GetContributingSampleTimesForInterval(
        Time startTime, 
        Time endTime,
        std::vector<Time> *outSampleTimes) override {
        return _impl.GetContributingSampleTimesForInterval(
            startTime, endTime, outSampleTimes);
    }

private:
    UsdExecImagingComputedTypedSampledDataSource(
        UsdExecImagingRequestAccessorInterfaceSharedPtr requestAccessor,
        UsdExecImagingValueKey valueKey)
        : _impl(std::move(requestAccessor), std::move(valueKey))
    {}

private:
    UsdExecImaging_ComputedDataSourceImpl _impl;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif