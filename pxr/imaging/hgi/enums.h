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

using HgiBits = uint32_t;


/// \enum HgiDeviceCapabilitiesBits
///
/// Describes what capabilities the requested device must have.
///
/// <ul>
/// <li>HgiDeviceCapabilitiesBitsPresentation:
///   The device must be capable of presenting graphics to screen</li>
/// </ul>
///
enum HgiDeviceCapabilitiesBits : HgiBits
{
    HgiDeviceCapabilitiesBitsPresentation = 1 << 0,
};

using HgiDeviceCapabilities = HgiBits;

/// \enum HgiTextureType
///
/// Describes the kind of texture.
///
/// <ul>
/// <li>HgiTextureType1D:
///   A one-dimensional texture.</li>
/// <li>HgiTextureType2D:
///   A two-dimensional texture.</li>
/// <li>HgiTextureType3D:
///   A three-dimensional texture.</li>
/// <li>HgiTextureType1DArray:
///   An array of one-dimensional textures.</li>
/// <li>HgiTextureType2DArray:
///   An array of two-dimensional textures.</li>
/// </ul>
///
enum HgiTextureType
{
    HgiTextureType1D = 0,
    HgiTextureType2D,
    HgiTextureType3D,
    HgiTextureType1DArray,
    HgiTextureType2DArray,

    HgiTextureTypeCount
};

/// \enum HgiTextureUsageBits
///
/// Describes how the texture will be used. If a texture has multiple uses you
/// can combine multiple bits.
///
/// <ul>
/// <li>HgiTextureUsageBitsColorTarget:
///   The texture is a color attachment rendered into via a render pass.</li>
/// <li>HgiTextureUsageBitsDepthTarget:
///   The texture is a depth attachment rendered into via a render pass.</li>
/// <li>HgiTextureUsageBitsStencilTarget:
///   The texture is a stencil attachment rendered into via a render pass.</li>
/// <li>HgiTextureUsageBitsShaderRead:
///   The texture is sampled from in a shader (sampling)</li>
/// <li>HgiTextureUsageBitsShaderWrite:
///   The texture is written into from in a shader (image store)
///   When a texture is used as HgiBindResourceTypeStorageImage you must
///   add this flag (even if you only read from the image).</li>
///
/// <li>HgiTextureUsageCustomBitsBegin:
///   This bit (and any bit after) can be used to attached custom, backend
///   specific  bits to the usage bit. </li>
/// </ul>
///
enum HgiTextureUsageBits : HgiBits
{
    HgiTextureUsageBitsColorTarget   = 1 << 0,
    HgiTextureUsageBitsDepthTarget   = 1 << 1,
    HgiTextureUsageBitsStencilTarget = 1 << 2,
    HgiTextureUsageBitsShaderRead    = 1 << 3,
    HgiTextureUsageBitsShaderWrite   = 1 << 4,

    HgiTextureUsageCustomBitsBegin = 1 << 5,
};

using HgiTextureUsage = HgiBits;

/// \enum HgiSamplerAddressMode
///
/// Various modes used during sampling of a texture.
///
enum HgiSamplerAddressMode
{
    HgiSamplerAddressModeClampToEdge = 0,
    HgiSamplerAddressModeMirrorClampToEdge,
    HgiSamplerAddressModeRepeat,
    HgiSamplerAddressModeMirrorRepeat,
    HgiSamplerAddressModeClampToBorderColor,

    HgiSamplerAddressModeCount
};

/// \enum HgiSamplerFilter
///
/// Sampler filtering modes that determine the pixel value that is returned.
///
/// <ul>
/// <li>HgiSamplerFilterNearest:
///   Returns the value of a single mipmap level.</li>
/// <li>HgiSamplerFilterLinear:
///   Combines the values of multiple mipmap levels.</li>
/// </ul>
///
enum HgiSamplerFilter
{
    HgiSamplerFilterNearest = 0,
    HgiSamplerFilterLinear  = 1,

