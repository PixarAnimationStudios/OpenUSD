//
// Copyright 2026 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_IMAGING_HD_CACHING_SAMPLED_DATASOURCE_H
#define PXR_IMAGING_HD_CACHING_SAMPLED_DATASOURCE_H

#include "pxr/imaging/hd/dataSource.h"

#include <tbb/concurrent_hash_map.h>

// This file provides two classes HdCachingSampledDataSource and
// HdCachingTypedSampledDataSource which cache values obtained from GetValue
// and GetTypedValue across subsequent calls. The values are freed when the 
// caching data source is freed. They are thread safe using a
// tbb::concurrent_hash_map as the container. Users should be careful to make
// sure this is freed correctly after use as tbb::concurrent_hash_map can have
// some unwanted memory overhead.

PXR_NAMESPACE_OPEN_SCOPE

/// \class HdCachingSampledDataSource
///
/// A sampled data source which maintains a cache of values from the upstream
/// sampled data source, with values persisting until all handles to the data
/// source are released. For a given data source (or copies of that data source)
/// GetValue for a shutter offset will receive answers drawn from the upstream
/// data source, but subsequent calls at that shutter offset against the same
/// handle will receive a cached value. Data source caching is useful when the
/// value is large, costly to pull from the upstream source, and will be queried
/// multiple times by the same consumer for the same shutterOffset, with the
/// memory consumed by the cache being freed when the consumer releases the data
/// source handle.
///
class HdCachingSampledDataSource : public HdSampledDataSource
{
public:
    HD_DECLARE_DATASOURCE(HdCachingSampledDataSource);
    using Time = HdSampledDataSource::Time;
    using TimeValueMap = tbb::concurrent_hash_map<Time, VtValue>;

    VtValue
    GetValue(Time shutterOffset) override
    {
        if (!_input) {
            return VtValue();
        }

        // Lookup value in cache
        TimeValueMap::const_accessor const_acc;
        if (_cache.find(const_acc, shutterOffset)) {
            return const_acc->second;
        }
        // Cache miss compute value and store
        TimeValueMap::accessor acc;
        if (_cache.insert(acc, shutterOffset)) {
            acc->second = _input->GetValue(shutterOffset);
        }
        return acc->second;
    }

    bool 
    GetContributingSampleTimesForInterval(
        Time startTime,
        Time endTime,
        std::vector<Time> * outSampleTimes) override
    {
        if (!_input) {
            return false;
        }

        return _input->GetContributingSampleTimesForInterval(
            startTime, endTime, outSampleTimes
        );
    }

protected:
    HdCachingSampledDataSource(const HdSampledDataSourceHandle& input)
        : _input(input) {}

    const HdSampledDataSourceHandle _input;
    TimeValueMap _cache;
};

HD_DECLARE_DATASOURCE_HANDLES(HdCachingSampledDataSource);

/// \class HdCachingTypedSampledDataSource
///
/// A typed sampled data source which maintains a cache of values from the
/// upstream sampled data source, with values persisting until all handles to
/// the data source are released. For a given data source (or copies of that
/// data source) GetValue and GetTypedValue for a shutter offset will receive
/// answers drawn from the upstream data source, but subsequent calls at that
/// shutter offset against the same handle will receive a cached value. Data
/// source caching is useful when the value is large, costly to pull from the
/// upstream source, and will be queried multiple times by the same consumer for
/// the same shutterOffset, with the memory consumed by the cache being freed
/// when the consumer releases the data source handle.
///
template <typename T>
class HdCachingTypedSampledDataSource : public HdTypedSampledDataSource<T> 
{
public:
    HD_DECLARE_DATASOURCE(HdCachingTypedSampledDataSource<T>);
    using Time = HdSampledDataSource::Time;
    using TimeValueMap = tbb::concurrent_hash_map<Time, T>;

    T GetTypedValue(Time shutterOffset) override
    {
        if (!_input) {
            return T();
        }

        // Lookup value in cache
        typename TimeValueMap::const_accessor const_acc;
        if (_cache.find(const_acc, shutterOffset)) {
            return const_acc->second;
        }
        // Cache miss compute value and store
        typename TimeValueMap::accessor acc;
        if (_cache.insert(acc, shutterOffset)) {
            acc->second = _input->GetTypedValue(shutterOffset);
        }
        return acc->second;
    }

    VtValue GetValue(Time shutterOffset) override
    {
        return VtValue(GetTypedValue(shutterOffset));
    }

    bool GetContributingSampleTimesForInterval(
        Time startTime,
        Time endTime,
        std::vector<Time> * outSampleTimes) override
    {
        if (!_input) {
            return false;
        }

        return _input->GetContributingSampleTimesForInterval(
            startTime, endTime, outSampleTimes
        );
    }

protected:
    HdCachingTypedSampledDataSource(const typename HdTypedSampledDataSource<T>::Handle& input)
        : _input(input) {}
    
    const typename HdTypedSampledDataSource<T>::Handle _input;
    TimeValueMap _cache;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_IMAGING_HD_CACHING_SAMPLED_DATASOURCE_H
