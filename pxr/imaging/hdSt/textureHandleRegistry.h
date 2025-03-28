//
// Copyright 2020 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_IMAGING_HD_ST_TEXTURE_HANDLE_REGISTRY_H
#define PXR_IMAGING_HD_ST_TEXTURE_HANDLE_REGISTRY_H

#include "pxr/pxr.h"
#include "pxr/imaging/hdSt/api.h"

#include "pxr/imaging/hdSt/textureObject.h"

#include "pxr/imaging/hd/enums.h"

#include <tbb/concurrent_vector.h>

#include <set>
#include <memory>

PXR_NAMESPACE_OPEN_SCOPE

class HdStResourceRegistry;
class HdStTextureIdentifier;
class HdSamplerParameters;
class HdSt_SamplerObjectRegistry;

using HdStTextureHandlePtr =
    std::weak_ptr<class HdStTextureHandle>;
using HdStTextureHandleSharedPtr =
    std::shared_ptr<class HdStTextureHandle>;
using HdStTextureObjectPtr =
    std::weak_ptr<class HdStTextureObject>;
using HdStTextureObjectSharedPtr =
    std::shared_ptr<class HdStTextureObject>;
using HdStSamplerObjectSharedPtr =
    std::shared_ptr<class HdStSamplerObject>;
using HdStShaderCodePtr =
    std::weak_ptr<class HdStShaderCode>;
using HdStShaderCodeSharedPtr =
    std::shared_ptr<class HdStShaderCode>;

/// \class HdSt_TextureHandleRegistry
///
/// Keeps track of texture handles and allocates the textures and
/// samplers using the HdSt_TextureObjectRegistry, respectively,
/// HdSt_SamplerObjectRegistry.  Its responsibilities including
/// tracking what texture handles are associated to a texture,
/// computing the target memory of a texture from the memory requests
/// in the texture handles, triggering sampler and texture garbage
/// collection, and determining what HdStShaderCode instances are
/// affecting by (re-)committing a texture.
///
class HdSt_TextureHandleRegistry final
{
public:
    HDST_API
    explicit HdSt_TextureHandleRegistry(HdStResourceRegistry * registry);

    HDST_API
    ~HdSt_TextureHandleRegistry();

    /// Allocate texture handle (thread-safe).
    ///
    /// See HdStResourceRegistry::AllocateTextureHandle for details.
    ///
    HDST_API
    HdStTextureHandleSharedPtr AllocateTextureHandle(
        const HdStTextureIdentifier &textureId,
        HdStTextureType textureType,
        const HdSamplerParameters &samplerParams,
        /// memoryRequest in bytes.
        size_t memoryRequest,
        HdStShaderCodePtr const &shaderCode);

    /// Mark texture dirty (thread-safe).
    /// 
    /// If set, the target memory of the texture will be recomputed
    /// during commit and the data structure tracking the associated
    /// handles will be updated potentially triggering texture garbage
    /// collection.
    ///
    HDST_API
    void MarkDirty(HdStTextureObjectPtr const &texture);

    /// Mark shader dirty (thread-safe).
    ///
    /// If set, the shader is scheduled to be updated (i.e., have its
    /// AddResourcesFromTextures called) on the next commit.
    ///
    HDST_API
    void MarkDirty(HdStShaderCodePtr const &shader);

    /// Mark that sampler garbage collection needs to happen during
    /// next commit (thead-safe).
    ///
    HDST_API
    void MarkSamplerGarbageCollectionNeeded();

    /// Get texture object registry.
    ///
    HdSt_TextureObjectRegistry * GetTextureObjectRegistry() const {
        return _textureObjectRegistry.get();
    }

    /// Get sampler object registry.
    ///
    HdSt_SamplerObjectRegistry * GetSamplerObjectRegistry() const {
        return _samplerObjectRegistry.get();
    }

    /// Commit textures. Return shader code instances that
    /// depend on the (re-)loaded textures so that they can add
    /// buffer sources based on the texture meta-data.
    ///
    /// Also garbage collect textures and samplers if necessary.
    ///
    HDST_API
    std::set<HdStShaderCodeSharedPtr> Commit();

    void GarbageCollect();

    /// Sets how much memory a single texture can consume in bytes by
    /// texture type.
    ///
    /// Only has an effect if non-zero and only applies to textures if
    /// no texture handle referencing the texture has a memory
    /// request.
    ///
    HDST_API
    void SetMemoryRequestForTextureType(HdStTextureType textureType, size_t memoryRequest);

    HDST_API
    size_t GetNumberOfTextureHandles() const;

private:
    void _ComputeMemoryRequest(HdStTextureObjectSharedPtr const &);
    void _ComputeMemoryRequests(const std::set<HdStTextureObjectSharedPtr> &);
    void _ComputeAllMemoryRequests();

    bool _GarbageCollectHandlesAndComputeTargetMemory();
    std::set<HdStShaderCodeSharedPtr> _Commit();

    class _TextureToHandlesMap;

    // Maps texture type to memory a single texture of that type can consume
    // (in bytes).
    // Will be taken into account when computing the maximum of all the
    // memory requests of the texture handles.
    std::map<HdStTextureType, size_t> _textureTypeToMemoryRequest;
    // Has _textureTypeToMemoryRequest changed since the last commit.
    bool _textureTypeToMemoryRequestChanged;

    // Handles that are new or for which the underlying texture has
    // changed: samplers might need to be (re-)allocated and the
    // corresponding shader code might need to update the shader bar.
    tbb::concurrent_vector<HdStTextureHandlePtr> _dirtyHandles;

    // Textures whose set of associated handles and target memory
    // might have changed.
    tbb::concurrent_vector<HdStTextureObjectPtr> _dirtyTextures;

    // Shaders that dropped a texture handle also need to be notified
    // (for example because they re-allocated the shader bar after dropping
    // the texture).
    tbb::concurrent_vector<HdStShaderCodePtr> _dirtyShaders;

    std::unique_ptr<class HdSt_SamplerObjectRegistry> _samplerObjectRegistry;
    std::unique_ptr<class HdSt_TextureObjectRegistry> _textureObjectRegistry;
    std::unique_ptr<_TextureToHandlesMap> _textureToHandlesMap;

};

PXR_NAMESPACE_CLOSE_SCOPE

#endif
