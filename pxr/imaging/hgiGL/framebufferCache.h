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
#ifndef PXR_IMAGING_HGI_GL_FRAMEBUFFER_CACHE_H
#define PXR_IMAGING_HGI_GL_FRAMEBUFFER_CACHE_H

#include <ostream>
#include <fstream>
#include <vector>

#include "pxr/pxr.h"
#include "pxr/imaging/hgi/graphicsCmdsDesc.h"
#include "pxr/imaging/hgiGL/api.h"

PXR_NAMESPACE_OPEN_SCOPE

using HgiGLDescriptorCacheVec = std::vector<struct HgiGLDescriptorCacheItem*>;


/// \class HgiGLFramebufferCache
///
/// Manages a cache of framebuffers based on graphics cmds descriptors.
///
class HgiGLFramebufferCache final
{
public:
    HGIGL_API
    HgiGLFramebufferCache();

    HGIGL_API
    ~HgiGLFramebufferCache();

    /// Get a framebuffer that matches the descriptor.
    /// If the framebuffer exists in the cache, it will be returned.
    /// If none exist that match the descriptor, it will be created.
    /// Do not hold onto the returned id. Re-acquire it every frame.
    ///
    /// When the cmds descriptor has resolved textures, two framebuffers are
    /// created for the MSAA and for the resolved textures. The bool flag can
    /// be used to access the respective ones.
    HGIGL_API
    uint32_t AcquireFramebuffer(HgiGraphicsCmdsDesc const& desc,
                                bool resolved = false);

    /// Clears all framebuffersfrom cache.
    /// This should generally only be called when the device is being destroyed.
    HGIGL_API
    void Clear();

private:
    friend std::ostream& operator<<(
        std::ostream& out,
        const HgiGLFramebufferCache& fbc);

    HgiGLDescriptorCacheVec _descriptorCache;
};


PXR_NAMESPACE_CLOSE_SCOPE

#endif
