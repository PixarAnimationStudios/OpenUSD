//
// Copyright 2020 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/imaging/hdSt/samplerObjectRegistry.h"

#include "pxr/imaging/hdSt/samplerObject.h"
#include "pxr/imaging/hdSt/ptexTextureObject.h"
#include "pxr/imaging/hdSt/textureObject.h"
#include "pxr/imaging/hdSt/udimTextureObject.h"

#include "pxr/base/trace/trace.h"

PXR_NAMESPACE_OPEN_SCOPE

HdSt_SamplerObjectRegistry::HdSt_SamplerObjectRegistry(
    HdStResourceRegistry * registry)
  : _garbageCollectionNeeded(false)
  , _resourceRegistry(registry)
{
}

HdSt_SamplerObjectRegistry::~HdSt_SamplerObjectRegistry() = default;

template<HdStTextureType textureType>
static
HdStSamplerObjectSharedPtr
_MakeTypedSamplerObject(
    HdStTextureObjectSharedPtr const &texture,
    HdSamplerParameters const &samplerParameters,
    HdSt_SamplerObjectRegistry * const samplerObjectRegistry)
{
    // e.g. HdStUvTextureObject
    using TextureObject = HdStTypedTextureObject<textureType>;
    // e.g. HdStUvSamplerObject
    using SamplerObject = HdStTypedSamplerObject<textureType>;

    const TextureObject * const typedTexture =
        dynamic_cast<TextureObject *>(texture.get());
    if (!typedTexture) {
        TF_CODING_ERROR("Bad texture object");
        return nullptr;
    }

    return std::make_shared<SamplerObject>(
        *typedTexture,
        samplerParameters,
        samplerObjectRegistry);
}

static
HdStSamplerObjectSharedPtr
_MakeSamplerObject(
    HdStTextureObjectSharedPtr const &texture,
    HdSamplerParameters const &samplerParameters,
    HdSt_SamplerObjectRegistry * const samplerObjectRegistry)
{
    switch(texture->GetTextureType()) {
    case HdStTextureType::Uv:
        return _MakeTypedSamplerObject<HdStTextureType::Uv>(
            texture,
            samplerParameters,
            samplerObjectRegistry);
    case HdStTextureType::Field:
        return _MakeTypedSamplerObject<HdStTextureType::Field>(
            texture,
            samplerParameters,
            samplerObjectRegistry);
    case HdStTextureType::Ptex:
        return _MakeTypedSamplerObject<HdStTextureType::Ptex>(
            texture,
            samplerParameters,
            samplerObjectRegistry);
    case HdStTextureType::Udim:
        return _MakeTypedSamplerObject<HdStTextureType::Udim>(
            texture,
            samplerParameters,
            samplerObjectRegistry);
    }

    TF_CODING_ERROR("Unsupported texture type");
    return nullptr;
}    

HdStSamplerObjectSharedPtr
HdSt_SamplerObjectRegistry::AllocateSampler(
    HdStTextureObjectSharedPtr const &texture,
    HdSamplerParameters const &samplerParameters)
{
    TRACE_FUNCTION();

    HdStSamplerObjectSharedPtr const result = _MakeSamplerObject(
        texture, samplerParameters, this);

    if (result) {
        // Record sampler object
        _samplerObjects.push_back(result);
    }

    return result;
}

void
HdSt_SamplerObjectRegistry::MarkGarbageCollectionNeeded()
{
    _garbageCollectionNeeded = true;
}

HdStResourceRegistry *
HdSt_SamplerObjectRegistry::GetResourceRegistry() const
{
    return _resourceRegistry;
}

// Remove all shared pointers to objects not referenced by any client.
void
HdSt_SamplerObjectRegistry::GarbageCollect()
{
    TRACE_FUNCTION();

    if (!_garbageCollectionNeeded) {
        return;
    }

    // Go from left to right, filling slots that became empty
    // with "shared" shared pointers from the right.
    size_t last = _samplerObjects.size();

    for (size_t i = 0; i < last; i++) {
        if (_samplerObjects[i].use_count() == 1) {
            while(true) {
                last--;
                if (i == last) {
                    break;
                }
                if (_samplerObjects[last].use_count() != 1) {
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
