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
#ifndef PXR_IMAGING_GLF_TEXTURE_CONTAINER_IMPL_H
#define PXR_IMAGING_GLF_TEXTURE_CONTAINER_IMPL_H

#include "pxr/imaging/glf/textureContainer.h"

#include "pxr/imaging/glf/textureHandle.h"
#include "pxr/base/trace/trace.h"

PXR_NAMESPACE_OPEN_SCOPE

template<typename Identifier>
GlfTextureHandlePtr
GlfTextureContainer<Identifier>::GetTextureHandle(
    Identifier const &identifier)
{
    GlfTextureHandleRefPtr &textureHandle = _textureHandles[identifier];
    if (!textureHandle) {
        // Create texture handle if it was not in map
        textureHandle = GlfTextureHandle::New(_CreateTexture(identifier));
    }
    return textureHandle;
}

template<typename Identifier>
void
GlfTextureContainer<Identifier>::GarbageCollect()
{
    TRACE_FUNCTION();

    // Garbage collection similar to texture registry.
    for (auto it = _textureHandles.begin(); it != _textureHandles.end(); ) {
        const GlfTextureHandleRefPtr &handle = it->second;
        // Delete texture if we hold on to the only reference.
        if (TF_VERIFY(handle) && handle->IsUnique()) {
            it = _textureHandles.erase(it);
        } else {
            ++it;
        }
    }
}

PXR_NAMESPACE_CLOSE_SCOPE

#endif
