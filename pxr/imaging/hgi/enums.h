//
// Copyright 2019 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
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
/// <li>HgiDeviceCapabilitiesBitsBindlessBuffers:
///   THe device can access GPU buffers using bindless handles</li>
/// <li>HgiDeviceCapabilitiesBitsConcurrentDispatch:
///   The device can execute commands concurrently</li>
/// <li>HgiDeviceCapabilitiesBitsUnifiedMemory:
///   The device shares all GPU and CPU memory</li>
/// <li>HgiDeviceCapabilitiesBitsBuiltinBarycentrics:
///   The device can provide built-in barycentric coordinates</li>
/// <li>HgiDeviceCapabilitiesBitsShaderDrawParameters:
///   The device can provide additional built-in shader variables corresponding
///   to draw command parameters</li>
/// <li>HgiDeviceCapabilitiesBitsMultiDrawIndirect:
///   The device supports multiple primitive, indirect drawing</li>
/// <li>HgiDeviceCapabilitiesBitsBindlessTextures:
///   The device can access GPU textures using bindless handles</li>
/// <li>HgiDeviceCapabilitiesBitsShaderDoublePrecision:
///   The device supports double precision types in shaders</li>
/// <li>HgiDeviceCapabilitiesBitsDepthRangeMinusOnetoOne:
///   The device's clip space depth ranges from [-1,1]</li>
/// <li>HgiDeviceCapabilitiesBitsCppShaderPadding:
///   Use CPP padding for shader language structures</li>
/// <li>HgiDeviceCapabilitiesBitsConservativeRaster:
///   The device supports conservative rasterization</li>
/// <li>HgiDeviceCapabilitiesBitsStencilReadback:
///   Supports reading back the stencil buffer from GPU to CPU.</li>
/// <li>HgiDeviceCapabilitiesBitsCustomDepthRange:
///   The device supports setting a custom depth range.</li>
/// <li>HgiDeviceCapabilitiesBitsMetalTessellation:
///   Supports Metal tessellation shaders</li>
/// <li>HgiDeviceCapabilitiesBitsBasePrimitiveOffset:
///   The device requires workaround for base primitive offset</li>
/// <li>HgiDeviceCapabilitiesBitsPrimitiveIdEmulation:
///   The device requires workaround for primitive id</li>
/// <li>HgiDeviceCapabilitiesBitsIndirectCommandBuffers:
///   Indirect command buffers are supported</li>
/// <li>HgiDeviceCapabilitiesBitsRoundPoints:
///   Points can be natively rasterized as disks</li>
/// </ul>
///
enum HgiDeviceCapabilitiesBits : HgiBits
{
    HgiDeviceCapabilitiesBitsPresentation             = 1 << 0,
    HgiDeviceCapabilitiesBitsBindlessBuffers          = 1 << 1,
    HgiDeviceCapabilitiesBitsConcurrentDispatch       = 1 << 2,
    HgiDeviceCapabilitiesBitsUnifiedMemory            = 1 << 3,
    HgiDeviceCapabilitiesBitsBuiltinBarycentrics      = 1 << 4,
    HgiDeviceCapabilitiesBitsShaderDrawParameters     = 1 << 5,
    HgiDeviceCapabilitiesBitsMultiDrawIndirect        = 1 << 6,
    HgiDeviceCapabilitiesBitsBindlessTextures         = 1 << 7,
    HgiDeviceCapabilitiesBitsShaderDoublePrecision    = 1 << 8,
    HgiDeviceCapabilitiesBitsDepthRangeMinusOnetoOne  = 1 << 9,
    HgiDeviceCapabilitiesBitsCppShaderPadding         = 1 << 10,
    HgiDeviceCapabilitiesBitsConservativeRaster       = 1 << 11,
    HgiDeviceCapabilitiesBitsStencilReadback          = 1 << 12,
    HgiDeviceCapabilitiesBitsCustomDepthRange         = 1 << 13,
    HgiDeviceCapabilitiesBitsMetalTessellation        = 1 << 14,
    HgiDeviceCapabilitiesBitsBasePrimitiveOffset      = 1 << 15,
    HgiDeviceCapabilitiesBitsPrimitiveIdEmulation     = 1 << 16,
    HgiDeviceCapabilitiesBitsIndirectCommandBuffers   = 1 << 17,
    HgiDeviceCapabilitiesBitsRoundPoints              = 1 << 18,
    HgiDeviceCapabilitiesBitsSingleSlotResourceArrays = 1 << 19,
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

/// \enum HgiBorderColor
///
/// Border color to use for clamped texture values.
///
/// <ul>
/// <li>HgiBorderColorTransparentBlack</li>
/// <li>HgiBorderColorOpaqueBlack</li>
/// <li>HgiBorderColorOpaqueWhite</li>
/// </ul>
///
enum HgiBorderColor
{
    HgiBorderColorTransparentBlack = 0,
    HgiBorderColorOpaqueBlack      = 1,
    HgiBorderColorOpaqueWhite      = 2,

