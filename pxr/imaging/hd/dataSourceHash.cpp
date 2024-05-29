//
// Copyright 2023 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/imaging/hd/dataSourceHash.h"

PXR_NAMESPACE_OPEN_SCOPE

namespace
{

struct _Args
{
    HdSampledDataSource::Time startTime;
    HdSampledDataSource::Time endTime;
};

template<typename Handle>
using _Pair = std::pair<Handle const &, const _Args &>;

template<typename Handle>
_Pair<Handle>
_MakePair(Handle const &ds, const _Args &args)
{
    return {ds, args};
}

}

template <class HashState>
void TfHashAppend(HashState &h, _Pair<HdSampledDataSourceHandle> ds);

template <class HashState>
void TfHashAppend(HashState &h, _Pair<HdVectorDataSourceHandle> ds);

template <class HashState>
void TfHashAppend(HashState &h, _Pair<HdContainerDataSourceHandle> ds);

template<class HashState>
void TfHashAppend(HashState &h, _Pair<HdDataSourceBaseHandle> ds);

template<class HashState>
void TfHashAppend(HashState &h, _Pair<HdDataSourceBaseHandle> ds)
{
    if (HdSampledDataSourceHandle const c =
            HdSampledDataSource::Cast(ds.first)) {
        h.Append(_MakePair(c, ds.second));
    }
    if (HdVectorDataSourceHandle const c =
            HdVectorDataSource::Cast(ds.first)) {
        h.Append(_MakePair(c, ds.second));
    }
    if (HdContainerDataSourceHandle const c =
            HdContainerDataSource::Cast(ds.first)) {
        h.Append(_MakePair(c, ds.second));
    }
}

template <class HashState>
void TfHashAppend(HashState &h, _Pair<HdSampledDataSourceHandle> ds)
{
    if (ds.second.startTime == ds.second.endTime) {
        h.Append(ds.first->GetValue(ds.second.startTime));
    } else {
        std::vector<HdSampledDataSource::Time> sampleTimes;
        ds.first->GetContributingSampleTimesForInterval(
            ds.second.startTime, ds.second.endTime, &sampleTimes);
        if (sampleTimes.empty()) {
            sampleTimes.push_back(ds.second.startTime);
        }
        h.Append(TfHashAsCStr("TSB"));
        for (const HdSampledDataSource::Time t : sampleTimes) {
            h.Append(TfHashAsCStr("Time"));
            h.Append(t);
            h.Append(TfHashAsCStr("Value"));
            h.Append(ds.first->GetValue(t));
        }
        h.Append(TfHashAsCStr("TSE"));
    }
}

template <class HashState>
void TfHashAppend(HashState &h, _Pair<HdVectorDataSourceHandle> ds)
{
    const size_t n = ds.first->GetNumElements();

    h.Append(TfHashAsCStr("VB"));
    for (size_t i = 0; i < n; ++i) {
        h.Append(TfHashAsCStr("Element"));
        if (HdDataSourceBaseHandle childDs = ds.first->GetElement(i)) {
            h.Append(_MakePair(childDs, ds.second));
        }
    }
    h.Append(TfHashAsCStr("VE"));
}

template <class HashState>
void TfHashAppend(HashState &h, _Pair<HdContainerDataSourceHandle> ds)
{
    std::vector<TfToken> names = ds.first->GetNames();
    std::sort(names.begin(), names.end());
    auto last = std::unique(names.begin(), names.end());
    names.erase(last, names.end());

    h.Append(TfHashAsCStr("CB"));
    for (const TfToken &name : names) {
        if (HdDataSourceBaseHandle const childDs = ds.first->Get(name)) {
            h.Append(TfHashAsCStr("Key"));
            h.Append(name);
            h.Append(TfHashAsCStr("Value"));
            h.Append(_MakePair(childDs, ds.second));
        }            
    }
    h.Append(TfHashAsCStr("CE"));
}

size_t
HdDataSourceHash(HdDataSourceBaseHandle const &ds,
                 const HdSampledDataSource::Time startTime,
                 const HdSampledDataSource::Time endTime)
{
    return TfHash()(_MakePair(ds, {startTime, endTime}));
}

PXR_NAMESPACE_CLOSE_SCOPE
