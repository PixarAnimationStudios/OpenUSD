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
#include "pxr/imaging/garch/glApi.h"

#include "pxr/imaging/hgiGL/contextArena.h"
#include "pxr/imaging/hgiGL/debugCodes.h"
#include "pxr/imaging/hgiGL/diagnostic.h"
#include "pxr/imaging/hgiGL/texture.h"
#include "pxr/imaging/hgi/graphicsCmdsDesc.h"

#include "pxr/base/tf/diagnostic.h"
#include "pxr/base/tf/weakPtr.h"
#include "pxr/base/tf/envSetting.h"
#include "pxr/base/trace/trace.h"

#include <vector>

PXR_NAMESPACE_OPEN_SCOPE

TF_DEFINE_ENV_SETTING(HGIGL_CONTEXT_ARENA_REPORT_ERRORS, true,
    "Report errors when FBOs managed by the cache aren't deleted successfully");

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
struct _DescriptorCacheItem
{
    _FramebufferDesc descriptor;
    uint32_t framebuffer = 0;
    HgiGLTextureConstPtrVector attachments;
};

void
_CreateFramebuffer(
    const _FramebufferDesc &desc,
    uint32_t * const framebuffer,
    HgiGLTextureConstPtrVector * const attachments)
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
    

_DescriptorCacheItem*
_CreateDescriptorCacheItem(const _FramebufferDesc& desc)
{
    TRACE_FUNCTION();

    _DescriptorCacheItem* const dci = new _DescriptorCacheItem();
    dci->descriptor = desc;
    _CreateFramebuffer(desc, &dci->framebuffer, &dci->attachments);

    return dci;
}

// Deletes the cache item and returns whether the associated framebuffer object
// was deleted successfully.
bool
_DestroyDescriptorCacheItem(_DescriptorCacheItem* dci, void* cache)
{
    TRACE_FUNCTION();

    bool fboDeleted = false;
    if (dci->framebuffer) {
        if (glIsFramebuffer(dci->framebuffer)) {
            TF_DEBUG(HGIGL_DEBUG_FRAMEBUFFER_CACHE).Msg(
                "Deleting FBO %u from cache cache %p\n",
                dci->framebuffer, cache);

            glDeleteFramebuffers(1, &dci->framebuffer);
            dci->framebuffer = 0;
            fboDeleted = true;

        } else if (_IsErrorReportingEnabled()) {
            TF_CODING_ERROR("_DestroyDescriptorCacheItem: Found invalid "
                            "framebuffer %d in cache.\n", dci->framebuffer);
        }
    }

    HGIGL_POST_PENDING_GL_ERRORS();
    delete dci;
    return fboDeleted;
}

// If any of the texture attachments of a framebuffer object were deleted,
// the cache item is deemed invalid.
bool
_IsValid(_DescriptorCacheItem* dci)
{
    for (const HgiGLTextureConstPtr &texture : dci->attachments) {
        if (!texture) { // TfWeakPtr validity check
            return false;
        }
    }
    return true;
}

} // end anonymous namespace

// -----------------------------------------------------------------------------
// HgiGLContextArena::_FramebufferCache
// -----------------------------------------------------------------------------

using _DescriptorCacheVec = std::vector<_DescriptorCacheItem*>;

// Creating a framebuffer object or changing its attachments are expensive
// operations when performed frequently.
// The framebuffer cache mitigates this cost by maintaing a list of
// active entries based on graphics cmd descriptors.
// Although unbounded, we expect it be small with the expectation that
// GarbageCollect() is called frequently (typically per frame).
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
                                bool resolved = false);

    /// Removes framebuffer entries that reference invalid texture handles from
    /// the cache.
    void GarbageCollect();

