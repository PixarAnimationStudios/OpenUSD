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
#ifndef PXR_IMAGING_HGIGL_CONTEXT_ARENA_H
#define PXR_IMAGING_HGIGL_CONTEXT_ARENA_H

#include "pxr/pxr.h"
#include "pxr/imaging/hgi/handle.h"
#include "pxr/imaging/hgiGL/api.h"

#include <ostream>
#include <memory>

PXR_NAMESPACE_OPEN_SCOPE

struct HgiGraphicsCmdsDesc;

/// \class HgiGLContextArena
///
/// Represents an arena for the HgiGL instance to manage container object
/// resources that are tied to the current GL context (and can't be shared).
/// (e.g. framebuffer objects)
///
/// See notes and relevant API in hgiGL/hgi.h
///
class HgiGLContextArena final
{
public:
    HGIGL_API
    ~HgiGLContextArena();

private:
    friend class HgiGL;
    friend class HgiGLDevice;

    HGIGL_API
    HgiGLContextArena();

    /// Returns a framebuffer id that matches the descriptor.
    uint32_t _AcquireFramebuffer(
        HgiGraphicsCmdsDesc const& desc,
        bool resolved = false);

    void _GarbageCollect();
    
    HgiGLContextArena & operator=(const HgiGLContextArena&) = delete;
    HgiGLContextArena(const HgiGLContextArena&) = delete;

    friend std::ostream& operator<<(
        std::ostream& out,
        const HgiGLContextArena& arena);

    // Implementation detail.
    class _FramebufferCache;
    std::unique_ptr<_FramebufferCache> _framebufferCache;
};

using HgiGLContextArenaHandle = HgiHandle<HgiGLContextArena>;

PXR_NAMESPACE_CLOSE_SCOPE

#endif
