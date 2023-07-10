//
// Copyright 2022 Pixar
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
#include "pxr/imaging/hgiWebGPU/conversions.h"
#include "pxr/base/tf/iterator.h"
#include "pxr/imaging/hgiWebGPU/api.h"

PXR_NAMESPACE_OPEN_SCOPE

//
// HgiFormat
//
static const wgpu::TextureFormat _PIXEL_FORMAT_DESC[] =
{
    wgpu::TextureFormat::R8Unorm,          // HgiFormatUNorm8,
    wgpu::TextureFormat::RG8Unorm,         // HgiFormatUNorm8Vec2,
    wgpu::TextureFormat::RGBA8Unorm,       // HgiFormatUNorm8Vec4,

    wgpu::TextureFormat::R8Snorm,          // HgiFormatSNorm8,
    wgpu::TextureFormat::RG8Snorm,         // HgiFormatSNorm8Vec2,
    wgpu::TextureFormat::RGBA8Snorm,       // HgiFormatSNorm8Vec4,

    wgpu::TextureFormat::R16Float,         // HgiFormatFloat16,
    wgpu::TextureFormat::RG16Float,        // HgiFormatFloat16Vec2,
    wgpu::TextureFormat::Undefined,        // Unsupported by WebGPU HgiFormatFloat16Vec3
    wgpu::TextureFormat::RGBA16Float,      // HgiFormatFloat16Vec4,

    wgpu::TextureFormat::R32Float,         // HgiFormatFloat32,
    wgpu::TextureFormat::RG32Float,        // HgiFormatFloat32Vec2,
    wgpu::TextureFormat::Undefined,        // Unsupported by WebGPU HgiFormatFloat32Vec3
    wgpu::TextureFormat::RGBA32Float,      // HgiFormatFloat32Vec4,

    wgpu::TextureFormat::R16Sint,          // HgiFormatInt16,
    wgpu::TextureFormat::RG16Sint,         // HgiFormatInt16Vec2,
    wgpu::TextureFormat::Undefined,        // Unsupported by WebGPU HgiFormatInt16Vec3
    wgpu::TextureFormat::RGBA16Sint,       // HgiFormatInt16Vec4,

    wgpu::TextureFormat::R16Uint,          // HgiFormatUInt16,
    wgpu::TextureFormat::RG16Uint,         // HgiFormatUInt16Vec2,
    wgpu::TextureFormat::Undefined,        // Unsupported by WebGPU HgiFormatUInt16Vec3
    wgpu::TextureFormat::RGBA16Uint,       // HgiFormatUInt16Vec4,

    wgpu::TextureFormat::R32Sint,          // HgiFormatInt32,
    wgpu::TextureFormat::RG32Sint,         // HgiFormatInt32Vec2,
    wgpu::TextureFormat::Undefined,        // Unsupported by WebGPU HgiFormatInt32Vec3
    wgpu::TextureFormat::RGBA32Sint,       // HgiFormatInt32Vec4,
    
    wgpu::TextureFormat::RGBA8UnormSrgb,   // HgiFormatUNorm8Vec4srgb,

    wgpu::TextureFormat::BC6HRGBFloat,     // HgiFormatBC6FloatVec3
    wgpu::TextureFormat::BC6HRGBUfloat,    // HgiFormatBC6UFloatVec3
    wgpu::TextureFormat::BC7RGBAUnorm,     // HgiFormatBC7UNorm8Vec4
    wgpu::TextureFormat::BC7RGBAUnormSrgb, // HgiFormatBC7UNorm8Vec4srgb
    wgpu::TextureFormat::BC1RGBAUnorm,     // HgiFormatBC1UNorm8Vec4
    wgpu::TextureFormat::BC3RGBAUnorm,     // HgiFormatBC3UNorm8Vec4

    wgpu::TextureFormat::Depth32FloatStencil8, // HgiFormatFloat32UInt8
    wgpu::TextureFormat::RGB10A2Unorm,// HgiFormatPackedInt1010102
};

// A few random format validations to make sure out table stays aligned with the HgiFormat table
constexpr bool _CompileTimeValidateHgiFormatTable() {
    return (TfArraySize(_PIXEL_FORMAT_DESC) == HgiFormatCount &&
            HgiFormatUNorm8 == 0 &&
            HgiFormatFloat16Vec4 == 9 &&
            HgiFormatFloat32Vec4 == 13 &&
            HgiFormatUInt16Vec4 == 21 &&
            HgiFormatUNorm8Vec4srgb == 26 &&
            HgiFormatBC3UNorm8Vec4 == 32) ? true : false;
}

