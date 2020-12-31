//
// Copyright 2020 benmalartre
//
// Unlicensed
//
#ifndef PXR_IMAGING_LOFI_TEXTURE_OBJECT_REGISTRY_H
#define PXR_IMAGING_LOFI_TEXTURE_OBJECT_REGISTRY_H

#include "pxr/pxr.h"
#include "pxr/imaging/plugin/LoFi/api.h"
#include "pxr/imaging/hd/enums.h"
#include "pxr/imaging/hd/instanceRegistry.h"

#include <tbb/concurrent_vector.h>
#include <vector>
#include <atomic>

PXR_NAMESPACE_OPEN_SCOPE

using LoFiTextureObjectSharedPtr =
    std::shared_ptr<class LoFiTextureObject>;
using LoFiTextureObjectPtr =
    std::weak_ptr<class LoFiTextureObject>;
using LoFiTextureObjectPtrVector =
    std::vector<LoFiTextureObjectPtr>;
class LoFiResourceRegistry;
class LoFiTextureIdentifier;


/// \class LoFiTextureObjectRegistry
///
/// A central registry for texture GPU resources.
///
class LoFiTextureObjectRegistry final
{
public:
    explicit LoFiTextureObjectRegistry(LoFiResourceRegistry * registry);
    ~LoFiTextureObjectRegistry();

    /// Allocate texture.
    ///
    /// This just creates the LoFiTextureObject, the actual GPU
    /// resources won't be allocated until the Commit phase.
    ///
    LOFI_API
    LoFiTextureObjectSharedPtr AllocateTextureObject(
        const LoFiTextureIdentifier &textureId,
        HdTextureType textureType);

    /// Create GPU texture objects, load textures from files and
    /// upload to GPU.
    ///
    LOFI_API
    std::set<LoFiTextureObjectSharedPtr> Commit();

    /// Free GPU resources of textures not used by any client.
    ///
    LOFI_API
    void GarbageCollect();

    /// Mark texture file path as dirty. All textures whose identifier
    /// contains the file path will be reloaded during the next Commit.
    ///
    LOFI_API
    void MarkTextureFilePathDirty(const TfToken &filePath);

    /// Mark that the GPU resource for a texture needs to be
    /// (re-)loaded, e.g., because the memory request changed.
    ///
    LOFI_API
    void MarkTextureObjectDirty(
        LoFiTextureObjectPtr const &textureObject);

    /// Get resource registry
    ///
    LOFI_API
    LoFiResourceRegistry * GetResourceRegistry() const {
        return _resourceRegistry;
    }

    /// The total GPU memory consumed by all textures managed by this registry.
    ///
    int64_t GetTotalTextureMemory() const {
        return _totalTextureMemory;
    }

    /// Add signed number to total texture memory amount. Called from
    /// texture objects when (de-)allocated GPU resources.
    ///
    LOFI_API
    void AdjustTotalTextureMemory(int64_t memDiff);

private:
    LoFiTextureObjectSharedPtr _MakeTextureObject(
        const LoFiTextureIdentifier &textureId,
        HdTextureType textureType);

    std::atomic<int64_t> _totalTextureMemory;

    // Registry for texture and sampler objects.
    HdInstanceRegistry<LoFiTextureObjectSharedPtr>
        _textureObjectRegistry;

    // Map file paths to texture objects for quick invalidation
    // by file path.
    std::unordered_map<TfToken, LoFiTextureObjectPtrVector,
                       TfToken::HashFunctor>
        _filePathToTextureObjects;

    // File paths for which GPU resources need to be (re-)loaded
    tbb::concurrent_vector<TfToken> _dirtyFilePaths;

    // Texture for which GPU resources need to be (re-)loaded
    tbb::concurrent_vector<LoFiTextureObjectPtr> _dirtyTextures;

    LoFiResourceRegistry *_resourceRegistry;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif
