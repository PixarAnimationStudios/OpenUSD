//
// Copyright 2016 Pixar
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
#include "pxr/imaging/hd/bufferArrayRange.h"
#include "pxr/imaging/hd/bufferSpec.h"
#include "pxr/imaging/hd/perfLog.h"
#include "pxr/imaging/hd/tokens.h"

HdBufferArrayRange::~HdBufferArrayRange() {
}

void
HdBufferArrayRange::AddBufferSpecs(HdBufferSpecVector *specs) const
{
    HD_TRACE_FUNCTION();

    HdBufferResourceNamedList const &resources = GetResources();

    TF_FOR_ALL(it, resources) {
        specs->push_back(HdBufferSpec(it->first,
                                      it->second->GetGLDataType(),
                                      it->second->GetNumComponents()));
    }
}

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
    if (index < 0 or static_cast<size_t>(index) >= _ranges.size()) {
        // out of range access is not an errorneous path.
        // (i.e. element/instance bars can be null if not exists)
        static HdBufferArrayRangeSharedPtr empty;
        return empty;
    }
    return _ranges[index];
}
