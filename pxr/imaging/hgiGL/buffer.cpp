//
// Copyright 2020 Pixar
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
#include <GL/glew.h>
#include "pxr/imaging/hgiGL/diagnostic.h"
#include "pxr/imaging/hgiGL/buffer.h"


PXR_NAMESPACE_OPEN_SCOPE

HgiGLBuffer::HgiGLBuffer(HgiBufferDesc const & desc)
    : HgiBuffer(desc)
    , _descriptor(desc)
    , _bufferId(0)
    , _mapped(nullptr)
{

    if (desc.byteSize == 0) {
        TF_CODING_ERROR("Buffers must have a non-zero length");
    }

    glCreateBuffers(1, &_bufferId);

    if ((_descriptor.usage & HgiBufferUsageVertex)  ||
        (_descriptor.usage & HgiBufferUsageIndex32) ||
        (_descriptor.usage & HgiBufferUsageUniform)) {
        glNamedBufferData(
            _bufferId,
            _descriptor.byteSize,
            _descriptor.initialData,
            GL_STATIC_DRAW);
    } else if (_descriptor.usage & HgiBufferUsageStorage) {
        GLbitfield flags =
            GL_MAP_READ_BIT       |
            GL_MAP_WRITE_BIT      |
            GL_MAP_PERSISTENT_BIT |
            GL_MAP_COHERENT_BIT;

        glNamedBufferStorage(
            _bufferId,
            _descriptor.byteSize,
            _descriptor.initialData,
            flags);
        _mapped = glMapNamedBufferRange(_bufferId, 0, desc.byteSize, flags);
    } else {
        TF_CODING_ERROR("Unknown HgiBufferUsage bit");
    }

    // Don't hold onto buffer data ptr locally. HgiBufferDesc states that:
    // "The application may alter or free this memory as soon as the constructor
    //  of the HgiBuffer has returned."
    _descriptor.initialData = nullptr;

    HGIGL_POST_PENDING_GL_ERRORS();
}

HgiGLBuffer::~HgiGLBuffer()
{
    if (_bufferId > 0) {
        if (_descriptor.usage & HgiBufferUsageStorage) {
            glUnmapNamedBuffer(_bufferId);
        }
        
        glDeleteBuffers(1, &_bufferId);
        _bufferId = 0;
    }

    HGIGL_POST_PENDING_GL_ERRORS();
}

PXR_NAMESPACE_CLOSE_SCOPE
