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
#include "pxr/imaging/hgiMetal/conversions.h"

#include "pxr/base/tf/iterator.h"
#include "pxr/base/tf/staticTokens.h"
#include "pxr/base/tf/stringUtils.h"

PXR_NAMESPACE_OPEN_SCOPE

//
// HgiFormat
//
static const MTLPixelFormat _PIXEL_FORMAT_DESC[] =
{
    MTLPixelFormatR8Unorm,      // HgiFormatUNorm8,
    MTLPixelFormatRG8Unorm,     // HgiFormatUNorm8Vec2,
    //MTLPixelFormatInvalid,    // Unsupported by HgiFormat
    MTLPixelFormatRGBA8Unorm,   // HgiFormatUNorm8Vec4,

    MTLPixelFormatR8Snorm,      // HgiFormatSNorm8,
    MTLPixelFormatRG8Snorm,     // HgiFormatSNorm8Vec2,
    //MTLPixelFormatInvalid,    // Unsupported by HgiFormat
    MTLPixelFormatRGBA8Snorm,   // HgiFormatSNorm8Vec4,

    MTLPixelFormatR16Float,     // HgiFormatFloat16,
    MTLPixelFormatRG16Float,    // HgiFormatFloat16Vec2,
    MTLPixelFormatInvalid,      // Unsupported by Metal
    MTLPixelFormatRGBA16Float,  // HgiFormatFloat16Vec4,

    MTLPixelFormatR32Float,     // HgiFormatFloat32,
    MTLPixelFormatRG32Float,    // HgiFormatFloat32Vec2,
    MTLPixelFormatInvalid,      // Unsupported by Metal
    MTLPixelFormatRGBA32Float,  // HgiFormatFloat32Vec4,

    MTLPixelFormatR32Sint,      // HgiFormatInt32,
    MTLPixelFormatRG32Sint,     // HgiFormatInt32Vec2,
    MTLPixelFormatInvalid,      // Unsupported by Metal
    MTLPixelFormatRGBA32Sint,   // HgiFormatInt32Vec4,
    
    //MTLPixelFormatRGB8Unorm_sRGB, // Unsupported by HgiFormat
    MTLPixelFormatRGBA8Unorm_sRGB,  // HgiFormatUNorm8Vec4sRGB,

    MTLPixelFormatBC6H_RGBFloat,  // HgiFormatBC6FloatVec3
    MTLPixelFormatBC6H_RGBUfloat, // HgiFormatBC6UFloatVec3

    MTLPixelFormatDepth32Float_Stencil8, // HgiFormatFloat32UInt8
    
    // Note: Update _VERTEX_FORMAT_DESC as well.
};

// A few random format validations to make sure out GL table stays aligned
// with the HgiFormat table.
constexpr bool _CompileTimeValidateHgiFormatTable() {
    return (TfArraySize(_PIXEL_FORMAT_DESC) == HgiFormatCount &&
            HgiFormatUNorm8 == 0 &&
            HgiFormatFloat16Vec4 == 9 &&
            HgiFormatFloat32Vec4 == 13 &&
            HgiFormatUNorm8Vec4srgb == 18) ? true : false;
}

static_assert(_CompileTimeValidateHgiFormatTable(),
              "_PIXEL_FORMAT_DESC array out of sync with HgiFormat enum");


//
// MTLVertexFormat
//
static const MTLVertexFormat _VERTEX_FORMAT_DESC[] =
{
    MTLVertexFormatUCharNormalized,     // HgiFormatUNorm8,
    MTLVertexFormatUChar2Normalized,    // HgiFormatUNorm8Vec2,
    //MTLVertexFormatUChar3Normalized,    // Unsupported by HgiFormat,
    MTLVertexFormatUChar4Normalized,    // HgiFormatUNorm8Vec4,

    MTLVertexFormatCharNormalized,      // HgiFormatSNorm8,
    MTLVertexFormatChar2Normalized,     // HgiFormatSNorm8Vec2,
    //MTLVertexFormatChar3Normalized,     // Unsupported by HgiFormat,
    MTLVertexFormatChar4Normalized,     // HgiFormatSNorm8Vec4,

    MTLVertexFormatHalf,                // HgiFormatFloat16,
    MTLVertexFormatHalf2,               // HgiFormatFloat16Vec2,
    MTLVertexFormatHalf3,               // HgiFormatFloat16Vec3,
    MTLVertexFormatHalf4,               // HgiFormatFloat16Vec4,

    MTLVertexFormatFloat,               // HgiFormatFloat32,
    MTLVertexFormatFloat2,              // HgiFormatFloat32Vec2,
    MTLVertexFormatFloat3,              // HgiFormatFloat32Vec3,
    MTLVertexFormatFloat4,              // HgiFormatFloat32Vec4,

    MTLVertexFormatInt,                 // HgiFormatInt32,
    MTLVertexFormatInt2,                // HgiFormatInt32Vec2,
    MTLVertexFormatInt3,                // HgiFormatInt32Vec3,
    MTLVertexFormatInt4,                // HgiFormatInt32Vec4,
    
    //MTLVertexFormatUChar4Normalized,  // Unsupported by HgiFormat
    MTLVertexFormatUChar4Normalized,    // HgiFormatUNorm8Vec4sRGB,

    MTLVertexFormatInvalid,             // HgiFormatBC6FloatVec3
    MTLVertexFormatInvalid,             // HgiFormatBC6UFloatVec3

    MTLVertexFormatInvalid,             // HgiFormatFloat32UInt8
};

