//
// Copyright 2026 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_IMAGING_HGI_WEBGPU_CONVERSIONS_H
#define PXR_IMAGING_HGI_WEBGPU_CONVERSIONS_H

#include "pxr/pxr.h"
#include "pxr/imaging/hgiWebGPU/api.h"
#include "pxr/imaging/hgi/enums.h"
#include "pxr/imaging/hgi/types.h"

#include <string>

PXR_NAMESPACE_OPEN_SCOPE

///
/// \class HgiWebGPUConversions
///
/// Converts from Hgi types to WebGPU types.
///
class HgiWebGPUConversions final
{
public:
    //
    // Hgi to WebGPU conversions
    //

    HGIWEBGPU_API
    static wgpu::TextureFormat GetPixelFormat(HgiFormat inFormat);

    HGIWEBGPU_API
    static wgpu::VertexFormat GetVertexFormat(HgiFormat inFormat);
    
    HGIWEBGPU_API
    static wgpu::CullMode GetCullMode(HgiCullMode cm);

    HGIWEBGPU_API
    static wgpu::BlendFactor GetBlendFactor(HgiBlendFactor bf);

    HGIWEBGPU_API
    static wgpu::BlendOperation GetBlendEquation(HgiBlendOp bo);
    
    HGIWEBGPU_API
    static wgpu::FrontFace GetWinding(HgiWinding winding);
    
    HGIWEBGPU_API
    static wgpu::LoadOp GetAttachmentLoadOp(HgiAttachmentLoadOp loadOp);

    HGIWEBGPU_API
    static wgpu::StoreOp GetAttachmentStoreOp(HgiAttachmentStoreOp storeOp);
    
    HGIWEBGPU_API
    static wgpu::CompareFunction GetCompareFunction(HgiCompareFunction cf);
    
    HGIWEBGPU_API
    static wgpu::TextureDimension GetTextureType(HgiTextureType tt);

    HGIWEBGPU_API
    static wgpu::AddressMode GetSamplerAddressMode(HgiSamplerAddressMode a);

    HGIWEBGPU_API
    static wgpu::FilterMode GetMinMagFilter(HgiSamplerFilter mf);

    HGIWEBGPU_API
    static wgpu::MipmapFilterMode GetMipFilter(HgiMipFilter mf);

    HGIWEBGPU_API
    static wgpu::BufferUsage GetBufferUsage(HgiBufferUsage usage);

    HGIWEBGPU_API
    static wgpu::BufferBindingType GetBindResourceType(HgiBindResourceType type);

    HGIWEBGPU_API
    static wgpu::BufferBindingType GetBufferBindingType(HgiBindingType type, bool isWritable);
    
    HGIWEBGPU_API
    static wgpu::ShaderStage GetShaderStages(HgiShaderStage stage);

    HGIWEBGPU_API
    static wgpu::TextureFormat GetDepthOrStencilTextureFormat(HgiTextureUsage usage, HgiFormat format);

    HGIWEBGPU_API
    static wgpu::StencilOperation GetStencilOp(HgiStencilOp op);

    HGIWEBGPU_API
    static wgpu::PrimitiveTopology GetPrimitiveTopology(HgiPrimitiveType const &type);

    HGIWEBGPU_API
    static wgpu::TextureViewDimension GetTextureViewDimension(HgiTextureType type);

    HGIWEBGPU_API
    static wgpu::TextureViewDimension GetTextureViewDimension(uint32_t dimensions, HgiShaderTextureType type);

    HGIWEBGPU_API
    static wgpu::ComponentSwizzle GetComponentSwizzle(HgiComponentSwizzle cs);

    HGIWEBGPU_API
    static wgpu::TextureSampleType GetTextureSampleType(HgiFormat const &type);

    HGIWEBGPU_API
    static wgpu::ColorWriteMask GetColorWriteMask(HgiColorMask const &mask);

    HGIWEBGPU_API
    static std::string GetImageLayoutFormatQualifier(HgiFormat inFormat);

};


PXR_NAMESPACE_CLOSE_SCOPE

#endif