static_assert(_CompileTimeValidateHgiFormatTable(),
              "_PIXEL_FORMAT_DESC array out of sync with HgiFormat enum");

//
// wgpu::VertexFormat
//
struct {
    HgiFormat hgiFormat;
    wgpu::VertexFormat webGPUVertexFormat;
} static const _formatTable[] =
    {
            {HgiFormatUNorm8,           wgpu::VertexFormat::Undefined},
            {HgiFormatUNorm8Vec2,       wgpu::VertexFormat::Unorm8x2},
            {HgiFormatUNorm8Vec4,       wgpu::VertexFormat::Unorm8x4},

            {HgiFormatSNorm8,           wgpu::VertexFormat::Undefined},
            {HgiFormatSNorm8Vec2,       wgpu::VertexFormat::Snorm8x2},
            {HgiFormatSNorm8Vec4,       wgpu::VertexFormat::Snorm8x4},

            {HgiFormatFloat16,          wgpu::VertexFormat::Undefined},
            {HgiFormatFloat16Vec2,      wgpu::VertexFormat::Float16x2},
            {HgiFormatFloat16Vec3,      wgpu::VertexFormat::Undefined},
            {HgiFormatFloat16Vec4,      wgpu::VertexFormat::Float16x4},

            {HgiFormatFloat32,          wgpu::VertexFormat::Float32},
            {HgiFormatFloat32Vec2,      wgpu::VertexFormat::Float32x2},
            {HgiFormatFloat32Vec3,      wgpu::VertexFormat::Float32x3},
            {HgiFormatFloat32Vec4,      wgpu::VertexFormat::Float32x4},

            {HgiFormatInt16,            wgpu::VertexFormat::Undefined},
            {HgiFormatInt16Vec2,        wgpu::VertexFormat::Undefined},
            {HgiFormatInt16Vec3,        wgpu::VertexFormat::Undefined},
            {HgiFormatInt16Vec4,        wgpu::VertexFormat::Undefined},

            {HgiFormatUInt16,           wgpu::VertexFormat::Undefined},
            {HgiFormatUInt16Vec2,       wgpu::VertexFormat::Undefined},
            {HgiFormatUInt16Vec3,       wgpu::VertexFormat::Undefined},
            {HgiFormatUInt16Vec4,       wgpu::VertexFormat::Undefined},

            {HgiFormatInt32,            wgpu::VertexFormat::Sint32},
            {HgiFormatInt32Vec2,        wgpu::VertexFormat::Sint32x2},
            {HgiFormatInt32Vec3,        wgpu::VertexFormat::Sint32x3},
            {HgiFormatInt32Vec4,        wgpu::VertexFormat::Sint32x4},

            {HgiFormatUNorm8Vec4srgb,   wgpu::VertexFormat::Undefined},

            {HgiFormatBC6FloatVec3,     wgpu::VertexFormat::Undefined},
            {HgiFormatBC6UFloatVec3,    wgpu::VertexFormat::Undefined},
            {HgiFormatBC7UNorm8Vec4,    wgpu::VertexFormat::Undefined},
            {HgiFormatBC7UNorm8Vec4srgb,wgpu::VertexFormat::Undefined},
            {HgiFormatBC1UNorm8Vec4,    wgpu::VertexFormat::Undefined},
            {HgiFormatBC3UNorm8Vec4,    wgpu::VertexFormat::Undefined},

            {HgiFormatFloat32UInt8,     wgpu::VertexFormat::Undefined},

            {HgiFormatPackedInt1010102, wgpu::VertexFormat::Undefined},
    };

static_assert(TfArraySize(_formatTable) == HgiFormatCount,
              "_formatTable array out of sync with HgiFormat enum");

//
// wgpu::VertexFormat
//
struct {
    HgiFormat hgiFormat;
    wgpu::TextureSampleType webGPUTextureSampleType;
} static const _textureSampleTypeTable[] =
    {
            {HgiFormatUNorm8,           wgpu::TextureSampleType::Uint},
            {HgiFormatUNorm8Vec2,       wgpu::TextureSampleType::Uint},
            {HgiFormatUNorm8Vec4,       wgpu::TextureSampleType::Uint},

            {HgiFormatSNorm8,           wgpu::TextureSampleType::Sint},
            {HgiFormatSNorm8Vec2,       wgpu::TextureSampleType::Sint},
            {HgiFormatSNorm8Vec4,       wgpu::TextureSampleType::Sint},

            {HgiFormatFloat16,          wgpu::TextureSampleType::Float},
            {HgiFormatFloat16Vec2,      wgpu::TextureSampleType::Float},
            {HgiFormatFloat16Vec3,      wgpu::TextureSampleType::Float},
            {HgiFormatFloat16Vec4,      wgpu::TextureSampleType::Float},

            {HgiFormatFloat32,          wgpu::TextureSampleType::Float},
            {HgiFormatFloat32Vec2,      wgpu::TextureSampleType::Float},
            {HgiFormatFloat32Vec3,      wgpu::TextureSampleType::Float},
            {HgiFormatFloat32Vec4,      wgpu::TextureSampleType::Float},

            {HgiFormatInt16,            wgpu::TextureSampleType::Sint},
            {HgiFormatInt16Vec2,        wgpu::TextureSampleType::Sint},
            {HgiFormatInt16Vec3,        wgpu::TextureSampleType::Sint},
            {HgiFormatInt16Vec4,        wgpu::TextureSampleType::Sint},

            {HgiFormatUInt16,           wgpu::TextureSampleType::Uint},
            {HgiFormatUInt16Vec2,       wgpu::TextureSampleType::Uint},
            {HgiFormatUInt16Vec3,       wgpu::TextureSampleType::Uint},
            {HgiFormatUInt16Vec4,       wgpu::TextureSampleType::Uint},

            {HgiFormatInt32,            wgpu::TextureSampleType::Sint},
            {HgiFormatInt32Vec2,        wgpu::TextureSampleType::Sint},
            {HgiFormatInt32Vec3,        wgpu::TextureSampleType::Sint},
            {HgiFormatInt32Vec4,        wgpu::TextureSampleType::Sint},

            {HgiFormatUNorm8Vec4srgb,   wgpu::TextureSampleType::Uint},

            {HgiFormatBC6FloatVec3,     wgpu::TextureSampleType::Float},
            {HgiFormatBC6UFloatVec3,    wgpu::TextureSampleType::Float},
            {HgiFormatBC7UNorm8Vec4,    wgpu::TextureSampleType::Undefined},
            {HgiFormatBC7UNorm8Vec4srgb,wgpu::TextureSampleType::Undefined},
            {HgiFormatBC1UNorm8Vec4,    wgpu::TextureSampleType::Undefined},
            {HgiFormatBC3UNorm8Vec4,    wgpu::TextureSampleType::Undefined},

            {HgiFormatFloat32UInt8,     wgpu::TextureSampleType::Undefined},

            {HgiFormatPackedInt1010102, wgpu::TextureSampleType::Undefined},
    };

static_assert(TfArraySize(_textureSampleTypeTable) == HgiFormatCount,
              "_textureSampleTypeTable array out of sync with HgiFormat enum");

struct {
    HgiBufferUsage hgiBufferUsage;
    wgpu::BufferUsage webGPUBufferUsage;
} static const _BufferUsageTable[] =
        {
    {HgiBufferUsageUniform, wgpu::BufferUsage::Uniform},
    {HgiBufferUsageIndex32, wgpu::BufferUsage::Index},
    {HgiBufferUsageVertex,  wgpu::BufferUsage::Vertex},
    {HgiBufferUsageStorage, wgpu::BufferUsage::Storage},
};

struct {
    HgiBindResourceType hgiBindResourceType;
    wgpu::BufferBindingType webGPUBindingType;
} static const _bufferBindResourceTypeTable[] =
{
    {HgiBindResourceTypeSampler, wgpu::BufferBindingType::Undefined},
    {HgiBindResourceTypeSampledImage, wgpu::BufferBindingType::Undefined},
    {HgiBindResourceTypeCombinedSamplerImage, wgpu::BufferBindingType::Undefined},
    {HgiBindResourceTypeStorageImage, wgpu::BufferBindingType::Undefined},
    {HgiBindResourceTypeUniformBuffer, wgpu::BufferBindingType::Uniform},
    {HgiBindResourceTypeStorageBuffer, wgpu::BufferBindingType::Storage},
    {HgiBindResourceTypeTessFactors, wgpu::BufferBindingType::Undefined},
};

