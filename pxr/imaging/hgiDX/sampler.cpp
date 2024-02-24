
//
// Copyright 2023 Pixar
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

#include "pch.h"
#include "pxr/base/tf/diagnostic.h"

#include "pxr/imaging/hgiDX/capabilities.h"
#include "pxr/imaging/hgiDX/device.h"
#include "pxr/imaging/hgiDX/sampler.h"

#include <float.h>

PXR_NAMESPACE_OPEN_SCOPE


HgiDXSampler::HgiDXSampler(HgiDXDevice* device, HgiSamplerDesc const& desc)
    : HgiSampler(desc)
    , _device(device)
{
}

HgiDXSampler::~HgiDXSampler()
{
}

uint64_t
HgiDXSampler::GetRawResource() const
{
	//
	// TODO: Mihai Impl
	TF_WARN("HgiDXSampler::GetRawResource not implemented yet.");
   return (uint64_t) 0;
}

D3D12_GPU_DESCRIPTOR_HANDLE 
HgiDXSampler::GetGPUDescHandle(int nIdx) const
{
   D3D12_GPU_DESCRIPTOR_HANDLE ret;

   const HgiSamplerDesc& hgiDesc = GetDescriptor();


   D3D12_SAMPLER_DESC samplerDesc = {};
   
   samplerDesc.Filter = _GetFilter(hgiDesc.minFilter, hgiDesc.magFilter, hgiDesc.mipFilter, hgiDesc.enableCompare);

   samplerDesc.AddressU = _GetAddressMode(hgiDesc.addressModeU);
   samplerDesc.AddressV = _GetAddressMode(hgiDesc.addressModeV);
   samplerDesc.AddressW = _GetAddressMode(hgiDesc.addressModeW);

   _GetBorderColor(hgiDesc.borderColor, samplerDesc.BorderColor);

   samplerDesc.ComparisonFunc = _GetCompareFc(hgiDesc.enableCompare, hgiDesc.compareFunction);

   //
   // TODO: how should I handle the remaining parameters?
   samplerDesc.MinLOD = 0;
   samplerDesc.MaxLOD = D3D12_FLOAT32_MAX;
   samplerDesc.MipLODBias = 0.0f;

   //
   // TODO: Ogl also have a code saying "// If the filter is nearest (I think point fox DX), we will not set GL_TEXTURE_MAX_ANISOTROPY_EXT."
   samplerDesc.MaxAnisotropy = 16.0; // setting this like Ogl does


   ID3D12DescriptorHeap* pHeap = _device->GetSamplersDescriptorHeap();
   UINT nHeapDescSize = _device->GetSamplersDescriptorHeapIncrementSize();
   D3D12_CPU_DESCRIPTOR_HANDLE cdh = CD3DX12_CPU_DESCRIPTOR_HANDLE(pHeap->GetCPUDescriptorHandleForHeapStart(), nIdx, nHeapDescSize);
   
   _device->GetDevice()->CreateSampler(&samplerDesc, cdh);

   ret = CD3DX12_GPU_DESCRIPTOR_HANDLE(pHeap->GetGPUDescriptorHandleForHeapStart(), nIdx, nHeapDescSize);

   return ret;
}

D3D12_FILTER
HgiDXSampler::_GetFilter(const HgiSamplerFilter& min, const HgiSamplerFilter& mag, const HgiMipFilter& mipFilter, bool bEnableComparison) 
{

   D3D12_FILTER ret = D3D12_FILTER_MIN_MAG_MIP_POINT; // just to have some valid value in case we run into something DX does not support

   if (HgiSamplerFilterNearest == min)
   {
      if (HgiSamplerFilterNearest == mag)
      {
         switch (mipFilter)
         {
         case HgiMipFilterNotMipmapped: // TODO: DX does not seem to support this one
         case HgiMipFilterNearest:
            ret = D3D12_FILTER_MIN_MAG_MIP_POINT;
            break;
         case HgiMipFilterLinear:
            ret = D3D12_FILTER_MIN_MAG_POINT_MIP_LINEAR;
            break;
         default:
            break;
         }
      }
      else
      {
         switch (mipFilter)
         {
         case HgiMipFilterNotMipmapped: // TODO: DX does not seem to support this one
         case HgiMipFilterNearest:
            ret = D3D12_FILTER_MIN_POINT_MAG_LINEAR_MIP_POINT;
            break;
         case HgiMipFilterLinear:
            ret = D3D12_FILTER_MIN_POINT_MAG_MIP_LINEAR;
            break;
         default:
            break;
         }
      }
   }
   else
   {
      if (HgiSamplerFilterNearest == mag)
      {
         switch (mipFilter)
         {
         case HgiMipFilterNotMipmapped: // TODO: DX does not seem to support this one
         case HgiMipFilterNearest:
            ret = D3D12_FILTER_MIN_LINEAR_MAG_MIP_POINT;
            break;
         case HgiMipFilterLinear:
            ret = D3D12_FILTER_MIN_LINEAR_MAG_POINT_MIP_LINEAR;
            break;
         default:
            break;
         }
      }
      else
      {
         ret = D3D12_FILTER_ANISOTROPIC;
         /* trying to emulate ogl behavior although I really do not understand it...
         switch (mipFilter)
         {
         case HgiMipFilterNotMipmapped: // TODO: DX does not seem to support this one
         case HgiMipFilterNearest:
            ret = D3D12_FILTER_MIN_MAG_LINEAR_MIP_POINT;
            break;
         case HgiMipFilterLinear:
            ret = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
            break;
         default:
            break;
         }*/
      }
   }

   if (bEnableComparison)
      ret = (D3D12_FILTER)(ret + 0x80);

   return ret;
}
D3D12_TEXTURE_ADDRESS_MODE
HgiDXSampler::_GetAddressMode(const HgiSamplerAddressMode& hgiAddr)
{
   D3D12_TEXTURE_ADDRESS_MODE ret = D3D12_TEXTURE_ADDRESS_MODE_WRAP;

   //
   // TODO: build a simple test case to test all modes and see that I mapped them correctly
   switch (hgiAddr)
   {
   case HgiSamplerAddressModeClampToEdge:
      ret = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
      break;
   case HgiSamplerAddressModeMirrorClampToEdge:
      ret = D3D12_TEXTURE_ADDRESS_MODE_MIRROR_ONCE;
      break;
   case HgiSamplerAddressModeRepeat:
      ret = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
      break;
   case HgiSamplerAddressModeMirrorRepeat:
      ret = D3D12_TEXTURE_ADDRESS_MODE_MIRROR;
      break;
   case HgiSamplerAddressModeClampToBorderColor:
      ret = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
      break;
   default:
      break;
   }

   return ret;
}
void
HgiDXSampler::_GetBorderColor(const HgiBorderColor& bc, float color[4])
{
   switch (bc)
   {
   case HgiBorderColorOpaqueBlack:
      color[0] = color[1] = color[2] = 0.0f;
      color[3] = 0.0f;
      break;
   case HgiBorderColorOpaqueWhite:
      color[0] = color[1] = color[2] = 1.0f;
      color[3] = 0.0f;
      break;
   case HgiBorderColorTransparentBlack:
   default:
      color[0] = color[1] = color[2] = 0.0f;
      color[3] = 1.0f;
      break;
   }
}
D3D12_COMPARISON_FUNC
HgiDXSampler::_GetCompareFc(bool bEnableCompare, const HgiCompareFunction& fc)
{
   D3D12_COMPARISON_FUNC ret = D3D12_COMPARISON_FUNC_NEVER;

   if (bEnableCompare)
   {
      switch (fc)
      {
      case HgiCompareFunctionNever:
         ret = D3D12_COMPARISON_FUNC_NEVER;
         break;
      case HgiCompareFunctionLess:
         ret = D3D12_COMPARISON_FUNC_LESS;
         break;
      case HgiCompareFunctionEqual:
         ret = D3D12_COMPARISON_FUNC_EQUAL;
         break;
      case HgiCompareFunctionLEqual:
         ret = D3D12_COMPARISON_FUNC_LESS_EQUAL;
         break;
      case HgiCompareFunctionGreater:
         ret = D3D12_COMPARISON_FUNC_GREATER;
         break;
      case HgiCompareFunctionNotEqual:
         ret = D3D12_COMPARISON_FUNC_NOT_EQUAL;
         break;
      case HgiCompareFunctionGEqual:
         ret = D3D12_COMPARISON_FUNC_GREATER_EQUAL;
         break;
      case HgiCompareFunctionAlways:
         ret = D3D12_COMPARISON_FUNC_ALWAYS;
         break;
      default:
         break;
      }
   }

   return ret;
}


PXR_NAMESPACE_CLOSE_SCOPE
