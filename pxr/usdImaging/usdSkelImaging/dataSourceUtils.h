//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_USD_IMAGING_USD_SKEL_IMAGING_DATA_SOURCE_UTILS_H
#define PXR_USD_IMAGING_USD_SKEL_IMAGING_DATA_SOURCE_UTILS_H

#include "pxr/imaging/hd/dataSource.h"

PXR_NAMESPACE_OPEN_SCOPE

/// \class UsdSkelImagingSharedPtrThunk
///
/// A thunk for shared pointers computing the result only once and using
/// atomic operations to store the cached result.
///
template<typename T>
class UsdSkelImagingSharedPtrThunk
{
public:
    using Handle = std::shared_ptr<T>;

    Handle Get()
    {
        if (auto const result = std::atomic_load(&_data)) {
            return result;
        }

        Handle expected;
        Handle desired = _Compute();

        if (std::atomic_compare_exchange_strong(&_data, &expected, desired)) {
            return desired;
        } else {
            return expected;
        }
    }

    void Invalidate() { std::atomic_store(&_data, Handle()); }

protected:
    virtual Handle _Compute() = 0;

private:
    Handle _data;
};

template<typename T>
auto UsdSkelImagingGetTypedValue(
    const T &ds,
    const HdSampledDataSource::Time shutterOffset = 0.0f)
        -> decltype(ds->GetTypedValue(0.0f))
{
    if (!ds) {
        return {};
    }
    return ds->GetTypedValue(shutterOffset);
}

PXR_NAMESPACE_CLOSE_SCOPE

#endif
