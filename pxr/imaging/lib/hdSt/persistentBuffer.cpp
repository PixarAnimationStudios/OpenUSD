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
#include "pxr/imaging/glf/contextCaps.h"

#include "pxr/imaging/hdSt/persistentBuffer.h"
#include "pxr/imaging/hd/perfLog.h"

#include "pxr/imaging/hf/perfLog.h"

PXR_NAMESPACE_OPEN_SCOPE


HdStPersistentBuffer::HdStPersistentBuffer(
    TfToken const &role, size_t dataSize, void* data)
    : HdStResourceGL(role)
    , _mappedAddress(0)
{
    HD_TRACE_FUNCTION();
    HF_MALLOC_TAG_FUNCTION();

    GlfContextCaps const &caps = GlfContextCaps::GetInstance();

    GLuint newId = 0;
    glGenBuffers(1, &newId);

    if (caps.bufferStorageEnabled) {
        GLbitfield access = 
            GL_MAP_READ_BIT | GL_MAP_WRITE_BIT |
            GL_MAP_PERSISTENT_BIT | GL_MAP_COHERENT_BIT;

        if (caps.directStateAccessEnabled) {
            glNamedBufferStorageEXT(newId, dataSize, data, access);
            _mappedAddress = glMapNamedBufferRangeEXT(newId, 0, dataSize, access);
        } else {
            glBindBuffer(GL_ARRAY_BUFFER, newId);
            glBufferStorage(GL_ARRAY_BUFFER, dataSize, data, access);
            _mappedAddress = glMapBufferRange(GL_ARRAY_BUFFER, 0, dataSize, access);
            glBindBuffer(GL_ARRAY_BUFFER, 0);
        }
    } else {
        if (caps.directStateAccessEnabled) {
            glNamedBufferDataEXT(newId, dataSize, data, GL_DYNAMIC_DRAW);
        } else {
            glBindBuffer(GL_ARRAY_BUFFER, newId);
            glBufferData(GL_ARRAY_BUFFER, dataSize, data, GL_DYNAMIC_DRAW);
            glBindBuffer(GL_ARRAY_BUFFER, 0);
        }
    }

    SetAllocation(newId, dataSize);
}

HdStPersistentBuffer::~HdStPersistentBuffer()
{
    GLuint id = GetId();
    if (id) {
        glDeleteBuffers(1, &id);
    }
    SetAllocation(0, 0);
}

PXR_NAMESPACE_CLOSE_SCOPE

