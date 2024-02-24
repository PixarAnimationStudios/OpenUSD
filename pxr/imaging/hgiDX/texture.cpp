
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
   , _bIsTextureView(false)
   , _device(device)
{
   // Describe and create a Texture2D.
   D3D12_RESOURCE_DESC dxTextureDesc = {};
   _GetDxResourceDesc(desc, dxTextureDesc);
   _txFormat = dxTextureDesc.Format;

   _txResState = _GetInitialResourceStates(desc);

   HRESULT hr = _device->GetDevice()->CreateCommittedResource(
                           &CD3DX12_HEAP_PROPERTIES(_GetHeapType(desc)),
                           _GetHeapFlags(desc),
                           &dxTextureDesc,
                           _txResState,
                           nullptr,
                           IID_PPV_ARGS(&_dxTexture));
   CheckResult(hr, "Failed to create required texture");

   if (!desc.debugName.empty())
   {
      _strName = HgiDXConversions::GetWideString(desc.debugName);
      _dxTexture->SetName(_strName.c_str());
   }

   // Upload texel data
   if (desc.initialData && desc.pixelsByteSize > 0) {
      UpdateData(desc.initialData, desc.pixelsByteSize);
   }
}

HgiDXTexture::HgiDXTexture(HgiDX* hgi,
                           HgiDXDevice* device,
                           HgiTextureViewDesc const& desc)
   : HgiTexture(desc.sourceTexture->GetDescriptor())
   , _bIsTextureView(true)
   , _descTV(desc)
   , _device(device)
{
}

HgiDXTexture::~HgiDXTexture()
{
}

