//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_IMAGING_HD_ST_RENDER_BUFFER_POOL_H
#define PXR_IMAGING_HD_ST_RENDER_BUFFER_POOL_H

#include "pxr/pxr.h"
#include "pxr/base/gf/vec3i.h"
#include "pxr/imaging/hdSt/api.h"
#include "pxr/imaging/hdSt/renderBuffer.h"
#include "pxr/imaging/hgi/hgi.h"
#include "pxr/imaging/hgi/enums.h"
#include "pxr/imaging/hgi/texture.h"

#include <deque>
#include <map>
#include <unordered_map>

PXR_NAMESPACE_OPEN_SCOPE

class HdStRenderBufferPool;

/// \class HdStPooledRenderBuffer
///
/// Represents a Render Buffer that may be re-used between render graphs
/// Client code has exclusive access to this buffer during render graph
/// execution, but there are no guarentees about contents before first usage
/// and after last usage.
///
class HdStPooledRenderBuffer final
{
public:
    HdStPooledRenderBuffer(
        std::shared_ptr<HdStRenderBuffer> buffer,
        HdStRenderBufferPool* pool,
        const SdfPath& graphPath,
        bool isDepth,
        uint16_t idx);
    ~HdStPooledRenderBuffer();

    HdStRenderBuffer* GetBuffer() const { return _buffer.get(); }
private:
    std::shared_ptr<HdStRenderBuffer> _buffer;
    HdStRenderBufferPool* _pool;
    const SdfPath _graphPath;
    bool _isDepth;
    uint16_t _idx;

    HdStPooledRenderBuffer & operator=(const HdStPooledRenderBuffer&) = delete;
    HdStPooledRenderBuffer(const HdStPooledRenderBuffer&) = delete;
};

using HdStPooledRenderBufferUniquePtr = std::unique_ptr<HdStPooledRenderBuffer>;

/// \class HdStRenderBufferPool
///
/// System for re-using HdStRenderBuffers between tasks in different graphs
/// that regenerate data per-frame. Ex: Shadow buffers
///
class HdStRenderBufferPool
{
public:
    // Allocate a RenderBuffer for usage during the current render graph
    [[nodiscard]]
    HDST_API
    HdStPooledRenderBufferUniquePtr Allocate(
        HdStResourceRegistry* registry,
        const SdfPath& graphPath,
        HdFormat fmt,
        GfVec2i dims,
        bool multiSampled,
        bool depth);

    HDST_API
    void Free(
        HdFormat fmt,
        GfVec2i dims,
        bool multiSampled,
        bool depth,
        const SdfPath& graphPath,
        uint16_t idx);

    // Frees allocations that are no longer in use by any render graphs
    HDST_API
    void Commit();
private:
    struct _PooledRenderBufferDesc
    {
        HdFormat fmt : 6;
        bool multiSampled : 1;
        bool depth : 1;
        GfVec2i dims;
        bool operator==(const _PooledRenderBufferDesc &other) const
        {
            return fmt == other.fmt &&
                dims == other.dims &&
                multiSampled == other.multiSampled &&
                depth == other.depth;
        }
        struct HashFunctor
        {
            size_t operator()(_PooledRenderBufferDesc const& desc) const
            {
                return (((desc.multiSampled << 0) | (desc.depth << 2)
                    | (desc.fmt << 3))
                    ^ (desc.dims[0] & 0xFFFF))
                    | ((size_t)(desc.dims[1] & 0xFFFF) << 32);
            }
        };
    };

    using _HdStRenderBufferVec =
        std::vector<std::shared_ptr<HdStRenderBuffer>>;

    class _AllocationTracker
    {
    public:
        uint16_t Allocate();
        void Free(uint16_t index);
    private:
        uint16_t max = 0;
        std::deque<uint16_t> freeList;
    };

    struct _HdStRenderBufferPoolEntry
    {
        std::map<const SdfPath, _AllocationTracker> allocs;
        _HdStRenderBufferVec buffers;
    };
    
    std::unordered_map<_PooledRenderBufferDesc,
        _HdStRenderBufferPoolEntry,
        _PooledRenderBufferDesc::HashFunctor> _pooledRenderBuffers;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif
