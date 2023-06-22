
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
#include "pxr/imaging/hgiDX/conversions.h"
#include "pxr/imaging/hgiDX/device.h"
#include "pxr/imaging/hgiDX/hgi.h"

PXR_NAMESPACE_OPEN_SCOPE

namespace {
   //
   // This is a (temporary, hacky) debug helper
   // TODO: either remove it completely (together with the helper methods)
   // or implement something more elegant, flexible, easy to use
   static HgiDXBuffer* _spBuff2Watch = nullptr;
}

HgiDXBuffer::HgiDXBuffer(  HgiDXDevice* device,
                           HgiBufferDesc const& desc)
   : HgiBuffer(desc)
   , _device(device)
   , _pStagingBuffer(nullptr)
{
   //
   // The way Storm works is: 
   //   - build one large buffer which is meant to get all the data from CPU
   //   - build many small buffers according to what is promised in the shaders
   //   - add many "BlitCommands" to copy data from the large buffer to the small ones GPU->GPU

   if (desc.byteSize == 0) {
      TF_CODING_ERROR("The size of buffer [%p] is zero.", this);
      return;
   }

   // Create a committed resource for the GPU resource in a default heap.
   
   //
   // because Storm does not want to tell us with accuracy what it will use the buffers for
   // I'll setup all my buffers as pure GPU for now
   // and when a CPU copy operation is requested, I'll build a new intermediary buffer on the fly and deal with it
   D3D12_HEAP_TYPE ht = D3D12_HEAP_TYPE_DEFAULT;
   _bufResState = D3D12_RESOURCE_STATE_COPY_DEST;

   CD3DX12_HEAP_PROPERTIES hpDef(ht);

   //
   // crazy experiment:
   // I'll allocate 10% extra in each buffer to see if the draw indirect isse is related to the 
   // allocated memory size
   size_t allocSize = desc.byteSize;

   CD3DX12_RESOURCE_DESC buffDesc(CD3DX12_RESOURCE_DESC::Buffer(allocSize, D3D12_RESOURCE_FLAG_NONE));
   HRESULT hr = _device->GetDevice()->CreateCommittedResource(&hpDef,
                                                              D3D12_HEAP_FLAG_NONE,
                                                              &buffDesc,
                                                              _bufResState,
                                                              nullptr, // TODO: DirectX complains about not setting this. Could I set it later?
                                                              IID_PPV_ARGS(_dxBuffer.ReleaseAndGetAddressOf()));
   CheckResult(hr, "Failed to create required buffer.");

   if (!desc.debugName.empty())
   {
      _strName = HgiDXConversions::GetWideString(desc.debugName);
      _dxBuffer->SetName(_strName.c_str());
   }

   //
   // buffers debug code
#ifdef DEBUG_BUFFERS
   std::stringstream buffer;
   buffer << "Info: Allocated new buffer: " << _dxBuffer.Get()
      << ",name: " << desc.debugName.c_str()
      << ",size: " << desc.byteSize
      << ",GPU address: " << GetGPUVirtualAddress()
      << ",called on thread : " << std::this_thread::get_id();
   TF_STATUS(buffer.str());
#endif
   
   // If we already have initial data...
   if (nullptr != desc.initialData)
      UpdateData(desc.initialData, desc.byteSize, 0, 0);
}

HgiDXBuffer::~HgiDXBuffer()
{
   //
   // buffers debug code
#ifdef DEBUG_BUFFERS
   std::stringstream buffer;
   buffer << "Info: Freeing buffer: " << _dxBuffer.Get() 
      << ",name: " << _descriptor.debugName.c_str() 
      << ",GPU address: " << GetGPUVirtualAddress()
      << ",called on thread : " << std::this_thread::get_id();
   TF_STATUS(buffer.str());
#endif

   _UnmapStagingBuffer();
}

size_t
HgiDXBuffer::GetByteSizeOfResource() const
{
   return _descriptor.byteSize;
}

D3D12_GPU_VIRTUAL_ADDRESS 
HgiDXBuffer::GetGPUVirtualAddress() const
{
   return _dxBuffer->GetGPUVirtualAddress();
}

uint64_t
HgiDXBuffer::GetRawResource() const
{
   //
   // TODO: I am not sure what this is supposed to mean and I never saw it called..
   return 0;
}

void 
HgiDXBuffer::_InitStagingBuffer()
{
   if (nullptr == _pStagingBuffer)
   {
      //
      // I'll just allocate a normal, separated CPU buffer for this right now
      _pStagingBuffer = new uint8_t[_descriptor.byteSize];
   }
}

void 
HgiDXBuffer::_UnmapStagingBuffer()
{
   if (nullptr == _pStagingBuffer)
   {
      delete[] _pStagingBuffer;
      _pStagingBuffer = nullptr;
   }
}

void*
HgiDXBuffer::GetCPUStagingAddress()
{
   _InitStagingBuffer();
   return _pStagingBuffer;
}

bool
HgiDXBuffer::IsCPUStagingAddress(const void* address) const
{
   return (address == _pStagingBuffer);
}

ID3D12Resource* 
HgiDXBuffer::GetResource()
{
   return _dxBuffer.Get();
}

void 
HgiDXBuffer::_BuildIntermediaryBuffer()
{
   if (nullptr == _dxIntermediaryBuffer)
   {
      D3D12_HEAP_TYPE ht = D3D12_HEAP_TYPE_UPLOAD;
      D3D12_RESOURCE_STATES rs = D3D12_RESOURCE_STATE_GENERIC_READ;

      CD3DX12_HEAP_PROPERTIES hpDef(ht);
      CD3DX12_RESOURCE_DESC buffDesc(CD3DX12_RESOURCE_DESC::Buffer(_descriptor.byteSize, D3D12_RESOURCE_FLAG_NONE));
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
HgiDXBuffer::UpdateData(const void* pData, size_t dataSize, size_t sourceByteOffset, size_t destinationByteOffset)
{
   //
   // I will do the copies on the graphics queue - the copy queue cannot transition resources...
   //ID3D12GraphicsCommandList* pCmdList = _device->GetCommandList(HgiDXDevice::eCommandType::kCopy);
   ID3D12GraphicsCommandList* pCmdList = _device->GetCommandList(HgiDXDevice::eCommandType::kGraphics);

   //
   // buffers debug code
#ifdef DEBUG_BUFFERS
   std::stringstream buffer;
   buffer << "Info: Updating buffer: " << _dxBuffer.Get() 
      << ",name: " << _descriptor.debugName.c_str() 
      << ",GPU address: " << GetGPUVirtualAddress()
      << ",size:" << dataSize 
      << ",offset: " << destinationByteOffset
      << ",called on thread : " << std::this_thread::get_id();
   TF_STATUS(buffer.str());
#endif

   if (nullptr != pCmdList)
   {
      _BuildIntermediaryBuffer();

      //
      // transition resource / target into "copy" mode
      if (_bufResState != D3D12_RESOURCE_STATE_COPY_DEST)
      {
         const D3D12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(_dxBuffer.Get(),
                                                                                     _bufResState,
                                                                                     D3D12_RESOURCE_STATE_COPY_DEST);

         pCmdList->ResourceBarrier(1, &barrier);
         _bufResState = D3D12_RESOURCE_STATE_COPY_DEST;
      }

      const uint8_t* src = static_cast<const uint8_t*>(pData);
      src += sourceByteOffset;

      D3D12_SUBRESOURCE_DATA subresourceData = {};
      subresourceData.pData = src;
      subresourceData.RowPitch = dataSize;
      subresourceData.SlicePitch = subresourceData.RowPitch;

      UpdateSubresources(pCmdList,
                         _dxBuffer.Get(), _dxIntermediaryBuffer.Get(),
                         destinationByteOffset, 0, 1, &subresourceData);
   }
   else
      TF_WARN("Cannot get valid command list. Failed to set buffer data.");

}

void 
HgiDXBuffer::UpdateData(HgiDXBuffer* pOtherGPUBuff, size_t dataSize, size_t sourceByteOffset, size_t destinationByteOffset)
{
   //
   // I will do the copies on the graphics queue - the copy queue cannot transition resources...
   //ID3D12GraphicsCommandList* pCmdList = _device->GetCommandList(HgiDXDevice::eCommandType::kCopy);
   ID3D12GraphicsCommandList* pCmdList = _device->GetCommandList(HgiDXDevice::eCommandType::kGraphics);

   //
   // buffers debug code
#ifdef DEBUG_BUFFERS
   std::stringstream buffer;
   buffer << "Info: Updating buffer: " << _dxBuffer.Get() 
      << ",name: " << _descriptor.debugName.c_str() 
      << ",GPU address: " << GetGPUVirtualAddress()
      << ",size: " << dataSize
      << ",offset: " << destinationByteOffset
      << ",called on thread : " << std::this_thread::get_id();
   TF_STATUS(buffer.str());
#endif

   if (nullptr != pOtherGPUBuff &&  nullptr != pCmdList)
   {
      //
      // transition source into "copy from" mode
      if (pOtherGPUBuff->_bufResState != D3D12_RESOURCE_STATE_COPY_SOURCE)
      {
         const D3D12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(pOtherGPUBuff->_dxBuffer.Get(),
                                                                                     pOtherGPUBuff->_bufResState,
                                                                                     D3D12_RESOURCE_STATE_COPY_SOURCE);

         pCmdList->ResourceBarrier(1, &barrier);
         pOtherGPUBuff->_bufResState = D3D12_RESOURCE_STATE_COPY_SOURCE;
      }


      //
      // transition destination into "copy to" mode
      if (_bufResState != D3D12_RESOURCE_STATE_COPY_DEST)
      {
         const D3D12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(_dxBuffer.Get(),
                                                                                     _bufResState,
                                                                                     D3D12_RESOURCE_STATE_COPY_DEST);

         pCmdList->ResourceBarrier(1, &barrier);
         _bufResState = D3D12_RESOURCE_STATE_COPY_DEST;
      }

      pCmdList->CopyBufferRegion(_dxBuffer.Get(), destinationByteOffset, pOtherGPUBuff->_dxBuffer.Get(), sourceByteOffset, dataSize);
   }
   else
      TF_WARN("Cannot get valid command list. Failed to set buffer data.");
}

void 
HgiDXBuffer::UpdateResourceState(ID3D12GraphicsCommandList* /*pCmdList*/, D3D12_RESOURCE_STATES newResState)
{
   if (_bufResState != newResState)
   {
      //
      // trying to always transition resources on the Graphics queue, even when we want to use them
      // on another queue, because only the graphics queue can handle trasitioning between any 2 states
      ID3D12GraphicsCommandList* pCmdList = _device->GetCommandList(HgiDXDevice::eCommandType::kGraphics);

      const D3D12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(_dxBuffer.Get(),
                                                                                  _bufResState,
                                                                                  newResState);
      pCmdList->ResourceBarrier(1, &barrier);

      _bufResState = newResState;
   }
}

//
// This is a debug method meant to be used to "save" a buffer for later investigation
void
HgiDXBuffer::SetWatchBuffer(HgiDXBuffer* pBuff2Watch)
{
   _spBuff2Watch = pBuff2Watch;
}

//
// This is a debug method meant to be used to retrieve a previously saved buffer for investigation
HgiDXBuffer*
HgiDXBuffer::GetWatchBuffer()
{
   return _spBuff2Watch;
}

//
// This is a debug method that allows checking the contents of a specific buffer in GPU memory
void
HgiDXBuffer::InspectBufferContents()
{
   //
   // The simplest way I could find to inspect a buffer in debug mode was to 
   // allocate a new buffer accessible by CPU and copy my interest data there
   // 
   // build a readback buffer
   Microsoft::WRL::ComPtr<ID3D12Resource> readbackBuffer;
   D3D12_HEAP_TYPE ht = D3D12_HEAP_TYPE_READBACK;
   D3D12_RESOURCE_STATES rs = D3D12_RESOURCE_STATE_COPY_DEST;

   CD3DX12_HEAP_PROPERTIES hpDef(ht);
   CD3DX12_RESOURCE_DESC buffDesc(CD3DX12_RESOURCE_DESC::Buffer(_descriptor.byteSize));
   HRESULT hr = _device->GetDevice()->CreateCommittedResource(&hpDef,
                                                              D3D12_HEAP_FLAG_NONE,
                                                              &buffDesc,
                                                              rs,
                                                              nullptr,
                                                              IID_PPV_ARGS(readbackBuffer.ReleaseAndGetAddressOf()));
   CheckResult(hr, "Failed to create readback buffer.");

   ID3D12GraphicsCommandList* pCmdList = _device->GetCommandList(HgiDXDevice::eCommandType::kGraphics);
   UpdateResourceState(pCmdList, D3D12_RESOURCE_STATE_COPY_SOURCE);

   //
   // copy GPU -> GPU (readable by CPU)
   pCmdList->CopyResource(readbackBuffer.Get(), _dxBuffer.Get());

   _device->SubmitCommandList(HgiDXDevice::eCommandType::kGraphics);

   // 
   // Map readback buffer to something I can understand.
   // The code below assumes that the GPU wrote FLOAT3s to the buffer.
   D3D12_RANGE readbackBufferRange{ 0, _descriptor.byteSize };
   GfVec3f* pReadbackBufferData{};
   CheckResult(
      readbackBuffer->Map
      (
         0,
         &readbackBufferRange,
         reinterpret_cast<void**>(&pReadbackBufferData)
      ), "Failed to map buffer data to output");


   D3D12_RANGE emptyRange{ 0, 0 };
   readbackBuffer->Unmap(0, &emptyRange);
}

PXR_NAMESPACE_CLOSE_SCOPE
