//
// Copyright 2021 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_IMAGING_HD_RETAINEDDATASOURCE_H
#define PXR_IMAGING_HD_RETAINEDDATASOURCE_H

#include "pxr/pxr.h"

#include "pxr/imaging/hd/api.h"
#include "pxr/imaging/hd/dataSource.h"

#include "pxr/usd/sdf/pathExpression.h"

#include "pxr/base/tf/smallVector.h"
#include "pxr/base/tf/denseHashMap.h"

PXR_NAMESPACE_OPEN_SCOPE

/// \class HdRetainedContainerDataSource
///
/// A retained container data source is a data source whose data are available
/// locally, meaning that they are fully stored and contained within the data
/// source. These are typically used for the kinds of operations that need to
/// break away from the more live data sources (e.g., those that query a
/// backing scene).
///
class HdRetainedContainerDataSource : public HdContainerDataSource
{
public:
    HD_DECLARE_DATASOURCE_ABSTRACT(HdRetainedContainerDataSource);

    HD_API
    static Handle New();

    HD_API
    static Handle New(
        size_t count, 
        const TfToken *names,
        const HdDataSourceBaseHandle *values);

    HD_API
    static Handle New(
        const TfToken &name1,
        const HdDataSourceBaseHandle &value1);

    HD_API
    static Handle New(
        const TfToken &name1,
        const HdDataSourceBaseHandle &value1,
        const TfToken &name2,
        const HdDataSourceBaseHandle &value2);

    HD_API 
    static Handle New(
        const TfToken &name1,
        const HdDataSourceBaseHandle &value1,
        const TfToken &name2,
        const HdDataSourceBaseHandle &value2,
        const TfToken &name3,
        const HdDataSourceBaseHandle &value3);

    HD_API
    static Handle New(
        const TfToken &name1,
        const HdDataSourceBaseHandle &value1,
        const TfToken &name2,
        const HdDataSourceBaseHandle &value2,
        const TfToken &name3,
        const HdDataSourceBaseHandle &value3,
        const TfToken &name4,
        const HdDataSourceBaseHandle &value4);

    HD_API
    static Handle New(
        const TfToken &name1,
        const HdDataSourceBaseHandle &value1,
        const TfToken &name2,
        const HdDataSourceBaseHandle &value2,
        const TfToken &name3,
        const HdDataSourceBaseHandle &value3,
        const TfToken &name4,
        const HdDataSourceBaseHandle &value4,
        const TfToken &name5,
        const HdDataSourceBaseHandle &value5);

    HD_API
    static Handle New(
        const TfToken &name1,
        const HdDataSourceBaseHandle &value1,
        const TfToken &name2,
        const HdDataSourceBaseHandle &value2,
        const TfToken &name3,
        const HdDataSourceBaseHandle &value3,
        const TfToken &name4,
        const HdDataSourceBaseHandle &value4,
        const TfToken &name5,
        const HdDataSourceBaseHandle &value5,
        const TfToken &name6,
        const HdDataSourceBaseHandle &value6);
};

HD_DECLARE_DATASOURCE_HANDLES(HdRetainedContainerDataSource);

//-----------------------------------------------------------------------------

/// \class HdRetainedSampledDataSource
///
/// A retained data source for sampled data. Typically used when the data needs
/// to be locally stored and cut off from any backing scene data.
///
class HdRetainedSampledDataSource : public HdSampledDataSource
{
public:
    HD_DECLARE_DATASOURCE(HdRetainedSampledDataSource);

    HdRetainedSampledDataSource(VtValue value)
    : _value(value) {}

    bool GetContributingSampleTimesForInterval(
        HdSampledDataSource::Time startTime,
        HdSampledDataSource::Time endTime,
        std::vector<HdSampledDataSource::Time> *outSampleTimes) override
    {
        return false;
    }

    VtValue GetValue(HdSampledDataSource::Time shutterOffset) override
    {
        return _value;
    }

private:
    VtValue _value;
};

HD_DECLARE_DATASOURCE_HANDLES(HdRetainedSampledDataSource);

//-----------------------------------------------------------------------------

/// \class HdRetainedTypedSampledDataSource
///
/// Similar to HdRetainedSampledDataSource but provides strongly typed
/// semantics.
///
template <typename T>
class HdRetainedTypedSampledDataSource : public HdTypedSampledDataSource<T>
{
public:
    //abstract to implement New outside in service of specialization
    HD_DECLARE_DATASOURCE_ABSTRACT(HdRetainedTypedSampledDataSource<T>);

    HdRetainedTypedSampledDataSource(const T &value)
    : _value(value) {}

    bool GetContributingSampleTimesForInterval(
        HdSampledDataSource::Time startTime,
        HdSampledDataSource::Time endTime,
        std::vector<HdSampledDataSource::Time> *outSampleTimes) override
    {
        return false;
    }

    VtValue GetValue(HdSampledDataSource::Time shutterOffset) override
    {
        return VtValue(_value);
    }

    T GetTypedValue(HdSampledDataSource::Time shutterOffset) override
    {
        return _value;
    }

    static 
    typename HdRetainedTypedSampledDataSource<T>::Handle New(const T &value);

protected:
    T _value;
};

// New is specializable for cases where instances may be shared for efficiency
template <typename T>
typename HdRetainedTypedSampledDataSource<T>::Handle
HdRetainedTypedSampledDataSource<T>::New(const T &value)
{
    return HdRetainedTypedSampledDataSource<T>::Handle(
        new HdRetainedTypedSampledDataSource<T>(value));
}

