//
// Copyright 2020 benmalartre
//
// Unlicensed
//
#ifndef PXR_IMAGING_LOFI_SAMPLER_OBJECT_REGISTRY_H
#define PXR_IMAGING_LOFI_SAMPLER_OBJECT_REGISTRY_H

#include "pxr/pxr.h"
#include "pxr/imaging/plugin/LoFi/api.h"

#include <memory>
#include <vector>

PXR_NAMESPACE_OPEN_SCOPE

class HdSamplerParameters;
using LoFiTextureObjectSharedPtr =
    std::shared_ptr<class LoFiTextureObject>;
using LoFiSamplerObjectSharedPtr =
    std::shared_ptr<class LoFiSamplerObject>;
class LoFiResourceRegistry;

/// \class LoFiSamplerObjectRegistry
///
/// A simple registry for GPU samplers and GL texture sampler handles
/// (for bindless textures).
///
/// The registry makes no attempt at de-duplication. But construction
/// is dispatched by texture type returing a matching sampler (e.g.,
/// LoFiFieldSamplerObject for a LoFiFieldTextureObject or
/// LoFiPtexSamplerObject for the (not yet existing)
/// LoFiPtexTextureObject). Also, it keeps a shared pointer to a sampler
/// around until garbage collection so that clients can safely drop their
/// shared pointers from different threads.
///
class LoFiSamplerObjectRegistry final
{
public:
    LOFI_API
    explicit LoFiSamplerObjectRegistry(LoFiResourceRegistry * registry);

    LOFI_API 
    ~LoFiSamplerObjectRegistry();

    /// Create new sampler object matching the given texture object.
    ///
    /// If createBindlessHandle, also creates a texture sampler handle
    /// (for bindless textures). The associated GPU resource is
    /// created immediately and the call is not thread-safe.
    LOFI_API 
    LoFiSamplerObjectSharedPtr AllocateSampler(
        LoFiTextureObjectSharedPtr const &texture,
        HdSamplerParameters const &samplerParameters,
        bool createBindlessHandle);

    /// Delete samplers no longer used by a client.
    LOFI_API 
    void GarbageCollect();

    LOFI_API
    void MarkGarbageCollectionNeeded();

    /// Get resource registry
    ///
    LOFI_API
    LoFiResourceRegistry * GetResourceRegistry() const;

private:
    std::vector<LoFiSamplerObjectSharedPtr> _samplerObjects;
    
    bool _garbageCollectionNeeded;
    LoFiResourceRegistry *_resourceRegistry;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif
