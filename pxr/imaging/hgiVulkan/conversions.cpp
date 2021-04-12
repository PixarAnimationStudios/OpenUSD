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
#include "pxr/imaging/hgiVulkan/vulkan.h"
#include "pxr/imaging/hgiVulkan/conversions.h"

#include "pxr/base/tf/diagnostic.h"
#include "pxr/base/tf/iterator.h"
#include "pxr/imaging/hgi/enums.h"
#include "pxr/imaging/hgi/types.h"

PXR_NAMESPACE_OPEN_SCOPE


static const uint32_t
_LoadOpTable[HgiAttachmentLoadOpCount][2] =
{
    {HgiAttachmentLoadOpDontCare, VK_ATTACHMENT_LOAD_OP_DONT_CARE},
    {HgiAttachmentLoadOpClear,    VK_ATTACHMENT_LOAD_OP_CLEAR},
    {HgiAttachmentLoadOpLoad,     VK_ATTACHMENT_LOAD_OP_LOAD}
};
static_assert(HgiAttachmentLoadOpCount==3, "");

static const uint32_t
_StoreOpTable[HgiAttachmentStoreOpCount][2] =
{
    {HgiAttachmentStoreOpDontCare, VK_ATTACHMENT_STORE_OP_DONT_CARE},
    {HgiAttachmentStoreOpStore,    VK_ATTACHMENT_STORE_OP_STORE}
};
static_assert(HgiAttachmentStoreOpCount==2, "");

static const uint32_t
_FormatTable[HgiFormatCount][2] =
{
    // HGI FORMAT              VK FORMAT
    {HgiFormatUNorm8,         VK_FORMAT_R8_UNORM},
    {HgiFormatUNorm8Vec2,     VK_FORMAT_R8G8_UNORM},
    // HgiFormatUNorm8Vec3, VK_FORMAT_R8G8B8_UNORM // not supported by HgiFormat
    {HgiFormatUNorm8Vec4,     VK_FORMAT_R8G8B8A8_UNORM},
    {HgiFormatSNorm8,         VK_FORMAT_R8_SNORM},
    {HgiFormatSNorm8Vec2,     VK_FORMAT_R8G8_SNORM},
    // HgiFormatSNorm8Vec3, VK_FORMAT_R8G8B8_SNORM // not supported by HgiFormat
    {HgiFormatSNorm8Vec4,     VK_FORMAT_R8G8B8A8_SNORM},
    {HgiFormatFloat16,        VK_FORMAT_R16_SFLOAT},
    {HgiFormatFloat16Vec2,    VK_FORMAT_R16G16_SFLOAT},
    {HgiFormatFloat16Vec3,    VK_FORMAT_R16G16B16_SFLOAT},
    {HgiFormatFloat16Vec4,    VK_FORMAT_R16G16B16A16_SFLOAT},
    {HgiFormatFloat32,        VK_FORMAT_R32_SFLOAT},
    {HgiFormatFloat32Vec2,    VK_FORMAT_R32G32_SFLOAT},
    {HgiFormatFloat32Vec3,    VK_FORMAT_R32G32B32_SFLOAT},
    {HgiFormatFloat32Vec4,    VK_FORMAT_R32G32B32A32_SFLOAT},
    {HgiFormatUInt16,         VK_FORMAT_R16_UINT},
    {HgiFormatUInt16Vec2,     VK_FORMAT_R16G16_UINT},
    {HgiFormatUInt16Vec3,     VK_FORMAT_R16G16B16_UINT},
    {HgiFormatUInt16Vec4,     VK_FORMAT_R16G16B16A16_UINT},
    {HgiFormatInt32,          VK_FORMAT_R32_SINT},
    {HgiFormatInt32Vec2,      VK_FORMAT_R32G32_SINT},
    {HgiFormatInt32Vec3,      VK_FORMAT_R32G32B32_SINT},
    {HgiFormatInt32Vec4,      VK_FORMAT_R32G32B32A32_SINT},
    {HgiFormatUNorm8Vec4srgb, VK_FORMAT_R8G8B8_SRGB},
    {HgiFormatBC6FloatVec3,   VK_FORMAT_BC6H_SFLOAT_BLOCK},
    {HgiFormatBC6UFloatVec3,  VK_FORMAT_BC6H_UFLOAT_BLOCK},
    {HgiFormatBC7UNorm8Vec4,  VK_FORMAT_BC7_UNORM_BLOCK},
    {HgiFormatBC7UNorm8Vec4srgb, VK_FORMAT_BC7_SRGB_BLOCK},
    {HgiFormatBC1UNorm8Vec4,  VK_FORMAT_BC1_RGBA_UNORM_BLOCK},
    {HgiFormatBC3UNorm8Vec4,  VK_FORMAT_BC3_UNORM_BLOCK},
    {HgiFormatFloat32UInt8,   VK_FORMAT_D32_SFLOAT_S8_UINT}
};

