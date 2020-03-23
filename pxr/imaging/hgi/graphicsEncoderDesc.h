//
// Copyright 2019 Pixar
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
#ifndef PXR_IMAGING_HGI_GRAPHICS_ENCODER_DESC_H
#define PXR_IMAGING_HGI_GRAPHICS_ENCODER_DESC_H

#include "pxr/pxr.h"
#include "pxr/imaging/hgi/api.h"
#include "pxr/imaging/hgi/attachmentDesc.h"
#include "pxr/imaging/hgi/texture.h"
#include <vector>

PXR_NAMESPACE_OPEN_SCOPE


/// \struct HgiGraphicsEncoderDesc
///
/// Describes the properties to begin a HgiGraphicsEncoder.
///
/// <ul>
/// <li>colorAttachmentDescs:
///   Describes each of the color attachments.</li>
/// <li>depthAttachmentDesc:
///   Describes the depth attachment (optional)</li>
/// <li>colorTextures:
///   The color attachment render targets.</li>
/// <li>depthAttachment:
///   The depth attachment render target (optional)</li>
/// <li>width:
///   Render target width (in pixels)</li>
/// <li>height:
///   Render target height (in pixels)</li>
/// </ul>
///
struct HgiGraphicsEncoderDesc
{
    HgiGraphicsEncoderDesc()
    : colorAttachmentDescs()
    , depthAttachmentDesc()
    , colorTextures()
    , depthTexture()
    , width(0)
    , height(0)
    {}

    inline bool HasAttachments() const {
        return !colorAttachmentDescs.empty() || depthTexture;
    }

    HgiAttachmentDescVector colorAttachmentDescs;
    HgiAttachmentDesc depthAttachmentDesc;

    HgiTextureHandleVector colorTextures;
    HgiTextureHandle depthTexture;

    uint32_t width;
    uint32_t height;
};

HGI_API
bool operator==(
    const HgiGraphicsEncoderDesc& lhs,
    const HgiGraphicsEncoderDesc& rhs);

HGI_API
bool operator!=(
    const HgiGraphicsEncoderDesc& lhs,
    const HgiGraphicsEncoderDesc& rhs);

HGI_API
std::ostream& operator<<(
    std::ostream& out,
    const HgiGraphicsEncoderDesc& encoder);


PXR_NAMESPACE_CLOSE_SCOPE

#endif
