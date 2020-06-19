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
#ifndef PXR_IMAGING_HD_ST_TEXTURE_OBJECT_REGISTRY_H
#define PXR_IMAGING_HD_ST_TEXTURE_OBJECT_REGISTRY_H

#include "pxr/pxr.h"
#include "pxr/imaging/hdSt/api.h"
#include "pxr/imaging/hd/enums.h"
#include "pxr/imaging/hd/instanceRegistry.h"

#include <tbb/concurrent_vector.h>

PXR_NAMESPACE_OPEN_SCOPE

using HdStTextureObjectSharedPtr =
    std::shared_ptr<class HdStTextureObject>;
using HdStTextureObjectPtr =
    std::weak_ptr<class HdStTextureObject>;
class HdStTextureIdentifier;
class Hgi;

/// \class HdSt_TextureObjectRegistry
///
/// A central registry for texture GPU resources.
///
class HdSt_TextureObjectRegistry final
{
public:
    explicit HdSt_TextureObjectRegistry(Hgi * hgi);
    ~HdSt_TextureObjectRegistry();

    /// Allocate texture.
    ///
    /// This just creates the HdStTextureObject, the actual GPU
    /// resources won't be allocated until the Commit phase.
    ///
    HDST_API
    HdStTextureObjectSharedPtr AllocateTextureObject(
        const HdStTextureIdentifier &textureId,
        HdTextureType textureType);

    /// Create GPU texture objects, load textures from files and
    /// upload to GPU.
    ///
    HDST_API
    std::set<HdStTextureObjectSharedPtr> Commit();

    /// Free GPU resources of textures not used by any client.
    ///
    HDST_API
    void GarbageCollect();

    /// Mark that the GPU resource for a texture needs to be
    /// (re-)loaded, e.g., because the memory request changed.
    ///
    HDST_API
    void MarkTextureObjectDirty(
        HdStTextureObjectPtr const &textureObject);

    /// Get Hgi instance
    ///
    Hgi * GetHgi() const { return _hgi; }

private:
    HdStTextureObjectSharedPtr _MakeTextureObject(
        const HdStTextureIdentifier &textureId,
        HdTextureType textureType);

    // Registry for texture and sampler objects.
    HdInstanceRegistry<HdStTextureObjectSharedPtr>
        _textureObjectRegistry;

    // Texture for which GPU resources need to be (re-)loaded
    tbb::concurrent_vector<HdStTextureObjectPtr> _dirtyTextures;

    Hgi *_hgi;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif
