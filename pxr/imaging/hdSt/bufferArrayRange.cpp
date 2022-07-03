//
// Copyright 2017 Pixar
//
// Licensed under the Apache License, Version 2.0 (the "Apache License")
// with the following modification; you may not use this file except in
// compliance with the Apache License and the following modification to it:
// Section 6. Trademarks. is deleted and replaced with:
//
// 6. Trademarks. This License does not grant permission to use the trade
//    names, trademarks, service marks, or product names of the Licensor
//    and its affiliates, except as required to comply with Section 4(c) of
//    the License and to reproduce the content of the NOTICE file.
//
// You may obtain a copy of the Apache License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the Apache License with the above modification is
// distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
// KIND, either express or implied. See the Apache License for the specific
// language governing permissions and limitations under the Apache License.
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
