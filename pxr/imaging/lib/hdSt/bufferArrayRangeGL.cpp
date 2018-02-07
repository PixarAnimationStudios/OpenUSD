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
#include "pxr/imaging/glf/glew.h"
#include "pxr/imaging/hdSt/bufferArrayRangeGL.h"
#include "pxr/imaging/hdSt/bufferResourceGL.h"
#include "pxr/imaging/hd/bufferSpec.h"
#include "pxr/imaging/hd/perfLog.h"
#include "pxr/imaging/hd/tokens.h"

PXR_NAMESPACE_OPEN_SCOPE


HdStBufferArrayRangeGL::~HdStBufferArrayRangeGL() 
{
}

void
HdStBufferArrayRangeGL::AddBufferSpecs(HdBufferSpecVector *specs) const
{
    HD_TRACE_FUNCTION();

    HdStBufferResourceGLNamedList const &resources = GetResources();

    TF_FOR_ALL(it, resources) {
        specs->emplace_back(it->first, it->second->GetTupleType());
    }
}

std::ostream &operator <<(std::ostream &out,
                          const HdStBufferArrayRangeGL &self)
{
    // call virtual
    self.DebugDump(out);
    return out;
}

void
HdStBufferArrayRangeGLContainer::Set(int index,
                                 HdStBufferArrayRangeGLSharedPtr const &range)
{
    HD_TRACE_FUNCTION();

    if (index < 0) {
        TF_CODING_ERROR("Index negative in HdStBufferArrayRangeGLContainer::Set()");
        return;
    }

    if (static_cast<size_t>(index) >= _ranges.size()) {
        HD_PERF_COUNTER_INCR(HdPerfTokens->bufferArrayRangeContainerResized);
        _ranges.resize(index + 1);
    }
    _ranges[index] = range;
}

HdStBufferArrayRangeGLSharedPtr const &
HdStBufferArrayRangeGLContainer::Get(int index) const
{
    if (index < 0 || static_cast<size_t>(index) >= _ranges.size()) {
        // out of range access is not an errorneous path.
        // (i.e. element/instance bars can be null if not exists)
        static HdStBufferArrayRangeGLSharedPtr empty;
        return empty;
    }
    return _ranges[index];
}


PXR_NAMESPACE_CLOSE_SCOPE