static_assert(TfArraySize(_bufferBindResourceTypeTable) == HgiBindResourceTypeCount,
              "_bufferBindResourceTypeTable array out of sync with HgiBindResourceType enum");

struct {
    HgiShaderStage hgiShaderStage;
    wgpu::ShaderStage webGPUShaderStage;
} static const _shaderStageTable[] =
{
    {HgiShaderStageVertex, wgpu::ShaderStage::Vertex},
    {HgiShaderStageFragment, wgpu::ShaderStage::Fragment},
    {HgiShaderStageCompute, wgpu::ShaderStage::Compute},
    {HgiShaderStageTessellationControl, wgpu::ShaderStage::None},
    {HgiShaderStageTessellationEval, wgpu::ShaderStage::None},
    {HgiShaderStageGeometry, wgpu::ShaderStage::None},
    {HgiShaderStagePostTessellationControl, wgpu::ShaderStage::None},
    {HgiShaderStagePostTessellationVertex, wgpu::ShaderStage::None},
    {HgiShaderStageCustomBitsBegin, wgpu::ShaderStage::None},
};

static_assert(HgiBufferUsageCustomBitsBegin == 1 << 4, "");

//
// HgiCullMode
//
struct {
    HgiCullMode hgiCullMode;
    wgpu::CullMode webGPUCullMode;
} static const _CullModeTable[] =
{
    {HgiCullModeNone,         wgpu::CullMode::None},
    {HgiCullModeFront,        wgpu::CullMode::Front},
    {HgiCullModeBack,         wgpu::CullMode::Back},
    {HgiCullModeFrontAndBack, wgpu::CullMode::None} // Unsupported
};

static_assert(TfArraySize(_CullModeTable) == HgiCullModeCount,
              "_CullModeTable array out of sync with HgiFormat enum");

//
// HgiBlendOp
//
struct {
    HgiBlendOp hgiBlendOp;
    wgpu::BlendOperation webGPUBlendOp;
} static const _blendEquationTable[] =
{
    {HgiBlendOpAdd,             wgpu::BlendOperation::Add},
    {HgiBlendOpSubtract,        wgpu::BlendOperation::Subtract},
    {HgiBlendOpReverseSubtract, wgpu::BlendOperation::ReverseSubtract},
    {HgiBlendOpMin,             wgpu::BlendOperation::Min},
    {HgiBlendOpMax,             wgpu::BlendOperation::Max},
};

static_assert(TfArraySize(_blendEquationTable) == HgiBlendOpCount,
              "_blendEquationTable array out of sync with HgiBlendOp enum");

//
// HgiBlendFactor
//
struct {
    HgiBlendFactor hgiBlendFactor;
    wgpu::BlendFactor webGPUBlendFactor;
} static const _blendFactorTable[] =
{
    {HgiBlendFactorZero,                wgpu::BlendFactor::Zero},
    {HgiBlendFactorOne,                 wgpu::BlendFactor::One},
    {HgiBlendFactorSrcColor,            wgpu::BlendFactor::Src},
    {HgiBlendFactorOneMinusSrcColor,    wgpu::BlendFactor::OneMinusSrc},
    {HgiBlendFactorDstColor,            wgpu::BlendFactor::Dst},
    {HgiBlendFactorOneMinusDstColor,    wgpu::BlendFactor::OneMinusDst},
    {HgiBlendFactorSrcAlpha,            wgpu::BlendFactor::SrcAlpha},
    {HgiBlendFactorOneMinusSrcAlpha,    wgpu::BlendFactor::OneMinusSrcAlpha},
    {HgiBlendFactorDstAlpha,            wgpu::BlendFactor::DstAlpha},
    {HgiBlendFactorOneMinusDstAlpha,    wgpu::BlendFactor::OneMinusDstAlpha},
    {HgiBlendFactorConstantColor,       wgpu::BlendFactor::Zero},      // Unsupported
    {HgiBlendFactorOneMinusConstantColor, wgpu::BlendFactor::Zero},    // Unsupported
    {HgiBlendFactorConstantAlpha,       wgpu::BlendFactor::Zero},      // Unsupported
    {HgiBlendFactorOneMinusConstantAlpha, wgpu::BlendFactor::Zero},    // Unsupported
    {HgiBlendFactorSrcAlphaSaturate,    wgpu::BlendFactor::SrcAlphaSaturated},
    {HgiBlendFactorSrc1Color,           wgpu::BlendFactor::Src},
    {HgiBlendFactorOneMinusSrc1Color,   wgpu::BlendFactor::OneMinusSrcAlpha},
    {HgiBlendFactorSrc1Alpha,           wgpu::BlendFactor::SrcAlpha},
    {HgiBlendFactorOneMinusSrc1Alpha,   wgpu::BlendFactor::OneMinusSrc},
};

