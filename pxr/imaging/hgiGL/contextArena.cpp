//
// Copyright 2020 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/imaging/garch/glApi.h"

#include "pxr/imaging/hgiGL/contextArena.h"
#include "pxr/imaging/hgiGL/debugCodes.h"
#include "pxr/imaging/hgiGL/diagnostic.h"
#include "pxr/imaging/hgiGL/texture.h"
#include "pxr/imaging/hgi/graphicsCmdsDesc.h"

#include "pxr/base/tf/diagnostic.h"
#include "pxr/base/tf/weakPtr.h"
#include "pxr/base/tf/hash.h"
#include "pxr/base/tf/envSetting.h"
#include "pxr/base/trace/trace.h"

#include <vector>

PXR_NAMESPACE_OPEN_SCOPE

TF_DEFINE_ENV_SETTING(HGIGL_CONTEXT_ARENA_REPORT_ERRORS, true,
    "Report errors when FBOs managed by the cache aren't deleted successfully");

class FramebufferCacheItem
{
public:
    virtual ~FramebufferCacheItem() = default;
};

namespace {

bool
_IsErrorReportingEnabled()
{
    static bool reportErrors =
        TfGetEnvSetting(HGIGL_CONTEXT_ARENA_REPORT_ERRORS);
    return reportErrors;
}

struct _FramebufferDesc
{
    _FramebufferDesc() = default;

    _FramebufferDesc(HgiGraphicsCmdsDesc const& desc, bool resolved)
        : depthFormat(desc.depthAttachmentDesc.format)
        , colorTextures(
            resolved && !desc.colorResolveTextures.empty()
                ? desc.colorResolveTextures : desc.colorTextures)
        , depthTexture(
            resolved && desc.depthResolveTexture
                ? desc.depthResolveTexture : desc.depthTexture)
    {
        TF_VERIFY(
            colorTextures.size() == desc.colorAttachmentDescs.size(),
            "Number of attachment descriptors and textures don't match");
    }

    HgiFormat depthFormat;
    HgiTextureHandleVector colorTextures;
    HgiTextureHandle depthTexture;

    // TfHash support.
    template <class HashState>
    friend void TfHashAppend(HashState &h, _FramebufferDesc const &desc)
    {
        h.Append((uint32_t)desc.depthFormat, desc.depthTexture.GetId());
        for (const auto& tex : desc.colorTextures) {
            h.Append(tex.GetId());
        }
    }
};

bool operator==(
    const _FramebufferDesc& lhs,
    const _FramebufferDesc& rhs)
{
    return lhs.depthFormat == rhs.depthFormat &&
           lhs.colorTextures == rhs.colorTextures &&
           lhs.depthTexture == rhs.depthTexture;
}

std::ostream& operator<<(
    std::ostream& out,
    const _FramebufferDesc& desc)
{
    out << "_FramebufferDesc: {";

    for (size_t i=0; i<desc.colorTextures.size(); i++) {
        out << "colorTexture" << i << " ";
        out << "dimensions:" << 
            desc.colorTextures[i]->GetDescriptor().dimensions << ", ";
    }

    if (desc.depthTexture) {
        out << "depthFormat " << desc.depthFormat;
        out << "depthTexture ";
        out << "dimensions:" << desc.depthTexture->GetDescriptor().dimensions;
    }

    out << "}";
    return out;
}

// -----------------------------------------------------------------------------

// Simple struct that tracks a framebuffer object and its texture attachments
// for a descriptor.
class _FramebufferCacheItem : public FramebufferCacheItem
{
public:
    virtual ~_FramebufferCacheItem() = default;
    _FramebufferDesc desc;
    uint32_t framebuffer = 0;
    HgiGLTextureConstPtrVector attachments;
};

using _FramebufferCacheItemPtr = std::shared_ptr<_FramebufferCacheItem>;
using _FramebufferCacheItemWeakPtr = std::weak_ptr<_FramebufferCacheItem>;

void
_CreateFramebuffer(
    const _FramebufferDesc &desc,
    uint32_t * const framebuffer,
    HgiGLTextureConstPtrVector * const attachments,
    const _FramebufferCacheItemPtr& cacheItem,
    HgiGLContextArena* arena)
{
    // Create framebuffer
    glCreateFramebuffers(1, framebuffer);
  
    // Bind color attachments
    const size_t numColorAttachments = desc.colorTextures.size();
    std::vector<GLenum> drawBuffers(numColorAttachments);

    //
    // Color attachments
    //
    for (size_t i=0; i<numColorAttachments; i++) {
        HgiGLTexture *const glTexture = static_cast<HgiGLTexture*>(
            desc.colorTextures[i].Get());

        if (!TF_VERIFY(glTexture, "Invalid attachment texture")) {
            continue;
        }

        glTexture->AddFramebuffer(cacheItem, arena);
        attachments->emplace_back(glTexture);

        const uint32_t textureName = glTexture->GetTextureId();
        if (!TF_VERIFY(glIsTexture(textureName), "Attachment not a texture")) {
            continue;
        }

        glNamedFramebufferTexture(
            *framebuffer,
            GL_COLOR_ATTACHMENT0 + i,
            textureName,
            /*level*/ 0);

        drawBuffers[i] = GL_COLOR_ATTACHMENT0 + i;
    }

    glNamedFramebufferDrawBuffers(
        *framebuffer,
        numColorAttachments,
        drawBuffers.data());

    //
    // Depth attachment
    //
    if (desc.depthTexture) {
        HgiGLTexture *const glTexture =
            static_cast<HgiGLTexture*>(desc.depthTexture.Get());

        const uint32_t textureName = glTexture->GetTextureId();

        glTexture->AddFramebuffer(cacheItem, arena);
        attachments->emplace_back(glTexture);

        if (TF_VERIFY(glIsTexture(textureName), "Attachment not a texture")) {

            const GLenum attachment =
                (desc.depthFormat == HgiFormatFloat32UInt8)?
                    GL_DEPTH_STENCIL_ATTACHMENT : GL_DEPTH_ATTACHMENT;

            glNamedFramebufferTexture(
                *framebuffer,
                attachment,
                textureName,
                0); // level
        }
    }

    // Note that if color or depth is multi-sample, they both have to be for GL.
    const GLenum status = glCheckNamedFramebufferStatus(
        *framebuffer,
        GL_FRAMEBUFFER);
    TF_VERIFY(status == GL_FRAMEBUFFER_COMPLETE);

    HGIGL_POST_PENDING_GL_ERRORS();
}
    

_FramebufferCacheItemPtr
_CreateFramebufferCacheItem(
    const _FramebufferDesc& desc,
    HgiGLContextArena* arena)
{
    TRACE_FUNCTION();

    _FramebufferCacheItemPtr const cacheItem =
        std::make_shared<_FramebufferCacheItem>();
    cacheItem->desc = desc;
    _CreateFramebuffer(desc, &cacheItem->framebuffer, &cacheItem->attachments,
        cacheItem, arena);

    return cacheItem;
}

// Deletes the cache item and returns whether the associated framebuffer object
// was deleted successfully.
bool
_DestroyFramebufferCacheItem(_FramebufferCacheItem* cacheItem, void* cache)
{
    TRACE_FUNCTION();

    bool fboDeleted = false;
    if (cacheItem->framebuffer) {
        if (glIsFramebuffer(cacheItem->framebuffer)) {
            TF_DEBUG(HGIGL_DEBUG_FRAMEBUFFER_CACHE).Msg(
                "Deleting FBO %u from cache cache %p\n",
                cacheItem->framebuffer, cache);

            glDeleteFramebuffers(1, &cacheItem->framebuffer);
            cacheItem->framebuffer = 0;
            fboDeleted = true;

        } else if (_IsErrorReportingEnabled()) {
            TF_CODING_ERROR("_DestroyFramebufferCacheItem: Found invalid "
                            "framebuffer %d in cache.\n",
                                cacheItem->framebuffer);
        }
    }

    HGIGL_POST_PENDING_GL_ERRORS();
    return fboDeleted;
}

} // end anonymous namespace

// -----------------------------------------------------------------------------
// HgiGLContextArena::_FramebufferCache
// -----------------------------------------------------------------------------

using _FramebufferCacheContainer =
    std::unordered_set<_FramebufferCacheItemPtr>;
using _FramebufferCacheLookupContainer =
    std::unordered_map<_FramebufferDesc, _FramebufferCacheItem*, TfHash>;

// Creating a framebuffer object or changing its attachments are expensive
// operations when performed frequently.
// The framebuffer cache mitigates this cost by maintaing a list of
// active entries based on graphics cmd descriptors.
//
class HgiGLContextArena::_FramebufferCache
{
public:
    _FramebufferCache() = default;

    ~_FramebufferCache();

    /// Get a framebuffer that matches the descriptor.
    /// If the framebuffer exists in the cache, it will be returned.
    /// If none exist that match the descriptor, it will be created.
    /// Do not hold onto the returned id. Re-acquire it every frame.
    ///
    /// When the cmds descriptor has resolved textures, two framebuffers are
    /// created for the MSAA and for the resolved textures. The bool flag can
    /// be used to access the respective ones.
    uint32_t AcquireFramebuffer(HgiGraphicsCmdsDesc const& desc,
                                bool resolved,
                                HgiGLContextArena* arena);

    // Removes a cache item from the pool. This is typically only done whenever
    // a texture associated with a cache item is deleted.
    void InvalidateCacheItem(_FramebufferCacheItemPtr& item);

private:
    /// Clears all framebuffers from cache.
    /// This should generally only be called when the arena is being destroyed.
    void _Clear();

