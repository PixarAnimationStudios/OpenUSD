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
#include "pxr/imaging/hdSt/textureHandle.h"

#include "pxr/imaging/hdSt/textureHandleRegistry.h"

#include "pxr/imaging/hdSt/samplerObject.h"
#include "pxr/imaging/hdSt/samplerObjectRegistry.h"

#include "pxr/base/tf/diagnostic.h"

PXR_NAMESPACE_OPEN_SCOPE

HdStTextureHandle::HdStTextureHandle(
    HdStTextureObjectSharedPtr const &textureObject,
    const HdSamplerParameters &samplerParams,
    const size_t memoryRequest,
    const bool createBindlessHandle,
    HdStShaderCodePtr const & shaderCode,
    HdSt_TextureHandleRegistry *textureHandleRegistry)
  : _textureObject(textureObject)
  , _samplerParams(samplerParams)
  , _memoryRequest(memoryRequest)
  , _createBindlessHandle(createBindlessHandle)
  , _shaderCode(shaderCode)
  , _textureHandleRegistry(textureHandleRegistry)
{
}  

HdStTextureHandle::~HdStTextureHandle()
{
    if (TF_VERIFY(_textureHandleRegistry)) {
        // The target memory of the texture might change, so mark dirty.
        _textureHandleRegistry->MarkDirty(_textureObject);
        // The shader needs to be updated after it dropped a texture
        // handle (i.e., because it re-allocated the shader bar after
        // dropping a texture).
        _textureHandleRegistry->MarkDirty(_shaderCode);
        _textureHandleRegistry->MarkSamplerGarbageCollectionNeeded();
    }
}

void
HdStTextureHandle::ReallocateSamplerIfNecessary()
{
    if (_samplerObject) {
        if (!_createBindlessHandle) {
            // There is no setter for sampler parameters,
            // so we only need to create a sampler once...
            return;
        }

        // ... except that the sampler object has a texture sampler
        // handle that needs to be re-created if the underlying texture
        // changes, so continue.
     
        if (TF_VERIFY(_textureHandleRegistry)) {
            _textureHandleRegistry->MarkSamplerGarbageCollectionNeeded();
        }

        _samplerObject = nullptr;
    }

    // Create sampler object through registry.
    HdSt_SamplerObjectRegistry * const samplerObjectRegistry =
        _textureHandleRegistry->GetSamplerObjectRegistry();

    _samplerObject =
        samplerObjectRegistry->AllocateSampler(
            _textureObject, _samplerParams, _createBindlessHandle);
}

PXR_NAMESPACE_CLOSE_SCOPE