static_assert(TfArraySize(_blendFactorTable) == HgiBlendFactorCount,
              "_blendFactorTable array out of sync with HgiBlendFactor enum");

//
// HgiWinding
//
struct {
    HgiWinding hgiWinding;
    wgpu::FrontFace webGPUWinding;
} static const _windingTable[] =
{
    {HgiWindingClockwise,           wgpu::FrontFace::CW},
    {HgiWindingCounterClockwise,    wgpu::FrontFace::CCW},
};

static_assert(TfArraySize(_windingTable) == HgiWindingCount,
              "_windingTable array out of sync with HgiWinding enum");

//
// HgiAttachmentLoadOp
//
struct {
    HgiAttachmentLoadOp hgiAttachmentLoadOp;
    wgpu::LoadOp webGPULoadOp;
} static const _attachmentLoadOpTable[] =
{
    {HgiAttachmentLoadOpDontCare,   wgpu::LoadOp::Clear},
    {HgiAttachmentLoadOpClear,      wgpu::LoadOp::Clear},
    {HgiAttachmentLoadOpLoad,       wgpu::LoadOp::Load},
};

static_assert(TfArraySize(_attachmentLoadOpTable) == HgiAttachmentLoadOpCount,
              "_attachmentLoadOpTable array out of sync with HgiAttachmentLoadOp enum");

//
// HgiAttachmentStoreOp
//
struct {
    HgiAttachmentStoreOp hgiAttachmentStoreOp;
    wgpu::StoreOp webGPUStoreOp;
} static const _attachmentStoreOpTable[] =
{
    {HgiAttachmentStoreOpDontCare,   wgpu::StoreOp::Discard},
    {HgiAttachmentStoreOpStore,      wgpu::StoreOp::Store},
};

static_assert(TfArraySize(_attachmentStoreOpTable) == HgiAttachmentStoreOpCount,
              "_attachmentStoreOpTable array out of sync with HgiFormat enum");

//
// HgiCompareFunction
//
struct {
    HgiCompareFunction hgiCompareFunction;
    wgpu::CompareFunction webGPUCF;
} static const _compareFnTable[] =
{
    {HgiCompareFunctionNever,       wgpu::CompareFunction::Never},
    {HgiCompareFunctionLess,        wgpu::CompareFunction::Less},
    {HgiCompareFunctionEqual,       wgpu::CompareFunction::Equal},
    {HgiCompareFunctionLEqual,      wgpu::CompareFunction::LessEqual},
    {HgiCompareFunctionGreater,     wgpu::CompareFunction::Greater},
    {HgiCompareFunctionNotEqual,    wgpu::CompareFunction::NotEqual},
    {HgiCompareFunctionGEqual,      wgpu::CompareFunction::GreaterEqual},
    {HgiCompareFunctionAlways,      wgpu::CompareFunction::Always},
};

static_assert(TfArraySize(_compareFnTable) == HgiCompareFunctionCount,
              "_compareFnTable array out of sync with HgiCompareFunction enum");

struct {
    HgiTextureType hgiTextureType;
    wgpu::TextureDimension webGPUTT;
} static const _textureTypeTable[HgiTextureTypeCount] =
{
    {HgiTextureType1D,           wgpu::TextureDimension::e1D},
    {HgiTextureType2D,           wgpu::TextureDimension::e2D},
    {HgiTextureType3D,           wgpu::TextureDimension::e3D},
    {HgiTextureType1DArray,      wgpu::TextureDimension::e1D}, // TODO: Not the correct conversion
    {HgiTextureType2DArray,      wgpu::TextureDimension::e1D}, // TODO: Not the correct conversion
};

static_assert(TfArraySize(_textureTypeTable) == HgiTextureTypeCount,
              "_textureTypeTable array out of sync with HgiTextureType enum");