    friend std::ostream& operator<<(
        std::ostream& out,
        const _FramebufferCache& fbc)
    {
        // Have to sort the cache so that output is stable across platforms
        // with different unordered_set implementations
        std::vector<_FramebufferCacheItemPtr> sortedItems(
            fbc._cache.begin(), fbc._cache.end());
        std::sort(sortedItems.begin(), sortedItems.end(),
            [](_FramebufferCacheItemPtr& self, _FramebufferCacheItemPtr& other)
        {
            const HgiTextureHandleVector& selfColors =
                self->desc.colorTextures;
            const HgiTextureHandleVector& otherColors =
                other->desc.colorTextures;
            uint32_t selfSize = selfColors.size();
            uint32_t otherSize = otherColors.size();
            if (selfSize != otherSize) {
                return selfSize < otherSize;
            }
            if (selfSize != 0) {
                for (uint32_t i = 0; i < selfColors.size(); i++) {
                    if (selfColors[i].GetId() != otherColors[i].GetId()) {
                        return selfColors[i].GetId() < otherColors[i].GetId();
                    }
                }
            }
            uint64_t selfDepth = self->desc.depthTexture ?
                self->desc.depthTexture.GetId() : 0;
            uint64_t otherDepth = other->desc.depthTexture ?
                other->desc.depthTexture.GetId() : 0;
            return selfDepth < otherDepth;
        });
        out << "_FramebufferCache: {" << std::endl;
        for (_FramebufferCacheItemPtr const& f : sortedItems) {
            out << "    " << f->desc << std::endl;
        }
        out << "}" << std::endl;
        return out;
    }

    _FramebufferCacheContainer _cache;
    _FramebufferCacheLookupContainer _lookup;
};


HgiGLContextArena::_FramebufferCache::~_FramebufferCache()
{
    _Clear();
}

uint32_t
HgiGLContextArena::_FramebufferCache::AcquireFramebuffer(
    HgiGraphicsCmdsDesc const& graphicsCmdsDesc,
    const bool resolved,
    HgiGLContextArena* arena)
{
    TRACE_FUNCTION();
    _FramebufferDesc desc(graphicsCmdsDesc, resolved);

    // Look for our framebuffer in cache based on the descriptor.
    auto iter = _lookup.find(desc);
    if (iter != _lookup.end()) {
        auto& item = iter->second;
        if (glIsFramebuffer(item->framebuffer)) {
            TF_DEBUG(HGIGL_DEBUG_FRAMEBUFFER_CACHE).Msg(
                "Cache Hit: Using FBO %u in cache %p.\n",
                item->framebuffer, &_cache);

            return item->framebuffer;
        } else if (_IsErrorReportingEnabled()) {
            TF_CODING_ERROR("AcquireFramebuffer: Found invalid framebuffer "
                            "%d in cache.\n", item->framebuffer);
        }
    }

    // Create a new framebuffer cache item if it was not found
    _FramebufferCacheItemPtr cacheItem =
        _CreateFramebufferCacheItem(desc, arena);
    _cache.insert(cacheItem);
    TF_DEBUG(HGIGL_DEBUG_FRAMEBUFFER_CACHE).Msg(
        "Cache Miss: Creating FBO %u in cache %p\n",
        cacheItem->framebuffer, (void*)this);
    _lookup[desc] = cacheItem.get();
    return cacheItem->framebuffer;
}

void
HgiGLContextArena::_FramebufferCache::InvalidateCacheItem(
    _FramebufferCacheItemPtr& item)
{
    TRACE_FUNCTION();
    auto it = _cache.find(item);
    if (it != _cache.end())
    {
        _lookup.erase((*it)->desc);
        _DestroyFramebufferCacheItem(it->get(), (void*)this);
        _cache.erase(it);
    }
}

void
HgiGLContextArena::_FramebufferCache::_Clear()
{
    TRACE_FUNCTION();

    const size_t numTotalEntries = _cache.size();
    size_t numClearedEntries = 0;
    for (const _FramebufferCacheItemPtr& cacheItem : _cache) {
        if (_DestroyFramebufferCacheItem(cacheItem.get(), (void*)this)) {
            numClearedEntries++;
        }
    }
    _lookup.clear();
    _cache.clear();

    TF_DEBUG(HGIGL_DEBUG_FRAMEBUFFER_CACHE).Msg(
        "Cleared %zu (of %zu) entries.\n",
        numClearedEntries, numTotalEntries);
}

// -----------------------------------------------------------------------------
// HgiGLContextArena
// -----------------------------------------------------------------------------

HgiGLContextArena::HgiGLContextArena()
    : _framebufferCache(
        std::make_unique<HgiGLContextArena::_FramebufferCache>())
{
}

HgiGLContextArena::~HgiGLContextArena() = default;

void 
HgiGLContextArena::InvalidateCacheItem(
    std::shared_ptr<FramebufferCacheItem>& item)
{
    _FramebufferCacheItemPtr castItem =
        std::dynamic_pointer_cast<_FramebufferCacheItem>(item);
    _framebufferCache->InvalidateCacheItem(castItem);
}

uint32_t
HgiGLContextArena::_AcquireFramebuffer(
    HgiGraphicsCmdsDesc const& desc,
    bool resolved)
{
    return _framebufferCache->AcquireFramebuffer(desc, resolved, this);
}

std::ostream& operator<<(
    std::ostream& out,
    const HgiGLContextArena& arena)
{
    out << *arena._framebufferCache.get();
    return out;
}

PXR_NAMESPACE_CLOSE_SCOPE
