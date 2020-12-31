//
// Copyright 2020 benmalartre
//
// Unlicensed
//
#ifndef PXR_IMAGING_LOFI_TEXTURE_HANDLE_REGISTRY_H
#define PXR_IMAGING_LOFI_TEXTURE_HANDLE_REGISTRY_H

#include "pxr/pxr.h"
#include "pxr/imaging/plugin/LoFi/api.h"

#include "pxr/imaging/plugin/LoFi/textureObject.h"

#include "pxr/imaging/hd/enums.h"

#include <tbb/concurrent_vector.h>

#include <set>
#include <memory>

PXR_NAMESPACE_OPEN_SCOPE

class LoFiResourceRegistry;
class LoFiTextureIdentifier;
class HdSamplerParameters;
class LoFiSamplerObjectRegistry;

using LoFiTextureHandlePtr =
    std::weak_ptr<class LoFiTextureHandle>;
using LoFiTextureHandleSharedPtr =
    std::shared_ptr<class LoFiTextureHandle>;
using LoFiTextureObjectPtr =
    std::weak_ptr<class LoFiTextureObject>;
using LoFiTextureObjectSharedPtr =
    std::shared_ptr<class LoFiTextureObject>;
using LoFiSamplerObjectSharedPtr =
    std::shared_ptr<class LoFiSamplerObject>;
using LoFiShaderCodePtr =
    std::weak_ptr<class LoFiShaderCode>;
using LoFiShaderCodeSharedPtr =
    std::shared_ptr<class LoFiShaderCode>;

/// \class LoFiTextureHandleRegistry
///
/// Keeps track of texture handles and allocates the textures and
/// samplers using the LoFiTextureObjectRegistry, respectively,
/// LoFiSamplerObjectRegistry.  Its responsibilities including
/// tracking what texture handles are associated to a texture,
/// computing the target memory of a texture from the memory requests
/// in the texture handles, triggering sampler and texture garbage
/// collection, and determining what LoFiShaderCode instances are
/// affecting by (re-)committing a texture.
///
class LoFiTextureHandleRegistry final
{
public:
    LOFI_API
    explicit LoFiTextureHandleRegistry(LoFiResourceRegistry * registry);

    LOFI_API
    ~LoFiTextureHandleRegistry();

    /// Allocate texture handle (thread-safe).
    ///
    /// See LoFiResourceRegistry::AllocateTextureHandle for details.
    ///
    LOFI_API
    LoFiTextureHandleSharedPtr AllocateTextureHandle(
        const LoFiTextureIdentifier &textureId,
        HdTextureType textureType,
        const HdSamplerParameters &samplerParams,
        /// memoryRequest in bytes.
        size_t memoryRequest,
        bool createBindlessHandle,
        LoFiShaderCodePtr const &shaderCode);

    /// Mark texture dirty (thread-safe).
    /// 
    /// If set, the target memory of the texture will be recomputed
    /// during commit and the data structure tracking the associated
    /// handles will be updated potentially triggering texture garbage
    /// collection.
    ///
    LOFI_API
    void MarkDirty(LoFiTextureObjectPtr const &texture);

    /// Mark shader dirty (thread-safe).
    ///
    /// If set, the shader is scheduled to be updated (i.e., have its
    /// AddResourcesFromTextures called) on the next commit.
    ///
    LOFI_API
    void MarkDirty(LoFiShaderCodePtr const &shader);

    /// Mark that sampler garbage collection needs to happen during
    /// next commit (thead-safe).
    ///
    LOFI_API
    void MarkSamplerGarbageCollectionNeeded();

    /// Get texture object registry.
    ///
    LoFiTextureObjectRegistry * GetTextureObjectRegistry() const {
        return _textureObjectRegistry.get();
    }

    /// Get sampler object registry.
    ///
    LoFiSamplerObjectRegistry * GetSamplerObjectRegistry() const {
        return _samplerObjectRegistry.get();
    }

    /// Commit textures. Return shader code instances that
    /// depend on the (re-)loaded textures so that they can add
    /// buffer sources based on the texture meta-data.
    ///
    /// Also garbage collect textures and samplers if necessary.
    ///
    LOFI_API
    std::set<LoFiShaderCodeSharedPtr> Commit();

    /// Sets how much memory a single texture can consume in bytes by
    /// texture type.
    ///
    /// Only has an effect if non-zero and only applies to textures if
    /// no texture handle referencing the texture has a memory
    /// request.
    ///
    LOFI_API
    void SetMemoryRequestForTextureType(HdTextureType textureType, size_t memoryRequest);

private:
    void _ComputeMemoryRequest(LoFiTextureObjectSharedPtr const &);
    void _ComputeMemoryRequests(const std::set<LoFiTextureObjectSharedPtr> &);
    void _ComputeAllMemoryRequests();

    bool _GarbageCollectHandlesAndComputeTargetMemory();
    void _GarbageCollectAndComputeTargetMemory();
    std::set<LoFiShaderCodeSharedPtr> _Commit();

    class _TextureToHandlesMap;

    // Maps texture type to memory a single texture of that type can consume
    // (in bytes).
    // Will be taken into account when computing the maximum of all the
    // memory requests of the texture handles.
    std::map<HdTextureType, size_t> _textureTypeToMemoryRequest;
    // Has _textureTypeToMemoryRequest changed since the last commit.
    bool _textureTypeToMemoryRequestChanged;

    // Handles that are new or for which the underlying texture has
    // changed: samplers might need to be (re-)allocated and the
    // corresponding shader code might need to update the shader bar.
    tbb::concurrent_vector<LoFiTextureHandlePtr> _dirtyHandles;

    // Textures whose set of associated handles and target memory
    // might have changed.
    tbb::concurrent_vector<LoFiTextureObjectPtr> _dirtyTextures;

    // Shaders that dropped a texture handle also need to be notified
    // (for example because they re-allocated the shader bar after dropping
    // the texture).
    tbb::concurrent_vector<LoFiShaderCodePtr> _dirtyShaders;

    std::unique_ptr<class LoFiSamplerObjectRegistry> _samplerObjectRegistry;
    std::unique_ptr<class LoFiTextureObjectRegistry> _textureObjectRegistry;
    std::unique_ptr<_TextureToHandlesMap> _textureToHandlesMap;

};

PXR_NAMESPACE_CLOSE_SCOPE

#endif
