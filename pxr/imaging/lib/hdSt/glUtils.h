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
#ifndef HDST_GL_UTILS_H
#define HDST_GL_UTILS_H

#include "pxr/pxr.h"
#include "pxr/imaging/glf/glew.h"
#include "pxr/imaging/hdSt/api.h"
#include "pxr/imaging/hd/types.h"
#include "pxr/base/vt/value.h"

PXR_NAMESPACE_OPEN_SCOPE

class HdStGLUtils {
public:

    HDST_API
    static bool IsGpuComputeEnabled();

    /// Reads the content of VBO back to VtArray.
    /// The \p vboOffset is expressed in bytes.
    HDST_API
    static VtValue ReadBuffer(GLint vbo,
                              HdTupleType tupleType,
                              int vboOffset,
                              int stride,
                              int numElements);

    /// Returns true if the shader has been successfully compiled.
    /// if not, returns false and fills the error log into reason.
    HDST_API
    static bool GetShaderCompileStatus(GLuint shader,
                                       std::string * reason = NULL);

    /// Returns true if the program has been successfully linked.
    /// if not, returns false and fills the error log into reason.
    HDST_API
    static bool GetProgramLinkStatus(GLuint program,
                                     std::string * reason = NULL);
};

/// \class HdStGLBufferRelocator
///
/// A utility class to perform batched buffer copy.
///
class HdStGLBufferRelocator {
public:
    HdStGLBufferRelocator(GLint srcBuffer, GLint dstBuffer) :
        _srcBuffer(srcBuffer), _dstBuffer(dstBuffer) {}

    /// Schedule the range to be copied. The consecutive ranges could be
    /// aggregated into a single copy where possible.
    HDST_API
    void AddRange(ptrdiff_t readOffset,
                  ptrdiff_t writeOffset,
                  ptrdiff_t copySize);

    /// Execute GL buffer copy command to flush all scheduled range copies.
    HDST_API
    void Commit();

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
    GLint _srcBuffer;
    GLint _dstBuffer;
};


PXR_NAMESPACE_CLOSE_SCOPE

#endif // HDST_GL_UTILS_H
