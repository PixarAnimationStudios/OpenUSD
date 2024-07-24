//
// Copyright 2019 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_IMAGING_HGI_GRAPHICS_CMDS_DESC_H
#define PXR_IMAGING_HGI_GRAPHICS_CMDS_DESC_H

#include "pxr/pxr.h"
#include "pxr/imaging/hgi/api.h"
#include "pxr/imaging/hgi/attachmentDesc.h"
#include "pxr/imaging/hgi/texture.h"
#include <vector>

PXR_NAMESPACE_OPEN_SCOPE


/// \struct HgiGraphicsCmdsDesc
///
/// Describes the properties to begin a HgiGraphicsCmds.
///
/// <ul>
/// <li>colorAttachmentDescs:
///   Describes each of the color attachments.</li>
/// <li>depthAttachmentDesc:
///   Describes the depth attachment (optional)</li>
/// <li>colorTextures:
///   The color attachment render targets.</li>
/// <li>colorResolveTextures:
///   The (optional) textures that the color textures will be resolved into
///   at the end of the render pass.</li>
/// <li>depthTexture:
///   The depth attachment render target (optional)</li>
/// <li>depthResolveTexture:
///   The (optional) texture that the depth texture will be resolved into
///   at the end of the render pass.</li>
/// <li>width:
///   Render target width (in pixels)</li>
/// <li>height:
///   Render target height (in pixels)</li>
/// </ul>
///
struct HgiGraphicsCmdsDesc
{
    HgiGraphicsCmdsDesc()
    : colorAttachmentDescs()
    , depthAttachmentDesc()
    , colorTextures()
    , colorResolveTextures()
    , depthTexture()
    , depthResolveTexture()
    {}

    inline bool HasAttachments() const {
        return !colorAttachmentDescs.empty() || depthTexture;
    }

    HgiAttachmentDescVector colorAttachmentDescs;
    HgiAttachmentDesc depthAttachmentDesc;

    HgiTextureHandleVector colorTextures;
    HgiTextureHandleVector colorResolveTextures;

    HgiTextureHandle depthTexture;
    HgiTextureHandle depthResolveTexture;
};

HGI_API
bool operator==(
    const HgiGraphicsCmdsDesc& lhs,
    const HgiGraphicsCmdsDesc& rhs);

HGI_API
bool operator!=(
    const HgiGraphicsCmdsDesc& lhs,
    const HgiGraphicsCmdsDesc& rhs);

HGI_API
std::ostream& operator<<(
    std::ostream& out,
    const HgiGraphicsCmdsDesc& desc);


PXR_NAMESPACE_CLOSE_SCOPE

#endif