// A few random format validations to make sure the table above stays in sync
// with the HgiFormat table.
constexpr bool _CompileTimeValidateHgiFormatTable() {
    return (HgiFormatCount==30 &&
            HgiFormatUNorm8 == 0 &&
            HgiFormatFloat16Vec4 == 9 &&
            HgiFormatFloat32Vec4 == 13 &&
            HgiFormatUInt16Vec4 == 17 &&
            HgiFormatUNorm8Vec4srgb == 22 &&
            HgiFormatBC3UNorm8Vec4 == 28) ? true : false;
}

static_assert(_CompileTimeValidateHgiFormatTable(), 
              "_FormatDesc array out of sync with HgiFormat enum");


static const uint32_t
_SampleCountTable[][2] =
{
    {HgiSampleCount1,  VK_SAMPLE_COUNT_1_BIT},
    {HgiSampleCount2,  VK_SAMPLE_COUNT_2_BIT},
    {HgiSampleCount4,  VK_SAMPLE_COUNT_4_BIT},
    {HgiSampleCount8,  VK_SAMPLE_COUNT_8_BIT},
    {HgiSampleCount16, VK_SAMPLE_COUNT_16_BIT}
};
static_assert(HgiSampleCountEnd==17, "");

static const uint32_t
_ShaderStageTable[][2] =
{
    {HgiShaderStageVertex,              VK_SHADER_STAGE_VERTEX_BIT},
    {HgiShaderStageFragment,            VK_SHADER_STAGE_FRAGMENT_BIT},
    {HgiShaderStageCompute,             VK_SHADER_STAGE_COMPUTE_BIT},
    {HgiShaderStageTessellationControl, VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT},
    {HgiShaderStageTessellationEval,    VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT},
    {HgiShaderStageGeometry,            VK_SHADER_STAGE_GEOMETRY_BIT},
};
static_assert(HgiShaderStageCustomBitsBegin == 1 << 6, "");

static const uint32_t
_TextureUsageTable[][2] =
{
    {HgiTextureUsageBitsColorTarget,   VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT},
    {HgiTextureUsageBitsDepthTarget,   VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT},
    {HgiTextureUsageBitsStencilTarget, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT},
    {HgiTextureUsageBitsShaderRead,    VK_IMAGE_USAGE_SAMPLED_BIT},
    {HgiTextureUsageBitsShaderWrite,   VK_IMAGE_USAGE_STORAGE_BIT}
};
static_assert(HgiTextureUsageCustomBitsBegin == 1 << 5, "");

static const uint32_t
_FormatFeatureTable[][2] =
{
    {HgiTextureUsageBitsColorTarget,   VK_FORMAT_FEATURE_COLOR_ATTACHMENT_BIT},
    {HgiTextureUsageBitsDepthTarget,   VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT},
    {HgiTextureUsageBitsStencilTarget, VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT},
    {HgiTextureUsageBitsShaderRead,    VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT},
    {HgiTextureUsageBitsShaderWrite,   VK_FORMAT_FEATURE_STORAGE_IMAGE_BIT},
};
static_assert(HgiTextureUsageCustomBitsBegin == 1 << 5, "");

static const uint32_t
_BufferUsageTable[][2] =
{
    {HgiBufferUsageUniform, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT},
    {HgiBufferUsageIndex32, VK_BUFFER_USAGE_INDEX_BUFFER_BIT},
    {HgiBufferUsageVertex,  VK_BUFFER_USAGE_VERTEX_BUFFER_BIT},
    {HgiBufferUsageStorage, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT},
};
static_assert(HgiBufferUsageCustomBitsBegin == 1 << 4, "");

