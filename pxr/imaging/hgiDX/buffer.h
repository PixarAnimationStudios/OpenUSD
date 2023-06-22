
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

#include "pxr/imaging/hgi/buffer.h"
#include "pxr/imaging/hgiDX/api.h"

PXR_NAMESPACE_OPEN_SCOPE

class HgiDX;
class HgiDXCommandBuffer;
class HgiDXDevice;

///
/// \class HgiDXBuffer
///
/// DirectX implementation of HgiBuffer
///
class HgiDXBuffer final : public HgiBuffer
{
public:
   // Constructor for making buffers
   HGIDX_API
   HgiDXBuffer(HgiDXDevice* device, HgiBufferDesc const& desc);

   HGIDX_API
   ~HgiDXBuffer() override;

   HGIDX_API
   size_t GetByteSizeOfResource() const override;

   /// This function returns the handle to the Hgi backend's gpu resource, cast
   /// to a uint64_t. Clients should avoid using this function and instead
   /// use Hgi base classes so that client code works with any Hgi platform.
   /// For transitioning code to Hgi, it can however we useful to directly
   /// access a platform's internal resource handles.
   /// There is no safety provided in using this. If you by accident pass a
   /// HgiMetal resource into an OpenGL call, bad things may happen.
   /// In OpenGL this returns the GLuint resource name.
   /// In Metal this returns the id<MTLBuffer> as uint64_t.
   /// In Vulkan this returns the VkBuffer as uint64_t.
   /// In DX12 this returns the ID3D12Resource* as uint64_t.
   HGI_API
   uint64_t GetRawResource() const override;

   HGIDX_API
   void* GetCPUStagingAddress() override;

   /// Returns true if the provided ptr matches the address of staging buffer.
   HGIDX_API
   bool IsCPUStagingAddress(const void* address) const;

   HGIDX_API
   struct ID3D12Resource* GetResource();

   HGIDX_API
   D3D12_GPU_VIRTUAL_ADDRESS GetGPUVirtualAddress() const;
  
   HGIDX_API
   void UpdateData(const void* pData, size_t dataSize, size_t sourceByteOffset, size_t destinationByteOffset);

   HGIDX_API
   void UpdateData(HgiDXBuffer* pOtherGPUBuff, size_t dataSize, size_t sourceByteOffset, size_t destinationByteOffset);

   HGIDX_API
   void UpdateResourceState(ID3D12GraphicsCommandList* pCmdList, D3D12_RESOURCE_STATES newResState);

   /// <summary>
   /// Below, some temporary, hacky debug helper methods
   /// </summary>
   HGIDX_API
   static void SetWatchBuffer(HgiDXBuffer* pBuff2Watch);
   HGIDX_API
   static HgiDXBuffer* GetWatchBuffer();
   HGIDX_API
   void InspectBufferContents();


private:
   void _InitStagingBuffer();
   void _UnmapStagingBuffer();
   void _BuildIntermediaryBuffer();

   HgiDXBuffer() = delete;
   HgiDXBuffer& operator=(const HgiDXBuffer&) = delete;
   HgiDXBuffer(const HgiDXBuffer&) = delete;


private:
   HgiDXDevice* _device;
   std::wstring _strName;

   D3D12_RESOURCE_STATES _bufResState;
   Microsoft::WRL::ComPtr<ID3D12Resource> _dxBuffer;
   Microsoft::WRL::ComPtr<ID3D12Resource> _dxIntermediaryBuffer;
   mutable void* _pStagingBuffer;

};


PXR_NAMESPACE_CLOSE_SCOPE
