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
#include "pxr/imaging/garch/glApi.h"

#include "pxr/imaging/hdSt/glUtils.h"
#include "pxr/imaging/hdSt/tokens.h"

#include "pxr/imaging/hgi/hgi.h"
#include "pxr/imaging/hgi/blitCmds.h"
#include "pxr/imaging/hgi/blitCmdsOps.h"

#include "pxr/imaging/glf/contextCaps.h"

#include "pxr/base/gf/vec2d.h"
#include "pxr/base/gf/vec2f.h"
#include "pxr/base/gf/vec2i.h"
#include "pxr/base/gf/vec3d.h"
#include "pxr/base/gf/vec3f.h"
#include "pxr/base/gf/vec3i.h"
#include "pxr/base/gf/vec4d.h"
#include "pxr/base/gf/vec4f.h"
#include "pxr/base/gf/vec4i.h"
#include "pxr/base/gf/matrix4f.h"
#include "pxr/base/gf/matrix4d.h"
#include "pxr/imaging/hd/perfLog.h"
#include "pxr/imaging/hd/tokens.h"
#include "pxr/base/vt/array.h"
#include "pxr/base/tf/envSetting.h"
#include "pxr/base/tf/iterator.h"

PXR_NAMESPACE_OPEN_SCOPE


template <typename T>
VtValue
_CreateVtArray(int numElements, int arraySize, int stride,
               std::vector<unsigned char> const &data)
{
    VtArray<T> array(numElements*arraySize);
    if (numElements == 0)
        return VtValue(array);

    const unsigned char *src = &data[0];
    unsigned char *dst = (unsigned char *)array.data();

    TF_VERIFY(data.size() == stride*(numElements-1) + arraySize*sizeof(T));

    if (stride == static_cast<int>(arraySize*sizeof(T))) {
        memcpy(dst, src, numElements*arraySize*sizeof(T));
    } else {
        // deinterleaving
        for (int i = 0; i < numElements; ++i) {
            memcpy(dst, src, arraySize*sizeof(T));
            dst += arraySize*sizeof(T);
            src += stride;
        }
    }
    return VtValue(array);
}

VtValue
HdStGLUtils::ReadBuffer(uint64_t vbo,
                        HdTupleType tupleType,
                        int vboOffset,
                        int stride,
                        int numElems)
{
    if (glBufferSubData == NULL) return VtValue();

    // HdTupleType represents scalar, vector, matrix, and array types.
    const int bytesPerElement = HdDataSizeOfTupleType(tupleType);
    const int arraySize = tupleType.count;
    
    // Stride is the byte distance between subsequent elements.
    // If stride was not provided (aka 0), we assume elements are
    // tightly packed and have no interleaved data.
    if (stride == 0) stride = bytesPerElement;
    TF_VERIFY(stride >= bytesPerElement);

    // Total VBO size is the sum of the strides required to cover
    // every element up to the last, which only requires bytesPerElement.
    //
    // +---------+---------+---------+
    // |   :SRC: |   :SRC: |   :SRC: |
    // +---------+---------+---------+
    //     <-------read range------>
    //     |       ^           | ^ |
    //     | stride * (n -1)   |   |
    //                       bytesPerElement
    //
    const GLsizeiptr vboSize = stride * (numElems-1) + bytesPerElement;

    GlfContextCaps const &caps = GlfContextCaps::GetInstance();

    // Read data from GL
    std::vector<unsigned char> tmp(vboSize);

    if (vbo > 0) {
        if (caps.directStateAccessEnabled) {
            glGetNamedBufferSubData(vbo, vboOffset, vboSize, &tmp[0]);
        } else {
            glBindBuffer(GL_ARRAY_BUFFER, vbo);
            glGetBufferSubData(GL_ARRAY_BUFFER, vboOffset, vboSize, &tmp[0]);
            glBindBuffer(GL_ARRAY_BUFFER, 0);
        }
    }

    // Convert data to Vt
    switch (tupleType.type) {
    case HdTypeInt8:
        return _CreateVtArray<char>(numElems, arraySize, stride, tmp);
    case HdTypeInt16:
        return _CreateVtArray<int16_t>(numElems, arraySize, stride, tmp);
    case HdTypeUInt16:
        return _CreateVtArray<uint16_t>(numElems, arraySize, stride, tmp);
    case HdTypeUInt32:
        return _CreateVtArray<uint32_t>(numElems, arraySize, stride, tmp);
    case HdTypeInt32:
        return _CreateVtArray<int32_t>(numElems, arraySize, stride, tmp);
    case HdTypeInt32Vec2:
        return _CreateVtArray<GfVec2i>(numElems, arraySize, stride, tmp);
    case HdTypeInt32Vec3:
        return _CreateVtArray<GfVec3i>(numElems, arraySize, stride, tmp);
    case HdTypeInt32Vec4:
        return _CreateVtArray<GfVec4i>(numElems, arraySize, stride, tmp);
    case HdTypeFloat:
        return _CreateVtArray<float>(numElems, arraySize, stride, tmp);
    case HdTypeFloatVec2:
        return _CreateVtArray<GfVec2f>(numElems, arraySize, stride, tmp);
    case HdTypeFloatVec3:
        return _CreateVtArray<GfVec3f>(numElems, arraySize, stride, tmp);
    case HdTypeFloatVec4:
        return _CreateVtArray<GfVec4f>(numElems, arraySize, stride, tmp);
    case HdTypeFloatMat4:
        return _CreateVtArray<GfMatrix4f>(numElems, arraySize, stride, tmp);
    case HdTypeDouble:
        return _CreateVtArray<double>(numElems, arraySize, stride, tmp);
    case HdTypeDoubleVec2:
        return _CreateVtArray<GfVec2d>(numElems, arraySize, stride, tmp);
    case HdTypeDoubleVec3:
        return _CreateVtArray<GfVec3d>(numElems, arraySize, stride, tmp);
    case HdTypeDoubleVec4:
        return _CreateVtArray<GfVec4d>(numElems, arraySize, stride, tmp);
    case HdTypeDoubleMat4:
        return _CreateVtArray<GfMatrix4d>(numElems, arraySize, stride, tmp);
    default:
        TF_CODING_ERROR("Unhandled data type %i", tupleType.type);
    }
    return VtValue();
}

// ---------------------------------------------------------------------------

void
HdStBufferRelocator::AddRange(ptrdiff_t readOffset,
                              ptrdiff_t writeOffset,
                              ptrdiff_t copySize)
{
    _CopyUnit unit(readOffset, writeOffset, copySize);
    if (_queue.empty() || (!_queue.back().Concat(unit))) {
        _queue.push_back(unit);
    }
}

void
HdStBufferRelocator::Commit(HgiBlitCmds* blitCmds)
{
    HgiBufferGpuToGpuOp blitOp;
    blitOp.gpuSourceBuffer = _srcBuffer;
    blitOp.gpuDestinationBuffer = _dstBuffer;
    
    TF_FOR_ALL (it, _queue) {
        blitOp.sourceByteOffset = it->readOffset;
        blitOp.byteSize = it->copySize;
        blitOp.destinationByteOffset = it->writeOffset;

        blitCmds->CopyBufferGpuToGpu(blitOp);
    }

    HD_PERF_COUNTER_ADD(HdStPerfTokens->copyBufferGpuToGpu,
                        (double)_queue.size());

    _queue.clear();
}

PXR_NAMESPACE_CLOSE_SCOPE

