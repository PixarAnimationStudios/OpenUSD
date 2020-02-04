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
#include "pxr/imaging/glf/contextCaps.h"
#include "pxr/imaging/glf/diagnostic.h"

#include "pxr/imaging/hdSt/copyComputation.h"
#include "pxr/imaging/hdSt/bufferArrayRangeGL.h"
#include "pxr/imaging/hdSt/bufferResourceGL.h"
#include "pxr/imaging/hd/perfLog.h"
#include "pxr/imaging/hd/tokens.h"
#include "pxr/imaging/hd/types.h"

#include "pxr/imaging/hf/perfLog.h"

PXR_NAMESPACE_OPEN_SCOPE


HdStCopyComputationGPU::HdStCopyComputationGPU(
    HdBufferArrayRangeSharedPtr const &src, TfToken const &name)
    : _src(src), _name(name)
{
}

void
HdStCopyComputationGPU::Execute(HdBufferArrayRangeSharedPtr const &range_,
                              HdResourceRegistry *resourceRegistry)
{
    HD_TRACE_FUNCTION();
    HF_MALLOC_TAG_FUNCTION();

    if (!glBufferSubData) {
        return;
    }

    HdStBufferArrayRangeGLSharedPtr srcRange =
        boost::static_pointer_cast<HdStBufferArrayRangeGL> (_src);
    HdStBufferArrayRangeGLSharedPtr dstRange =
        boost::static_pointer_cast<HdStBufferArrayRangeGL> (range_);

    HdStBufferResourceGLSharedPtr srcRes = srcRange->GetResource(_name);
    HdStBufferResourceGLSharedPtr dstRes = dstRange->GetResource(_name);

    if (!TF_VERIFY(srcRes)) {
        return;
    }
    if (!TF_VERIFY(dstRes)) {
        return;
    }

    int srcResSize = HdDataSizeOfTupleType(srcRes->GetTupleType()) *
                     srcRange->GetNumElements();
    int dstResSize = HdDataSizeOfTupleType(dstRes->GetTupleType()) *
                     dstRange->GetNumElements();

    if (!TF_VERIFY(srcResSize <= dstResSize)) {
        // The number of elements in the BAR *can* differ during migration.
        // One example is during mesh refinement when migration is necessary,
        // and we copy only the unrefined data over.
        TF_CODING_ERROR("Migration error for %s: Source resource (%d) size is "
            "larger than destination resource size (%d)\n",
            _name.GetText(), srcResSize, dstResSize);
        return;
    }

    GLintptr readOffset = srcRange->GetByteOffset(_name) + srcRes->GetOffset();
    GLintptr writeOffset = dstRange->GetByteOffset(_name) + dstRes->GetOffset();
    GLsizeiptr copySize = srcResSize;

    GlfContextCaps const &caps = GlfContextCaps::GetInstance();

    // Unfortunately at the time the copy computation is added, we don't
    // know if the source buffer has 0 length.  So we can get here with
    // a zero sized copy.
    if (copySize > 0) {

        // If the buffer's have 0 size, resources for them would not have
        // be allocated, so the check for resource allocation has been moved
        // until after the copy size check.

        GLint srcId = srcRes->GetId();
        GLint dstId = dstRes->GetId();

        if (!TF_VERIFY(srcId)) {
            return;
        }
        if (!TF_VERIFY(dstId)) {
            return;
        }

        HD_PERF_COUNTER_INCR(HdPerfTokens->glCopyBufferSubData);

        if (caps.directStateAccessEnabled) {
            glCopyNamedBufferSubData(srcId, dstId,
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

    GLF_POST_PENDING_GL_ERRORS();
}

int
HdStCopyComputationGPU::GetNumOutputElements() const
{
    return _src->GetNumElements();
}

void
HdStCopyComputationGPU::GetBufferSpecs(HdBufferSpecVector *specs) const
{
    HdStBufferArrayRangeGLSharedPtr srcRange =
        boost::static_pointer_cast<HdStBufferArrayRangeGL> (_src);

    specs->emplace_back(_name, srcRange->GetResource(_name)->GetTupleType());
}

PXR_NAMESPACE_CLOSE_SCOPE

