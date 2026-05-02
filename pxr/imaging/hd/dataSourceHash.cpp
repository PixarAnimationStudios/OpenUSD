//
// Copyright 2023 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/imaging/hd/dataSourceHash.h"

#include "pxr/base/tf/token.h"
#include "pxr/base/vt/array.h"
#include "pxr/base/vt/types.h"
#include "pxr/base/vt/value.h"
#include "pxr/base/vt/visitValue.h"

#include <string>
#include <vector>

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

// _HashVisitor will be used to intercept sampled data source values
// and provide an alternate hash value to the one that would otherwise
// result from the value type's TfHash implementation. This is important
// for some value types whose TfHash implementations are not stable from
// one run to the next. Currently, these include:
//
//  - TfToken: default hash includes a pointer address
//
// Ideally, value type hash implementations should be stable, and that is the
// preferred solution to unstable data source hashes. However, sometimes a
// value type's hash needs to be unstable for performance reasons (as is the
// case with TfToken), so this visitor should restrict itself to those cases.
//
// TODO: There exist other specializations of HdTypedSampledDataSource that may
// produce hash instability:
//
//  - ArResolverContext
//  - UsdStageRefPtr
//  - HdExtComputationCpuCallbackSharedPtr
//  - HdLegacyTaskFactorySharedPtr
//  - HdsiPrimManagingSceneIndexObserver::PrimFactoryBaseHandle
//  - HdsiPrimTypeNoticeBatchingSceneIndex::PrimTypePriorityFunctorHandle
//
// These should be investigated first if data source hash instability becomes
// a problem again.
struct _HashVisitor
{
    VtValue
    operator()(const TfToken& x) const
    {
        return VtValue(x.GetString());
    }

    VtValue
    operator()(const VtArray<TfToken>& x) const
    {
        VtArray<std::string> stringArray(x.size());
        for (const TfToken& t : x) {
            stringArray.push_back(t.GetString());
        }
        return VtValue(stringArray);
    }

    VtValue
    operator()(const std::vector<TfToken>& x) const
    {
        return (*this)(VtArray<TfToken>(x.cbegin(), x.cend()));
    }

    VtValue
    operator()(const VtValue& x) const
    {
        return x;
    }
};

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
        h.Append(VtVisitValue(
            ds.first->GetValue(ds.second.startTime), _HashVisitor()));
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
            h.Append(VtVisitValue(ds.first->GetValue(t), _HashVisitor()));
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
            // TfToken::Hash() includes a pointer address, so use the hash of
            // the string value instead.
            h.Append(name.GetString());
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
