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
#ifndef PXR_IMAGING_HGI_ENUMS_H
#define PXR_IMAGING_HGI_ENUMS_H

#include "pxr/pxr.h"
#include "pxr/imaging/hgi/api.h"
#include <cstdint>

PXR_NAMESPACE_OPEN_SCOPE

typedef uint32_t HgiBits;


/// \enum HgiDeviceCapabilitiesBits
///
/// Describes what capabilities the requested device must have.
///
/// <ul>
/// <li>HgiDeviceCapabilitiesBitsPresentation:
///   The device must be capable of presenting graphics to screen</li>
/// </ul>
///
enum HgiDeviceCapabilitiesBits : HgiBits {
    HgiDeviceCapabilitiesBitsPresentation = 1 << 0,
};

typedef HgiBits HgiDeviceCapabilities;


/// \enum HgiTextureUsageBits
///
/// Describes how the texture will be used. If a texture has multiple uses you
/// can combine multiple bits.
///
/// <ul>
/// <li>HgiTextureUsageBitsColorTarget:
///   The texture is an color attachment rendered into via a render pass.</li>
/// <li>HgiTextureUsageBitsDepthTarget:
///   The texture is an depth attachment rendered into via a render pass.</li>
/// <li>HgiTextureUsageBitsShaderRead:
///   The texture is sampled from in a shader (image load / sampling)</li>
/// <li>HgiTextureUsageBitsShaderWrite:
///   The texture is written into from in a shader (image store)</li>
/// </ul>
///
enum HgiTextureUsageBits : HgiBits {
    HgiTextureUsageBitsColorTarget  = 1 << 0,
    HgiTextureUsageBitsDepthTarget  = 1 << 1,
    HgiTextureUsageBitsShaderRead   = 1 << 2,
    HgiTextureUsageBitsShaderWrite  = 1 << 3,
};

typedef HgiBits HgiTextureUsage;


/// \enum HgiSampleCount
///
/// Sample count for multi-sampling
///
enum HgiSampleCount {
    HgiSampleCount1  = 1,
    HgiSampleCount4  = 4,
    HgiSampleCount16 = 16,
};


/// \enum HgiAttachmentLoadOp
///
/// Describes what will happen to the attachment pixel data prior to rendering.
///
/// <ul>
/// <li>HgiAttachmentLoadOpDontCare:
///   All pixels are rendered to. Pixel data in render target starts undefined.</li>
/// <li>HgiAttachmentLoadOpClear:
///   The attachment  pixel data is cleared to a specified color value.</li>
/// <li>HgiAttachmentLoadOpLoad:
///   Previous pixel data is loaded into attachment prior to rendering.</li>
/// </ul>
///
enum HgiAttachmentLoadOp {
    HgiAttachmentLoadOpDontCare = 0,
    HgiAttachmentLoadOpClear,
    HgiAttachmentLoadOpLoad,
};


/// \enum HgiAttachmentStoreOp
///
/// Describes what will happen to the attachment pixel data after rendering.
///
/// <ul>
/// <li>HgiAttachmentStoreOpDontCare:
///   Pixel data is undefined after rendering has completed (no store cost)</li>
/// <li>HgiAttachmentStoreOpStore:
///   The attachment pixel data is stored in memory.</li>
/// </ul>
///
enum HgiAttachmentStoreOp {
    HgiAttachmentStoreOpDontCare = 0,
    HgiAttachmentStoreOpStore,
};


PXR_NAMESPACE_CLOSE_SCOPE

#endif