template <>
HdRetainedTypedSampledDataSource<bool>::Handle
HdRetainedTypedSampledDataSource<bool>::New(const bool &value);

//-----------------------------------------------------------------------------

/// \class HdRetainedTypedMultisampleDataSource
///
/// Similar to HdRetainedTypedSampledDataSource but is capable of holding on to
/// multiple samples at once.
///
template <typename T>
class HdRetainedTypedMultisampledDataSource : public HdTypedSampledDataSource<T>
{
public:
    HD_DECLARE_DATASOURCE(HdRetainedTypedMultisampledDataSource<T>);

    HdRetainedTypedMultisampledDataSource(
        size_t count, 
        HdSampledDataSource::Time *sampleTimes,
        T *sampleValues);

    bool GetContributingSampleTimesForInterval(
        HdSampledDataSource::Time startTime,
        HdSampledDataSource::Time endTime,
        std::vector<HdSampledDataSource::Time> *outSampleTimes) override;

    VtValue GetValue(HdSampledDataSource::Time shutterOffset) override
    {
        return VtValue(GetTypedValue(shutterOffset));
    }

    T GetTypedValue(HdSampledDataSource::Time shutterOffset) override;

private:
    typedef std::pair<HdSampledDataSource::Time, T> _SamplePair;
    TfSmallVector<_SamplePair, 6> _sampledValues;
};

template <typename T>
HdRetainedTypedMultisampledDataSource<T>::HdRetainedTypedMultisampledDataSource(
    size_t count, 
    HdSampledDataSource::Time *sampleTimes,
    T *sampleValues)
{
    _sampledValues.reserve(count);

    // XXX: For now, assume that sample times are ordered.
    // We could sort them if needed.
    for (size_t i = 0; i < count; ++i) {
        _sampledValues.emplace_back(sampleTimes[i], sampleValues[i]);
    }
}

template <typename T>
bool
HdRetainedTypedMultisampledDataSource<T>::GetContributingSampleTimesForInterval(
    HdSampledDataSource::Time startTime,
    HdSampledDataSource::Time endTime,
    std::vector<HdSampledDataSource::Time> *outSampleTimes)
{
    if (_sampledValues.size() < 2) {
        return false;
    }

    if (outSampleTimes) {
        outSampleTimes->clear();

        // XXX: Include all stored samples for now.
        outSampleTimes->reserve(_sampledValues.size());

        for (const auto & iter : _sampledValues) {
            outSampleTimes->push_back(iter.first);
        }
    }

    return true;
}

template <typename T>
T
HdRetainedTypedMultisampledDataSource<T>::GetTypedValue(
    HdSampledDataSource::Time shutterOffset)
{
    if (_sampledValues.empty()) {
        return T();
    }

    const HdSampledDataSource::Time epsilon = 0.0001;

    for (size_t i = 0, e = _sampledValues.size(); i < e; ++i) {

        const HdSampledDataSource::Time & sampleTime = _sampledValues[i].first;

        if (sampleTime > shutterOffset) {
            
            // If we're first and we're already bigger, return us.
            if (i < 1) {
                return _sampledValues[i].second;
            } else {

                // This will always be positive
                const HdSampledDataSource::Time delta = 
                    sampleTime - shutterOffset;

                // If we're kinda equal, go for it
                if (delta < epsilon) {
                    return _sampledValues[i].second;
                }

                // Since we're already over the requested time, let's see
                // if it's closer, use it instead of me. In the case of a
                // tie, use the earlier.
                const HdSampledDataSource::Time previousDelta =
                        shutterOffset - _sampledValues[i - 1].first;

                if (previousDelta <= delta) {
                    return _sampledValues[i - 1].second;
                } else {
                    return _sampledValues[i].second;
                }
            }
        } else {
            if (fabs(sampleTime - shutterOffset) < epsilon) {
                return _sampledValues[i].second;
            }
        }
    }

    // Never were in range, return our last sample
    return _sampledValues.back().second;
}

//-----------------------------------------------------------------------------

/// \class HdRetainedSmallVectorDataSource
///
/// A retained data source version of HdVectorDataSource. 
///
/// Internally it uses a TfSmallVector with up to 32 locally stored entries
/// for storage.
///
class HdRetainedSmallVectorDataSource : public HdVectorDataSource
{
public:
    HD_DECLARE_DATASOURCE(HdRetainedSmallVectorDataSource);

    HD_API
    HdRetainedSmallVectorDataSource(
        size_t count, 
        const HdDataSourceBaseHandle *values);

    HD_API
    size_t GetNumElements() override;

    HD_API
    HdDataSourceBaseHandle GetElement(size_t element) override;

private:
    TfSmallVector<HdDataSourceBaseHandle, 32> _values;
};

HD_DECLARE_DATASOURCE_HANDLES(HdRetainedSmallVectorDataSource);

// Utilities //////////////////////////////////////////////////////////////////

/// Given a VtValue, attempt to create a RetainedTypedSampledDataSource of
/// the appropriate type via type dispatch.
HD_API
HdSampledDataSourceHandle
HdCreateTypedRetainedDataSource(VtValue const &v);

/// Attempt to make a copy of the given data source using the sample at
/// time 0.0f if it or a descendant data source is sampled.
HD_API
HdDataSourceBaseHandle
HdMakeStaticCopy(HdDataSourceBaseHandle const &ds);

/// Attempt to make a copy of the given container data source using the sample
/// at time 0.0f if a descendant data source is sampled.
HD_API
HdContainerDataSourceHandle
HdMakeStaticCopy(HdContainerDataSourceHandle const &ds);

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_IMAGING_HD_RETAINEDDATASOURCE_H
