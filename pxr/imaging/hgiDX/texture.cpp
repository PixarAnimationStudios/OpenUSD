
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
#include "pxr/imaging/hgiDX/buffer.h"
#include "pxr/imaging/hgiDX/capabilities.h"
#include "pxr/imaging/hgiDX/conversions.h"
#include "pxr/imaging/hgiDX/device.h"
#include "pxr/imaging/hgiDX/hgi.h"
#include "pxr/imaging/hgiDX/texture.h"
#include <algorithm>

PXR_NAMESPACE_OPEN_SCOPE


HgiDXTexture::HgiDXTexture(HgiDX* hgi,
                           HgiDXDevice* device,
                           HgiTextureDesc const& desc)
   : HgiTexture(desc)
   , _isTextureView(false)
   , _device(device)
{
   // Describe and create a Texture2D.
   D3D12_RESOURCE_DESC textureDesc = {};
   _GetDxResourceDesc(desc, textureDesc);
   _txFormat = textureDesc.Format;

   _txResState = _GetResourceStates(desc);

   HRESULT hr = _device->GetDevice()->CreateCommittedResource(
                           &CD3DX12_HEAP_PROPERTIES(_GetHeapType(desc)),
                           _GetHeapFlags(desc),
                           &textureDesc,
                           _txResState,
                           nullptr,
                           IID_PPV_ARGS(&_dxTexture));
   CheckResult(hr, "Failed to create required texture");

   if (!desc.debugName.empty())
   {
      _strName = HgiDXConversions::GetWideString(desc.debugName);
      _dxTexture->SetName(_strName.c_str());
   }

}

HgiDXTexture::HgiDXTexture(HgiDX* hgi,
                           HgiDXDevice* device,
                           HgiTextureViewDesc const& desc)
   : HgiTexture(desc.sourceTexture->GetDescriptor())
   , _isTextureView(true)
   , _device(device)
{
   // Update the texture descriptor to reflect the view desc
   _descriptor.debugName = desc.debugName;
   _descriptor.format = desc.format;
   _descriptor.layerCount = desc.layerCount;
   _descriptor.mipLevels = desc.mipLevels;

   HgiDXTexture* srcTexture = static_cast<HgiDXTexture*>(desc.sourceTexture.Get());
   HgiTextureDesc const& srcTexDesc = desc.sourceTexture->GetDescriptor();
   bool isDepthBuffer = srcTexDesc.usage & HgiTextureUsageBitsDepthTarget;

   //
   // TODO: Impl
   TF_RUNTIME_ERROR("HgiDXTexture view not implemented yet");
}

HgiDXTexture::~HgiDXTexture()
{
}

size_t
HgiDXTexture::GetByteSizeOfResource() const
{
   return _GetByteSizeOfResource(_descriptor);
}

uint64_t
HgiDXTexture::GetRawResource() const
{
   //
   // TODO: figure out what this might be needed for
   TF_RUNTIME_ERROR("GetRawResource not implemented yet");
   return (uint64_t)0;
}

ID3D12Resource* 
HgiDXTexture::GetResource()
{
   return _dxTexture.Get();
}

void*
HgiDXTexture::GetCPUStagingAddress()
{
   //
   // TODO: Implement this just like I did fot buffers
   TF_RUNTIME_ERROR("GetCPUStagingAddress not implemented yet");

   // This lets the client code memcpy into the staging buffer directly.
   // The staging data must be explicitely copied to the device-local
   // GPU buffer via CopyTextureCpuToGpu cmd by the client.
   return nullptr;
}

ID3D12DescriptorHeap* 
HgiDXTexture::GetGPUDescHeap() const
{
   if (nullptr == _descHeap.Get())
   {
      // create the descriptor heap that will store our srv
      D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {};
      heapDesc.NumDescriptors = 1;
      heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
      heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
      HRESULT hr = _device->GetDevice()->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(_descHeap.ReleaseAndGetAddressOf()));
      CheckResult(hr, "Failed to create descriptor heap");

      // now we create a shader resource view (descriptor that points to the texture and describes it)
      D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
      srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
      srvDesc.Format = _txFormat;
      srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
      srvDesc.Texture2D.MipLevels = GetDescriptor().mipLevels;
      _device->GetDevice()->CreateShaderResourceView(_dxTexture.Get(), &srvDesc, _descHeap->GetCPUDescriptorHandleForHeapStart());
   }

   return _descHeap.Get();
}


bool
HgiDXTexture::IsCPUStagingAddress(const void* address) const
{
   //
   // TODO: Implement this just like I did fot buffers
   TF_RUNTIME_ERROR("IsCPUStagingAddress not implemented yet");

   return false;
}


void
HgiDXTexture::CopyBufferToTexture(HgiDXCommandBuffer* cb,
                                  HgiDXBuffer* srcBuffer,
                                  GfVec3i const& dstTexelOffset,
                                  int mipLevel)
{
   //
   // TODO: Mihai Impl
   TF_RUNTIME_ERROR("CopyBufferToTexture not implemented yet");
}

