//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/imaging/hd/bufferArrayRange.h"
#include "pxr/imaging/hd/bufferSpec.h"
#include "pxr/imaging/hd/perfLog.h"
#include "pxr/imaging/hd/tokens.h"

PXR_NAMESPACE_OPEN_SCOPE

HdBufferArrayRange::HdBufferArrayRange() = default;
HdBufferArrayRange::~HdBufferArrayRange() = default;

std::ostream &operator <<(std::ostream &out,
                          const HdBufferArrayRange &self)
{
    // call virtual
    self.DebugDump(out);
    return out;
}

void
HdBufferArrayRangeContainer::Set(int index,
                                 HdBufferArrayRangeSharedPtr const &range)
{
    HD_TRACE_FUNCTION();

    if (index < 0) {
        TF_CODING_ERROR("Index negative in HdBufferArrayRangeContainer::Set()");
        return;
    }

    if (static_cast<size_t>(index) >= _ranges.size()) {
        HD_PERF_COUNTER_INCR(HdPerfTokens->bufferArrayRangeContainerResized);
        _ranges.resize(index + 1);
    }
    _ranges[index] = range;
}

HdBufferArrayRangeSharedPtr const &
HdBufferArrayRangeContainer::Get(int index) const
{
    if (index < 0 || static_cast<size_t>(index) >= _ranges.size()) {
        // out of range access is not an errorneous path.
        // (i.e. element/instance bars can be null if not exists)
        static HdBufferArrayRangeSharedPtr empty;
        return empty;
    }
    return _ranges[index];
}

void
HdBufferArrayRangeContainer::Resize(int size)
{
    HD_TRACE_FUNCTION();

    if (size < 0) {
        TF_CODING_ERROR("Size negative in "
            "HdBufferArrayRangeContainer::Resize()");
        return;
    }

    HD_PERF_COUNTER_INCR(HdPerfTokens->bufferArrayRangeContainerResized);
    _ranges.resize(size);
}

PXR_NAMESPACE_CLOSE_SCOPE

