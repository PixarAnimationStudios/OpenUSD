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
#include "pxr/imaging/hdSt/glConversions.h"
#include "pxr/imaging/hd/perfLog.h"
#include "pxr/imaging/hd/tokens.h"
#include "pxr/imaging/hdSt/renderContextCaps.h"
#include "pxr/imaging/hdSt/glUtils.h"
#include "pxr/base/vt/array.h"
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

    if (stride == arraySize*sizeof(T)) {
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
HdStGLUtils::ReadBuffer(GLint vbo,
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

    HdStRenderContextCaps const &caps = HdStRenderContextCaps::GetInstance();

    // Read data from GL
    std::vector<unsigned char> tmp(vboSize);
    if (caps.directStateAccessEnabled) {
        glGetNamedBufferSubDataEXT(vbo, vboOffset, vboSize, &tmp[0]);
    } else {
        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        glGetBufferSubData(GL_ARRAY_BUFFER, vboOffset, vboSize, &tmp[0]);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
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

bool
HdStGLUtils::GetShaderCompileStatus(GLuint shader, std::string * reason)
{
    // glew has to be initialized
    if (!glGetShaderiv) return true;

    GLint status = 0;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &status);
    if (reason) {
        GLint infoLength = 0;
        glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &infoLength);
        if (infoLength > 0) {
            char *infoLog = new char[infoLength];;
            glGetShaderInfoLog(shader, infoLength, NULL, infoLog);
            reason->assign(infoLog, infoLength);
            delete[] infoLog;
        }
    }
    return (status == GL_TRUE);
}

bool
HdStGLUtils::GetProgramLinkStatus(GLuint program, std::string * reason)
{
    // glew has to be initialized
    if (!glGetProgramiv) return true;

    GLint status = 0;
    glGetProgramiv(program, GL_LINK_STATUS, &status);
    if (reason) {
        GLint infoLength = 0;
        glGetProgramiv(program, GL_INFO_LOG_LENGTH, &infoLength);
        if (infoLength > 0) {
            char *infoLog = new char[infoLength];;
            glGetProgramInfoLog(program, infoLength, NULL, infoLog);
            reason->assign(infoLog, infoLength);
            delete[] infoLog;
        }
    }
    return (status == GL_TRUE);
}

// ---------------------------------------------------------------------------

void
HdStGLBufferRelocator::AddRange(GLintptr readOffset,
                              GLintptr writeOffset,
                              GLsizeiptr copySize)
{
    _CopyUnit unit(readOffset, writeOffset, copySize);
    if (_queue.empty() || (!_queue.back().Concat(unit))) {
        _queue.push_back(unit);
    }
}

void
HdStGLBufferRelocator::Commit()
{
    HdStRenderContextCaps const &caps = HdStRenderContextCaps::GetInstance();

    if (caps.copyBufferEnabled) {
        // glCopyBuffer
        if (!caps.directStateAccessEnabled) {
            glBindBuffer(GL_COPY_READ_BUFFER, _srcBuffer);
            glBindBuffer(GL_COPY_WRITE_BUFFER, _dstBuffer);
        }

        TF_FOR_ALL (it, _queue) {
            if (ARCH_LIKELY(caps.directStateAccessEnabled)) {
                glNamedCopyBufferSubDataEXT(_srcBuffer,
                                            _dstBuffer,
                                            it->readOffset,
                                            it->writeOffset,
                                            it->copySize);
            } else {
                glCopyBufferSubData(GL_COPY_READ_BUFFER,
                                    GL_COPY_WRITE_BUFFER,
                                    it->readOffset,
                                    it->writeOffset,
                                    it->copySize);
            }
        }
        HD_PERF_COUNTER_ADD(HdPerfTokens->glCopyBufferSubData,
                            (double)_queue.size());

        if (!caps.directStateAccessEnabled) {
            glBindBuffer(GL_COPY_READ_BUFFER, 0);
            glBindBuffer(GL_COPY_WRITE_BUFFER, 0);
        }
    } else {
        // read back to CPU and send it to GPU again
        // (workaround for a driver crash)
        TF_FOR_ALL (it, _queue) {
            std::vector<char> data(it->copySize);
            glBindBuffer(GL_ARRAY_BUFFER, _srcBuffer);
            glGetBufferSubData(GL_ARRAY_BUFFER, it->readOffset, it->copySize,
                               &data[0]);
            glBindBuffer(GL_ARRAY_BUFFER, _dstBuffer);
            glBufferSubData(GL_ARRAY_BUFFER, it->writeOffset, it->copySize,
                            &data[0]);
        }
        glBindBuffer(GL_ARRAY_BUFFER, 0);
    }

    _queue.clear();
}

PXR_NAMESPACE_CLOSE_SCOPE

