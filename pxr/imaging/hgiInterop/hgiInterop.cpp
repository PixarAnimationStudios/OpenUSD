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
#include "pxr/imaging/hgiInterop/hgiInterop.h"
#include "pxr/imaging/hgi/hgi.h"
#include "pxr/imaging/hgi/tokens.h"

#if defined(PXR_GL_SUPPORT_ENABLED)
#include "pxr/imaging/hgiInterop/opengl.h"
#endif

#if defined(PXR_VULKAN_SUPPORT_ENABLED)
#include "pxr/imaging/hgiInterop/vulkan.h"
#endif

#if defined(PXR_METAL_SUPPORT_ENABLED)
#include "pxr/imaging/hgiMetal/hgi.h"
#include "pxr/imaging/hgiInterop/metal.h"
#endif

PXR_NAMESPACE_OPEN_SCOPE

struct HgiInteropImpl
{
#if defined(PXR_GL_SUPPORT_ENABLED)
    std::unique_ptr<HgiInteropOpenGL> _openGLToOpenGL;
#endif
#if defined(PXR_VULKAN_SUPPORT_ENABLED)
    std::unique_ptr<HgiInteropVulkan> _vulkanToOpenGL;
#endif
#if defined(PXR_METAL_SUPPORT_ENABLED)
    std::unique_ptr<HgiInteropMetal> _metalToOpenGL;
#endif
};

HgiInterop::HgiInterop() 
 : _hgiInteropImpl(std::make_unique<HgiInteropImpl>())
{}

HgiInterop::~HgiInterop() = default;

void HgiInterop::TransferToApp(
    Hgi *srcHgi,
    HgiTextureHandle const &srcColor,
    HgiTextureHandle const &srcDepth,
    TfToken const &dstApi,
    VtValue const &dstFramebuffer,
    GfVec4i const &dstRegion)
{
    TfToken const& srcApi = srcHgi->GetAPIName();

    if (dstApi != HgiTokens->OpenGL) {
        TF_CODING_ERROR("Unsupported destination Hgi backend: %s",
                        dstApi.GetText());
        return;
    }

#if defined(PXR_GL_SUPPORT_ENABLED)
    if (srcApi == HgiTokens->OpenGL) {
        // Transfer OpenGL textures to OpenGL application
        if (!_hgiInteropImpl->_openGLToOpenGL) {
            _hgiInteropImpl->_openGLToOpenGL =
                std::make_unique<HgiInteropOpenGL>();
        }
        return _hgiInteropImpl->_openGLToOpenGL->CompositeToInterop(
            srcColor, srcDepth, dstFramebuffer, dstRegion);
    }
#endif

#if defined(PXR_VULKAN_SUPPORT_ENABLED)
    if (srcApi == HgiTokens->Vulkan) {
        // Transfer Vulkan textures to OpenGL application
        // XXX: It's possible that if we use the same HgiInterop with a 
        // different Hgi instance passed to this function, HgiInteropVulkan 
        // will have the wrong Hgi instance since we wouldn't recreate it here.
        // We should fix this.
        if (!_hgiInteropImpl->_vulkanToOpenGL) {
            _hgiInteropImpl->_vulkanToOpenGL =
                std::make_unique<HgiInteropVulkan>(srcHgi);
        }
        return _hgiInteropImpl->_vulkanToOpenGL->CompositeToInterop(
            srcColor, srcDepth, dstFramebuffer, dstRegion);
    }
#endif

#if defined(PXR_METAL_SUPPORT_ENABLED)
    if (srcApi == HgiTokens->Metal) {
        // Transfer Metal textures to OpenGL application
        // XXX: It's possible that if we use the same HgiInterop with a 
        // different Hgi instance passed to this function, HgiInteropMetal 
        // will have the wrong Hgi instance since we wouldn't recreate it here.
        // We should fix this.
        if (!_hgiInteropImpl->_metalToOpenGL) {
            _hgiInteropImpl->_metalToOpenGL =
                std::make_unique<HgiInteropMetal>(srcHgi);
        }
        return _hgiInteropImpl->_metalToOpenGL->CompositeToInterop(
            srcColor, srcDepth, dstFramebuffer, dstRegion);
    }
#endif

    TF_CODING_ERROR("Unsupported source Hgi backend: %s", srcApi.GetText());
}

PXR_NAMESPACE_CLOSE_SCOPE
