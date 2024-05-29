//
// Copyright 2020 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
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
