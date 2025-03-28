//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/imaging/hdSt/bufferUtils.h"
#include "pxr/imaging/hdSt/resourceRegistry.h"
#include "pxr/imaging/hdSt/tokens.h"

#include "pxr/imaging/hgi/hgi.h"
#include "pxr/imaging/hgi/blitCmds.h"
#include "pxr/imaging/hgi/blitCmdsOps.h"

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
_CreateVtArray(int numElements, int arraySize, int stride, int elementStride,
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
            src += elementStride != 0 ? elementStride : stride;
        }
    }
    return VtValue(array);
}

VtValue
_CreateVtValue(HdType type, int numElements, int arraySize, int stride,
               int elementStride, std::vector<unsigned char> const &data) {
    // Convert data to Vt
    switch (type) {
    case HdTypeInt8:
        return _CreateVtArray<char>(numElements, arraySize, stride, 
            elementStride, data);
    case HdTypeInt16:
        return _CreateVtArray<int16_t>(numElements, arraySize, stride, 
            elementStride, data);
    case HdTypeUInt16:
        return _CreateVtArray<uint16_t>(numElements, arraySize, stride, 
            elementStride, data);
    case HdTypeUInt32:
        return _CreateVtArray<uint32_t>(numElements, arraySize, stride, 
            elementStride, data);
    case HdTypeInt32:
        return _CreateVtArray<int32_t>(numElements, arraySize, stride, 
            elementStride, data);
    case HdTypeInt32Vec2:
        return _CreateVtArray<GfVec2i>(numElements, arraySize, stride, 
            elementStride, data);
    case HdTypeInt32Vec3:
        return _CreateVtArray<GfVec3i>(numElements, arraySize, stride, 
            elementStride, data);
    case HdTypeInt32Vec4:
        return _CreateVtArray<GfVec4i>(numElements, arraySize, stride, 
            elementStride, data);
    case HdTypeFloat:
        return _CreateVtArray<float>(numElements, arraySize, stride, 
            elementStride, data);
    case HdTypeFloatVec2:
        return _CreateVtArray<GfVec2f>(numElements, arraySize, stride, 
            elementStride, data);
    case HdTypeFloatVec3:
        return _CreateVtArray<GfVec3f>(numElements, arraySize, stride, 
            elementStride, data);
    case HdTypeFloatVec4:
        return _CreateVtArray<GfVec4f>(numElements, arraySize, stride, 
            elementStride, data);
    case HdTypeFloatMat4:
        return _CreateVtArray<GfMatrix4f>(numElements, arraySize, stride, 
            elementStride, data);
    case HdTypeDouble:
        return _CreateVtArray<double>(numElements, arraySize, stride, 
            elementStride, data);
    case HdTypeDoubleVec2:
        return _CreateVtArray<GfVec2d>(numElements, arraySize, stride, 
            elementStride, data);
    case HdTypeDoubleVec3:
        return _CreateVtArray<GfVec3d>(numElements, arraySize, stride, 
            elementStride, data);
    case HdTypeDoubleVec4:
        return _CreateVtArray<GfVec4d>(numElements, arraySize, stride, 
            elementStride, data);
    case HdTypeDoubleMat4:
        return _CreateVtArray<GfMatrix4d>(numElements, arraySize, stride, 
            elementStride, data);
    default:
        TF_CODING_ERROR("Unhandled data type %i", type);
    }
    return VtValue();
}

VtValue
HdStReadBuffer(HgiBufferHandle const& buffer,
               HdTupleType tupleType,
               int offset,
               int stride,
               int numElements,
               int elementStride,
               HdStResourceRegistry *resourceRegistry)
{
    const int bytesPerElement = HdDataSizeOfTupleType(tupleType);
    const int arraySize = tupleType.count;

    // Stride is the byte distance between subsequent elements.
    // If stride was not provided (aka 0), we assume elements are
    // tightly packed and have no interleaved data.
    if (stride == 0) {
        stride = bytesPerElement;
    }
    TF_VERIFY(stride >= bytesPerElement);

    // Total buffer size is the sum of the strides required to cover
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
    const size_t dataSize = stride * (numElements - 1) + bytesPerElement;
    std::vector<unsigned char> tmp(dataSize);

    if (!buffer) {
        TF_WARN("Cannot read from invalid buffer handle");
        return _CreateVtValue(
            tupleType.type, numElements, arraySize, stride, elementStride, tmp);
    }

    // Submit and wait for all the work recorded up to this point.
    // The GPU work must complete before we can read-back the GPU buffer.
    resourceRegistry->SubmitBlitWork(HgiSubmitWaitTypeWaitUntilCompleted);
    resourceRegistry->SubmitComputeWork(HgiSubmitWaitTypeWaitUntilCompleted);

    // Submit GPU buffer read back
    HgiBufferGpuToCpuOp copyOp;
    copyOp.byteSize = dataSize;
    copyOp.cpuDestinationBuffer = tmp.data();
    copyOp.destinationByteOffset = 0;
    copyOp.gpuSourceBuffer = buffer;
    copyOp.sourceByteOffset = offset;

    HgiBlitCmds* blitCmds = resourceRegistry->GetGlobalBlitCmds();
    blitCmds->CopyBufferGpuToCpu(copyOp);
    resourceRegistry->SubmitBlitWork(HgiSubmitWaitTypeWaitUntilCompleted);
    
    return _CreateVtValue(tupleType.type,
        numElements, arraySize, stride, elementStride, tmp);
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

