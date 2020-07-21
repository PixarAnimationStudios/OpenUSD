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

#include "pxr/imaging/hgi/hgi.h"
#include "pxr/imaging/hgi/buffer.h"

PXR_NAMESPACE_OPEN_SCOPE


HdStPersistentBuffer::HdStPersistentBuffer(
    Hgi* hgi, TfToken const &role, size_t dataSize, void* data)
    : HdResource(role)
    , _hgi(hgi)
{
    HD_TRACE_FUNCTION();
    HF_MALLOC_TAG_FUNCTION();

    HgiBufferDesc bufDesc;
    bufDesc.byteSize = dataSize;
    bufDesc.usage = HgiBufferUsageUniform;
    bufDesc.initialData = data;
    _buffer = _hgi->CreateBuffer(bufDesc);
    
    SetSize(dataSize);
}

HdStPersistentBuffer::~HdStPersistentBuffer()
{
    _hgi->DestroyBuffer(&_buffer);
}

PXR_NAMESPACE_CLOSE_SCOPE

