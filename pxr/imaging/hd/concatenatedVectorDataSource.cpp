//
// Copyright 2026 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/imaging/hd/concatenatedVectorDataSource.h"

PXR_NAMESPACE_OPEN_SCOPE

HdConcatenatedVectorDataSource::HdConcatenatedVectorDataSource(
    const std::initializer_list<HdVectorDataSourceHandle> &sources)
  : _sources(sources.begin(), sources.end())
{
}

HdConcatenatedVectorDataSource::HdConcatenatedVectorDataSource(
    const size_t count,
    HdVectorDataSourceHandle * const sources)
{
    _sources.reserve(count);
    for (size_t i = 0; i < count; ++i) {
        _sources.push_back(sources[i]);
    }
}

HdConcatenatedVectorDataSource::HdConcatenatedVectorDataSource(
    const HdVectorDataSourceHandle &src1,
    const HdVectorDataSourceHandle &src2)
{
    _sources = { src1, src2 };
}

HdConcatenatedVectorDataSource::HdConcatenatedVectorDataSource(
    const HdVectorDataSourceHandle &src1,
    const HdVectorDataSourceHandle &src2,
    const HdVectorDataSourceHandle &src3)
{
    _sources = { src1, src2, src3 };
}

HdVectorDataSourceHandle
HdConcatenatedVectorDataSource::ConcatenatedVectorDataSources(
        const HdVectorDataSourceHandle &src1,
        const HdVectorDataSourceHandle &src2)
{
    if (!src1) {
        return src2;
    }
    if (!src2) {
        return src1;
    }
    return HdConcatenatedVectorDataSource::New(src1, src2);
}

size_t
HdConcatenatedVectorDataSource::GetNumElements()
{
    size_t total = 0;

    for (const HdVectorDataSourceHandle &source : _sources) {
        if (source) {
            total += source->GetNumElements();
        }
    }

    return total;
}

HdDataSourceBaseHandle
HdConcatenatedVectorDataSource::GetElement(const size_t i)
{
    size_t running_total = 0;

    for (const HdVectorDataSourceHandle &source : _sources) {
        if (source) {
            const size_t next_total = running_total + source->GetNumElements();
            if (running_total <= i && i < next_total) {
                return source->GetElement(i - running_total);
            }
            running_total = next_total;
        }
    }

    return nullptr;
}

PXR_NAMESPACE_CLOSE_SCOPE