static const uint32_t
_CullModeTable[HgiCullModeCount][2] =
{
    {HgiCullModeNone,         VK_CULL_MODE_NONE},
    {HgiCullModeFront,        VK_CULL_MODE_FRONT_BIT},
    {HgiCullModeBack,         VK_CULL_MODE_BACK_BIT},
    {HgiCullModeFrontAndBack, VK_CULL_MODE_FRONT_AND_BACK}
};
static_assert(HgiCullModeCount==4, "");

static const uint32_t
_PolygonModeTable[HgiPolygonModeCount][2] =
{
    {HgiPolygonModeFill,  VK_POLYGON_MODE_FILL},
    {HgiPolygonModeLine,  VK_POLYGON_MODE_LINE},
    {HgiPolygonModePoint, VK_POLYGON_MODE_POINT}
};
static_assert(HgiPolygonModeCount==3, "");

static const uint32_t
_WindingTable[HgiWindingCount][2] =
{
    {HgiWindingClockwise,        VK_FRONT_FACE_CLOCKWISE},
    {HgiWindingCounterClockwise, VK_FRONT_FACE_COUNTER_CLOCKWISE}
};
static_assert(HgiWindingCount==2, "");

static const uint32_t
_BindResourceTypeTable[HgiBindResourceTypeCount][2] =
{
    {HgiBindResourceTypeSampler,              VK_DESCRIPTOR_TYPE_SAMPLER},
    {HgiBindResourceTypeSampledImage,         VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE},
    {HgiBindResourceTypeCombinedSamplerImage, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER},
    {HgiBindResourceTypeStorageImage,         VK_DESCRIPTOR_TYPE_STORAGE_IMAGE},
    {HgiBindResourceTypeUniformBuffer,        VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER},
    {HgiBindResourceTypeStorageBuffer,        VK_DESCRIPTOR_TYPE_STORAGE_BUFFER}
};
static_assert(HgiBindResourceTypeCount==6, "");

static const uint32_t
_blendEquationTable[HgiBlendOpCount][2] =
{
    {HgiBlendOpAdd,             VK_BLEND_OP_ADD},
    {HgiBlendOpSubtract,        VK_BLEND_OP_SUBTRACT},
    {HgiBlendOpReverseSubtract, VK_BLEND_OP_REVERSE_SUBTRACT},
    {HgiBlendOpMin,             VK_BLEND_OP_MIN},
    {HgiBlendOpMax,             VK_BLEND_OP_MAX},
};
static_assert(HgiBlendOpCount==5, "");

static const uint32_t
_blendFactorTable[HgiBlendFactorCount][2] =
{
    {HgiBlendFactorZero,                  VK_BLEND_FACTOR_ZERO},
    {HgiBlendFactorOne,                   VK_BLEND_FACTOR_ONE},
    {HgiBlendFactorSrcColor,              VK_BLEND_FACTOR_SRC_COLOR},
    {HgiBlendFactorOneMinusSrcColor,      VK_BLEND_FACTOR_ONE_MINUS_SRC_COLOR},
    {HgiBlendFactorDstColor,              VK_BLEND_FACTOR_DST_COLOR},
    {HgiBlendFactorOneMinusDstColor,      VK_BLEND_FACTOR_ONE_MINUS_DST_COLOR},
    {HgiBlendFactorSrcAlpha,              VK_BLEND_FACTOR_SRC_ALPHA},
    {HgiBlendFactorOneMinusSrcAlpha,      VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA},
    {HgiBlendFactorDstAlpha,              VK_BLEND_FACTOR_DST_ALPHA},
    {HgiBlendFactorOneMinusDstAlpha,      VK_BLEND_FACTOR_ONE_MINUS_DST_ALPHA},
    {HgiBlendFactorConstantColor,         VK_BLEND_FACTOR_CONSTANT_COLOR},
    {HgiBlendFactorOneMinusConstantColor, VK_BLEND_FACTOR_ONE_MINUS_CONSTANT_COLOR},
    {HgiBlendFactorConstantAlpha,         VK_BLEND_FACTOR_CONSTANT_ALPHA},
    {HgiBlendFactorOneMinusConstantAlpha, VK_BLEND_FACTOR_ONE_MINUS_CONSTANT_ALPHA},
    {HgiBlendFactorSrcAlphaSaturate,      VK_BLEND_FACTOR_SRC_ALPHA_SATURATE},
    {HgiBlendFactorSrc1Color,             VK_BLEND_FACTOR_SRC1_COLOR},
    {HgiBlendFactorOneMinusSrc1Color,     VK_BLEND_FACTOR_ONE_MINUS_SRC1_COLOR},
    {HgiBlendFactorSrc1Alpha,             VK_BLEND_FACTOR_SRC1_ALPHA},
    {HgiBlendFactorOneMinusSrc1Alpha,     VK_BLEND_FACTOR_ONE_MINUS_SRC1_ALPHA},
};
static_assert(HgiBlendFactorCount==19, "");