constexpr bool _CompileTimeValidateHgiVertexFormatTable() {
    return (TfArraySize(_VERTEX_FORMAT_DESC) == HgiFormatCount &&
            HgiFormatUNorm8 == 0 &&
            HgiFormatFloat16Vec4 == 9 &&
            HgiFormatFloat32Vec4 == 13 &&
            HgiFormatUNorm8Vec4srgb == 18) ? true : false;
}

static_assert(_CompileTimeValidateHgiVertexFormatTable(),
              "_VertexFormatDesc array out of sync with HgiFormat enum");

//
// HgiCullMode
//
struct {
    HgiCullMode hgiCullMode;
    MTLCullMode metalCullMode;
} static const _CullModeTable[] =
{
    {HgiCullModeNone,         MTLCullModeNone},
    {HgiCullModeFront,        MTLCullModeFront},
    {HgiCullModeBack,         MTLCullModeBack},
    {HgiCullModeFrontAndBack, MTLCullModeNone} // Unsupported
};

static_assert(TfArraySize(_CullModeTable) == HgiCullModeCount,
              "_CullModeTable array out of sync with HgiFormat enum");

//
// HgiPolygonMode
//
struct {
    HgiPolygonMode hgiFillMode;
    MTLTriangleFillMode metalFillMode;
} static const _PolygonModeTable[] =
{
    {HgiPolygonModeFill,  MTLTriangleFillModeFill},
    {HgiPolygonModeLine,  MTLTriangleFillModeLines},
    {HgiPolygonModePoint, MTLTriangleFillModeFill}, // Unsupported
};

static_assert(TfArraySize(_PolygonModeTable) == HgiPolygonModeCount,
              "_PolygonModeTable array out of sync with HgiFormat enum");

//
// HgiBlendOp
//
struct {
    HgiBlendOp hgiBlendOp;
    MTLBlendOperation metalBlendOp;
} static const _blendEquationTable[] =
{
    {HgiBlendOpAdd,             MTLBlendOperationAdd},
    {HgiBlendOpSubtract,        MTLBlendOperationSubtract},
    {HgiBlendOpReverseSubtract, MTLBlendOperationReverseSubtract},
    {HgiBlendOpMin,             MTLBlendOperationMin},
    {HgiBlendOpMax,             MTLBlendOperationMax},
};

static_assert(TfArraySize(_blendEquationTable) == HgiBlendOpCount,
              "_blendEquationTable array out of sync with HgiFormat enum");

//
// HgiBlendFactor
//
struct {
    HgiBlendFactor hgiBlendFactor;
    MTLBlendFactor metalBlendFactor;
} static const _blendFactorTable[] =
{
    {HgiBlendFactorZero,                MTLBlendFactorZero},
    {HgiBlendFactorOne,                 MTLBlendFactorOne},
    {HgiBlendFactorSrcColor,            MTLBlendFactorSourceColor},
    {HgiBlendFactorOneMinusSrcColor,    MTLBlendFactorOneMinusSourceColor},
    {HgiBlendFactorDstColor,            MTLBlendFactorDestinationColor},
    {HgiBlendFactorOneMinusDstColor,    MTLBlendFactorOneMinusDestinationColor},
    {HgiBlendFactorSrcAlpha,            MTLBlendFactorSourceAlpha},
    {HgiBlendFactorOneMinusSrcAlpha,    MTLBlendFactorOneMinusSourceAlpha},
    {HgiBlendFactorDstAlpha,            MTLBlendFactorDestinationAlpha},
    {HgiBlendFactorOneMinusDstAlpha,    MTLBlendFactorOneMinusDestinationAlpha},
    {HgiBlendFactorConstantColor,       MTLBlendFactorZero},  // Unsupported
    {HgiBlendFactorOneMinusConstantColor, MTLBlendFactorZero},  // Unsupported
    {HgiBlendFactorConstantAlpha,       MTLBlendFactorZero},  // Unsupported
    {HgiBlendFactorOneMinusConstantAlpha, MTLBlendFactorZero},  // Unsupported
    {HgiBlendFactorSrcAlphaSaturate,    MTLBlendFactorSourceAlphaSaturated},
    {HgiBlendFactorSrc1Color,           MTLBlendFactorSource1Color},
    {HgiBlendFactorOneMinusSrc1Color,   MTLBlendFactorOneMinusSource1Color},
    {HgiBlendFactorSrc1Alpha,           MTLBlendFactorSourceAlpha},
    {HgiBlendFactorOneMinusSrc1Alpha,   MTLBlendFactorOneMinusSource1Alpha},
};

