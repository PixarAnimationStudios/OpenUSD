//
// Copyright 2020 benmalartre
//
// Unlicensed
//
#include "pxr/imaging/plugin/LoFi/textureHandle.h"

#include "pxr/imaging/plugin/LoFi/textureHandleRegistry.h"

#include "pxr/imaging/plugin/LoFi/samplerObject.h"
#include "pxr/imaging/plugin/LoFi/samplerObjectRegistry.h"

#include "pxr/base/tf/diagnostic.h"

PXR_NAMESPACE_OPEN_SCOPE

LoFiTextureHandle::LoFiTextureHandle(
    LoFiTextureObjectSharedPtr const &textureObject,
    const HdSamplerParameters &samplerParams,
    const size_t memoryRequest,
    const bool createBindlessHandle,
    LoFiShaderCodePtr const & shaderCode,
    LoFiTextureHandleRegistry *textureHandleRegistry)
  : _textureObject(textureObject)
  , _samplerParams(samplerParams)
  , _memoryRequest(memoryRequest)
  , _createBindlessHandle(createBindlessHandle)
  , _shaderCode(shaderCode)
  , _textureHandleRegistry(textureHandleRegistry)
{
}  

LoFiTextureHandle::~LoFiTextureHandle()
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
LoFiTextureHandle::ReallocateSamplerIfNecessary()
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
    LoFiSamplerObjectRegistry * const samplerObjectRegistry =
        _textureHandleRegistry->GetSamplerObjectRegistry();

    _samplerObject =
        samplerObjectRegistry->AllocateSampler(
            _textureObject, _samplerParams, _createBindlessHandle);
}

PXR_NAMESPACE_CLOSE_SCOPE
