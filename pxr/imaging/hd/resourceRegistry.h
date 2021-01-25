//
// Copyright 2016 Pixar
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
#ifndef PXR_IMAGING_HD_RESOURCE_REGISTRY_H
#define PXR_IMAGING_HD_RESOURCE_REGISTRY_H

#include "pxr/pxr.h"
#include "pxr/imaging/hd/api.h"
#include "pxr/imaging/hd/version.h"

#include "pxr/imaging/hd/perfLog.h"

#include "pxr/imaging/hf/perfLog.h"

#include "pxr/base/tf/singleton.h"
#include "pxr/base/tf/token.h"
#include "pxr/base/vt/dictionary.h"

#include <memory>

PXR_NAMESPACE_OPEN_SCOPE


using HdResourceRegistrySharedPtr = std::shared_ptr<class HdResourceRegistry>;

/// \class HdResourceRegistry
///
/// A central registry for resources.
///
class HdResourceRegistry  {
public:
    HF_MALLOC_TAG_NEW("new HdResourceRegistry");

    HD_API
    HdResourceRegistry();

    HD_API
    virtual ~HdResourceRegistry();

    /// Commits all in-flight source data.
    HD_API
    void Commit();

    /// cleanup all buffers and remove if empty
    HD_API
    void GarbageCollect();

    /// Globally unique id for texture, see HdRenderIndex::GetTextureKey() for
    /// details.
    typedef size_t TextureKey;

    /// Invalidate any shaders registered with this registry.
    HD_API
    virtual void InvalidateShaderRegistry();

    /// Generic method to inform RenderDelegate a resource needs to be reloaded.
    /// This method can be used by the application to inform the renderDelegate
    /// that a resource, which may not have any prim representation in Hydra, 
    /// needs to be reloaded. For example a texture found in a material network.
    /// The `path` can be absolute or relative. It should usually match the
    /// path found for textures during HdMaterial::Sync.
    HD_API
    virtual void ReloadResource(
        TfToken const& resourceType,
        std::string const& path);

    /// Returns a report of resource allocation by role in bytes and
    /// a summary total allocation of GPU memory in bytes for this registry.
    HD_API
    virtual VtDictionary GetResourceAllocation() const;

protected:
    /// A hook for derived registries to perform additional resource commits.
    HD_API
    virtual void _Commit();

    /// Hooks for derived registries to perform additional GC when
    /// GarbageCollect() is invoked.
    HD_API
    virtual void _GarbageCollect();

private:
    // Not copyable
    HdResourceRegistry(const HdResourceRegistry&) = delete;
    HdResourceRegistry& operator=(const HdResourceRegistry&) = delete;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif //PXR_IMAGING_HD_RESOURCE_REGISTRY_H
