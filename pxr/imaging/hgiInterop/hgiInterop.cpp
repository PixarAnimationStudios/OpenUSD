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


#if defined(PXR_METAL_SUPPORT_ENABLED)
    #include "pxr/imaging/hgiMetal/hgi.h"
    #include "pxr/imaging/hgiInterop/metal.h"
#else
    #include "pxr/imaging/hgiGL/hgi.h"
    #include "pxr/imaging/hgiInterop/opengl.h"
#endif

PXR_NAMESPACE_OPEN_SCOPE

HgiInterop::HgiInterop()
{
}

HgiInterop::~HgiInterop()
{
}

void HgiInterop::TransferToApp(
    Hgi *hgi,
    TfToken const& interopDst,
    HgiTextureHandle const &color,
    HgiTextureHandle const &depth)
{
    TfToken const& gfxApi = hgi->GetAPIName();

#if defined(PXR_METAL_SUPPORT_ENABLED)
    if (gfxApi==HgiTokens->Metal && interopDst==HgiTokens->OpenGL) {
        // Transfer Metal textures to OpenGL application
        if (!_metalToOpenGL) {
            _metalToOpenGL.reset(new HgiInteropMetal(hgi));
        }
        _metalToOpenGL->CompositeToInterop(color, depth);
    } else {
        TF_CODING_ERROR("Unsupported Hgi backed: %s", gfxApi.GetText());
    }
#else
    if (gfxApi==HgiTokens->OpenGL && interopDst==HgiTokens->OpenGL) {
        // Transfer OpenGL textures to OpenGL application
        if (!_openGLToOpenGL) {
            _openGLToOpenGL.reset(new HgiInteropOpenGL());
        }
        _openGLToOpenGL->CompositeToInterop(color, depth);
    } else if (gfxApi==HgiTokens->Vulkan && interopDst==HgiTokens->OpenGL) {
        // Transfer Vulkan textures to OpenGL application
        TF_CODING_ERROR("TODO Implement Vulkan/GL interop");
    } else {
        TF_CODING_ERROR("Unsupported Hgi backed: %s", gfxApi.GetText());
    }
#endif
}

PXR_NAMESPACE_CLOSE_SCOPE