size_t
HgiDXTexture::GetByteSizeOfResource() const
{
   if (_bIsTextureView)
   {
      //
      // modify the desc to reflect this view's data
      HgiTextureDesc desc = _descriptor;
      desc.layerCount = _descTV.layerCount;
      desc.mipLevels = _descTV.mipLevels;

      return _GetByteSizeOfResource(desc);
   }
   else
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
HgiDXTexture::GetResource() const
{
   ID3D12Resource* pRet = nullptr;

   if (_bIsTextureView)
   {
      HgiDXTexture* srcTexture = static_cast<HgiDXTexture*>(_descTV.sourceTexture.Get());
      if (nullptr != srcTexture)
         pRet = srcTexture->_dxTexture.Get();
   }
   else
      pRet = _dxTexture.Get();

   return pRet;
}

DXGI_FORMAT 
HgiDXTexture::GetResourceFormat() const
{
   DXGI_FORMAT ret;

   if (_bIsTextureView)
   {
      HgiDXTexture* srcTexture = static_cast<HgiDXTexture*>(_descTV.sourceTexture.Get());
      if (nullptr != srcTexture)
         ret = srcTexture->_txFormat;
   }
   else
      ret = _txFormat;

   return ret;
}

D3D12_RESOURCE_STATES invalidResourceState;

D3D12_RESOURCE_STATES& 
HgiDXTexture::GetResourceState()
{
   if (_bIsTextureView)
   {
      HgiDXTexture* srcTexture = static_cast<HgiDXTexture*>(_descTV.sourceTexture.Get());
      if (nullptr != srcTexture)
         return srcTexture->_txResState;
      else
         return invalidResourceState;
   }
   else
      return _txResState;
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

D3D12_GPU_DESCRIPTOR_HANDLE
HgiDXTexture::GetGPUDescHandle(int nIdx, D3D12_DESCRIPTOR_RANGE_TYPE drt) const
{
   // create the descriptor heap that will store our srv
   /*
   D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {};
   heapDesc.NumDescriptors = 1;
   heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
   heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
   HRESULT hr = _device->GetDevice()->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(_descHeap.ReleaseAndGetAddressOf()));
   CheckResult(hr, "Failed to create descriptor heap");*/

   //
   // now we create a shader resource view (descriptor that points to the texture and describes it)
   // and since we have no high level idea how many textures (views) we need for this
   // particular draw step, we will rely (for now) on whoever's calling us to manage the heap index for this texture

   ID3D12DescriptorHeap* pHeap = _device->GetCbvSrvUavDescriptorHeap();
   UINT nHeapDescSize = _device->GetCbvSrvUavDescriptorHeapIncrementSize();

   ID3D12Resource* pTxResource = GetResource();
   if (nullptr != pTxResource)
   {

      switch (drt)
      {
         //
         // TODO: I do not understand how I could create a cbv out of a texture
         // let's hope srv works for all cases for now
      case D3D12_DESCRIPTOR_RANGE_TYPE_CBV:
      case D3D12_DESCRIPTOR_RANGE_TYPE_SRV:
      {
         D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
         srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
         srvDesc.Format = GetResourceFormat();
         srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;

         if (_bIsTextureView)
         {
            srvDesc.Texture2D.MipLevels = _descTV.mipLevels;
            srvDesc.Texture2D.MostDetailedMip = _descTV.sourceFirstMip;
            srvDesc.Texture2D.PlaneSlice = _descTV.sourceFirstLayer;
         }
         else
         {
            srvDesc.Texture2D.MipLevels = pTxResource->GetDesc().MipLevels;
            srvDesc.Texture2D.MostDetailedMip = 0;
            srvDesc.Texture2D.PlaneSlice = 0;
         }

         D3D12_CPU_DESCRIPTOR_HANDLE cdh = CD3DX12_CPU_DESCRIPTOR_HANDLE(pHeap->GetCPUDescriptorHandleForHeapStart(), nIdx, nHeapDescSize);
         _device->GetDevice()->CreateShaderResourceView(pTxResource, &srvDesc, cdh);

      } break;
      case D3D12_DESCRIPTOR_RANGE_TYPE_UAV:
      {
         D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
         uavDesc.Format = GetResourceFormat();
         uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;

         if (_bIsTextureView)
         {
            uavDesc.Texture2D.MipSlice = _descTV.sourceFirstMip;
            uavDesc.Texture2D.PlaneSlice = _descTV.sourceFirstLayer;

            ////
            //// TODO: if this actually works, make it simpler
            //if (_descTV.sourceFirstMip != 0)
            //{
            //   const HgiTextureDesc& hgiDesc = _descTV.sourceTexture->GetDescriptor();
            //   const std::vector<HgiMipInfo> mipInfos =
            //      HgiGetMipInfos(
            //         hgiDesc.format,
            //         hgiDesc.dimensions,
            //         hgiDesc.layerCount,
            //         size_t(-1));

            //   if(mipInfos.size() > (_descTV.sourceFirstMip+1))
            //      byteOffset = mipInfos[_descTV.sourceFirstMip].byteOffset;
            //}
         }
         else
         {
            uavDesc.Texture2D.MipSlice = 0; // TODO: hoping this slice is actually an "index"
            uavDesc.Texture2D.PlaneSlice = 0;
         }

         D3D12_CPU_DESCRIPTOR_HANDLE cdh = CD3DX12_CPU_DESCRIPTOR_HANDLE(pHeap->GetCPUDescriptorHandleForHeapStart(), nIdx, nHeapDescSize);

         //
         // Usually the "counter" resource is not mandatory, and I do not know what to do with it right now.
         _device->GetDevice()->CreateUnorderedAccessView(pTxResource, nullptr, &uavDesc, cdh);
      } break;
      default:
         TF_RUNTIME_ERROR("Unexpected request for GPU_DESCRIPTOR_HANDLE.");
         break;
      }

      return CD3DX12_GPU_DESCRIPTOR_HANDLE(pHeap->GetGPUDescriptorHandleForHeapStart(), nIdx, nHeapDescSize);
   }
   else
   {
      TF_RUNTIME_ERROR("Invalid texture resource.");
      return CD3DX12_GPU_DESCRIPTOR_HANDLE();
   }
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
      ID3D12Resource* pTxResource = GetResource();
      D3D12_RESOURCE_DESC dxDesc = pTxResource->GetDesc();
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

void 
HgiDXTexture::_InitIntermediaryBuffer()
{
   if (nullptr == _dxIntermediaryBuffer)
   {
      ID3D12Resource* pTxResource = GetResource();
      D3D12_RESOURCE_DESC dxDesc = pTxResource->GetDesc();

      UINT nFirstSubResource = 0;
      UINT nMipMaps = 0;
      if (_bIsTextureView)
      {
         nMipMaps = _descTV.mipLevels;
         nFirstSubResource = _descTV.sourceFirstMip; // TODO: are these things referring to the same concept?
      }
      else
      {
         nMipMaps = dxDesc.MipLevels;
         nFirstSubResource = 0;
      }

      UINT64 requiredSize = GetRequiredIntermediateSize(pTxResource, nFirstSubResource, nMipMaps);

      D3D12_HEAP_TYPE ht = D3D12_HEAP_TYPE_UPLOAD;
      D3D12_RESOURCE_STATES rs = D3D12_RESOURCE_STATE_GENERIC_READ;

      CD3DX12_HEAP_PROPERTIES hpDef(ht);
      CD3DX12_RESOURCE_DESC buffDesc(CD3DX12_RESOURCE_DESC::Buffer(requiredSize, D3D12_RESOURCE_FLAG_NONE));
      HRESULT hr = _device->GetDevice()->CreateCommittedResource(&hpDef,
                                                                 D3D12_HEAP_FLAG_NONE,
                                                                 &buffDesc,
                                                                 rs,
                                                                 nullptr,
                                                                 IID_PPV_ARGS(_dxIntermediaryBuffer.ReleaseAndGetAddressOf()));
      CheckResult(hr, "Failed to create required buffer.");
   }
}

void
HgiDXTexture::UpdateData(const void* pData, size_t dataSize)
{
   //
   // somewhere below there's a pretty bad mistake. 
   // The kind that probably causes an access violation in the GPU and restarts the machine :)
   //return;

   //
   // I will do the copies on the graphics queue - the copy queue cannot transition resources...
   //ID3D12GraphicsCommandList* pCmdList = _device->GetCommandList(HgiDXDevice::eCommandType::kCopy);
   ID3D12GraphicsCommandList* pCmdList = _device->GetCommandList(HgiDXDevice::eCommandType::kGraphics);
   if (nullptr != pCmdList)
   {
      ID3D12Resource* pTxResource = GetResource();
      D3D12_RESOURCE_STATES& resState = GetResourceState();

      if (nullptr != pTxResource)
      {

         _InitIntermediaryBuffer();

         //
         // transition resource / target into "copy" mode
         if (resState != D3D12_RESOURCE_STATE_COPY_DEST)
         {
            const D3D12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(pTxResource,
                                                                                        resState,
                                                                                        D3D12_RESOURCE_STATE_COPY_DEST);

            pCmdList->ResourceBarrier(1, &barrier);
            resState = D3D12_RESOURCE_STATE_COPY_DEST;
         }

         const HgiTextureDesc& desc = GetDescriptor();
         const std::vector<HgiMipInfo> mipInfos =
            HgiGetMipInfos(
               desc.format,
               desc.dimensions,
               desc.layerCount,
               //desc.pixelsByteSize); // this probably never has the correct information
               dataSize);

         size_t nFirstMip = 0;
         size_t mipLevels = 0;

         if (_bIsTextureView)
         {
            nFirstMip = _descTV.sourceFirstMip;
            mipLevels = std::min(mipInfos.size(), size_t(_descTV.mipLevels));
         }
         else
         {
            nFirstMip = 0;
            mipLevels = std::min(mipInfos.size(), size_t(desc.mipLevels));
         }

         const UINT8* const pInitialData = reinterpret_cast<const UINT8*>(pData);

         std::vector<D3D12_SUBRESOURCE_DATA> subresourceArr;
         subresourceArr.reserve(mipLevels);

         for (size_t mip = nFirstMip; mip < mipLevels; mip++) {
            const HgiMipInfo& mipInfo = mipInfos[mip];

            size_t rowPitch, slicePitch;
            _GetSurfaceInfo(mipInfo.dimensions[0],
                            mipInfo.dimensions[1],
                            HgiDXConversions::GetTextureFormat(desc.format),
                            &slicePitch,
                            &rowPitch,
                            nullptr);

            D3D12_SUBRESOURCE_DATA subresourceData;
            subresourceData.pData = pInitialData + mipInfo.byteOffset;
            subresourceData.RowPitch = rowPitch;
            subresourceData.SlicePitch = slicePitch;

            subresourceArr.push_back(subresourceData);
         }

         UpdateSubresources(pCmdList, pTxResource, _dxIntermediaryBuffer.Get(), 0, 0, subresourceArr.size(), subresourceArr.data());
      }
      else
      {
         TF_RUNTIME_ERROR("Invalid texture resource.");
      }
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

   if (mipLevel != 0)
   {
      TF_RUNTIME_ERROR("MipLevel is not properly implemented yet during readback data.");
   }

   if (nullptr != pCmdList)
   {
      ID3D12Resource* pTxResource = GetResource();
      D3D12_RESOURCE_STATES& resState = GetResourceState();

      if (nullptr != pTxResource)
      {
         if (resState != D3D12_RESOURCE_STATE_COPY_SOURCE)
         {
            const D3D12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(pTxResource,
                                                                                        resState,
                                                                                        D3D12_RESOURCE_STATE_COPY_SOURCE);

            pCmdList->ResourceBarrier(1, &barrier);
            resState = D3D12_RESOURCE_STATE_COPY_SOURCE;
         }

         _InitReadbackBuffer();

         //
         // TODO: let's get a copy of the whole thing going 
         // before I wonder why I get only "sourceOffset" instead of a "source box"
         CD3DX12_TEXTURE_COPY_LOCATION src(pTxResource, 0);

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

         D3D12_RESOURCE_DESC dxDesc = pTxResource->GetDesc();

         //
         // eliminate the pitch padding now...
         byte* sptr = (byte*)pMappedMemory;
         byte* dptr = (byte*)cpuDestinationBuffer + destinationByteOffset;
         size_t dstPos = destinationByteOffset;
         size_t dstRowSize = dxDesc.Width * HgiGetDataSizeOfFormat(GetDescriptor().format);
         size_t srcRowSize = _copyDestLocation.PlacedFootprint.Footprint.RowPitch;
         size_t dstSize = destinationBufferByteSize;
         for (uint32_t idx = 0; idx < dxDesc.Height; idx++)
         {
            dstPos += dstRowSize;
            dstSize -= dstPos;

            if (dstPos <= destinationBufferByteSize)
            {
               memcpy_s(dptr, dstSize, sptr, dstRowSize);
               dptr += dstRowSize;
               sptr += srcRowSize;
            }
            else
               TF_WARN("Not enough room in buffer to copy the texture data");
         }

         _readbackBuffer->Unmap(0, &writeRange);
      }
      else
      {
         TF_RUNTIME_ERROR("Invalid texture resource.");
      }
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
   if (nullptr == pOtherMSTx)
   {
      TF_RUNTIME_ERROR("Invalid texture resource.");
      return;
   }

   ID3D12Resource* pResThis = GetResource();
   ID3D12Resource* pResOther = pOtherMSTx->GetResource();

   D3D12_RESOURCE_STATES& resStateThis = GetResourceState();
   D3D12_RESOURCE_STATES& resStateOther = pOtherMSTx->GetResourceState();

   if (resStateOther != D3D12_RESOURCE_STATE_RESOLVE_SOURCE)
   {
      const D3D12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(pResOther,
                                                                                  resStateOther,
                                                                                  D3D12_RESOURCE_STATE_RESOLVE_SOURCE);
      pCmdList->ResourceBarrier(1, &barrier);
      resStateOther = D3D12_RESOURCE_STATE_RESOLVE_SOURCE;
   }

   if (resStateThis != D3D12_RESOURCE_STATE_RESOLVE_DEST)
   {
      const D3D12_RESOURCE_BARRIER barrier2 = CD3DX12_RESOURCE_BARRIER::Transition(pResThis,
                                                                                   resStateThis,
                                                                                   D3D12_RESOURCE_STATE_RESOLVE_DEST);
      pCmdList->ResourceBarrier(1, &barrier2);
      resStateThis = D3D12_RESOURCE_STATE_RESOLVE_DEST;
   }

   pCmdList->ResolveSubresource(pResThis, 0, pResOther, 0, pOtherMSTx->GetResourceFormat());
}

void 
HgiDXTexture::UpdateResourceState(ID3D12GraphicsCommandList* pCmdList, D3D12_RESOURCE_STATES newResState)
{
   ID3D12Resource* pTxResource = GetResource();
   D3D12_RESOURCE_STATES& resState = GetResourceState();

   if (resState != newResState)
   {
      const D3D12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(pTxResource,
                                                                                    resState,
                                                                                    newResState);
      pCmdList->ResourceBarrier(1, &barrier);

      resState = newResState;
   }
}

void
HgiDXTexture::_GetDxResourceDesc(const HgiTextureDesc& hgiDesc, D3D12_RESOURCE_DESC& dxTxDesc)
{
   dxTxDesc.Width = hgiDesc.dimensions[0];
   dxTxDesc.Height = hgiDesc.dimensions[1];
   dxTxDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;

   dxTxDesc.Format = HgiDXConversions::GetTextureFormat(hgiDesc.format);

   //
   // Maybe this does not belong here
   // Seems hgi would load an image from disk and initial data will only have info about the most detailed mip
   // but it expects the memory to be allocated for other mips to be generated later
   // So checking the "pixelsByteSize" at this time will always result in a single mip value 
   // which is not what we want
   //const std::vector<HgiMipInfo> mipInfos =
   //   HgiGetMipInfos(
   //      hgiDesc.format,
   //      hgiDesc.dimensions,
   //      hgiDesc.layerCount,
   //      hgiDesc.pixelsByteSize);
   //const size_t mipLevels = std::min(mipInfos.size(), size_t(hgiDesc.mipLevels));

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

   if (desc.usage & HgiTextureUsageBitsShaderWrite)
      ret |= D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;

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
HgiDXTexture::_GetInitialResourceStates(HgiTextureDesc const& desc)
{
   if (desc.usage & HgiTextureUsageBitsColorTarget)
      return D3D12_RESOURCE_STATE_RENDER_TARGET;
   else if (desc.usage & HgiTextureUsageBitsDepthTarget)
      return D3D12_RESOURCE_STATE_DEPTH_WRITE;
   else
      return D3D12_RESOURCE_STATE_COPY_DEST;
}

//--------------------------------------------------------------------------------------
// Get surface information for a particular format
//--------------------------------------------------------------------------------------
void 
HgiDXTexture::_GetSurfaceInfo(size_t width,
                              size_t height,
                              DXGI_FORMAT fmt,
                              size_t* outNumBytes,
                              size_t* outRowBytes,
                              size_t* outNumRows)
{
   size_t numBytes = 0;
   size_t rowBytes = 0;
   size_t numRows = 0;

   bool bc = false;
   bool packed = false;
   bool planar = false;
   size_t bpe = 0;
   switch (fmt)
   {
   case DXGI_FORMAT_BC1_TYPELESS:
   case DXGI_FORMAT_BC1_UNORM:
   case DXGI_FORMAT_BC1_UNORM_SRGB:
   case DXGI_FORMAT_BC4_TYPELESS:
   case DXGI_FORMAT_BC4_UNORM:
   case DXGI_FORMAT_BC4_SNORM:
      bc = true;
      bpe = 8;
      break;

   case DXGI_FORMAT_BC2_TYPELESS:
   case DXGI_FORMAT_BC2_UNORM:
   case DXGI_FORMAT_BC2_UNORM_SRGB:
   case DXGI_FORMAT_BC3_TYPELESS:
   case DXGI_FORMAT_BC3_UNORM:
   case DXGI_FORMAT_BC3_UNORM_SRGB:
   case DXGI_FORMAT_BC5_TYPELESS:
   case DXGI_FORMAT_BC5_UNORM:
   case DXGI_FORMAT_BC5_SNORM:
   case DXGI_FORMAT_BC6H_TYPELESS:
   case DXGI_FORMAT_BC6H_UF16:
   case DXGI_FORMAT_BC6H_SF16:
   case DXGI_FORMAT_BC7_TYPELESS:
   case DXGI_FORMAT_BC7_UNORM:
   case DXGI_FORMAT_BC7_UNORM_SRGB:
      bc = true;
      bpe = 16;
      break;

   case DXGI_FORMAT_R8G8_B8G8_UNORM:
   case DXGI_FORMAT_G8R8_G8B8_UNORM:
   case DXGI_FORMAT_YUY2:
      packed = true;
      bpe = 4;
      break;

   case DXGI_FORMAT_Y210:
   case DXGI_FORMAT_Y216:
      packed = true;
      bpe = 8;
      break;

   case DXGI_FORMAT_NV12:
   case DXGI_FORMAT_420_OPAQUE:
      planar = true;
      bpe = 2;
      break;

   case DXGI_FORMAT_P010:
   case DXGI_FORMAT_P016:
      planar = true;
      bpe = 4;
      break;
   }

   if (bc)
   {
      size_t numBlocksWide = 0;
      if (width > 0)
      {
         numBlocksWide = std::max<size_t>(1, (width + 3) / 4);
      }
      size_t numBlocksHigh = 0;
      if (height > 0)
      {
         numBlocksHigh = std::max<size_t>(1, (height + 3) / 4);
      }
      rowBytes = numBlocksWide * bpe;
      numRows = numBlocksHigh;
      numBytes = rowBytes * numBlocksHigh;
   }
   else if (packed)
   {
      rowBytes = ((width + 1) >> 1) * bpe;
      numRows = height;
      numBytes = rowBytes * height;
   }
   else if (fmt == DXGI_FORMAT_NV11)
   {
      rowBytes = ((width + 3) >> 2) * 4;
      numRows = height * 2; // Direct3D makes this simplifying assumption, although it is larger than the 4:1:1 data
      numBytes = rowBytes * numRows;
   }
   else if (planar)
   {
      rowBytes = ((width + 1) >> 1) * bpe;
      numBytes = (rowBytes * height) + ((rowBytes * height + 1) >> 1);
      numRows = height + ((height + 1) >> 1);
   }
   else
   {
      size_t bpp = _BitsPerPixel(fmt);
      rowBytes = (width * bpp + 7) / 8; // round up to nearest byte
      numRows = height;
      numBytes = rowBytes * height;
   }

   if (outNumBytes)
   {
      *outNumBytes = numBytes;
   }
   if (outRowBytes)
   {
      *outRowBytes = rowBytes;
   }
   if (outNumRows)
   {
      *outNumRows = numRows;
   }
}

size_t 
HgiDXTexture::_BitsPerPixel(DXGI_FORMAT fmt)
{
   switch (static_cast<int>(fmt))
   {
   case DXGI_FORMAT_R32G32B32A32_TYPELESS:
   case DXGI_FORMAT_R32G32B32A32_FLOAT:
   case DXGI_FORMAT_R32G32B32A32_UINT:
   case DXGI_FORMAT_R32G32B32A32_SINT:
      return 128;

   case DXGI_FORMAT_R32G32B32_TYPELESS:
   case DXGI_FORMAT_R32G32B32_FLOAT:
   case DXGI_FORMAT_R32G32B32_UINT:
   case DXGI_FORMAT_R32G32B32_SINT:
      return 96;

   case DXGI_FORMAT_R16G16B16A16_TYPELESS:
   case DXGI_FORMAT_R16G16B16A16_FLOAT:
   case DXGI_FORMAT_R16G16B16A16_UNORM:
   case DXGI_FORMAT_R16G16B16A16_UINT:
   case DXGI_FORMAT_R16G16B16A16_SNORM:
   case DXGI_FORMAT_R16G16B16A16_SINT:
   case DXGI_FORMAT_R32G32_TYPELESS:
   case DXGI_FORMAT_R32G32_FLOAT:
   case DXGI_FORMAT_R32G32_UINT:
   case DXGI_FORMAT_R32G32_SINT:
   case DXGI_FORMAT_R32G8X24_TYPELESS:
   case DXGI_FORMAT_D32_FLOAT_S8X24_UINT:
   case DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS:
   case DXGI_FORMAT_X32_TYPELESS_G8X24_UINT:
   case DXGI_FORMAT_Y416:
   case DXGI_FORMAT_Y210:
   case DXGI_FORMAT_Y216:
      return 64;

   case DXGI_FORMAT_R10G10B10A2_TYPELESS:
   case DXGI_FORMAT_R10G10B10A2_UNORM:
   case DXGI_FORMAT_R10G10B10A2_UINT:
   case DXGI_FORMAT_R11G11B10_FLOAT:
   case DXGI_FORMAT_R8G8B8A8_TYPELESS:
   case DXGI_FORMAT_R8G8B8A8_UNORM:
   case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB:
   case DXGI_FORMAT_R8G8B8A8_UINT:
   case DXGI_FORMAT_R8G8B8A8_SNORM:
   case DXGI_FORMAT_R8G8B8A8_SINT:
   case DXGI_FORMAT_R16G16_TYPELESS:
   case DXGI_FORMAT_R16G16_FLOAT:
   case DXGI_FORMAT_R16G16_UNORM:
   case DXGI_FORMAT_R16G16_UINT:
   case DXGI_FORMAT_R16G16_SNORM:
   case DXGI_FORMAT_R16G16_SINT:
   case DXGI_FORMAT_R32_TYPELESS:
   case DXGI_FORMAT_D32_FLOAT:
   case DXGI_FORMAT_R32_FLOAT:
   case DXGI_FORMAT_R32_UINT:
   case DXGI_FORMAT_R32_SINT:
   case DXGI_FORMAT_R24G8_TYPELESS:
   case DXGI_FORMAT_D24_UNORM_S8_UINT:
   case DXGI_FORMAT_R24_UNORM_X8_TYPELESS:
   case DXGI_FORMAT_X24_TYPELESS_G8_UINT:
   case DXGI_FORMAT_R9G9B9E5_SHAREDEXP:
   case DXGI_FORMAT_R8G8_B8G8_UNORM:
   case DXGI_FORMAT_G8R8_G8B8_UNORM:
   case DXGI_FORMAT_B8G8R8A8_UNORM:
   case DXGI_FORMAT_B8G8R8X8_UNORM:
   case DXGI_FORMAT_R10G10B10_XR_BIAS_A2_UNORM:
   case DXGI_FORMAT_B8G8R8A8_TYPELESS:
   case DXGI_FORMAT_B8G8R8A8_UNORM_SRGB:
   case DXGI_FORMAT_B8G8R8X8_TYPELESS:
   case DXGI_FORMAT_B8G8R8X8_UNORM_SRGB:
   case DXGI_FORMAT_AYUV:
   case DXGI_FORMAT_Y410:
   case DXGI_FORMAT_YUY2:
   //case XBOX_DXGI_FORMAT_R10G10B10_7E3_A2_FLOAT:
   //case XBOX_DXGI_FORMAT_R10G10B10_6E4_A2_FLOAT:
   //case XBOX_DXGI_FORMAT_R10G10B10_SNORM_A2_UNORM:
      return 32;

   case DXGI_FORMAT_P010:
   case DXGI_FORMAT_P016:
   //case XBOX_DXGI_FORMAT_D16_UNORM_S8_UINT:
   //case XBOX_DXGI_FORMAT_R16_UNORM_X8_TYPELESS:
   //case XBOX_DXGI_FORMAT_X16_TYPELESS_G8_UINT:
   //case WIN10_DXGI_FORMAT_V408:
      return 24;

   case DXGI_FORMAT_R8G8_TYPELESS:
   case DXGI_FORMAT_R8G8_UNORM:
   case DXGI_FORMAT_R8G8_UINT:
   case DXGI_FORMAT_R8G8_SNORM:
   case DXGI_FORMAT_R8G8_SINT:
   case DXGI_FORMAT_R16_TYPELESS:
   case DXGI_FORMAT_R16_FLOAT:
   case DXGI_FORMAT_D16_UNORM:
   case DXGI_FORMAT_R16_UNORM:
   case DXGI_FORMAT_R16_UINT:
   case DXGI_FORMAT_R16_SNORM:
   case DXGI_FORMAT_R16_SINT:
   case DXGI_FORMAT_B5G6R5_UNORM:
   case DXGI_FORMAT_B5G5R5A1_UNORM:
   case DXGI_FORMAT_A8P8:
   case DXGI_FORMAT_B4G4R4A4_UNORM:
   //case WIN10_DXGI_FORMAT_P208:
   //case WIN10_DXGI_FORMAT_V208:
      return 16;

   case DXGI_FORMAT_NV12:
   case DXGI_FORMAT_420_OPAQUE:
   case DXGI_FORMAT_NV11:
      return 12;

   case DXGI_FORMAT_R8_TYPELESS:
   case DXGI_FORMAT_R8_UNORM:
   case DXGI_FORMAT_R8_UINT:
   case DXGI_FORMAT_R8_SNORM:
   case DXGI_FORMAT_R8_SINT:
   case DXGI_FORMAT_A8_UNORM:
   case DXGI_FORMAT_AI44:
   case DXGI_FORMAT_IA44:
   case DXGI_FORMAT_P8:
   //case XBOX_DXGI_FORMAT_R4G4_UNORM:
      return 8;

   case DXGI_FORMAT_R1_UNORM:
      return 1;

   case DXGI_FORMAT_BC1_TYPELESS:
   case DXGI_FORMAT_BC1_UNORM:
   case DXGI_FORMAT_BC1_UNORM_SRGB:
   case DXGI_FORMAT_BC4_TYPELESS:
   case DXGI_FORMAT_BC4_UNORM:
   case DXGI_FORMAT_BC4_SNORM:
      return 4;

   case DXGI_FORMAT_BC2_TYPELESS:
   case DXGI_FORMAT_BC2_UNORM:
   case DXGI_FORMAT_BC2_UNORM_SRGB:
   case DXGI_FORMAT_BC3_TYPELESS:
   case DXGI_FORMAT_BC3_UNORM:
   case DXGI_FORMAT_BC3_UNORM_SRGB:
   case DXGI_FORMAT_BC5_TYPELESS:
   case DXGI_FORMAT_BC5_UNORM:
   case DXGI_FORMAT_BC5_SNORM:
   case DXGI_FORMAT_BC6H_TYPELESS:
   case DXGI_FORMAT_BC6H_UF16:
   case DXGI_FORMAT_BC6H_SF16:
   case DXGI_FORMAT_BC7_TYPELESS:
   case DXGI_FORMAT_BC7_UNORM:
   case DXGI_FORMAT_BC7_UNORM_SRGB:
      return 8;

   default:
      return 0;
   }
}

PXR_NAMESPACE_CLOSE_SCOPE