void 
HgiDXTexture::_InitReadbackBuffer()
{
   if (nullptr == _readbackBuffer.Get())
   {
      D3D12_RESOURCE_DESC dxDesc = _dxTexture->GetDesc();
      UINT64 totalResourceSize = 0;
      UINT64 fpRowPitch = 0;
      UINT fpRowCount = 0;
      _device->GetDevice()->GetCopyableFootprints( &dxDesc,
                                                   0,
                                                   1,
                                                   0,
                                                   nullptr,
                                                   &fpRowCount,
                                                   &fpRowPitch,
                                                   &totalResourceSize);
      
      //
      // round / align row pitch to a multiple of 256
      const UINT64 dstRowPitch = (fpRowPitch + 255) & ~0xFFu;

      D3D12_HEAP_PROPERTIES readbackHeapProperties{ CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_READBACK) };
      D3D12_RESOURCE_DESC readbackBufferDesc{ CD3DX12_RESOURCE_DESC::Buffer(dstRowPitch * dxDesc.Height) };

      HRESULT hr = _device->GetDevice()->CreateCommittedResource(&readbackHeapProperties,
                                                                 D3D12_HEAP_FLAG_NONE,
                                                                 &readbackBufferDesc,
                                                                 D3D12_RESOURCE_STATE_COPY_DEST,
                                                                 nullptr,
                                                                 IID_PPV_ARGS(_readbackBuffer.ReleaseAndGetAddressOf()));

      CheckResult(hr, "Failed to create readback buffer");

      D3D12_PLACED_SUBRESOURCE_FOOTPRINT bufferFootprint = {};
      bufferFootprint.Footprint.Width = static_cast<UINT>(dxDesc.Width);
      bufferFootprint.Footprint.Height = dxDesc.Height;
      bufferFootprint.Footprint.Depth = 1;
      bufferFootprint.Footprint.RowPitch = static_cast<UINT>(dstRowPitch);
      bufferFootprint.Footprint.Format = dxDesc.Format;

      _copyDestLocation = CD3DX12_TEXTURE_COPY_LOCATION(_readbackBuffer.Get(), bufferFootprint);
   }
}

HGIDX_API
void 
HgiDXTexture::ReadbackData(GfVec3i sourceTexelOffset,
                           uint32_t mipLevel,
                           void* cpuDestinationBuffer,
                           size_t destinationByteOffset,
                           size_t destinationBufferByteSize)
{
   ID3D12GraphicsCommandList* pCmdList = _device->GetCommandList(HgiDXDevice::eCommandType::kGraphics);

   if (nullptr != pCmdList)
   {
      if (_txResState != D3D12_RESOURCE_STATE_COPY_SOURCE)
      {
         const D3D12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(_dxTexture.Get(),
                                                                                     _txResState,
                                                                                     D3D12_RESOURCE_STATE_COPY_SOURCE);

         pCmdList->ResourceBarrier(1, &barrier);
         _txResState = D3D12_RESOURCE_STATE_COPY_SOURCE;
      }

      _InitReadbackBuffer();

      //
      // TODO: let's get a copy of the whole thing going 
      // before I wonder why I get only "sourceOffset" instead of a "source box"
      CD3DX12_TEXTURE_COPY_LOCATION src(_dxTexture.Get(), 0);

      pCmdList->CopyTextureRegion(&_copyDestLocation, 0, 0, 0, &src, nullptr);

      //
      // I need the copy to execute before I can copy the data
      _device->SubmitCommandList(HgiDXDevice::eCommandType::kGraphics);

      D3D12_RANGE readRange{ 0, destinationBufferByteSize };
      D3D12_RANGE writeRange = { 0, 0 };
      FLOAT* pReadbackBufferData{};
      
      void* pMappedMemory = nullptr;
      HRESULT hr = _readbackBuffer->Map(0,
                                        &readRange,
                                        &pMappedMemory);
      CheckResult(hr, "Failed to map readback buffer to output buffer");

      D3D12_RESOURCE_DESC dxDesc = _dxTexture->GetDesc();

      //
      // eliminate the pitch padding now...
      byte* sptr = (byte*)pMappedMemory;
      byte* dptr = (byte*)cpuDestinationBuffer + destinationByteOffset;
      size_t dstPos = destinationByteOffset;
      size_t dstRowSize = dxDesc.Width * HgiGetDataSizeOfFormat(GetDescriptor().format);
      size_t srcRowSize = _copyDestLocation.PlacedFootprint.Footprint.RowPitch;
      for (uint32_t idx = 0; idx < dxDesc.Height; idx++)
      {
         dstPos += dstRowSize;

         if (dstPos <= destinationBufferByteSize)
         {
            memcpy(dptr, sptr, dstRowSize);
            dptr += dstRowSize;
            sptr += srcRowSize;
         }
         else
            TF_WARN("Not enough room in buffer to copy the texture data");
      }

      _readbackBuffer->Unmap(0, &writeRange);
   }
}

