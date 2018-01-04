//
// Copyright 2017 Pixar
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
#include "pxr/imaging/hdSt/bufferResourceGL.h"
#include "pxr/imaging/hdSt/renderContextCaps.h"

#include "pxr/base/tf/diagnostic.h"
#include "pxr/base/tf/staticTokens.h"

PXR_NAMESPACE_OPEN_SCOPE


HdStBufferResourceGL::HdStBufferResourceGL(TfToken const &role,
                                   int glDataType,
                                   short numComponents,
                                   int arraySize,
                                   int offset,
                                   int stride)
    : HdBufferResource(role, glDataType, numComponents, arraySize, offset, stride),
      _gpuAddr(0),
      _texId(0),
      _id(0)
{
    /*NOTHING*/
}

HdStBufferResourceGL::~HdStBufferResourceGL()
{
    TF_VERIFY(_texId == 0);
}

void
HdStBufferResourceGL::SetAllocation(GLuint id, size_t size)
{
    _id = id;
    HdResource::SetSize(size);

    HdStRenderContextCaps const & caps = HdStRenderContextCaps::GetInstance();

    // note: gpu address remains valid until the buffer object is deleted,
    // or when the data store is respecified via BufferData/BufferStorage.
    // It doesn't change even when we make the buffer resident or non-resident.
    // https://www.opengl.org/registry/specs/NV/shader_buffer_load.txt
    if (id != 0 && caps.bindlessBufferEnabled) {
        glGetNamedBufferParameterui64vNV(
            id, GL_BUFFER_GPU_ADDRESS_NV, (GLuint64EXT*)&_gpuAddr);
    } else {
        _gpuAddr = 0;
    }

    // release texid if exist. SetAllocation is guaranteed to be called
    // at the destruction of the hosting buffer array.
    if (_texId) {
        glDeleteTextures(1, &_texId);
        _texId = 0;
    }
}

GLuint
HdStBufferResourceGL::GetTextureBuffer()
{
    // XXX: need change tracking.

    if (_texId == 0) {
        glGenTextures(1, &_texId);

        GLenum format = GL_R32F;
        if (_glDataType == GL_FLOAT) {
            if (_numComponents <= 4) {
                static const GLenum floats[]
                    = { GL_R32F, GL_RG32F, GL_RGB32F, GL_RGBA32F };
                format = floats[_numComponents-1];
            }
        } else if (_glDataType == GL_INT) {
            if (_numComponents <= 4) {
                static const GLenum ints[]
                    = { GL_R32I, GL_RG32I, GL_RGB32I, GL_RGBA32I };
                format = ints[_numComponents-1];
            }
        } else {
            TF_CODING_ERROR("unsupported type: 0x%x numComponents = %d\n",
                            _glDataType, _numComponents);
        }

        glBindTexture(GL_TEXTURE_BUFFER, _texId);
        glTexBuffer(GL_TEXTURE_BUFFER, format, GetId());
        glBindTexture(GL_TEXTURE_BUFFER, 0);
    }
    return _texId;
}


PXR_NAMESPACE_CLOSE_SCOPE
