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
#ifndef PXR_IMAGING_HD_ST_GL_UTILS_H
#define PXR_IMAGING_HD_ST_GL_UTILS_H

#include "pxr/pxr.h"
#include "pxr/imaging/hdSt/api.h"
#include "pxr/imaging/hd/types.h"
#include "pxr/imaging/hgi/buffer.h"

#include "pxr/base/vt/value.h"

PXR_NAMESPACE_OPEN_SCOPE

class HdStGLUtils {
public:

    /// Reads the content of VBO back to VtArray.
    /// The \p vboOffset is expressed in bytes.
    HDST_API
    static VtValue ReadBuffer(uint64_t vbo,
                              HdTupleType tupleType,
                              int vboOffset,
                              int stride,
                              int numElements);
};

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