static const uint32_t
_CompareOpTable[HgiCompareFunctionCount][2] =
{
    {HgiCompareFunctionNever,    VK_COMPARE_OP_NEVER},
    {HgiCompareFunctionLess,     VK_COMPARE_OP_LESS},
    {HgiCompareFunctionEqual,    VK_COMPARE_OP_EQUAL},
    {HgiCompareFunctionLEqual,   VK_COMPARE_OP_LESS_OR_EQUAL},
    {HgiCompareFunctionGreater,  VK_COMPARE_OP_GREATER},
    {HgiCompareFunctionNotEqual, VK_COMPARE_OP_NOT_EQUAL},
    {HgiCompareFunctionGEqual,   VK_COMPARE_OP_GREATER_OR_EQUAL},
    {HgiCompareFunctionAlways,   VK_COMPARE_OP_ALWAYS}
};
static_assert(HgiCompareFunctionCount==8, "");

static const uint32_t
_textureTypeTable[HgiTextureTypeCount][2] =
{
    {HgiTextureType1D,      VK_IMAGE_TYPE_1D},
    {HgiTextureType2D,      VK_IMAGE_TYPE_2D},
    {HgiTextureType3D,      VK_IMAGE_TYPE_3D},
    {HgiTextureType1DArray, VK_IMAGE_TYPE_2D},
    {HgiTextureType2DArray, VK_IMAGE_TYPE_2D}
};
static_assert(HgiTextureTypeCount==5, "");

static const uint32_t
_textureViewTypeTable[HgiTextureTypeCount][2] =
{
    {HgiTextureType1D,      VK_IMAGE_VIEW_TYPE_1D},
    {HgiTextureType2D,      VK_IMAGE_VIEW_TYPE_2D},
    {HgiTextureType3D,      VK_IMAGE_VIEW_TYPE_3D},
    {HgiTextureType1DArray, VK_IMAGE_VIEW_TYPE_1D_ARRAY},
    {HgiTextureType2DArray, VK_IMAGE_VIEW_TYPE_2D_ARRAY}
};
static_assert(HgiTextureTypeCount==5, "");

static const uint32_t
_samplerAddressModeTable[HgiSamplerAddressModeCount][2] =
{
    {HgiSamplerAddressModeClampToEdge,        VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE},
    {HgiSamplerAddressModeMirrorClampToEdge,  VK_SAMPLER_ADDRESS_MODE_MIRROR_CLAMP_TO_EDGE},
    {HgiSamplerAddressModeRepeat,             VK_SAMPLER_ADDRESS_MODE_REPEAT},
    {HgiSamplerAddressModeMirrorRepeat,       VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT},
    {HgiSamplerAddressModeClampToBorderColor, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER}
};
static_assert(HgiSamplerAddressModeCount==5, "");

static const uint32_t
_samplerFilterTable[HgiSamplerFilterCount][2] =
{
    {HgiSamplerFilterNearest, VK_FILTER_NEAREST},
    {HgiSamplerFilterLinear,  VK_FILTER_LINEAR}
};
static_assert(HgiSamplerFilterCount==2, "");