    HgiSamplerFilterCount
};

/// \enum HgiMipFilter
///
/// Sampler filtering modes that determine the pixel value that is returned.
///
/// <ul>
/// <li>HgiMipFilterNotMipmapped:
///   Texture is always sampled at mipmap level 0. (ie. max lod=0)</li>
/// <li>HgiMipFilterNearest:
///   Returns the value of a single mipmap level.</li>
/// <li>HgiMipFilterLinear:
///   Linear interpolates the values of up to two mipmap levels.</li>
/// </ul>
///
enum HgiMipFilter
{
    HgiMipFilterNotMipmapped = 0,
    HgiMipFilterNearest      = 1,
    HgiMipFilterLinear       = 2,

    HgiMipFilterCount
};

/// \enum HgiSampleCount
///
/// Sample count for multi-sampling
///
enum HgiSampleCount
{
    HgiSampleCount1  = 1,
    HgiSampleCount2  = 2,
    HgiSampleCount4  = 4,
    HgiSampleCount8  = 8,
    HgiSampleCount16 = 16,

    HgiSampleCountEnd
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
enum HgiAttachmentLoadOp
{
    HgiAttachmentLoadOpDontCare = 0,
    HgiAttachmentLoadOpClear,
    HgiAttachmentLoadOpLoad,
    
    HgiAttachmentLoadOpCount
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
enum HgiAttachmentStoreOp
{
    HgiAttachmentStoreOpDontCare = 0,
    HgiAttachmentStoreOpStore,
    
    HgiAttachmentStoreOpCount
};

/// \enum HgiBufferUsageBits
///
/// Describes the properties and usage of the buffer.
///
/// <ul>
/// <li>HgiBufferUsageUniform:
///   Shader uniform buffer </li>
/// <li>HgiBufferUsageIndex32:
///   Topology 32 bit indices.</li>
/// <li>HgiBufferUsageVertex:
///   Vertex attributes.</li>
/// <li>HgiBufferUsageStorage:
///   Shader storage buffer / Argument buffer.</li>
///
/// <li>HgiBufferUsageCustomBitsBegin:
///   This bit (and any bit after) can be used to attached custom, backend
///   specific  bits to the usage bit. </li>
/// </ul>
///
enum HgiBufferUsageBits : HgiBits
{
    HgiBufferUsageUniform = 1 << 0,
    HgiBufferUsageIndex32 = 1 << 1,
    HgiBufferUsageVertex  = 1 << 2,
    HgiBufferUsageStorage = 1 << 3,

    HgiBufferUsageCustomBitsBegin = 1 << 4,
};
using HgiBufferUsage = HgiBits;

/// \enum HgiShaderStage
///
/// Describes the stage a shader function operates in.
///
/// <ul>
/// <li>HgiShaderStageVertex:
///   Vertex Shader.</li>
/// <li>HgiShaderStageFragment:
///   Fragment Shader.</li>
/// <li>HgiShaderStageCompute:
///   Compute Shader.</li>
/// <li>HgiShaderStageTessellationControl:
///   Transforms the control points of the low order surface (patch).
///   This runs before the tessellator fixed function stage.</li>
/// <li>HgiShaderStageTessellationEval:
///   Generates the surface geometry (the points) from the transformed control
///   points for every coordinate coming out of the tessellator fixed function
///  stage. </li>
/// <li>HgiShaderStageGeometry:
///   Governs the processing of Primitives.</li>
/// </ul>
///
enum HgiShaderStageBits : HgiBits
{
    HgiShaderStageVertex               = 1 << 0,
    HgiShaderStageFragment             = 1 << 1,
    HgiShaderStageCompute              = 1 << 2,
    HgiShaderStageTessellationControl  = 1 << 3,
    HgiShaderStageTessellationEval     = 1 << 4,
    HgiShaderStageGeometry             = 1 << 5,

