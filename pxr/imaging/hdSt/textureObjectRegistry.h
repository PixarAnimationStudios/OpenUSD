//
// Copyright 2020 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_IMAGING_HD_ST_TEXTURE_OBJECT_REGISTRY_H
#define PXR_IMAGING_HD_ST_TEXTURE_OBJECT_REGISTRY_H

#include "pxr/pxr.h"
#include "pxr/imaging/hdSt/api.h"
#include "pxr/imaging/hdSt/enums.h"
#include "pxr/imaging/hd/instanceRegistry.h"

#include <tbb/concurrent_vector.h>
#include <vector>
#include <atomic>

PXR_NAMESPACE_OPEN_SCOPE

using HdStTextureObjectSharedPtr =
    std::shared_ptr<class HdStTextureObject>;
using HdStTextureObjectPtr =
    std::weak_ptr<class HdStTextureObject>;
using HdStTextureObjectPtrVector =
    std::vector<HdStTextureObjectPtr>;
class HdStResourceRegistry;
class HdStTextureIdentifier;


/// \class HdSt_TextureObjectRegistry
///
/// A central registry for texture GPU resources.
///
class HdSt_TextureObjectRegistry final
{
public:
    HDST_API
    explicit HdSt_TextureObjectRegistry(HdStResourceRegistry * registry);
    HDST_API
    ~HdSt_TextureObjectRegistry();

    /// Allocate texture.
    ///
    /// This just creates the HdStTextureObject, the actual GPU
    /// resources won't be allocated until the Commit phase.
    ///
    HDST_API
    HdStTextureObjectSharedPtr AllocateTextureObject(
        const HdStTextureIdentifier &textureId,
        HdStTextureType textureType);

    /// Create GPU texture objects, load textures from files and
    /// upload to GPU.
    ///
    HDST_API
    std::set<HdStTextureObjectSharedPtr> Commit();

    /// Free GPU resources of textures not used by any client.
    ///
    HDST_API
    void GarbageCollect();

    /// Mark texture file path as dirty. All textures whose identifier
    /// contains the file path will be reloaded during the next Commit.
    ///
    HDST_API
    void MarkTextureFilePathDirty(const TfToken &filePath);

    /// Mark that the GPU resource for a texture needs to be
    /// (re-)loaded, e.g., because the memory request changed.
    ///
    HDST_API
    void MarkTextureObjectDirty(
        HdStTextureObjectPtr const &textureObject);

    /// Get resource registry
    ///
    HDST_API
    HdStResourceRegistry * GetResourceRegistry() const {
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
    HDST_API
    void AdjustTotalTextureMemory(int64_t memDiff);

    /// The number of texture objects.
    size_t GetNumberOfTextureObjects() const {
        return _textureObjectRegistry.size();
    }

private:
    HdStTextureObjectSharedPtr _MakeTextureObject(
        const HdStTextureIdentifier &textureId,
        HdStTextureType textureType);

    std::atomic<int64_t> _totalTextureMemory;

    // Registry for texture and sampler objects.
    HdInstanceRegistry<HdStTextureObjectSharedPtr>
        _textureObjectRegistry;

    // Map file paths to texture objects for quick invalidation
    // by file path.
    std::unordered_map<TfToken, HdStTextureObjectPtrVector,
                       TfToken::HashFunctor>
        _filePathToTextureObjects;

    // File paths for which GPU resources need to be (re-)loaded
    tbb::concurrent_vector<TfToken> _dirtyFilePaths;

    // Texture for which GPU resources need to be (re-)loaded
    tbb::concurrent_vector<HdStTextureObjectPtr> _dirtyTextures;

    HdStResourceRegistry *_resourceRegistry;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif
