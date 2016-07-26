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
#ifndef HD_GL_UTILS_H
#define HD_GL_UTILS_H

#include "pxr/imaging/hd/version.h"
#include "pxr/base/vt/value.h"

// XXX: could be moved into GlopUtils

class HdGLUtils {
public:
    /// Reads the content of VBO back to VtArray.
    /// The \p vboOffset is expressed in bytes.
    static VtValue ReadBuffer(GLint vbo,
                              int glDataType,
                              int numComponents,
                              int arraySize,
                              int vboOffset,
                              int stride,
                              int numElements);

    /// Returns true if the shader has been successfully compiled.
    /// if not, returns false and fills the error log into reason.
    static bool GetShaderCompileStatus(GLuint shader,
                                       std::string * reason = NULL);

    /// Returns true if the program has been successfully linked.
    /// if not, returns false and fills the error log into reason.
    static bool GetProgramLinkStatus(GLuint program,
                                     std::string * reason = NULL);

};

/// a utility class to perform batched buffer copy
///
class HdGLBufferRelocator {
public:
    HdGLBufferRelocator(GLint srcBuffer, GLint dstBuffer) :
        _srcBuffer(srcBuffer), _dstBuffer(dstBuffer) {}

    /// Schedule the range to be copied. The consecutive ranges could be
    /// aggregated into a single copy where possible.
    void AddRange(GLintptr readOffset,
                  GLintptr writeOffset,
                  GLsizeiptr copySize);

    /// Execute GL buffer copy command to flush all scheduled range copies.
    void Commit();

private:
    struct _CopyUnit {
        _CopyUnit(GLintptr read, GLintptr write, GLsizeiptr size)
            : readOffset(read), writeOffset(write), copySize(size) {}

        bool Concat(_CopyUnit const &next) {
            if (readOffset  + copySize == next.readOffset and
                writeOffset + copySize == next.writeOffset) {
                copySize += next.copySize;
                return true;
            }
            return false;
        }

        GLintptr readOffset;
        GLintptr writeOffset;
        GLsizeiptr copySize;
    };

    std::vector<_CopyUnit> _queue;
    GLint _srcBuffer;
    GLint _dstBuffer;
};

#endif // HD_GL_UTILS_H