    HgiShaderStageCustomBitsBegin      = 1 << 6,
};
using HgiShaderStage = HgiBits;

/// \enum HgiBindResourceType
///
/// Describes the type of the resource to be bound.
///
/// <ul>
/// <li>HgiBindResourceTypeSampler:
///   Sampler.
///   Glsl example: uniform sampler samplerOnly</li>
/// <li>HgiBindResourceTypeSampledImage:
///   Image for use with sampling ops.
///   Glsl example: uniform texture2D textureOnly
///   texture(sampler2D(textureOnly, samplerOnly), ...)</li>
/// <li>HgiBindResourceTypeCombinedSamplerImage:
///   Image and sampler combined into one.
///   Glsl example: uniform sampler2D texSmp;
///   texture(texSmp, ...)</li>
/// <li>HgiBindResourceTypeStorageImage:
///   Storage image used for image store/load ops (Unordered Access View).</li>
/// <li>HgiBindResourceTypeUniformBuffer:
///   Uniform buffer (UBO).</li>
/// <li>HgiBindResourceTypeStorageBuffer:
///   Shader storage buffer (SSBO).</li>
/// </ul>
///
enum HgiBindResourceType
{
    HgiBindResourceTypeSampler = 0,
    HgiBindResourceTypeSampledImage,
    HgiBindResourceTypeCombinedSamplerImage,
    HgiBindResourceTypeStorageImage,
    HgiBindResourceTypeUniformBuffer,
    HgiBindResourceTypeStorageBuffer,

    HgiBindResourceTypeCount
};

/// \enum HgiPolygonMode
///
/// Controls polygon mode during rasterization
///
/// <ul>
/// <li>HgiPolygonModeFill:
///   Polygons are filled.</li>
/// <li>HgiPolygonModeLine:
///   Polygon edges are drawn as line segments.</li>
/// <li>HgiPolygonModePoint:
///   Polygon vertices are drawn as points.</li>
/// </ul>
///
enum HgiPolygonMode
{
    HgiPolygonModeFill = 0,
    HgiPolygonModeLine,
    HgiPolygonModePoint,

    HgiPolygonModeCount
};

/// \enum HgiCullMode
///
/// Controls primitive (faces) culling.
///
/// <ul>
/// <li>HgiPolygonModeNone:
///   No primitive are discarded.</li>
/// <li>HgiPolygonModeFront:
///   Front-facing primitive are discarded.</li>
/// <li>HgiPolygonModeBack:
///   Back-facing primitive are discarded.</li>
/// <li>HgiPolygonModeFrontAndBack:
///   All primitive are discarded.</li>
/// </ul>
///
enum HgiCullMode
{
    HgiCullModeNone = 0,
    HgiCullModeFront,
    HgiCullModeBack,
    HgiCullModeFrontAndBack,

    HgiCullModeCount
};

/// \enum HgiWinding
///
/// Determines the front-facing orientation of a primitive (face).
///
/// <ul>
/// <li>HgiWindingClockwise:
///   Primitives with clockwise vertex-order are front facing.</li>
/// <li>HgiWindingCounterClockwise:
///   Primitives with counter-clockwise vertex-order are front facing.</li>
/// </ul>
///
enum HgiWinding
{
    HgiWindingClockwise = 0,
    HgiWindingCounterClockwise,

    HgiWindingCount
};


/// \enum HgiBlendOp
///
/// Blend operations
///
enum HgiBlendOp
{
    HgiBlendOpAdd = 0,
    HgiBlendOpSubtract,
    HgiBlendOpReverseSubtract,
    HgiBlendOpMin,
    HgiBlendOpMax,

