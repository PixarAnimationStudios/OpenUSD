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
#ifndef PXR_IMAGING_HGIINTEROP_HGIINTEROP_H
#define PXR_IMAGING_HGIINTEROP_HGIINTEROP_H

#include "pxr/pxr.h"
#include "pxr/base/tf/token.h"
#include "pxr/imaging/hgiInterop/api.h"
#include "pxr/imaging/hgi/texture.h"

#include <memory>

PXR_NAMESPACE_OPEN_SCOPE

class Hgi;
class HgiInteropMetal;
class HgiInteropOpenGL;

/// \class HgiInterop
///
/// Hydra Graphics Interface Interop.
///
/// HgiInterop provides functionality to transfer render targets between
/// supported APIs as efficiently as possible.
///
class HgiInterop final
{
public:
    HGIINTEROP_API
    HgiInterop();

    HGIINTEROP_API
    ~HgiInterop();

    /// Transfer (blit) the provided textures to the application / viewer.
    /// `hgi`: 
    ///     Determines the source format/platform of the textures.
    ///     Eg. if hgi is of type HgiMetal, the textures are HgiMetalTexture.
    /// `interopDst`: 
    ///     Determines what target format/platform the application is using.
    ///     E.g. If hgi==HgiMetal and interopDst==OpenGL then TransferToApp
    ///     will present the metal textures to the gl application.
    /// `color`: is the source color aov texture to present to screen.
    /// `depth`: (optional) is the depth aov texture to present to screen.
    HGIINTEROP_API
    void TransferToApp(
        Hgi *hgi,
        TfToken const& interopDst,
        HgiTextureHandle const &color,
        HgiTextureHandle const &depth);

private:
    HgiInterop & operator=(const HgiInterop&) = delete;
    HgiInterop(const HgiInterop&) = delete;

#if defined(PXR_METAL_SUPPORT_ENABLED)
    std::unique_ptr<HgiInteropMetal> _metalToOpenGL;
#else
    std::unique_ptr<HgiInteropOpenGL> _openGLToOpenGL;
#endif
};


PXR_NAMESPACE_CLOSE_SCOPE

#endif
