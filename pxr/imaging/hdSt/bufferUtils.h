//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_IMAGING_HD_ST_BUFFER_UTILS_H
#define PXR_IMAGING_HD_ST_BUFFER_UTILS_H

#include "pxr/pxr.h"
#include "pxr/imaging/hdSt/api.h"
#include "pxr/imaging/hd/types.h"
#include "pxr/imaging/hgi/buffer.h"

#include "pxr/base/vt/value.h"

PXR_NAMESPACE_OPEN_SCOPE

class HdStResourceRegistry;

/// Reads the content of buffer back to VtArray.
/// The \p offset is expressed in bytes.
HDST_API
VtValue HdStReadBuffer(HgiBufferHandle const& buffer,
                       HdTupleType tupleType,
                       int offset,
                       int stride,
                       int numElements,
                       int elementStride,
                       HdStResourceRegistry *resourceRegistry);

/// \class HdStBufferRelocator
///
/// A utility class to perform batched buffer copy.
///
class HdStBufferRelocator {
public:
    HdStBufferRelocator(
        HgiBufferHandle const& srcBuffer, HgiBufferHandle const& dstBuffer) :
        _srcBuffer(srcBuffer), _dstBuffer(dstBuffer) {}

    /// Schedule the range to be copied. The consecutive ranges could be
    /// aggregated into a single copy where possible.
    HDST_API
    void AddRange(ptrdiff_t readOffset,
                  ptrdiff_t writeOffset,
                  ptrdiff_t copySize);

    /// Execute Hgi buffer copy command to flush all scheduled range copies.
    HDST_API
    void Commit(class HgiBlitCmds* blitCmds);

private:
    struct _CopyUnit {
        _CopyUnit(ptrdiff_t read, ptrdiff_t write, ptrdiff_t size)
            : readOffset(read), writeOffset(write), copySize(size) {}

        bool Concat(_CopyUnit const &next) {
            if (readOffset  + copySize == next.readOffset &&
                writeOffset + copySize == next.writeOffset) {
                copySize += next.copySize;
                return true;
            }
            return false;
        }

        ptrdiff_t readOffset;
        ptrdiff_t writeOffset;
        ptrdiff_t copySize;
    };

    std::vector<_CopyUnit> _queue;
    HgiBufferHandle _srcBuffer;
    HgiBufferHandle _dstBuffer;
};


PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_IMAGING_HD_ST_GL_UTILS_H
