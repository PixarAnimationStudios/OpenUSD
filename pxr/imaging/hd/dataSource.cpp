//
// Copyright 2021 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/imaging/hd/dataSource.h"

#include "pxr/base/trace/trace.h"

#include <iostream>

PXR_NAMESPACE_OPEN_SCOPE

HdDataSourceBase::~HdDataSourceBase() = default;

// ---------------------------------------------------------------------------

HdDataSourceBaseHandle
HdContainerDataSource::Get(
    const HdContainerDataSourceHandle &container,
    const HdDataSourceLocator &locator)
{
    if (!container) {
        return nullptr;
    }

    const size_t count = locator.GetElementCount();
    if (count == 0) {
        return container;
    }

    HdDataSourceBaseHandle child;
    HdContainerDataSourceHandle tmp = container;

    for (size_t i = 0; i < count; ++i) {
        child = tmp->Get(locator.GetElement(i));
        if (!child) {
            break;
        }

        if (i < count - 1) {
            tmp = HdContainerDataSource::Cast(child);
            if (!tmp) {
                return nullptr;
            }
        }
    }

    return child;
}

// ----------------------------------------------------------------------------

// Wrapper around std::set_union to compute the set-wise union of two
// sorted vectors of sample times.
static std::vector<HdSampledDataSource::Time>
_Union(const std::vector<HdSampledDataSource::Time> &a,
       const std::vector<HdSampledDataSource::Time> &b)
{
    std::vector<HdSampledDataSource::Time> result;
    std::set_union(a.begin(), a.end(),
                   b.begin(), b.end(),
                   std::back_inserter(result));
    return result;
}

// Computes union of contributing sample times from several data sources.
bool
HdGetMergedContributingSampleTimesForInterval(
    const size_t count,
    const HdSampledDataSourceHandle * const inputDataSources,
    const HdSampledDataSource::Time startTime,
    const HdSampledDataSource::Time endTime,
    std::vector<HdSampledDataSource::Time> * const outSampleTimes)
{
    TRACE_FUNCTION();

    bool result = false;
    for (size_t i = 0; i < count; i++) {
        if (!inputDataSources[i]) {
            continue;
        }
        std::vector<HdSampledDataSource::Time> times;
        if (!inputDataSources[i]->GetContributingSampleTimesForInterval(
                startTime, endTime, &times)) {
            continue;
        }
        if (times.empty()) {
            continue;
        }
        result = true;
        if (!outSampleTimes) {
            continue;
        }
        if (outSampleTimes->empty()) {
            *outSampleTimes = std::move(times);
        } else {
            *outSampleTimes = _Union(*outSampleTimes, times);
        }
    }

    // TODO:
    //
    // We can potentially drop sample times outside of startTime and endTime.
    // For example, assume that startTime = 0 and endTime = 1 and that we
    // have two input data sources.
    // Furthermore, assume that the first input data source gives samples for
    // -1 and 1 and second for -2 and 1.
    // Currently, the function would return -2, -1 and 1 as times. But both
    // -2 and -1 are before the startTime, so we can drop -2.

    return result;
}

void
HdDebugPrintDataSource(
    std::ostream &s,
    HdDataSourceBaseHandle dataSource,
    int level)
{
    if (HdContainerDataSourceHandle handle =
            HdContainerDataSource::Cast(dataSource)) {
        TfTokenVector names = handle->GetNames();
        std::sort(names.begin(), names.end(), std::less<TfToken>());

        for (TfToken const &name : names) {
            // null children from a container should be treated as not present
            if (HdDataSourceBaseHandle childhandle = handle->Get(name)) {
                s << std::string(level, '\t') << "[" << name << "]\n";
                HdDebugPrintDataSource(s, childhandle, level+1);
            }
        }
    } else if (HdVectorDataSourceHandle handle =
                HdVectorDataSource::Cast(dataSource)) {
        size_t nElements = handle->GetNumElements();
        for (size_t i = 0; i < nElements; ++i) {
            s << std::string(level, '\t') << "[" << i << "]\n";
            HdDebugPrintDataSource(s, handle->GetElement(i), level+1);
        }
    } else if (HdSampledDataSourceHandle handle =
                HdSampledDataSource::Cast(dataSource)) {
        s << std::string(level, '\t') << handle->GetValue(0.0f) << "\n";
    } else if (dataSource) {
        s << std::string(level, '\t') << "UNKNOWN\n";
    } else {
        s << std::string(level, '\t') << "NULL\n";
    }
}

void
HdDebugPrintDataSource(HdDataSourceBaseHandle dataSource, int level)
{
    HdDebugPrintDataSource(std::cout, dataSource, level);
}

PXR_NAMESPACE_CLOSE_SCOPE
