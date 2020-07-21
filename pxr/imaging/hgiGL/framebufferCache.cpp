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
#include <GL/glew.h>
#include <memory>

#include "pxr/imaging/hgiGL/diagnostic.h"
#include "pxr/imaging/hgiGL/framebufferCache.h"
#include "pxr/imaging/hgiGL/texture.h"
#include "pxr/imaging/hgi/graphicsCmdsDesc.h"
#include "pxr/base/tf/diagnostic.h"
#include "pxr/base/trace/trace.h"


PXR_NAMESPACE_OPEN_SCOPE

struct HgiGLDescriptorCacheItem {
    HgiGraphicsCmdsDesc descriptor;
    uint32_t framebuffer = 0;
};

static HgiGLDescriptorCacheItem*
_CreateDescriptorCacheItem(const HgiGraphicsCmdsDesc& desc)
{
    HgiGLDescriptorCacheItem* dci = new HgiGLDescriptorCacheItem();
    dci->descriptor = desc;

    // Create framebuffer
    glCreateFramebuffers(1, &dci->framebuffer);

    // Bind color attachments
    size_t numColorAttachments = desc.colorAttachmentDescs.size();
    std::vector<GLenum> drawBuffers(numColorAttachments);

    TF_VERIFY(desc.colorTextures.size() == numColorAttachments,
        "Number of attachment descriptors and textures don't match");

    //
    // Color attachments
    //
    for (size_t i=0; i<numColorAttachments; i++) {
        HgiGLTexture* glTexture = static_cast<HgiGLTexture*>(
            desc.colorTextures[i].Get());

        if (!TF_VERIFY(glTexture, "Invalid attachment texture")) {
            continue;
        }

        uint32_t textureName = glTexture->GetTextureId();
        if (!TF_VERIFY(glIsTexture(textureName), "Attachment not a texture")) {
            continue;
        }

        glNamedFramebufferTexture(
            dci->framebuffer,
            GL_COLOR_ATTACHMENT0 + i,
            textureName,
            /*level*/ 0);

        drawBuffers[i] = GL_COLOR_ATTACHMENT0 + i;
    }

    glNamedFramebufferDrawBuffers(
        dci->framebuffer,
        numColorAttachments,
        drawBuffers.data());

    //
    // Depth attachment
    //
    HgiTextureHandle depthTex = desc.depthTexture;
    if (depthTex) {
        HgiGLTexture* glTexture = static_cast<HgiGLTexture*>(depthTex.Get());

        uint32_t textureName = glTexture->GetTextureId();

        if (TF_VERIFY(glIsTexture(textureName), "Attachment not a texture")) {
            GLenum attachment =
                (desc.depthAttachmentDesc.format == HgiFormatFloat32UInt8)?
                    GL_DEPTH_STENCIL_ATTACHMENT : GL_DEPTH_ATTACHMENT;

            glNamedFramebufferTexture(
                dci->framebuffer,
                attachment,
                textureName,
                0); // level
        }
    }

    // Note that if color or depth is multi-sample, they both have to be for GL.
    GLenum status = glCheckNamedFramebufferStatus(
        dci->framebuffer,
        GL_FRAMEBUFFER);
    TF_VERIFY(status == GL_FRAMEBUFFER_COMPLETE);

    HGIGL_POST_PENDING_GL_ERRORS();
    return dci;
}

static void
_DestroyDescriptorCacheItem(HgiGLDescriptorCacheItem* dci)
{
    if (dci->framebuffer && glIsFramebuffer(dci->framebuffer)) {
        glDeleteFramebuffers(1, &dci->framebuffer);
        dci->framebuffer = 0;
    }

    delete dci;
    HGIGL_POST_PENDING_GL_ERRORS();
}

HgiGLFramebufferCache::HgiGLFramebufferCache()
{
}

HgiGLFramebufferCache::~HgiGLFramebufferCache()
{
    Clear();
}

uint32_t
HgiGLFramebufferCache::AcquireFramebuffer(HgiGraphicsCmdsDesc const& desc)
{
    // We keep a small cache of descriptor / framebuffer combos since it is
    // potentially an expensive state change to attach textures to GL FBs.

    HgiGLDescriptorCacheItem* dci = nullptr;

    // Look for our framebuffer in cache
    for (size_t i=0; i<_descriptorCache.size(); i++) {
        HgiGLDescriptorCacheItem* item = _descriptorCache[i];
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
        const size_t descriptorLRUsize = 32;
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
