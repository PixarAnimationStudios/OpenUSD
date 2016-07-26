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
#include "pxr/imaging/hd/bufferResource.h"

#include "pxr/base/tf/diagnostic.h"
#include "pxr/base/tf/staticTokens.h"

TF_DEFINE_PRIVATE_TOKENS(
    _tokens,
    ((_float, "float"))
    (vec2)
    (vec3)
    (vec4)
    (mat4)
    ((_double, "double"))
    (dvec2)
    (dvec3)
    (dvec4)
    (dmat4)
    ((_int, "int"))
    (ivec2)
    (ivec3)
    (ivec4)
    (uvec2)
    ((sampler2D, "sampler2D"))
    ((sampler2DArray, "sampler2DArray"))
    ((isamplerBuffer, "isamplerBuffer"))
);

HdBufferResource::HdBufferResource(TfToken const &role,
                                   int glDataType,
                                   short numComponents,
                                   int arraySize,
                                   int offset,
                                   int stride)
    : HdResource(role),
      _glDataType(glDataType),
      _numComponents(numComponents),
      _arraySize(arraySize),
      _offset(offset),
      _stride(stride),
      _gpuAddr(0),
      _texId(0)
{
    /*NOTHING*/
}

HdBufferResource::~HdBufferResource()
{
    TF_VERIFY(_texId == 0);
}

void
HdBufferResource::SetAllocation(GLuint id, GLsizeiptr size)
{
    // call base class
    HdResource::SetAllocation(id, size);

    // note: gpu address remains valid until the buffer object is deleted,
    // or when the data store is respecified via BufferData/BufferStorage.
    // It doesn't change even when we make the buffer resident or non-resident.
    // https://www.opengl.org/registry/specs/NV/shader_buffer_load.txt
    if (id != 0 and glGetNamedBufferParameterui64vNV) {
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
HdBufferResource::GetTextureBuffer()
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

TfToken
HdBufferResource::GetGLTypeName() const
{
    if (_glDataType == GL_FLOAT) {
        if (_numComponents <= 4) {
            static TfToken fvecs[] = { _tokens->_float,
                                       _tokens->vec2,
                                       _tokens->vec3,
                                       _tokens->vec4 };
            return fvecs[_numComponents-1];
        } else if (_numComponents == 16) {
            return _tokens->mat4;
        }
    } else if (_glDataType == GL_DOUBLE) {
        if (_numComponents <= 4) {
            static TfToken dvecs[] = { _tokens->_double,
                                       _tokens->dvec2,
                                       _tokens->dvec3,
                                       _tokens->dvec4 };
            return dvecs[_numComponents-1];
        } else if (_numComponents == 16) {
            return _tokens->dmat4;
        }
    } else if (_glDataType == GL_INT) {
        if (_numComponents <= 4) {
            static TfToken ivecs[] = { _tokens->_int,
                                       _tokens->ivec2,
                                       _tokens->ivec3,
                                       _tokens->ivec4 };
            return ivecs[_numComponents-1];
        }
    } else if (_glDataType == GL_SAMPLER_2D) {
        return _tokens->uvec2;
    } else if (_glDataType == GL_SAMPLER_2D_ARRAY) {
        return _tokens->uvec2;
    } else if (_glDataType == GL_INT_SAMPLER_BUFFER) {
        return _tokens->uvec2;
    }

    TF_CODING_ERROR("unsupported type: 0x%x numComponents = %d\n",
                    _glDataType, _numComponents);

    // for graceful error handling in the downstream code, returns
    // float instead of empty
    return _tokens->_float;
}
