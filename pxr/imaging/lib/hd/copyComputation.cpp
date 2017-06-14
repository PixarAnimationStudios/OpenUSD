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
#include "pxr/imaging/glf/glew.h"
#include "pxr/imaging/hd/copyComputation.h"
#include "pxr/imaging/hd/bufferArrayRangeGL.h"
#include "pxr/imaging/hd/bufferResourceGL.h"
#include "pxr/imaging/hd/perfLog.h"
#include "pxr/imaging/hd/renderContextCaps.h"
#include "pxr/imaging/hd/tokens.h"

#include "pxr/imaging/hf/perfLog.h"

PXR_NAMESPACE_OPEN_SCOPE


HdCopyComputationGPU::HdCopyComputationGPU(
    HdBufferArrayRangeSharedPtr const &src, TfToken const &name)
    : _src(src), _name(name)
{
}

void
HdCopyComputationGPU::Execute(HdBufferArrayRangeSharedPtr const &range_)
{
    HD_TRACE_FUNCTION();
    HF_MALLOC_TAG_FUNCTION();

    if (!glBufferSubData) {
        return;
    }

    HdBufferArrayRangeGLSharedPtr srcRange =
        boost::static_pointer_cast<HdBufferArrayRangeGL> (_src);
    HdBufferArrayRangeGLSharedPtr range =
        boost::static_pointer_cast<HdBufferArrayRangeGL> (range_);

    HdBufferResourceGLSharedPtr src = srcRange->GetResource(_name);
    HdBufferResourceGLSharedPtr dst = range->GetResource(_name);

    if (!TF_VERIFY(src)) {
        return;
    }
    if (!TF_VERIFY(dst)) {
        return;
    }

    int srcBytesPerElement = src->GetNumComponents() * src->GetComponentSize();
    int dstBytesPerElement = dst->GetNumComponents() * dst->GetComponentSize();

    if (!TF_VERIFY(srcBytesPerElement == dstBytesPerElement)) {
        return;
    }

    GLintptr readOffset = _src->GetOffset() *  srcBytesPerElement;
    GLintptr writeOffset = range->GetOffset() * dstBytesPerElement;
    GLsizeiptr copySize = _src->GetNumElements() * srcBytesPerElement;

    if (!TF_VERIFY(_src->GetNumElements() <= range->GetNumElements())) {
         return;
    }

    HdRenderContextCaps const &caps = HdRenderContextCaps::GetInstance();

    // Unfortunately at the time the copy computation is added, we don't
    // know if the source buffer has 0 length.  So we can get here with
    // a zero sized copy.
    if (copySize > 0) {

        // If the buffer's have 0 size, resources for them would not have
        // be allocated, so the check for resource allocation has been moved
        // until after the copy size check.

        GLint srcId = src->GetId();
        GLint dstId = dst->GetId();

        if (!TF_VERIFY(srcId)) {
            return;
        }
        if (!TF_VERIFY(dstId)) {
            return;
        }

        HD_PERF_COUNTER_INCR(HdPerfTokens->glCopyBufferSubData);

        if (caps.directStateAccessEnabled) {
            glNamedCopyBufferSubDataEXT(srcId, dstId,
                                        readOffset, writeOffset, copySize);
        } else {
            glBindBuffer(GL_COPY_READ_BUFFER, srcId);
            glBindBuffer(GL_COPY_WRITE_BUFFER, dstId);
            glCopyBufferSubData(GL_COPY_READ_BUFFER, GL_COPY_WRITE_BUFFER,
                                readOffset, writeOffset, copySize);

            glBindBuffer(GL_COPY_READ_BUFFER, 0);
            glBindBuffer(GL_COPY_WRITE_BUFFER, 0);
        }
    }
}

int
HdCopyComputationGPU::GetNumOutputElements() const
{
    return _src->GetNumElements();
}

void
HdCopyComputationGPU::AddBufferSpecs(HdBufferSpecVector *specs) const
{
    HdBufferArrayRangeGLSharedPtr srcRange =
        boost::static_pointer_cast<HdBufferArrayRangeGL> (_src);

    HdBufferResourceGLSharedPtr const &resource = srcRange->GetResource(_name);
    specs->push_back(HdBufferSpec(_name,
                                  resource->GetGLDataType(),
                                  resource->GetNumComponents()));
}

PXR_NAMESPACE_CLOSE_SCOPE

