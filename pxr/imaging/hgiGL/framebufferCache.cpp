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

#include "pxr/imaging/hgiGL/diagnostic.h"
#include "pxr/imaging/hgiGL/framebufferCache.h"
#include "pxr/imaging/hgiGL/texture.h"
#include "pxr/imaging/hgi/graphicsCmdsDesc.h"
#include "pxr/base/tf/diagnostic.h"
#include "pxr/base/trace/trace.h"

#include <memory>

PXR_NAMESPACE_OPEN_SCOPE

namespace {

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

}

struct HgiGLDescriptorCacheItem
{
    _FramebufferDesc descriptor;
    uint32_t framebuffer = 0;
};

static void
_CreateFramebuffer(
    const _FramebufferDesc &desc,
    uint32_t * const framebuffer)
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
    

static HgiGLDescriptorCacheItem*
_CreateDescriptorCacheItem(const _FramebufferDesc& desc)
{
    TRACE_FUNCTION();

    HgiGLDescriptorCacheItem* const dci = new HgiGLDescriptorCacheItem();
    dci->descriptor = desc;

    // Create framebuffer
    _CreateFramebuffer(desc, &dci->framebuffer);

    // Bind color attachments
    return dci;
}

static void
_DestroyDescriptorCacheItem(HgiGLDescriptorCacheItem* dci)
{
    TRACE_FUNCTION();

    if (dci->framebuffer) {
        if (glIsFramebuffer(dci->framebuffer)) {
            glDeleteFramebuffers(1, &dci->framebuffer);
            dci->framebuffer = 0;
        }
    }

    delete dci;
    HGIGL_POST_PENDING_GL_ERRORS();
}

HgiGLFramebufferCache::HgiGLFramebufferCache() = default;

HgiGLFramebufferCache::~HgiGLFramebufferCache()
{
    Clear();
}

uint32_t
HgiGLFramebufferCache::AcquireFramebuffer(
    HgiGraphicsCmdsDesc const& graphicsCmdsDesc,
    const bool resolved)
{
    TRACE_FUNCTION();

    // We keep a small cache of descriptor / framebuffer combos since it is
    // potentially an expensive state change to attach textures to GL FBs.

    HgiGLDescriptorCacheItem* dci = nullptr;

    _FramebufferDesc desc(graphicsCmdsDesc, resolved);

    // Look for our framebuffer in cache
    for (size_t i=0; i<_descriptorCache.size(); i++) {
        HgiGLDescriptorCacheItem* const item = _descriptorCache[i];
        if (desc == item->descriptor) {
            // If the GL context is changed we cannot re-use the framebuffer as
            // framebuffers cannot be shared between contexts.
            if (glIsFramebuffer(item->framebuffer)) {
                dci = item;

                // Move descriptor to end of 'LRU cache' as it is still used.
                if (i < _descriptorCache.size()) {
                    _descriptorCache.erase(_descriptorCache.begin() + i);
                    _descriptorCache.push_back(dci);
                }
            }
            break;
        }
    }

    // Create a new descriptor cache item if it was not found
    if (!dci) {
        dci = _CreateDescriptorCacheItem(desc);
        _descriptorCache.push_back(dci);

        // Destroy oldest descriptor / FB in LRU cache vector.
        // The size of the cache is small enough and we only store ptrs so we
        // use a vector instead of a linked list LRU.
        const size_t descriptorLRUsize = 64;
        if (_descriptorCache.size() == descriptorLRUsize) {
            _DestroyDescriptorCacheItem(_descriptorCache.front());
            _descriptorCache.erase(_descriptorCache.begin());
        }
    }

    return dci->framebuffer;
}

void
HgiGLFramebufferCache::Clear()
{
    for (HgiGLDescriptorCacheItem* dci : _descriptorCache) {
        _DestroyDescriptorCacheItem(dci);
    }
    _descriptorCache.clear();
}

std::ostream& operator<<(
    std::ostream& out,
    const HgiGLFramebufferCache& fbc)
{
    out << "HgiGLFramebufferCache: {"
        << "descriptor cache: { ";

    for (HgiGLDescriptorCacheItem const * d : fbc._descriptorCache) {
        out << d->descriptor;
    }

    out << "}}";
    return out;
}

PXR_NAMESPACE_CLOSE_SCOPE
