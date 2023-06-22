
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

PXR_NAMESPACE_OPEN_SCOPE

class HgiDXCapabilities;
class HgiDXTexture;

/// \class HgiDXDevice
///
/// DirectX implementation of GPU device.
///
class HgiDXDevice final
{
public:

   enum eCommandType
   {
      kGraphics,
      kCompute,
      kCopy
   };

   HGIDX_API
   HgiDXDevice();

   HGIDX_API
   ~HgiDXDevice();

   /// Returns the device capablities / features it supports.
   HGIDX_API
   HgiDXCapabilities const& GetDeviceCapabilities() const;

   /// Wait for all queued up commands to have been processed on device.
   /// This should ideally never be used as it creates very big stalls, but
   /// is useful for unit testing.
   HGIDX_API
   void WaitForIdle();

   HGIDX_API
   struct ID3D12Device* GetDevice();

   HGIDX_API
   struct IDXGIFactory4* GetFactory();

   HGIDX_API
   struct ID3D12CommandQueue* GetCommandQueue(eCommandType type);

   HGIDX_API
   struct ID3D12GraphicsCommandList* GetCommandList(eCommandType type);

   HGIDX_API
   void SubmitCommandList(eCommandType type);

   HGIDX_API
   struct ID3D12DescriptorHeap* GetRTVDescriptorHeap();

   HGIDX_API
   UINT GetRTVDescriptorHeapIncrementSize();
   
   HGIDX_API
   UINT GetDSVDescriptorHeapIncrementSize();

   HGIDX_API
   struct ID3D12DescriptorHeap* GetDSVDescriptorHeap();

   HGIDX_API
   const DXGI_ADAPTER_DESC1& GetAdapterInfo();

   HGIDX_API
   D3D12_CPU_DESCRIPTOR_HANDLE CreateRenderTargetView(ID3D12Resource* pRes, UINT nTexIdx);

   HGIDX_API
   D3D12_CPU_DESCRIPTOR_HANDLE CreateDepthStencilView(ID3D12Resource* pRes, UINT nTexIdx);

private:
   HgiDXDevice& operator=(const HgiDXDevice&) = delete;
   HgiDXDevice(const HgiDXDevice&) = delete;

   void _GetAdapter(IDXGIAdapter1** ppAdapter);
   void _InitCommandLists();
   void _WaitForCommandListToExecute(eCommandType type);

private:
   std::unique_ptr<HgiDXCapabilities> _capabilities;
   DXGI_ADAPTER_DESC1 _adapterDesc;

   D3D_FEATURE_LEVEL _d3dMinFeatureLevel;
   DWORD _dxgiFactoryFlags;
   Microsoft::WRL::ComPtr<IDXGIFactory4> _dxgiFactory;
   Microsoft::WRL::ComPtr<ID3D12Device> _dxDevice;

   Microsoft::WRL::ComPtr<ID3D12CommandQueue> _commandQueueGraphics;
   Microsoft::WRL::ComPtr<ID3D12CommandQueue> _commandQueueCompute;
   Microsoft::WRL::ComPtr<ID3D12CommandQueue> _commandQueueCopy;

   Microsoft::WRL::ComPtr<ID3D12CommandAllocator> _commandAllocatorGraphics;
   Microsoft::WRL::ComPtr<ID3D12CommandAllocator> _commandAllocatorCompute;
   Microsoft::WRL::ComPtr<ID3D12CommandAllocator> _commandAllocatorCopy;

   Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> _commandListGraphics;
   Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> _commandListCompute;
   Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> _commandListCopy;

   Microsoft::WRL::ComPtr<ID3D12Fence> _fenceCommandListGraphics;
   Microsoft::WRL::Wrappers::Event _hFenceGraphicsSignalEvent;
   UINT _fenceValueGraphics;

   Microsoft::WRL::ComPtr<ID3D12Fence> _fenceCommandListCompute;
   Microsoft::WRL::Wrappers::Event _hFenceComputeSignalEvent;
   UINT _fenceValueCompute;

   Microsoft::WRL::ComPtr<ID3D12Fence> _fenceCommandListCopy;
   Microsoft::WRL::Wrappers::Event _hFenceCopySignalEvent;
   UINT _fenceValueCopy;

   bool _bGraphicsCmdListClosed;
   bool _bComputeCmdListClosed;
   bool _bCopyCmdListClosed;

   // Direct3D rendering objects.
   UINT _rtvDescriptorHeapIncrementSize;
   UINT _dsvDescriptorHeapIncrementSize;
   Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> _rtvDescriptorHeap;
   Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> _dsvDescriptorHeap;
};


PXR_NAMESPACE_CLOSE_SCOPE
