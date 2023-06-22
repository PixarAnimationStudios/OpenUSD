
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

#pragma once

#include "pxr/pxr.h"
#include "pxr/imaging/hgiDX/api.h"
#include "pxr/imaging/hgi/texture.h"


PXR_NAMESPACE_OPEN_SCOPE

class HgiDX;
class HgiDXBuffer;
class HgiDXCommandBuffer;
class HgiDXDevice;


/// \class HgiDXTexture
///
/// Represents a DirectX GPU texture resource.
///
class HgiDXTexture final : public HgiTexture
{
public:
   static const uint32_t NO_PENDING_WRITES = 0;

   HGIDX_API
   ~HgiDXTexture() override;

   HGIDX_API
   size_t GetByteSizeOfResource() const override;

   HGIDX_API
   uint64_t GetRawResource() const override;

   /// Creates (on first use) and returns the CPU staging buffer that can be
   /// used to upload new texture data to the image.
   /// After memcpy-ing new data into the returned address the client
   /// must use BlitCmds CopyTextureCpuToGpu to schedule the transfer
   /// from this staging buffer to the GPU texture.
   HGIDX_API
   void* GetCPUStagingAddress();

   ID3D12DescriptorHeap* GetGPUDescHeap() const;

   /// Returns true if the provided ptr matches the address of staging buffer.
   HGIDX_API
   bool IsCPUStagingAddress(const void* address) const;

   /// Schedule a copy of texels from the provided buffer into the texture.
   /// If mipLevel is less than one, all mip levels will be copied from buffer.
   HGIDX_API
   void CopyBufferToTexture(  HgiDXCommandBuffer* cb,
                              HgiDXBuffer* srcBuffer,
                              GfVec3i const& dstTexelOffset = GfVec3i(0),
                              int mipLevel = -1);

   HGIDX_API
   void ReadbackData(GfVec3i sourceTexelOffset, 
                     uint32_t mipLevel, 
                     void* cpuDestinationBuffer, 
                     size_t destinationByteOffset,
                     size_t destinationBufferByteSize);

   HGIDX_API
   struct ID3D12Resource* GetResource();

   HGIDX_API
   D3D12_CPU_DESCRIPTOR_HANDLE GetRenderTargetView(UINT nTexIdx);

   HGIDX_API
   D3D12_CPU_DESCRIPTOR_HANDLE GetDepthStencilView(UINT nTexIdx);

   HGIDX_API
   void Resolve(ID3D12GraphicsCommandList* pCmdList, HgiDXTexture* pOtherMSTx);

   HGIDX_API
   void UpdateResourceState(ID3D12GraphicsCommandList* pCmdList, D3D12_RESOURCE_STATES newResState);


protected:
   friend class HgiDX;

   HGIDX_API
   HgiDXTexture(HgiDX* hgi, HgiDXDevice* device, HgiTextureDesc const& desc);

   // Texture view constructor to alias another texture's data.
   HGIDX_API 
   HgiDXTexture(HgiDX* hgi, HgiDXDevice* device, HgiTextureViewDesc const& desc);

private:
   static D3D12_RESOURCE_FLAGS _GetTextureFlags(HgiTextureDesc const& desc);
   static D3D12_HEAP_TYPE _GetHeapType(HgiTextureDesc const& desc);
   static D3D12_HEAP_FLAGS _GetHeapFlags(HgiTextureDesc const& desc);
   static D3D12_RESOURCE_STATES _GetResourceStates(HgiTextureDesc const& desc);
   static void _GetDxResourceDesc(const HgiTextureDesc&, D3D12_RESOURCE_DESC&);
   void _InitReadbackBuffer();


private:
   HgiDXTexture() = delete;
   HgiDXTexture& operator=(const HgiDXTexture&) = delete;
   HgiDXTexture(const HgiDXTexture&) = delete;

   std::wstring _strName;
   bool _isTextureView;
   HgiDXDevice* _device;

   Microsoft::WRL::ComPtr<ID3D12Resource> _dxTexture;
   Microsoft::WRL::ComPtr<ID3D12Resource> _readbackBuffer;
   CD3DX12_TEXTURE_COPY_LOCATION _copyDestLocation;

   mutable Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> _descHeap;
   D3D12_RESOURCE_STATES _txResState;
   DXGI_FORMAT _txFormat;


};


PXR_NAMESPACE_CLOSE_SCOPE

