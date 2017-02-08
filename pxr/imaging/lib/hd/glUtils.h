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

#include "pxr/pxr.h"
#include "pxr/imaging/hd/version.h"
#include "pxr/base/vt/value.h"

#include <algorithm>
#include <cmath>

PXR_NAMESPACE_OPEN_SCOPE


struct HdVec4f_2_10_10_10_REV {
    // we treat packed type as single-component values
    static const size_t dimension = 1;

    HdVec4f_2_10_10_10_REV() { }

    template <typename Vec3Type>
    HdVec4f_2_10_10_10_REV(Vec3Type const &value) {
        x = to10bits(value[0]);
        y = to10bits(value[1]);
        z = to10bits(value[2]);
        w = 0;
    }

    // ref. GL spec 2.3.5.2
    //   Conversion from floating point to normalized fixed point
    template <typename R>
    int to10bits(R v) {
        return int(
            std::round(
                std::min(std::max(v, static_cast<R>(-1)), static_cast<R>(1))
                *static_cast<R>(511)));
    }

    bool operator==(const HdVec4f_2_10_10_10_REV &other) const {
        return (other.w == w && 
                other.z == z && 
                other.y == y && 
                other.x == x);
    }

    int x : 10;
    int y : 10;
    int z : 10;
    int w : 2;
};

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

/// \class HdGLBufferRelocator
///
/// A utility class to perform batched buffer copy.
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
            if (readOffset  + copySize == next.readOffset &&
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


PXR_NAMESPACE_CLOSE_SCOPE

#endif // HD_GL_UTILS_H