    HgiBorderColorCount
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
    HgiBufferUsageUniform  = 1 << 0,
    HgiBufferUsageIndex32  = 1 << 1,
    HgiBufferUsageVertex   = 1 << 2,
    HgiBufferUsageStorage  = 1 << 3,
    HgiBufferUsageIndirect = 1 << 4,

    HgiBufferUsageCustomBitsBegin = 1 << 5,
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
///  stage.</li>
/// <li>HgiShaderStageGeometry:
///   Governs the processing of Primitives.</li>
/// <li>HgiShaderStagePostTessellationControl:
///   Metal specific stage which computes tess factors
///   and modifies user post tess vertex data.</li>
/// <li>HgiShaderStagePostTessellationVertex:
///   Metal specific stage which performs tessellation and 
///   vertex processing.</li>
/// </ul>
///
enum HgiShaderStageBits : HgiBits
{
    HgiShaderStageVertex                 = 1 << 0,
    HgiShaderStageFragment               = 1 << 1,
    HgiShaderStageCompute                = 1 << 2,
    HgiShaderStageTessellationControl    = 1 << 3,
    HgiShaderStageTessellationEval       = 1 << 4,
    HgiShaderStageGeometry               = 1 << 5,
    HgiShaderStagePostTessellationControl = 1 << 6,
    HgiShaderStagePostTessellationVertex = 1 << 7,
    HgiShaderStageCustomBitsBegin        = 1 << 8,
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
/// <li>HgiBindResourceTypeTessFactors:
///   Tessellation factors for Metal tessellation.</li>
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
    HgiBindResourceTypeTessFactors,

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

/// \enum HgiColorMaskBits
///
/// Describes whether to permit or restrict writing to color components
/// of a color attachment.
///
enum HgiColorMaskBits : HgiBits
{
    HgiColorMaskRed             = 1 << 0,
    HgiColorMaskGreen           = 1 << 1,
    HgiColorMaskBlue            = 1 << 2,
    HgiColorMaskAlpha           = 1 << 3,
};
using HgiColorMask = HgiBits;

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

/// \enum HgiStencilOp
///
/// Stencil operations.
///
enum HgiStencilOp
{
    HgiStencilOpKeep = 0,
    HgiStencilOpZero,
    HgiStencilOpReplace,
    HgiStencilOpIncrementClamp,
    HgiStencilOpDecrementClamp,
    HgiStencilOpInvert,
    HgiStencilOpIncrementWrap,
    HgiStencilOpDecrementWrap,

    HgiStencilOpCount
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
/// <li>HgiPrimitiveTypeLineListWithAdjacency:
///   A four-vertex encoding used to draw untriangulated quads.
///   Rasterize two triangles for every separate set of four vertices.</li>
/// </ul>
///
enum HgiPrimitiveType
{
    HgiPrimitiveTypePointList = 0,
    HgiPrimitiveTypeLineList,
    HgiPrimitiveTypeLineStrip,
    HgiPrimitiveTypeTriangleList,
    HgiPrimitiveTypePatchList,
    HgiPrimitiveTypeLineListWithAdjacency,

    HgiPrimitiveTypeCount
};

/// \enum HgiVertexBufferStepFunction
///
/// Describes the rate at which vertex attributes are pulled from buffers.
///
/// <ul>
/// <li>HgiVertexBufferStepFunctionConstant:
///   The same attribute data is used for every vertex.</li>
/// <li>HgiVertexBufferStepFunctionPerVertex:
///   New attribute data is fetched for each vertex.</li>
/// <li>HgiVertexBufferStepFunctionPerInstance:
///   New attribute data is fetched for each instance.</li>
/// <li>HgiVertexBufferStepFunctionPerPatch:
///   New attribute data is fetched for each patch.</li>
/// <li>HgiVertexBufferStepFunctionPerPatchControlPoint:
///   New attribute data is fetched for each patch control point.</li>
/// <li>HgiVertexBufferStepFunctionPerDrawCommand:
///   New attribute data is fetched for each draw in a multi-draw command.</li>
/// </ul>
///
enum HgiVertexBufferStepFunction
{
    HgiVertexBufferStepFunctionConstant = 0,
    HgiVertexBufferStepFunctionPerVertex,
    HgiVertexBufferStepFunctionPerInstance,
    HgiVertexBufferStepFunctionPerPatch,
    HgiVertexBufferStepFunctionPerPatchControlPoint,
    HgiVertexBufferStepFunctionPerDrawCommand,