struct {
    HgiSamplerAddressMode hgiAddressMode;
    wgpu::AddressMode webGPUAM;
} static const _samplerAddressModeTable[HgiSamplerAddressModeCount] =
{
    {HgiSamplerAddressModeClampToEdge,        wgpu::AddressMode::ClampToEdge},
    {HgiSamplerAddressModeMirrorClampToEdge,  wgpu::AddressMode::ClampToEdge},
    {HgiSamplerAddressModeRepeat,             wgpu::AddressMode::Repeat},
    {HgiSamplerAddressModeMirrorRepeat,       wgpu::AddressMode::MirrorRepeat},
    {HgiSamplerAddressModeClampToBorderColor, wgpu::AddressMode::ClampToEdge}
};

struct {
    HgiSamplerFilter hgiSamplerFilter;
    wgpu::FilterMode webGPUSF;
} static const _samplerFilterTable[HgiSamplerFilterCount] =
{
    {HgiSamplerFilterNearest, wgpu::FilterMode::Nearest},
    {HgiSamplerFilterLinear,  wgpu::FilterMode::Linear}
};

struct {
    HgiMipFilter hgiMipFilter;
    wgpu::FilterMode webGPUMF;
} static const _mipFilterTable[HgiMipFilterCount] =
{
    {HgiMipFilterNotMipmapped, wgpu::FilterMode::Linear}, // TODO: no correct correspondence
    {HgiMipFilterNearest,      wgpu::FilterMode::Nearest},
    {HgiMipFilterLinear,       wgpu::FilterMode::Linear}
};

struct {
    HgiPrimitiveType hgiPrimitiveType;
    wgpu::PrimitiveTopology webGPUPT;
} static const _primitiveTypeTable[HgiPrimitiveTypeCount] =
{
    {HgiPrimitiveTypePointList,    wgpu::PrimitiveTopology::PointList},
    {HgiPrimitiveTypeLineList,     wgpu::PrimitiveTopology::LineList},
    {HgiPrimitiveTypeLineStrip,    wgpu::PrimitiveTopology::LineStrip},
    {HgiPrimitiveTypeTriangleList, wgpu::PrimitiveTopology::TriangleList},
    {HgiPrimitiveTypePatchList,    wgpu::PrimitiveTopology::TriangleList} // Unsupported
};

struct {
    HgiStencilOp hgiStencilOp;
    wgpu::StencilOperation webGPUStencilOp;
} static const _stencilOpTable[HgiStencilOpCount] =
{
        {HgiStencilOpKeep,    wgpu::StencilOperation::Keep},
        {HgiStencilOpZero,     wgpu::StencilOperation::Zero},
        {HgiStencilOpReplace,    wgpu::StencilOperation::Replace},
        {HgiStencilOpIncrementClamp, wgpu::StencilOperation::IncrementClamp},
        {HgiStencilOpDecrementClamp,    wgpu::StencilOperation::DecrementClamp},
        {HgiStencilOpInvert,    wgpu::StencilOperation::Invert},
        {HgiStencilOpIncrementWrap,    wgpu::StencilOperation::IncrementWrap},
        {HgiStencilOpDecrementWrap,    wgpu::StencilOperation::DecrementWrap},
};

static_assert(TfArraySize(_stencilOpTable) == HgiStencilOpCount,
              "_stencilOpTable array out of sync with HgiStencilOp enum");

struct {
    HgiPrimitiveType hgiPrimitiveType;
    wgpu::PrimitiveTopology webGPUPrimitiveTopology;
} static const _primitiveTopologyTable[HgiPrimitiveTypeCount] =
{
        {HgiPrimitiveTypePointList,                 wgpu::PrimitiveTopology::PointList},
        {HgiPrimitiveTypeLineList,                  wgpu::PrimitiveTopology::LineList},
        {HgiPrimitiveTypeLineStrip,                 wgpu::PrimitiveTopology::LineStrip},
        {HgiPrimitiveTypeTriangleList,              wgpu::PrimitiveTopology::TriangleList},
        {HgiPrimitiveTypePatchList,                 wgpu::PrimitiveTopology::TriangleList}, // TODO: Not the correct conversion
        {HgiPrimitiveTypeLineListWithAdjacency,     wgpu::PrimitiveTopology::TriangleList}  // TODO: Not the correct conversion
};

static_assert(TfArraySize(_primitiveTopologyTable) == HgiPrimitiveTypeCount,
              "_primitiveTopologyTable array out of sync with HgiPrimitiveType enum");