    HgiBlendOpCount
};

/// \enum HgiBlendFactor
///
/// Blend factors
///
enum HgiBlendFactor
{
    HgiBlendFactorZero = 0,
    HgiBlendFactorOne,
    HgiBlendFactorSrcColor,
    HgiBlendFactorOneMinusSrcColor,
    HgiBlendFactorDstColor,
    HgiBlendFactorOneMinusDstColor,
    HgiBlendFactorSrcAlpha,
    HgiBlendFactorOneMinusSrcAlpha,
    HgiBlendFactorDstAlpha,
    HgiBlendFactorOneMinusDstAlpha,
    HgiBlendFactorConstantColor,
    HgiBlendFactorOneMinusConstantColor,
    HgiBlendFactorConstantAlpha,
    HgiBlendFactorOneMinusConstantAlpha,
    HgiBlendFactorSrcAlphaSaturate,
    HgiBlendFactorSrc1Color,
    HgiBlendFactorOneMinusSrc1Color,
    HgiBlendFactorSrc1Alpha,
    HgiBlendFactorOneMinusSrc1Alpha,

    HgiBlendFactorCount
};


/// \enum HgiCompareFunction
///
/// Compare functions.
///
enum HgiCompareFunction
{
    HgiCompareFunctionNever = 0,
    HgiCompareFunctionLess,
    HgiCompareFunctionEqual,
    HgiCompareFunctionLEqual,
    HgiCompareFunctionGreater,
    HgiCompareFunctionNotEqual,
    HgiCompareFunctionGEqual,
    HgiCompareFunctionAlways,

    HgiCompareFunctionCount
};

/// \enum HgiComponentSwizzle
///
/// Swizzle for a component.
///
enum HgiComponentSwizzle
{
    HgiComponentSwizzleZero = 0,
    HgiComponentSwizzleOne,
    HgiComponentSwizzleR,
    HgiComponentSwizzleG,
    HgiComponentSwizzleB,
    HgiComponentSwizzleA,

    HgiComponentSwizzleCount
};

/// \enum HgiPrimitiveType
///
/// What the stream of vertices being rendered represents
///
/// <ul>
/// <li>HgiPrimitiveTypePointList:
///   Rasterize a point at each vertex.</li>
/// <li>HgiPrimitiveTypeLineList:
///   Rasterize a line between each separate pair of vertices.</li>
/// <li>HgiPrimitiveTypeLineStrip:
///   Rasterize a line between each pair of adjacent vertices.</li>
/// <li>HgiPrimitiveTypeTriangleList:
///   Rasterize a triangle for every separate set of three vertices.</li>
/// <li>HgiPrimitiveTypePatchList:
///   A user-defined number of vertices, which is tessellated into
///   points, lines, or triangles.</li>
/// </ul>
///
enum HgiPrimitiveType
{
    HgiPrimitiveTypePointList = 0,
    HgiPrimitiveTypeLineList,
    HgiPrimitiveTypeLineStrip,
    HgiPrimitiveTypeTriangleList,
    HgiPrimitiveTypePatchList,

    HgiPrimitiveTypeCount
};

/// \enum HgiSubmitWaitType
///
/// Describes command submission wait behavior.
///
/// <ul>
/// <li>HgiSubmitWaitTypeNoWait:
///   CPU should not wait for the GPU to finish processing the cmds.</li>
/// <li>HgiSubmitWaitTypeWaitUntilCompleted:
///   The CPU waits ("blocked") until the GPU has consumed the cmds.</li>
/// </ul>
///
enum HgiSubmitWaitType
{
    HgiSubmitWaitTypeNoWait = 0,
    HgiSubmitWaitTypeWaitUntilCompleted,
};

/// \enum HgiMemoryBarrier
///
/// Describes what objects the memory barrier affects.
///
/// <ul>
/// <li>HgiMemoryBarrierNone:
///   No barrier (no-op).</li>
/// <li>HgiMemoryBarrierAll:
///   The barrier affects all memory writes and reads.</li>
/// </ul>
///
enum HgiMemoryBarrierBits
{
    HgiMemoryBarrierNone = 0,
    HgiMemoryBarrierAll  = 1 << 0
};
using HgiMemoryBarrier = HgiBits;

PXR_NAMESPACE_CLOSE_SCOPE

#endif
