//
// Copyright 2020 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_IMAGING_HGI_ATTACHMENT_DESC_H
#define PXR_IMAGING_HGI_ATTACHMENT_DESC_H

#include "pxr/pxr.h"
#include "pxr/imaging/hgi/api.h"
#include "pxr/imaging/hgi/enums.h"
#include "pxr/imaging/hgi/types.h"
#include "pxr/base/gf/vec4f.h"
#include <vector>

PXR_NAMESPACE_OPEN_SCOPE


/// \struct HgiAttachmentDesc
///
/// Describes the properties of a framebuffer attachment.
///
/// <ul>
/// <li>format:
///   The format of the attachment.
///   Must match what is set in HgiTextureDesc.</li>
/// <li>usage:
///   Describes how the texture is intended to be used.
///   Must match what is set in HgiTextureDesc.</li>
/// <li>loadOp:
///   The operation to perform on the attachment pixel data prior to rendering.</li>
/// <li>storeOp:
///   The operation to perform on the attachment pixel data after rendering.</li>
/// <li>clearValue:
///   The value to clear the attachment with (r,g,b,a) or (depth,stencil,x,x)</li>
/// <li>colorMask:
///   Whether to permit or restrict writing to component channels.</li>
/// <li>blendEnabled:
///   Determines if a blend operation should be applied to the attachment.</li>
/// <li> ***BlendFactor:
///   The blend factors for source and destination.</li>
/// <li> ***BlendOp: 
///   The blending operation.</li>
/// <li> blendConstantColor:
///   The constant color for blend operations.</li>
///
struct HgiAttachmentDesc
{
    HgiAttachmentDesc() 
    : format(HgiFormatInvalid)
    , usage(0)
    , loadOp(HgiAttachmentLoadOpLoad)
    , storeOp(HgiAttachmentStoreOpStore)
    , clearValue(0)
    , colorMask(HgiColorMaskRed | HgiColorMaskGreen |
                HgiColorMaskBlue | HgiColorMaskAlpha)
    , blendEnabled(false)
    , srcColorBlendFactor(HgiBlendFactorZero)
    , dstColorBlendFactor(HgiBlendFactorZero)
    , colorBlendOp(HgiBlendOpAdd)
    , srcAlphaBlendFactor(HgiBlendFactorZero)
    , dstAlphaBlendFactor(HgiBlendFactorZero)
    , alphaBlendOp(HgiBlendOpAdd)
    , blendConstantColor(0.0f, 0.0f, 0.0f, 0.0f)
    {}

    HgiFormat format;
    HgiTextureUsage usage;
    HgiAttachmentLoadOp loadOp;
    HgiAttachmentStoreOp storeOp;
    GfVec4f clearValue;
    HgiColorMask colorMask;
    bool blendEnabled;
    HgiBlendFactor srcColorBlendFactor;
    HgiBlendFactor dstColorBlendFactor;
    HgiBlendOp colorBlendOp;
    HgiBlendFactor srcAlphaBlendFactor;
    HgiBlendFactor dstAlphaBlendFactor;
    HgiBlendOp alphaBlendOp;
    GfVec4f blendConstantColor;
};

using HgiAttachmentDescVector = std::vector<HgiAttachmentDesc>;

HGI_API
bool operator==(
    const HgiAttachmentDesc& lhs,
    const HgiAttachmentDesc& rhs);

HGI_API
bool operator!=(
    const HgiAttachmentDesc& lhs,
    const HgiAttachmentDesc& rhs);

HGI_API
std::ostream& operator<<(
    std::ostream& out,
    const HgiAttachmentDesc& attachment);


PXR_NAMESPACE_CLOSE_SCOPE

#endif