static const uint32_t
_mipFilterTable[HgiMipFilterCount][2] =
{
    {HgiMipFilterNotMipmapped, VK_SAMPLER_MIPMAP_MODE_NEAREST /*unused*/},
    {HgiMipFilterNearest,      VK_SAMPLER_MIPMAP_MODE_NEAREST},
    {HgiMipFilterLinear,       VK_SAMPLER_MIPMAP_MODE_LINEAR}
};
static_assert(HgiMipFilterCount==3, "");

static const uint32_t
_componentSwizzleTable[HgiComponentSwizzleCount][2] =
{
    {HgiComponentSwizzleZero, VK_COMPONENT_SWIZZLE_ZERO},
    {HgiComponentSwizzleOne,  VK_COMPONENT_SWIZZLE_ONE},
    {HgiComponentSwizzleR,    VK_COMPONENT_SWIZZLE_R},
    {HgiComponentSwizzleG,    VK_COMPONENT_SWIZZLE_G},
    {HgiComponentSwizzleB,    VK_COMPONENT_SWIZZLE_B},
    {HgiComponentSwizzleA,    VK_COMPONENT_SWIZZLE_A}
};
static_assert(HgiComponentSwizzleCount==6, "");

static const uint32_t
_primitiveTypeTable[HgiPrimitiveTypeCount][2] =
{
    {HgiPrimitiveTypePointList,    VK_PRIMITIVE_TOPOLOGY_POINT_LIST},
    {HgiPrimitiveTypeLineList,     VK_PRIMITIVE_TOPOLOGY_LINE_LIST},
    {HgiPrimitiveTypeLineStrip,    VK_PRIMITIVE_TOPOLOGY_LINE_STRIP},
    {HgiPrimitiveTypeTriangleList, VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST},
    {HgiPrimitiveTypePatchList,    VK_PRIMITIVE_TOPOLOGY_PATCH_LIST}
};
static_assert(HgiPrimitiveTypeCount==5, "");

VkFormat
HgiVulkanConversions::GetFormat(HgiFormat inFormat)
{
    if (!TF_VERIFY(inFormat!=HgiFormatInvalid)) {
        return VK_FORMAT_UNDEFINED;
    }
    return VkFormat(_FormatTable[inFormat][1]);
}

HgiFormat
HgiVulkanConversions::GetFormat(VkFormat inFormat)
{
    if (!TF_VERIFY(inFormat!=VK_FORMAT_UNDEFINED)) {
        return HgiFormatInvalid;
    }

    // While HdFormat/HgiFormat do not support BGRA channel ordering it may
    // be used for the native window swapchain on some platforms.
    if (inFormat == VK_FORMAT_B8G8R8A8_UNORM) {
        return HgiFormatUNorm8Vec4;
    }

    for (auto const& f : _FormatTable) {
        if (f[1] == inFormat) return HgiFormat(f[0]);
    }

    TF_CODING_ERROR("Missing format table entry");
    return HgiFormatInvalid;
}

VkImageAspectFlags
HgiVulkanConversions::GetImageAspectFlag(HgiTextureUsage usage)
{
    if (usage & HgiTextureUsageBitsDepthTarget) {
        if (usage & HgiTextureUsageBitsStencilTarget) {
            return VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;
        }
        return VK_IMAGE_ASPECT_DEPTH_BIT;
    }

    return VK_IMAGE_ASPECT_COLOR_BIT;
}

VkImageUsageFlags
HgiVulkanConversions::GetTextureUsage(HgiTextureUsage tu)
{
    VkImageUsageFlags vkFlags = 0;
    for (const auto& f : _TextureUsageTable) {
        if (tu & f[0]) vkFlags |= f[1];
    }

    if (vkFlags==0) {
        TF_CODING_ERROR("Missing texture usage table entry");
    }
    return vkFlags;
}

VkFormatFeatureFlags
HgiVulkanConversions::GetFormatFeature(HgiTextureUsage tu)
{
    VkFormatFeatureFlags vkFlags = 0;
    for (const auto& f : _FormatFeatureTable) {
        if (tu & f[0]) vkFlags |= f[1];
    }

    if (vkFlags==0) {
        TF_CODING_ERROR("Missing texture usage table entry");
    }
    return vkFlags;
}

