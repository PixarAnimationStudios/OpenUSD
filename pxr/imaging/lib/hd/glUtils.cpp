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
#include "pxr/imaging/hd/conversions.h"
#include "pxr/imaging/hd/glUtils.h"
#include "pxr/imaging/hd/perfLog.h"
#include "pxr/imaging/hd/renderContextCaps.h"
#include "pxr/imaging/hd/tokens.h"
#include "pxr/base/vt/array.h"

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

    if (stride == sizeof(T)) {
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
HdGLUtils::ReadBuffer(GLint vbo,
                      int glDataType,
                      int numComponents,
                      int arraySize,
                      int vboOffset,
                      int stride,
                      int numElements)
{
    if (glBufferSubData == NULL) return VtValue();

    int bytesPerElement = (int)(numComponents *
        HdConversions::GetComponentSize(glDataType));
    if (stride == 0) stride = bytesPerElement;
    TF_VERIFY(stride >= bytesPerElement);

    // +---------+---------+---------+
    // |   :SRC: |   :SRC: |   :SRC: |
    // +---------+---------+---------+
    //     <-------read range------>
    //     |       ^           | ^ |
    //     | stride * (n -1)   |   |
    //                       bytesPerElement

    GLsizeiptr vboSize = stride * (numElements-1) + bytesPerElement * arraySize;

    HdRenderContextCaps const &caps = HdRenderContextCaps::GetInstance();

    // read data
    std::vector<unsigned char> tmp(vboSize);
    if (caps.directStateAccessEnabled) {
        glGetNamedBufferSubDataEXT(vbo, vboOffset, vboSize, &tmp[0]);
    } else {
        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        glGetBufferSubData(GL_ARRAY_BUFFER, vboOffset, vboSize, &tmp[0]);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
    }

    VtValue result;
    // create VtArray
    switch(glDataType){
    case GL_BYTE:
        switch(numComponents) {
        case 1: result = _CreateVtArray<char>(numElements, arraySize, stride, tmp); break;
        default: break;
        }
        break;
    case GL_SHORT:
        switch(numComponents) {
        case 1: result = _CreateVtArray<short>(numElements, arraySize, stride, tmp); break;
        default: break;
        }
        break;
    case GL_UNSIGNED_SHORT:
        switch(numComponents) {
        case 1: result = _CreateVtArray<unsigned short>(numElements, arraySize, stride, tmp); break;
        default: break;
        }
        break;
    case GL_INT:
        switch(numComponents) {
        case 1: result = _CreateVtArray<int>(numElements, arraySize, stride, tmp); break;
        case 2: result = _CreateVtArray<GfVec2i>(numElements, arraySize, stride, tmp); break;
        case 3: result = _CreateVtArray<GfVec3i>(numElements, arraySize, stride, tmp); break;
        case 4: result = _CreateVtArray<GfVec4i>(numElements, arraySize, stride, tmp); break;
        default: break;
        }
        break;
    case GL_FLOAT:
        switch(numComponents) {
        case 1: result = _CreateVtArray<float>(numElements, arraySize, stride, tmp); break;
        case 2: result = _CreateVtArray<GfVec2f>(numElements, arraySize, stride, tmp); break;
        case 3: result = _CreateVtArray<GfVec3f>(numElements, arraySize, stride, tmp); break;
        case 4: result = _CreateVtArray<GfVec4f>(numElements, arraySize, stride, tmp); break;
        case 16: result = _CreateVtArray<GfMatrix4f>(numElements, arraySize, stride, tmp); break;
        default: break;
        }
        break;
    case GL_DOUBLE:
        switch(numComponents) {
        case 1: result = _CreateVtArray<double>(numElements, arraySize, stride, tmp); break;
        case 2: result = _CreateVtArray<GfVec2d>(numElements, arraySize, stride, tmp); break;
        case 3: result = _CreateVtArray<GfVec3d>(numElements, arraySize, stride, tmp); break;
        case 4: result = _CreateVtArray<GfVec4d>(numElements, arraySize, stride, tmp); break;
        case 16: result = _CreateVtArray<GfMatrix4d>(numElements, arraySize, stride, tmp); break;
        default: break;
        }
        break;
    default:
        TF_CODING_ERROR("Invalid data type");
        break;
    }

    return result;
}

bool
HdGLUtils::GetShaderCompileStatus(GLuint shader, std::string * reason)
{
    // glew has to be initialized
    if (not glGetShaderiv) return true;

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
HdGLUtils::GetProgramLinkStatus(GLuint program, std::string * reason)
{
    // glew has to be initialized
    if (not glGetProgramiv) return true;

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
HdGLBufferRelocator::AddRange(GLintptr readOffset,
                              GLintptr writeOffset,
                              GLsizeiptr copySize)
{
    _CopyUnit unit(readOffset, writeOffset, copySize);
    if (_queue.empty() or (not _queue.back().Concat(unit))) {
        _queue.push_back(unit);
    }
}

void
HdGLBufferRelocator::Commit()
{
    HdRenderContextCaps const &caps = HdRenderContextCaps::GetInstance();

    if (caps.copyBufferEnabled) {
        // glCopyBuffer
        if (not caps.directStateAccessEnabled) {
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

        if (not caps.directStateAccessEnabled) {
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