static_assert(TfArraySize(_blendFactorTable) == HgiBlendFactorCount,
              "_blendFactorTable array out of sync with HgiFormat enum");

//
// HgiWinding
//
struct {
    HgiWinding hgiWinding;
    MTLWinding metalWinding;
} static const _windingTable[] =
{
    {HgiWindingClockwise,           MTLWindingClockwise},
    {HgiWindingCounterClockwise,    MTLWindingCounterClockwise},
};

static_assert(TfArraySize(_windingTable) == HgiWindingCount,
              "_windingTable array out of sync with HgiFormat enum");

//
// HgiAttachmentLoadOp
//
struct {
    HgiAttachmentLoadOp hgiAttachmentLoadOp;
    MTLLoadAction metalLoadOp;
} static const _attachmentLoadOpTable[] =
{
    {HgiAttachmentLoadOpDontCare,   MTLLoadActionDontCare},
    {HgiAttachmentLoadOpClear,      MTLLoadActionClear},
    {HgiAttachmentLoadOpLoad,       MTLLoadActionLoad},
};

static_assert(TfArraySize(_attachmentLoadOpTable) == HgiAttachmentLoadOpCount,
              "_attachmentLoadOpTable array out of sync with HgiFormat enum");

//
// HgiAttachmentStoreOp
//
struct {
    HgiAttachmentStoreOp hgiAttachmentStoreOp;
    MTLStoreAction metalStoreOp;
} static const _attachmentStoreOpTable[] =
{
    {HgiAttachmentStoreOpDontCare,   MTLStoreActionDontCare},
    {HgiAttachmentStoreOpStore,      MTLStoreActionStore},
};

static_assert(TfArraySize(_attachmentStoreOpTable) == HgiAttachmentStoreOpCount,
              "_attachmentStoreOpTable array out of sync with HgiFormat enum");

//
// HgiCompareFunction
//
struct {
    HgiCompareFunction hgiCompareFunction;
    MTLCompareFunction metalCF;
} static const _compareFnTable[] =
{
    {HgiCompareFunctionNever,       MTLCompareFunctionNever},
    {HgiCompareFunctionLess,        MTLCompareFunctionLess},
    {HgiCompareFunctionEqual,       MTLCompareFunctionEqual},
    {HgiCompareFunctionLEqual,      MTLCompareFunctionLessEqual},
    {HgiCompareFunctionGreater,     MTLCompareFunctionGreater},
    {HgiCompareFunctionNotEqual,    MTLCompareFunctionNotEqual},
    {HgiCompareFunctionGEqual,      MTLCompareFunctionGreaterEqual},
    {HgiCompareFunctionAlways,      MTLCompareFunctionAlways},
};

static_assert(TfArraySize(_compareFnTable) == HgiCompareFunctionCount,
              "_compareFnTable array out of sync with HgiFormat enum");

struct {
    HgiTextureType hgiTextureType;
    MTLTextureType metalTT;
} static const _textureTypeTable[HgiTextureTypeCount] =
{
    {HgiTextureType1D,           MTLTextureType1D},
    {HgiTextureType2D,           MTLTextureType2D},
    {HgiTextureType3D,           MTLTextureType3D}
};

static_assert(TfArraySize(_compareFnTable) == HgiCompareFunctionCount,
              "_compareFnTable array out of sync with HgiFormat enum");

struct {
    HgiSamplerAddressMode hgiAddressMode;
    MTLSamplerAddressMode metalAM;
} static const _samplerAddressModeTable[HgiSamplerAddressModeCount] =
{
    {HgiSamplerAddressModeClampToEdge,        MTLSamplerAddressModeClampToEdge},
    {HgiSamplerAddressModeMirrorClampToEdge,  MTLSamplerAddressModeMirrorClampToEdge},
    {HgiSamplerAddressModeRepeat,             MTLSamplerAddressModeRepeat},
    {HgiSamplerAddressModeMirrorRepeat,       MTLSamplerAddressModeMirrorRepeat},
    {HgiSamplerAddressModeClampToBorderColor, MTLSamplerAddressModeClampToBorderColor}
};