wgpu::TextureFormat
HgiWebGPUConversions::GetPixelFormat(HgiFormat inFormat)
{
    if (inFormat == HgiFormatInvalid) {
        return wgpu::TextureFormat::Undefined;
    }

    if ((inFormat < 0) || (inFormat >= HgiFormatCount))
    {
        TF_CODING_ERROR("Unexpected HgiFormat %d", inFormat);
        return wgpu::TextureFormat::RGBA8Unorm;
    }

    auto outFormat = _PIXEL_FORMAT_DESC[inFormat];
    if (outFormat == wgpu::TextureFormat::Undefined)
    {
        TF_CODING_ERROR("Unsupported HgiFormat %d", inFormat);
        return wgpu::TextureFormat::RGBA8Unorm;
    }
    return outFormat;
}

wgpu::VertexFormat
HgiWebGPUConversions::GetVertexFormat(HgiFormat inFormat)
{
    if ((inFormat < 0) || (inFormat >= HgiFormatCount))
    {
        TF_CODING_ERROR("Unexpected HgiFormat %d", inFormat);
        return wgpu::VertexFormat::Float32x4;
    }

    auto outFormat = _formatTable[inFormat].webGPUVertexFormat;
    if (outFormat == wgpu::VertexFormat::Undefined)
    {
        TF_CODING_ERROR("Unsupported HgiFormat %d", inFormat);
        return wgpu::VertexFormat::Float32x4;
    }
    return outFormat;
}

wgpu::CullMode
HgiWebGPUConversions::GetCullMode(HgiCullMode cm)
{
    return _CullModeTable[cm].webGPUCullMode;
}

wgpu::BlendFactor
HgiWebGPUConversions::GetBlendFactor(HgiBlendFactor bf)
{
    return _blendFactorTable[bf].webGPUBlendFactor;
}

wgpu::BlendOperation
HgiWebGPUConversions::GetBlendEquation(HgiBlendOp bo)
{
    return _blendEquationTable[bo].webGPUBlendOp;
}

wgpu::FrontFace
HgiWebGPUConversions::GetWinding(HgiWinding winding)
{
    return _windingTable[winding].webGPUWinding;
}

wgpu::LoadOp
HgiWebGPUConversions::GetAttachmentLoadOp(HgiAttachmentLoadOp loadOp)
{
    return _attachmentLoadOpTable[loadOp].webGPULoadOp;
}

wgpu::StoreOp
HgiWebGPUConversions::GetAttachmentStoreOp(HgiAttachmentStoreOp storeOp)
{
    return _attachmentStoreOpTable[storeOp].webGPUStoreOp;
}

wgpu::CompareFunction
HgiWebGPUConversions::GetCompareFunction(HgiCompareFunction cf)
{
    return _compareFnTable[cf].webGPUCF;
}

wgpu::TextureDimension
HgiWebGPUConversions::GetTextureType(HgiTextureType tt)
{
    return _textureTypeTable[tt].webGPUTT;
}

wgpu::AddressMode
HgiWebGPUConversions::GetSamplerAddressMode(HgiSamplerAddressMode a)
{
    return _samplerAddressModeTable[a].webGPUAM;
}

wgpu::FilterMode
HgiWebGPUConversions::GetMinMagFilter(HgiSamplerFilter mf)
{
    return _samplerFilterTable[mf].webGPUSF;
}

wgpu::FilterMode
HgiWebGPUConversions::GetMipFilter(HgiMipFilter mf)
{
    return _mipFilterTable[mf].webGPUMF;
}

wgpu::PrimitiveTopology
HgiWebGPUConversions::GetPrimitiveType(HgiPrimitiveType pt)
{
    return _primitiveTypeTable[pt].webGPUPT;
}

wgpu::BufferUsage
HgiWebGPUConversions::GetBufferUsage(HgiBufferUsage usage)
{
    wgpu::BufferUsage wgpuFlags = wgpu::BufferUsage::None;

    for (const auto& f : _BufferUsageTable) {
        if (usage & f.hgiBufferUsage) {
            wgpuFlags |= f.webGPUBufferUsage;
        }
    }

    if (wgpuFlags==wgpu::BufferUsage::None) {
        TF_CODING_ERROR("Missing buffer usage table entry");
    }
    return wgpuFlags;
}

wgpu::BufferBindingType
HgiWebGPUConversions::GetBindResourceType(HgiBindResourceType type)
{
    wgpu::BufferBindingType bindingType = _bufferBindResourceTypeTable[type].webGPUBindingType;;
    if (bindingType==wgpu::BufferBindingType::Undefined) {
        TF_CODING_ERROR("Missing binding type usage table entry");
    }
    return bindingType;
}