D3D12_CPU_DESCRIPTOR_HANDLE 
HgiDXTexture::GetRenderTargetView(UINT nTexIdx)
{
   return _device->CreateRenderTargetView(GetResource(), nTexIdx);
}

D3D12_CPU_DESCRIPTOR_HANDLE 
HgiDXTexture::GetDepthStencilView(UINT nTexIdx)
{
   return _device->CreateDepthStencilView(GetResource(), nTexIdx);
}

void
HgiDXTexture::Resolve(ID3D12GraphicsCommandList* pCmdList, HgiDXTexture* pOtherMSTx)
{
   if (pOtherMSTx->_txResState != D3D12_RESOURCE_STATE_RESOLVE_SOURCE)
   {
      const D3D12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(pOtherMSTx->_dxTexture.Get(),
                                                                                    pOtherMSTx->_txResState,
                                                                                    D3D12_RESOURCE_STATE_RESOLVE_SOURCE);
      pCmdList->ResourceBarrier(1, &barrier);
      pOtherMSTx->_txResState = D3D12_RESOURCE_STATE_RESOLVE_SOURCE;
   }

   if (_txResState != D3D12_RESOURCE_STATE_RESOLVE_DEST)
   {
      const D3D12_RESOURCE_BARRIER barrier2 = CD3DX12_RESOURCE_BARRIER::Transition(_dxTexture.Get(),
                                                                                    _txResState,
                                                                                    D3D12_RESOURCE_STATE_RESOLVE_DEST);
      pCmdList->ResourceBarrier(1, &barrier2);
      _txResState = D3D12_RESOURCE_STATE_RESOLVE_DEST;
   }

   pCmdList->ResolveSubresource(_dxTexture.Get(), 0, pOtherMSTx->_dxTexture.Get(), 0, pOtherMSTx->_txFormat);
}

void 
HgiDXTexture::UpdateResourceState(ID3D12GraphicsCommandList* pCmdList, D3D12_RESOURCE_STATES newResState)
{
   if (_txResState != newResState)
   {
      const D3D12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(_dxTexture.Get(),
                                                                                    _txResState,
                                                                                    newResState);
      pCmdList->ResourceBarrier(1, &barrier);

      _txResState = newResState;
   }
}

void
HgiDXTexture::_GetDxResourceDesc(const HgiTextureDesc& hgiDesc, D3D12_RESOURCE_DESC& dxTxDesc)
{
   dxTxDesc.Width = hgiDesc.dimensions[0];
   dxTxDesc.Height = hgiDesc.dimensions[1];
   dxTxDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;

   dxTxDesc.Format = HgiDXConversions::GetTextureFormat(hgiDesc.format);

   dxTxDesc.MipLevels = hgiDesc.mipLevels;
   dxTxDesc.DepthOrArraySize = hgiDesc.layerCount;

   dxTxDesc.SampleDesc.Count = hgiDesc.sampleCount;
   dxTxDesc.SampleDesc.Quality = 0; // TODO: could we set this better? have some option somewhere for it?

   dxTxDesc.Flags = _GetTextureFlags(hgiDesc);
}

D3D12_RESOURCE_FLAGS 
HgiDXTexture::_GetTextureFlags(HgiTextureDesc const& desc)
{
   D3D12_RESOURCE_FLAGS ret = D3D12_RESOURCE_FLAG_NONE;

   if (desc.usage & HgiTextureUsageBitsColorTarget)
      ret = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
   else if (desc.usage & HgiTextureUsageBitsDepthTarget)
      ret = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;
   else if (desc.usage & HgiTextureUsageBitsStencilTarget)
      ret = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

   return ret;
}

D3D12_HEAP_TYPE 
HgiDXTexture::_GetHeapType(HgiTextureDesc const& desc)
{
   //
   // Specifies the default heap.
   // This heap type experiences the most bandwidth for the GPU, but cannot provide CPU access.
   // The GPU can read and write to the memory from this pool, and resource transition barriers may be changed.
   // The majority of heapsand resources are expected to be located here, and are typically populated through resources in upload heaps.
   return D3D12_HEAP_TYPE_DEFAULT;
}

D3D12_HEAP_FLAGS 
HgiDXTexture::_GetHeapFlags(HgiTextureDesc const& desc)
{
   //
   // There are tons of options here, but I do not know enough 
   // at this time to take good decisions about this...
   return D3D12_HEAP_FLAG_NONE;
}

D3D12_RESOURCE_STATES 
HgiDXTexture::_GetResourceStates(HgiTextureDesc const& desc)
{
   if (desc.usage & HgiTextureUsageBitsColorTarget)
      return D3D12_RESOURCE_STATE_RENDER_TARGET;
   else if (desc.usage & HgiTextureUsageBitsDepthTarget)
      return D3D12_RESOURCE_STATE_DEPTH_WRITE;
   else
      return D3D12_RESOURCE_STATE_COPY_DEST;
}

PXR_NAMESPACE_CLOSE_SCOPE
