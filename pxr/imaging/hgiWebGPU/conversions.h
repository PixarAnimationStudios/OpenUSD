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
#ifndef PXR_IMAGING_HGI_WEBGPU_CONVERSIONS_H
#define PXR_IMAGING_HGI_WEBGPU_CONVERSIONS_H

#include "pxr/pxr.h"
#include "pxr/imaging/hgiWebGPU/api.h"
#include "pxr/imaging/hgi/enums.h"
#include "pxr/imaging/hgi/types.h"

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
    static wgpu::FilterMode GetMipFilter(HgiMipFilter mf);

    HGIWEBGPU_API
    static wgpu::PrimitiveTopology GetPrimitiveType(HgiPrimitiveType pt);

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
    static wgpu::TextureViewDimension GetTextureViewDimension(uint32_t const dimensions);

    HGIWEBGPU_API
    static wgpu::TextureSampleType GetTextureSampleType(HgiFormat const &type);

};


PXR_NAMESPACE_CLOSE_SCOPE

#endif

