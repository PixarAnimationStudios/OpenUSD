//
// Copyright 2020 benmalartre
//
// Unlicensed
//
#include "pxr/imaging/plugin/LoFi/samplerObjectRegistry.h"

#include "pxr/imaging/plugin/LoFi/samplerObject.h"
#include "pxr/imaging/plugin/LoFi/ptexTextureObject.h"
#include "pxr/imaging/plugin/LoFi/textureObject.h"
#include "pxr/imaging/plugin/LoFi/udimTextureObject.h"

#include "pxr/base/trace/trace.h"

PXR_NAMESPACE_OPEN_SCOPE

LoFiSamplerObjectRegistry::LoFiSamplerObjectRegistry(
    LoFiResourceRegistry * registry)
  : _garbageCollectionNeeded(false)
  , _resourceRegistry(registry)
{
}

LoFiSamplerObjectRegistry::~LoFiSamplerObjectRegistry() = default;

template<HdTextureType textureType>
static
LoFiSamplerObjectSharedPtr
_MakeTypedSamplerObject(
    LoFiTextureObjectSharedPtr const &texture,
    HdSamplerParameters const &samplerParameters,
    const bool createBindlessHandle,
    LoFiSamplerObjectRegistry * const samplerObjectRegistry)
{
    // e.g. LoFiUvTextureObject
    using TextureObject = LoFiTypedTextureObject<textureType>;
    // e.g. LoFiUvSamplerObject
    using SamplerObject = LoFiTypedSamplerObject<textureType>;

    const TextureObject * const typedTexture =
        dynamic_cast<TextureObject *>(texture.get());
    if (!typedTexture) {
        TF_CODING_ERROR("Bad texture object");
        return nullptr;
    }

    return std::make_shared<SamplerObject>(
        *typedTexture,
        samplerParameters,
        createBindlessHandle,
        samplerObjectRegistry);
}

static
LoFiSamplerObjectSharedPtr
_MakeSamplerObject(
    LoFiTextureObjectSharedPtr const &texture,
    HdSamplerParameters const &samplerParameters,
    const bool createBindlessHandle,
    LoFiSamplerObjectRegistry * const samplerObjectRegistry)
{
    switch(texture->GetTextureType()) {
    case HdTextureType::Uv:
        return _MakeTypedSamplerObject<HdTextureType::Uv>(
            texture,
            samplerParameters,
            createBindlessHandle,
            samplerObjectRegistry);
    case HdTextureType::Field:
        return _MakeTypedSamplerObject<HdTextureType::Field>(
            texture,
            samplerParameters,
            createBindlessHandle,
            samplerObjectRegistry);
    case HdTextureType::Ptex:
        return _MakeTypedSamplerObject<HdTextureType::Ptex>(
            texture,
            samplerParameters,
            createBindlessHandle,
            samplerObjectRegistry);
    case HdTextureType::Udim:
        return _MakeTypedSamplerObject<HdTextureType::Udim>(
            texture,
            samplerParameters,
            createBindlessHandle,
            samplerObjectRegistry);
    }

    TF_CODING_ERROR("Unsupported texture type");
    return nullptr;
}    

LoFiSamplerObjectSharedPtr
LoFiSamplerObjectRegistry::AllocateSampler(
    LoFiTextureObjectSharedPtr const &texture,
    HdSamplerParameters const &samplerParameters,
    bool const createBindlessHandle)
{
    TRACE_FUNCTION();

    LoFiSamplerObjectSharedPtr const result = _MakeSamplerObject(
        texture, samplerParameters, createBindlessHandle, this);

    if (result) {
        // Record sampler object
        _samplerObjects.push_back(result);
    }

    return result;
}

void
LoFiSamplerObjectRegistry::MarkGarbageCollectionNeeded()
{
    _garbageCollectionNeeded = true;
}

LoFiResourceRegistry *
LoFiSamplerObjectRegistry::GetResourceRegistry() const
{
    return _resourceRegistry;
}

// Remove all shared pointers to objects not referenced by any client.
void
LoFiSamplerObjectRegistry::GarbageCollect()
{
    TRACE_FUNCTION();

    if (!_garbageCollectionNeeded) {
        return;
    }

    // Go from left to right, filling slots that became empty
    // with "shared" shared pointers from the right.
    size_t last = _samplerObjects.size();

    for (size_t i = 0; i < last; i++) {
        if (_samplerObjects[i].unique()) {
            while(true) {
                last--;
                if (i == last) {
                    break;
                }
                if (!_samplerObjects[last].unique()) {
                    _samplerObjects[i] = _samplerObjects[last];
                    break;
                }
            }
        }
    }
    
    _samplerObjects.resize(last);

    _garbageCollectionNeeded = false;
}

PXR_NAMESPACE_CLOSE_SCOPE
