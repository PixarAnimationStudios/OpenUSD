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
#include "pxr/imaging/garch/glApi.h"

#include "pxr/imaging/glf/contextCaps.h"

#include "pxr/imaging/hdSt/bufferResource.h"

#include "pxr/base/tf/diagnostic.h"
#include "pxr/base/tf/staticTokens.h"

PXR_NAMESPACE_OPEN_SCOPE


HdStBufferResource::HdStBufferResource(TfToken const &role,
                                           HdTupleType tupleType,
                                           int offset,
                                           int stride)
    : HdBufferResource(role, tupleType, offset, stride),
      _gpuAddr(0)
{
    /*NOTHING*/
}

HdStBufferResource::~HdStBufferResource()
{
    /*NOTHING*/
}

void
HdStBufferResource::SetAllocation(HgiBufferHandle const& id, size_t size)
{
    _id = id;
    HdResource::SetSize(size);

    GlfContextCaps const & caps = GlfContextCaps::GetInstance();

    // note: gpu address remains valid until the buffer object is deleted,
    // or when the data store is respecified via BufferData/BufferStorage.
    // It doesn't change even when we make the buffer resident or non-resident.
    // https://www.opengl.org/registry/specs/NV/shader_buffer_load.txt
    if (id && caps.bindlessBufferEnabled) {
        glGetNamedBufferParameterui64vNV(
            id->GetRawResource(), GL_BUFFER_GPU_ADDRESS_NV, (GLuint64EXT*)&_gpuAddr);
    } else {
        _gpuAddr = 0;
    }
}

PXR_NAMESPACE_CLOSE_SCOPE
