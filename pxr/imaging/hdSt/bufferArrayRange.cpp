//
// Copyright 2017 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/imaging/hdSt/bufferArrayRange.h"
#include "pxr/imaging/hdSt/bufferResource.h"
#include "pxr/imaging/hd/bufferSpec.h"
#include "pxr/imaging/hd/perfLog.h"
#include "pxr/imaging/hd/tokens.h"

PXR_NAMESPACE_OPEN_SCOPE

HdStBufferArrayRange::HdStBufferArrayRange(
    HdStResourceRegistry* resourceRegistry)
    : _resourceRegistry(resourceRegistry)
{
}

HdStBufferArrayRange::~HdStBufferArrayRange() 
{
}

void
HdStBufferArrayRange::GetBufferSpecs(HdBufferSpecVector *specs) const
{
    HD_TRACE_FUNCTION();

    HdStBufferResourceNamedList const &resources = GetResources();

    TF_FOR_ALL(it, resources) {
        specs->emplace_back(it->first, it->second->GetTupleType());
    }
}

HdStResourceRegistry*
HdStBufferArrayRange::GetResourceRegistry()
{
    return _resourceRegistry;
}

HdStResourceRegistry*
HdStBufferArrayRange::GetResourceRegistry() const
{
    return _resourceRegistry;
}

std::ostream &operator <<(std::ostream &out,
                          const HdStBufferArrayRange &self)
{
    // call virtual
    self.DebugDump(out);
    return out;
}

void
HdStBufferArrayRangeContainer::Set(int index,
                                 HdStBufferArrayRangeSharedPtr const &range)
{
    HD_TRACE_FUNCTION();

    if (index < 0) {
        TF_CODING_ERROR("Index negative in HdStBufferArrayRangeContainer::Set()");
        return;
    }

    if (static_cast<size_t>(index) >= _ranges.size()) {
        HD_PERF_COUNTER_INCR(HdPerfTokens->bufferArrayRangeContainerResized);
        _ranges.resize(index + 1);
    }
    _ranges[index] = range;
}

HdStBufferArrayRangeSharedPtr const &
HdStBufferArrayRangeContainer::Get(int index) const
{
    if (index < 0 || static_cast<size_t>(index) >= _ranges.size()) {
        // out of range access is not an errorneous path.
        // (i.e. element/instance bars can be null if not exists)
        static HdStBufferArrayRangeSharedPtr empty;
        return empty;
    }
    return _ranges[index];
}


PXR_NAMESPACE_CLOSE_SCOPE