private:
    /// Clears all framebuffers from cache.
    /// This should generally only be called when the arena is being destroyed.
    void _Clear();

    friend std::ostream& operator<<(
        std::ostream& out,
        const _FramebufferCache& fbc)
    {
        out << "_FramebufferCache: {" << std::endl;
        for (_DescriptorCacheItem const * d : fbc._descriptorCache) {
            out << "    " << d->descriptor << std::endl;
        }
        out << "}" << std::endl;
        return out;
    }

    _DescriptorCacheVec _descriptorCache;
};


HgiGLContextArena::_FramebufferCache::~_FramebufferCache()
{
    _Clear();
}

uint32_t
HgiGLContextArena::_FramebufferCache::AcquireFramebuffer(
    HgiGraphicsCmdsDesc const& graphicsCmdsDesc,
    const bool resolved)
{
    TRACE_FUNCTION();

    _DescriptorCacheItem* dci = nullptr;

    _FramebufferDesc desc(graphicsCmdsDesc, resolved);

    // Look for our framebuffer in cache based on the descriptor.
    for (size_t i=0; i<_descriptorCache.size(); i++) {
        _DescriptorCacheItem* const item = _descriptorCache[i];
        if (desc == item->descriptor) {
            if (glIsFramebuffer(item->framebuffer)) {
                TF_DEBUG(HGIGL_DEBUG_FRAMEBUFFER_CACHE).Msg(
                    "Cache Hit: Using FBO %u in cache %p.\n",
                    item->framebuffer, &_descriptorCache);

                return item->framebuffer;
            } else if (_IsErrorReportingEnabled()) {
                TF_CODING_ERROR("AcquireFramebuffer: Found invalid framebuffer "
                                "%d in cache.\n", item->framebuffer);
            }
        }
    }

    // Create a new descriptor cache item if it was not found
    if (!dci) {
        dci = _CreateDescriptorCacheItem(desc);
        _descriptorCache.push_back(dci);
        TF_DEBUG(HGIGL_DEBUG_FRAMEBUFFER_CACHE).Msg(
            "Cache Miss: Creating FBO %u in cache %p\n",
            dci->framebuffer, (void*)this);
    }

    return dci->framebuffer;
}

void
HgiGLContextArena::_FramebufferCache::GarbageCollect()
{
    TRACE_FUNCTION();

    const size_t numTotalEntries = _descriptorCache.size();
    size_t numStaleEntries = 0;
    // Remove FBO entries refering to texture attachments that were deleted.
    for (auto it = _descriptorCache.begin(); it != _descriptorCache.end();) {
        _DescriptorCacheItem* const dci = *it;

        if (!_IsValid(dci)) {
            if (_DestroyDescriptorCacheItem(dci, (void*)this)) {
                numStaleEntries++;
            }
            it = _descriptorCache.erase(it);
        } else {
            it++;
        }
    }

    TF_DEBUG(HGIGL_DEBUG_FRAMEBUFFER_CACHE).Msg(
        "Garbage collected %zu (of %zu) stale entries.\n",
        numStaleEntries, numTotalEntries);
}

void
HgiGLContextArena::_FramebufferCache::_Clear()
{
    TRACE_FUNCTION();

    const size_t numTotalEntries = _descriptorCache.size();
    size_t numClearedEntries = 0;
    for (_DescriptorCacheItem* dci : _descriptorCache) {
        if (_DestroyDescriptorCacheItem(dci, (void*)this)) {
            numClearedEntries++;
        }
    }
    _descriptorCache.clear();

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

uint32_t
HgiGLContextArena::_AcquireFramebuffer(
    HgiGraphicsCmdsDesc const& desc,
    bool resolved)
{
    return _framebufferCache.get()->AcquireFramebuffer(desc, resolved);
}

void
HgiGLContextArena::_GarbageCollect()
{
    _framebufferCache.get()->GarbageCollect();
}

std::ostream& operator<<(
    std::ostream& out,
    const HgiGLContextArena& arena)
{
    out << *arena._framebufferCache.get();
    return out;
}

PXR_NAMESPACE_CLOSE_SCOPE