wgpu::BufferBindingType
HgiWebGPUConversions::GetBufferBindingType(HgiBindingType type, bool isWritable)
{
    switch (type) {
        case HgiBindingTypePointer:
        case HgiBindingTypeValue:
        case HgiBindingTypeArray:
            if (isWritable) {
                return wgpu::BufferBindingType::Storage;
            } else {
                return wgpu::BufferBindingType::ReadOnlyStorage;
            }
            break;
        case HgiBindingTypeUniformArray:
        case HgiBindingTypeUniformValue:
            return wgpu::BufferBindingType::Uniform;
        default:
            TF_CODING_ERROR("Unsupported HgiBindingType");
            return wgpu::BufferBindingType::Undefined;
    }
}

wgpu::ShaderStage
HgiWebGPUConversions::GetShaderStages(HgiShaderStage stage)
{
    wgpu::ShaderStage wgpuFlags = wgpu::ShaderStage::None;

    for (const auto& f : _shaderStageTable) {
        if (stage & f.hgiShaderStage) {
            wgpuFlags |= f.webGPUShaderStage;
        }
    }

    if (wgpuFlags == wgpu::ShaderStage::None) {
        TF_CODING_ERROR("Missing shader stage table entry");
    }
    return wgpuFlags;
}

wgpu::TextureFormat
HgiWebGPUConversions::GetDepthOrStencilTextureFormat(HgiTextureUsage usage, HgiFormat format)
{
    if (usage & HgiTextureUsageBitsDepthTarget && usage & HgiTextureUsageBitsStencilTarget) {
        if (format == HgiFormatFloat32UInt8) {
            return wgpu::TextureFormat::Depth32FloatStencil8;
        } else if (format == HgiFormatFloat32) {
            TF_WARN("depth24plus-stencil8 has limited copying capabilities");
            return wgpu::TextureFormat::Depth24PlusStencil8;
        }
    } else if (usage & HgiTextureUsageBitsDepthTarget) {
        if (format == HgiFormatUInt16) {
            return wgpu::TextureFormat::Depth16Unorm;
        } else if (format == HgiFormatFloat32) {
            return wgpu::TextureFormat::Depth32Float;
        } else if (format == HgiFormatFloat32UInt8) {
            TF_WARN("depth24plus has limited copying capabilities");
            return wgpu::TextureFormat::Depth24Plus;
        }
    } else if (usage & HgiTextureUsageBitsStencilTarget) {
        if (format == HgiFormatUNorm8) {
            return wgpu::TextureFormat::Stencil8;
        }
    }
    TF_CODING_ERROR("Unsupported depth-or-stencil format");
    return wgpu::TextureFormat::Undefined;
}


wgpu::StencilOperation
HgiWebGPUConversions::GetStencilOp(HgiStencilOp op)
{
    return _stencilOpTable[op].webGPUStencilOp;
}

wgpu::PrimitiveTopology
HgiWebGPUConversions::GetPrimitiveTopology(HgiPrimitiveType const &type)
{
    return _primitiveTopologyTable[type].webGPUPrimitiveTopology;
}

wgpu::TextureViewDimension
HgiWebGPUConversions::GetTextureViewDimension(uint32_t const dimensions)
{
    if (dimensions < 1 || dimensions > 3) {
        TF_CODING_ERROR("Invalid TextureViewDimension " + std::to_string(dimensions));
        return wgpu::TextureViewDimension::Undefined;
    }
    // TODO: Handle rest of wgpu::TextureViewDimension (e.g. wgpu::TextureViewDimension_CubeArray)
    switch (dimensions) {
        case 1:
            return wgpu::TextureViewDimension::e1D;
        case 2:
            return wgpu::TextureViewDimension::e2D;
        case 3:
            return wgpu::TextureViewDimension::e3D;
    }
    return wgpu::TextureViewDimension::Undefined;
}

wgpu::TextureSampleType
HgiWebGPUConversions::GetTextureSampleType(HgiFormat const &type)
{
    wgpu::TextureSampleType textureSampleType = _textureSampleTypeTable[type].webGPUTextureSampleType;
    if (textureSampleType == wgpu::TextureSampleType::Undefined) {
        TF_CODING_ERROR("Missing texture sample type entry");
    }
    return textureSampleType;
}

PXR_NAMESPACE_CLOSE_SCOPE