    HgiVertexBufferStepFunctionCount
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

/// \enum HgiBindingType
///
/// Describes the type of shader resource binding model to use.
///
/// <ul>
/// <li>HgiBindingTypeValue:
///   Shader declares binding as a value.
///   Glsl example: buffer { int parameter; };
///   Msl example: int parameter;</li>
/// <li>HgiBindingTypeUniformValue:
///   Shader declares binding as a uniform block value.
///   Glsl example: uniform { int parameter; };
///   Msl example: int parameter;</li>
/// <li>HgiBindingTypeArray:
///   Shader declares binding as array value.
///   Glsl example: buffer { int parameter[n]; };
///   Msl example: int parameter[n];</li>
/// <li>HgiBindingTypeUniformArray:
///   Shader declares binding as uniform block array value.
///   Glsl example: uniform { int parameter[n]; };
///   Msl example: int parameter[n];</li>
/// <li>HgiBindingTypePointer:
///   Shader declares binding as pointer value.
///   Glsl example: buffer { int parameter[] };
///   Msl example: int *parameter;</li>
/// </ul>
///
enum HgiBindingType
{
    HgiBindingTypeValue = 0,
    HgiBindingTypeUniformValue,
    HgiBindingTypeArray,
    HgiBindingTypeUniformArray,
    HgiBindingTypePointer,
};

/// \enum HgiInterpolationType
///
/// Describes the type of parameter interpolation.
///
/// <ul>
/// <li>HgiInterpolationDefault:
///   The shader input will have default interpolation.
///   Glsl example: vec2 parameter;
///   Msl example: vec2 parameter;</li>
/// <li>HgiInterpolationFlat:
///   The shader input will have no interpolation.
///   Glsl example: flat vec2 parameter;
///   Msl example: vec2 parameter[[flat]];</li>
/// <li>HgiBindingTypeNoPerspective:
///   The shader input will be linearly interpolated in screen-space
///   Glsl example: noperspective vec2 parameter;
///   Msl example: vec2 parameter[[center_no_perspective]];</li>
/// </ul>
///
enum HgiInterpolationType
{
    HgiInterpolationDefault = 0,
    HgiInterpolationFlat,
    HgiInterpolationNoPerspective,
};

/// \enum HgiSamplingType
///
/// Describes the type of parameter sampling.
///
/// <ul>
/// <li>HgiSamplingDefault:
///   The shader input will have default sampling.
///   Glsl example: vec2 parameter;
///   Msl example: vec2 parameter;</li>
/// <li>HgiSamplingCentroid:
///   The shader input will have centroid sampling.
///   Glsl example: centroid vec2 parameter;
///   Msl example: vec2 parameter[[centroid_perspective]];</li>
/// <li>HgiSamplingSample:
///   The shader input will have per-sample sampling.
///   Glsl example: sample vec2 parameter;
///   Msl example: vec2 parameter[[sample_perspective]];</li>
/// </ul>
///
enum HgiSamplingType
{
    HgiSamplingDefault = 0,
    HgiSamplingCentroid,
    HgiSamplingSample,
};

/// \enum HgiStorageType
///
/// Describes the type of parameter storage.
///
/// <ul>
/// <li>HgiStorageDefault:
///   The shader input will have default storage.
///   Glsl example: vec2 parameter;</li>
/// <li>HgiStoragePatch:
///   The shader input will have per-patch storage.
///   Glsl example: patch vec2 parameter;</li>
/// </ul>
///
enum HgiStorageType
{
    HgiStorageDefault = 0,
    HgiStoragePatch,
};

/// \enum HgiShaderTextureType
///
/// Describes the type of texture to be used in shader gen.
///
/// <ul>
/// <li>HgiShaderTextureTypeTexture:
///   Indicates a regular texture.</li>
/// <li>HgiShaderTextureTypeShadowTexture:
///   Indicates a shadow texture.</li>
/// <li>HgiShaderTextureTypeArrayTexture:
///   Indicates an array texture.</li>
/// </ul>
///
enum HgiShaderTextureType
{
    HgiShaderTextureTypeTexture = 0,
    HgiShaderTextureTypeShadowTexture,
    HgiShaderTextureTypeArrayTexture
};

/// \enum HgiComputeDispatch
///
/// Specifies the dispatch method for compute encoders.
///
/// <ul>
/// <li>HgiComputeDispatchSerial:
///   Kernels are dispatched serially.</li>
/// <li>HgiComputeDispatchConcurrent:
///   Kernels are dispatched concurrently, if supported by the API</li>
/// </ul>
///
enum HgiComputeDispatch
{
    HgiComputeDispatchSerial = 0,
    HgiComputeDispatchConcurrent
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif
