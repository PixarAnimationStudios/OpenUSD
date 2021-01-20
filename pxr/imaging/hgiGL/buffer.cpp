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
#include "pxr/imaging/garch/glApi.h"

#include "pxr/imaging/hgiGL/diagnostic.h"
#include "pxr/imaging/hgiGL/buffer.h"


PXR_NAMESPACE_OPEN_SCOPE

HgiGLBuffer::HgiGLBuffer(HgiBufferDesc const & desc)
    : HgiBuffer(desc)
    , _bufferId(0)
    , _mapped(nullptr)
    , _cpuStaging(nullptr)
{

    if (desc.byteSize == 0) {
        TF_CODING_ERROR("Buffers must have a non-zero length");
    }

    glCreateBuffers(1, &_bufferId);

    if (!_descriptor.debugName.empty()) {
        HgiGLObjectLabel(GL_BUFFER, _bufferId,  _descriptor.debugName);
    }

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
            flags | GL_DYNAMIC_STORAGE_BIT);

        _mapped = glMapNamedBufferRange(_bufferId, 0, desc.byteSize, flags);
    } else {
        TF_CODING_ERROR("Unknown HgiBufferUsage bit");
    }

    // glBindVertexBuffer (graphics cmds) needs to know the stride of each
    // vertex buffer. Make sure user provides it.
    if (_descriptor.usage & HgiBufferUsageVertex) {
        TF_VERIFY(desc.vertexStride > 0);
    }

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

    if (_cpuStaging) {
        free(_cpuStaging);
        _cpuStaging = nullptr;
    }

    HGIGL_POST_PENDING_GL_ERRORS();
}

size_t
HgiGLBuffer::GetByteSizeOfResource() const
{
    return _descriptor.byteSize;
}

uint64_t
HgiGLBuffer::GetRawResource() const
{
    return (uint64_t) _bufferId;
}

void*
HgiGLBuffer::GetCPUStagingAddress()
{
    if (!_cpuStaging) {
        _cpuStaging = malloc(_descriptor.byteSize);
    }

    // This lets the client code memcpy into the cpu staging buffer directly.
    // The staging data must be explicitely copied to the GPU buffer
    // via CopyBufferCpuToGpu cmd by the client.
    return _cpuStaging;
}

PXR_NAMESPACE_CLOSE_SCOPE
