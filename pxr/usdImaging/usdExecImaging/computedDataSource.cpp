//
// Copyright 2026 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/usdImaging/usdExecImaging/computedDataSource.h"

#include "pxr/base/tf/diagnostic.h"

PXR_NAMESPACE_OPEN_SCOPE

namespace
{

// A stub implementation of UsdExecImagingRequestAccessorInterface that always
// produces empty values.
//
// This implementation is used when a client erroneously constructs a data
// source with a null request accessor. By using this stub implementation, the
// data sources can always dereference their request accessors without fear of
// dereferencing a null pointer.
//
class _EmptyRequestAccessor final
    : public UsdExecImagingRequestAccessorInterface
{
public:
    VtValue GetComputedValue(const UsdExecImagingValueKey &valueKey) override {
        return {};
    }
};

} // anonymous namespace

UsdExecImaging_ComputedDataSourceImpl::UsdExecImaging_ComputedDataSourceImpl(
    UsdExecImagingRequestAccessorInterfaceSharedPtr requestAccessor,
    UsdExecImagingValueKey valueKey)
    : _requestAccessor(std::move(requestAccessor))
    , _valueKey(std::move(valueKey))
{
    if (!_requestAccessor) {
        TF_CODING_ERROR(
            "UsdExecImaging data source constructed with null request "
            "accessor.");
        _requestAccessor = std::make_shared<_EmptyRequestAccessor>();
    }
}

VtValue
UsdExecImaging_ComputedDataSourceImpl::GetValue(Time shutterOffset) const
{
    return _requestAccessor->GetComputedValue(_valueKey);
}

bool
UsdExecImaging_ComputedDataSourceImpl::GetContributingSampleTimesForInterval(
        Time startTime, 
        Time endTime,
        std::vector<Time> * outSampleTimes) const
{
    return false;
}

UsdExecImagingComputedSampledDataSource::
~UsdExecImagingComputedSampledDataSource() = default;

PXR_NAMESPACE_CLOSE_SCOPE