VkAttachmentLoadOp
HgiVulkanConversions::GetLoadOp(HgiAttachmentLoadOp op)
{
    return VkAttachmentLoadOp(_LoadOpTable[op][1]);
}

VkAttachmentStoreOp
HgiVulkanConversions::GetStoreOp(HgiAttachmentStoreOp op)
{
    return VkAttachmentStoreOp(_StoreOpTable[op][1]);
}

VkSampleCountFlagBits
HgiVulkanConversions::GetSampleCount(HgiSampleCount sc)
{
    for (auto const& s : _SampleCountTable) {
        if (s[0] == sc) return VkSampleCountFlagBits(s[1]);
    }

    TF_CODING_ERROR("Missing Sample table entry");
    return VK_SAMPLE_COUNT_1_BIT;
}

VkShaderStageFlags
HgiVulkanConversions::GetShaderStages(HgiShaderStage ss)
{
    VkShaderStageFlags vkFlags = 0;
    for (const auto& f : _ShaderStageTable) {
        if (ss & f[0]) vkFlags |= f[1];
    }

    if (vkFlags==0) {
        TF_CODING_ERROR("Missing shader stage table entry");
    }
    return vkFlags;
}

VkBufferUsageFlags
HgiVulkanConversions::GetBufferUsage(HgiBufferUsage bu)
{
    VkBufferUsageFlags vkFlags = 0;
    for (const auto& f : _BufferUsageTable) {
        if (bu & f[0]) vkFlags |= f[1];
    }

    if (vkFlags==0) {
        TF_CODING_ERROR("Missing buffer usage table entry");
    }
    return vkFlags;
}

VkCullModeFlags
HgiVulkanConversions::GetCullMode(HgiCullMode cm)
{
    return VkCullModeFlags(_CullModeTable[cm][1]);
}

VkPolygonMode
HgiVulkanConversions::GetPolygonMode(HgiPolygonMode pm)
{
    return VkPolygonMode(_PolygonModeTable[pm][1]);
}

VkFrontFace
HgiVulkanConversions::GetWinding(HgiWinding wd)
{
    return VkFrontFace(_WindingTable[wd][1]);
}

VkDescriptorType
HgiVulkanConversions::GetDescriptorType(HgiBindResourceType rt)
{
    return VkDescriptorType(_BindResourceTypeTable[rt][1]);
}

VkBlendFactor
HgiVulkanConversions::GetBlendFactor(HgiBlendFactor bf)
{
    return VkBlendFactor(_blendFactorTable[bf][1]);
}

VkBlendOp
HgiVulkanConversions::GetBlendEquation(HgiBlendOp bo)
{
    return VkBlendOp(_blendEquationTable[bo][1]);
}

VkCompareOp
HgiVulkanConversions::GetDepthCompareFunction(HgiCompareFunction cf)
{
    return VkCompareOp(_CompareOpTable[cf][1]);
}

VkImageType
HgiVulkanConversions::GetTextureType(HgiTextureType tt)
{
    return VkImageType(_textureTypeTable[tt][1]);
}

VkImageViewType
HgiVulkanConversions::GetTextureViewType(HgiTextureType tt)
{
    return VkImageViewType(_textureViewTypeTable[tt][1]);
}

VkSamplerAddressMode
HgiVulkanConversions::GetSamplerAddressMode(HgiSamplerAddressMode a)
{
    return VkSamplerAddressMode(_samplerAddressModeTable[a][1]);
}

VkFilter
HgiVulkanConversions::GetMinMagFilter(HgiSamplerFilter mf)
{
    return VkFilter(_samplerFilterTable[mf][1]);
}

VkSamplerMipmapMode
HgiVulkanConversions::GetMipFilter(HgiMipFilter mf)
{
    return VkSamplerMipmapMode(_mipFilterTable[mf][1]);
}

VkComponentSwizzle
HgiVulkanConversions::GetComponentSwizzle(HgiComponentSwizzle cs)
{
    return VkComponentSwizzle(_componentSwizzleTable[cs][1]);
}

VkPrimitiveTopology
HgiVulkanConversions::GetPrimitiveType(HgiPrimitiveType pt)
{
    return VkPrimitiveTopology(_primitiveTypeTable[pt][1]);
}

PXR_NAMESPACE_CLOSE_SCOPE