struct {
    HgiSamplerFilter hgiSamplerFilter;
    MTLSamplerMinMagFilter metalSF;
} static const _samplerFilterTable[HgiSamplerFilterCount] =
{
    {HgiSamplerFilterNearest, MTLSamplerMinMagFilterNearest},
    {HgiSamplerFilterLinear,  MTLSamplerMinMagFilterLinear}
};

struct {
    HgiMipFilter hgiMipFilter;
    MTLSamplerMipFilter metalMF;
} static const _mipFilterTable[HgiMipFilterCount] =
{
    {HgiMipFilterNotMipmapped, MTLSamplerMipFilterNotMipmapped},
    {HgiMipFilterNearest,      MTLSamplerMipFilterNearest},
    {HgiMipFilterLinear,       MTLSamplerMipFilterLinear}
};

MTLPixelFormat
HgiMetalConversions::GetPixelFormat(HgiFormat inFormat)
{
    if (inFormat == HgiFormatInvalid) {
        return MTLPixelFormatInvalid;
    }

    if ((inFormat < 0) || (inFormat >= HgiFormatCount))
    {
        TF_CODING_ERROR("Unexpected HdFormat %d", inFormat);
        return MTLPixelFormatRGBA8Unorm;
    }

    MTLPixelFormat outFormat = _PIXEL_FORMAT_DESC[inFormat];
    if (outFormat == MTLPixelFormatInvalid)
    {
        TF_CODING_ERROR("Unsupported HdFormat %d", inFormat);
        return MTLPixelFormatRGBA8Unorm;
    }
    return outFormat;
}

MTLVertexFormat
HgiMetalConversions::GetVertexFormat(HgiFormat inFormat)
{
    if ((inFormat < 0) || (inFormat >= HgiFormatCount))
    {
        TF_CODING_ERROR("Unexpected HdFormat %d", inFormat);
        return MTLVertexFormatFloat4;
    }

    MTLVertexFormat outFormat = _VERTEX_FORMAT_DESC[inFormat];
    if (outFormat == MTLVertexFormatInvalid)
    {
        TF_CODING_ERROR("Unsupported HdFormat %d", inFormat);
        return MTLVertexFormatFloat4;
    }
    return outFormat;
}

MTLCullMode
HgiMetalConversions::GetCullMode(HgiCullMode cm)
{
    return _CullModeTable[cm].metalCullMode;
}

MTLTriangleFillMode
HgiMetalConversions::GetPolygonMode(HgiPolygonMode pm)
{
    return _PolygonModeTable[pm].metalFillMode;
}

MTLBlendFactor
HgiMetalConversions::GetBlendFactor(HgiBlendFactor bf)
{
    return _blendFactorTable[bf].metalBlendFactor;
}

MTLBlendOperation
HgiMetalConversions::GetBlendEquation(HgiBlendOp bo)
{
    return _blendEquationTable[bo].metalBlendOp;
}

MTLWinding
HgiMetalConversions::GetWinding(HgiWinding winding)
{
    return _windingTable[winding].metalWinding;
}

MTLLoadAction
HgiMetalConversions::GetAttachmentLoadOp(HgiAttachmentLoadOp loadOp)
{
    return _attachmentLoadOpTable[loadOp].metalLoadOp;
}

MTLStoreAction
HgiMetalConversions::GetAttachmentStoreOp(HgiAttachmentStoreOp storeOp)
{
    return _attachmentStoreOpTable[storeOp].metalStoreOp;
}

MTLCompareFunction
HgiMetalConversions::GetDepthCompareFunction(HgiCompareFunction cf)
{
    return _compareFnTable[cf].metalCF;
}

MTLTextureType
HgiMetalConversions::GetTextureType(HgiTextureType tt)
{
    return _textureTypeTable[tt].metalTT;
}

MTLSamplerAddressMode
HgiMetalConversions::GetSamplerAddressMode(HgiSamplerAddressMode a)
{
    return _samplerAddressModeTable[a].metalAM;
}

MTLSamplerMinMagFilter
HgiMetalConversions::GetMinMagFilter(HgiSamplerFilter mf)
{
    return _samplerFilterTable[mf].metalSF;
}

MTLSamplerMipFilter
HgiMetalConversions::GetMipFilter(HgiMipFilter mf)
{
    return _mipFilterTable[mf].metalMF;
}

PXR_NAMESPACE_CLOSE_SCOPE
