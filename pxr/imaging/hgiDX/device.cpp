
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
#include "pxr/imaging/hgiDX/capabilities.h"
#include "pxr/imaging/hgiDX/device.h"
#include "pxr/imaging/hgiDX/hgi.h"
#include "pxr/imaging/hgiDX/texture.h"

#include "pxr/base/tf/diagnostic.h"
#include "pxr/base/tf/envSetting.h"
#include "pxr/base/tf/getEnv.h"

//#ifdef _DEBUG
#include <dxgidebug.h>
//#endif

PXR_NAMESPACE_OPEN_SCOPE

TF_DEFINE_ENV_SETTING(HGI_DX_FORCE_WARP, false, "Force WARP (DirectX Software Rendering).");

// TODO: I need to understand better what this number means and how it should be used
// and also what are the expected needs of HdSt...
//
// An additional complication is that I would like to reserve one of these for the 
// final step: present to wnd or offscreen when I want to render again 
static const uint32_t s_nMaxRenderTargetDescs = 6;

HgiDXDevice::HgiDXDevice()
   : _d3dMinFeatureLevel(D3D_FEATURE_LEVEL_11_0)
   , _bGraphicsCmdListClosed(false)
   , _bComputeCmdListClosed(false)
   , _bCopyCmdListClosed(false)
   , _fenceValueGraphics(0)
   , _fenceValueCompute(0)
   , _fenceValueCopy(0)
{
   bool bHookDebug = false;

#if defined(_DEBUG)
   bHookDebug = true;
#endif

   if (!bHookDebug)
      bHookDebug = (TfGetenvInt("HGI_ENABLE_DX_DEBUG_SHADERS", 0) > 0);

   if (bHookDebug)
   {
      // Enable the debug layer (requires the Graphics Tools "optional feature").
      //
      // NOTE: Enabling the debug layer after device creation will invalidate the active device.
      {
         ComPtr<ID3D12Debug> debugController;
         if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(debugController.GetAddressOf()))))
         {
            debugController->EnableDebugLayer();

            ID3D12Debug5* pDbg5 = nullptr;
            debugController->QueryInterface(&pDbg5);
            if (nullptr != pDbg5)
            {
               pDbg5->SetEnableAutoName(TRUE);
               pDbg5->Release();
            }
         }
         else
         {
            OutputDebugStringA("WARNING: Direct3D Debug Device is not available\n");
         }

         ComPtr<IDXGIInfoQueue> dxgiInfoQueue;
         if (SUCCEEDED(DXGIGetDebugInterface1(0, IID_PPV_ARGS(dxgiInfoQueue.GetAddressOf()))))
         {
            _dxgiFactoryFlags = DXGI_CREATE_FACTORY_DEBUG;

            dxgiInfoQueue->SetBreakOnSeverity(DXGI_DEBUG_ALL, DXGI_INFO_QUEUE_MESSAGE_SEVERITY_ERROR, true);
            dxgiInfoQueue->SetBreakOnSeverity(DXGI_DEBUG_ALL, DXGI_INFO_QUEUE_MESSAGE_SEVERITY_CORRUPTION, true);

            DXGI_INFO_QUEUE_MESSAGE_ID hide[] =
            {
                80 /* IDXGISwapChain::GetContainingOutput: The swapchain's adapter does not control the output on which the swapchain's window resides. */,
            };
            DXGI_INFO_QUEUE_FILTER filter = {};
            filter.DenyList.NumIDs = static_cast<UINT>(std::size(hide));
            filter.DenyList.pIDList = hide;
            dxgiInfoQueue->AddStorageFilterEntries(DXGI_DEBUG_DXGI, &filter);
         }
      }
   }

   HRESULT hr = CreateDXGIFactory2(_dxgiFactoryFlags, IID_PPV_ARGS(_dxgiFactory.ReleaseAndGetAddressOf()));
   CheckResult(hr, "Failed to create DirectX factory");


   ComPtr<IDXGIAdapter1> adapter;
   _GetAdapter(adapter.GetAddressOf());

   // Create the DX12 API device object.
   hr = D3D12CreateDevice(adapter.Get(),
                          _d3dMinFeatureLevel,
                          IID_PPV_ARGS(_dxDevice.ReleaseAndGetAddressOf()));

   CheckResult(hr, "Failed to create DirectX device");

   _dxDevice->SetName(L"DeviceResources");

   if (bHookDebug)
   {
      // Configure debug device (if active).
      ComPtr<ID3D12InfoQueue> d3dInfoQueue;
      if (SUCCEEDED(_dxDevice.As(&d3dInfoQueue)))
      {
//#ifdef _DEBUG
         d3dInfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_CORRUPTION, true);
         d3dInfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR, true);
//#endif
         D3D12_MESSAGE_ID hide[] =
         {
            //D3D12_MESSAGE_ID_MAP_INVALID_NULLRANGE,
            //D3D12_MESSAGE_ID_UNMAP_INVALID_NULLRANGE,
            // Workarounds for debug layer issues on hybrid-graphics systems
            D3D12_MESSAGE_ID_EXECUTECOMMANDLISTS_WRONGSWAPCHAINBUFFERREFERENCE,
            //D3D12_MESSAGE_ID_RESOURCE_BARRIER_MISMATCHING_COMMAND_LIST_TYPE,
         };
         D3D12_INFO_QUEUE_FILTER filter = {};
         filter.DenyList.NumIDs = static_cast<UINT>(std::size(hide));
         filter.DenyList.pIDList = hide;
         d3dInfoQueue->AddStorageFilterEntries(&filter);
      }
   }

   _capabilities = std::make_unique<HgiDXCapabilities>(this);


   // Create descriptor heaps for render target views and depth stencil views.
   D3D12_DESCRIPTOR_HEAP_DESC rtvDescriptorHeapDesc = {};
   rtvDescriptorHeapDesc.NumDescriptors = s_nMaxRenderTargetDescs; 
   rtvDescriptorHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;

   hr = _dxDevice->CreateDescriptorHeap(&rtvDescriptorHeapDesc, IID_PPV_ARGS(_rtvDescriptorHeap.ReleaseAndGetAddressOf()));
   CheckResult(hr, "Failed to create render target heap descriptor");
   _rtvDescriptorHeap->SetName(L"RTVDescriptorHeap");

   D3D12_DESCRIPTOR_HEAP_DESC dsvDescriptorHeapDesc = {};
   dsvDescriptorHeapDesc.NumDescriptors = s_nMaxRenderTargetDescs;
   dsvDescriptorHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;

   hr = _dxDevice->CreateDescriptorHeap(&dsvDescriptorHeapDesc, IID_PPV_ARGS(_dsvDescriptorHeap.ReleaseAndGetAddressOf()));
   CheckResult(hr, "Failed to create depth stencil heap descriptor");
   _dsvDescriptorHeap->SetName(L"DSVDescriptorHeap");

   _rtvDescriptorHeapIncrementSize = _dxDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
   _dsvDescriptorHeapIncrementSize = _dxDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);

   //
   // create the command queue and list 
   _InitCommandLists();
}

HgiDXDevice::~HgiDXDevice()
{
}

// This method acquires the first available hardware adapter that supports Direct3D 12.
// If no such adapter can be found, try WARP. Otherwise throw an exception.
void HgiDXDevice::_GetAdapter(IDXGIAdapter1** ppAdapter)
{
   *ppAdapter = nullptr;

   static bool const bForceWarp = TfGetEnvSetting(HGI_DX_FORCE_WARP);

   ComPtr<IDXGIAdapter1> adapter;
   if (!bForceWarp)
   {
      ComPtr<IDXGIFactory6> factory6;
      HRESULT hr = _dxgiFactory.As(&factory6);
      if (SUCCEEDED(hr))
      {
         for (UINT adapterIndex = 0;
              SUCCEEDED(factory6->EnumAdapterByGpuPreference(adapterIndex,
                                                             DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE,
                                                             IID_PPV_ARGS(adapter.ReleaseAndGetAddressOf())));
              adapterIndex++)
         {
            DXGI_ADAPTER_DESC1 desc;
            hr = adapter->GetDesc1(&desc);
            CheckResult(hr, "Failed to get Adapter descriptor");

            if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)
            {
               // Don't select the Basic Render Driver adapter.
               continue;
            }

            // Check to see if the adapter supports Direct3D 12, but don't create the actual device yet.
            if (SUCCEEDED(D3D12CreateDevice(adapter.Get(), _d3dMinFeatureLevel, __uuidof(ID3D12Device), nullptr)))
            {
#ifdef _DEBUG
               wchar_t buff[256] = {};
               swprintf_s(buff, L"Direct3D Adapter (%u): VID:%04X, PID:%04X - %ls\n", adapterIndex, desc.VendorId, desc.DeviceId, desc.Description);
               OutputDebugStringW(buff);
#endif
               break;
            }
         }
      }

      if (!adapter)
      {
         for (UINT adapterIndex = 0;
              SUCCEEDED(_dxgiFactory->EnumAdapters1(adapterIndex,
                                                    adapter.ReleaseAndGetAddressOf()));
              ++adapterIndex)
         {
            DXGI_ADAPTER_DESC1 desc;
            hr = adapter->GetDesc1(&desc);
            CheckResult(hr, "Failed to get Adapter descriptor");

            if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)
            {
               // Don't select the Basic Render Driver adapter.
               continue;
            }

            // Check to see if the adapter supports Direct3D 12, but don't create the actual device yet.
            if (SUCCEEDED(D3D12CreateDevice(adapter.Get(), _d3dMinFeatureLevel, __uuidof(ID3D12Device), nullptr)))
            {
#ifdef _DEBUG
               wchar_t buff[256] = {};
               swprintf_s(buff, L"Direct3D Adapter (%u): VID:%04X, PID:%04X - %ls\n", adapterIndex, desc.VendorId, desc.DeviceId, desc.Description);
               OutputDebugStringW(buff);
#endif
               break;
            }
         }
      }
   }

   if (!adapter)
   {
      // Try WARP12 instead
      if (FAILED(_dxgiFactory->EnumWarpAdapter(IID_PPV_ARGS(adapter.ReleaseAndGetAddressOf()))))
      {
         throw std::runtime_error("WARP12 not available. Enable the 'Graphics Tools' optional feature");
      }

      OutputDebugStringA("Direct3D Adapter - WARP12\n");
   }

   if (nullptr != adapter)
   {
      //
      // This is useful information that I will want to surface to the users somehow.
      // Ideally we'd have a unified way to query for some user-visible information from all HGI
      // but for now, I want this at least for mine
      adapter->GetDesc1(&_adapterDesc);
   }
   else 
   {
      throw std::runtime_error("No Direct3D 12 device found");
   }

   *ppAdapter = adapter.Detach();
}


HgiDXCapabilities const&
HgiDXDevice::GetDeviceCapabilities() const
{
   return *_capabilities;
}

void
HgiDXDevice::WaitForIdle()
{
   //
   // Wait for all queues / commands.
   _WaitForCommandListToExecute(eCommandType::kGraphics);
   _WaitForCommandListToExecute(eCommandType::kCompute);
   _WaitForCommandListToExecute(eCommandType::kCopy);
}

void 
HgiDXDevice::_WaitForCommandListToExecute(eCommandType type)
{
   if (eCommandType::kGraphics == type)
   {
      if (nullptr != _commandQueueGraphics && nullptr != _fenceCommandListGraphics && _hFenceGraphicsSignalEvent.IsValid())
      {
         // Schedule a Signal command in the GPU queue.
         const UINT64 fenceNextValue = _fenceValueGraphics + 1;
         if (SUCCEEDED(_commandQueueGraphics->Signal(_fenceCommandListGraphics.Get(), fenceNextValue)))
         {
            // Wait until the Signal has been processed.
            if (SUCCEEDED(_fenceCommandListGraphics->SetEventOnCompletion(fenceNextValue, _hFenceGraphicsSignalEvent.Get())))
            {
               std::ignore = WaitForSingleObjectEx(_hFenceGraphicsSignalEvent.Get(), INFINITE, FALSE);

               // Increment the fence value for the current frame.
               UINT64 newValue = _fenceCommandListGraphics->GetCompletedValue();
               
               if (UINT64_MAX == newValue)
               {
                  //
                  // This means the device was removed.
                  // TODO: we might want to recover / refresh the device in such cases somehow
                  // otherwise this results in an unavoidable application crash.
                  HRESULT hrRemovedReason = _dxDevice->GetDeviceRemovedReason();
                  CheckResult(hrRemovedReason, "Device was removed");
               }

               _fenceValueGraphics = newValue;
            }
         }
      }
   }
   else if(eCommandType::kCompute == type)
   {
      if (nullptr != _commandQueueCompute && nullptr != _fenceCommandListCompute && _hFenceComputeSignalEvent.IsValid())
      {
         // Schedule a Signal command in the GPU queue.
         const UINT64 fenceNextValue = _fenceValueCompute + 1;
         if (SUCCEEDED(_commandQueueCompute->Signal(_fenceCommandListCompute.Get(), fenceNextValue)))
         {
            // Wait until the Signal has been processed.
            if (SUCCEEDED(_fenceCommandListCompute->SetEventOnCompletion(fenceNextValue, _hFenceComputeSignalEvent.Get())))
            {
               std::ignore = WaitForSingleObjectEx(_hFenceComputeSignalEvent.Get(), INFINITE, FALSE);

               // Increment the fence value for the current frame.
               UINT64 newValue = _fenceCommandListCompute->GetCompletedValue();

               if (UINT64_MAX == newValue)
               {
                  //
                  // This means the device was removed.
                  // TODO: we might want to recover / refresh the device in such cases somehow
                  // otherwise this results in an unavoidable application crash.
                  HRESULT hrRemovedReason = _dxDevice->GetDeviceRemovedReason();
                  CheckResult(hrRemovedReason, "Device was removed");
               }

               _fenceValueCompute = newValue;
            }
         }
      }
   }
   else // eCommandType::kCopy
   {
      if (nullptr != _commandQueueCopy && nullptr != _fenceCommandListCopy && _hFenceCopySignalEvent.IsValid())
      {
         // Schedule a Signal command in the GPU queue.
         const UINT64 fenceNextValue = _fenceValueCopy + 1;
         if (SUCCEEDED(_commandQueueCopy->Signal(_fenceCommandListCopy.Get(), fenceNextValue)))
         {
            // Wait until the Signal has been processed.
            if (SUCCEEDED(_fenceCommandListCopy->SetEventOnCompletion(fenceNextValue, _hFenceCopySignalEvent.Get())))
            {
               std::ignore = WaitForSingleObjectEx(_hFenceCopySignalEvent.Get(), INFINITE, FALSE);

               // Increment the fence value for the current frame.
               UINT64 newValue = _fenceCommandListCopy->GetCompletedValue();

               if (UINT64_MAX == newValue)
               {
                  //
                  // This means the device was removed.
                  // TODO: we might want to recover / refresh the device in such cases somehow
                  // otherwise this results in an unavoidable application crash.
                  HRESULT hrRemovedReason = _dxDevice->GetDeviceRemovedReason();
                  CheckResult(hrRemovedReason, "Device was removed");
               }

               _fenceValueCopy = newValue;
            }
         }
      }
   }
}

ID3D12Device*
HgiDXDevice::GetDevice()
{
   return _dxDevice.Get();
}

IDXGIFactory4* 
HgiDXDevice::GetFactory()
{
   return _dxgiFactory.Get();
}

void 
HgiDXDevice::_InitCommandLists()
{
   //
   // build a graphics dedicated queue, list, ...
   _dxDevice->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(_commandAllocatorGraphics.ReleaseAndGetAddressOf()));

   HRESULT hr = _dxDevice->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, _commandAllocatorGraphics.Get(), nullptr, IID_PPV_ARGS(_commandListGraphics.ReleaseAndGetAddressOf()));
   CheckResult(hr, "Failed to create command list");

   _commandListGraphics->SetName(L"Graphics Command List");

   //
   // allocate a fence as well to be able to know when a command queue finished executting
   hr = _dxDevice->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(_fenceCommandListGraphics.ReleaseAndGetAddressOf()));
   CheckResult(hr, "Failed to create command list execution fence");

   _hFenceGraphicsSignalEvent.Attach(CreateEventEx(nullptr, nullptr, 0, EVENT_MODIFY_STATE | SYNCHRONIZE));
   if (!_hFenceGraphicsSignalEvent.IsValid())
      TF_RUNTIME_ERROR("Failed to create fence signal event");

   _fenceValueGraphics = _fenceCommandListGraphics->GetCompletedValue();

   //
   // build a separated compute queue, list, ...
   _dxDevice->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_COMPUTE, IID_PPV_ARGS(_commandAllocatorCompute.ReleaseAndGetAddressOf()));

   hr = _dxDevice->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_COMPUTE, _commandAllocatorCompute.Get(), nullptr, IID_PPV_ARGS(_commandListCompute.ReleaseAndGetAddressOf()));
   CheckResult(hr, "Failed to create command list");

   _commandListCompute->SetName(L"Compute Command List");

   //
   // allocate a fence as well to be able to know when a command queue finished executting
   hr = _dxDevice->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(_fenceCommandListCompute.ReleaseAndGetAddressOf()));
   CheckResult(hr, "Failed to create command list execution fence");

   _hFenceComputeSignalEvent.Attach(CreateEventEx(nullptr, nullptr, 0, EVENT_MODIFY_STATE | SYNCHRONIZE));
   if (!_hFenceComputeSignalEvent.IsValid())
      TF_RUNTIME_ERROR("Failed to create fence signal event");

   _fenceValueCompute = _fenceCommandListCompute->GetCompletedValue();


   //
   // and a separated copy queue, list, ...
   _dxDevice->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_COPY, IID_PPV_ARGS(_commandAllocatorCopy.ReleaseAndGetAddressOf()));

   hr = _dxDevice->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_COPY, _commandAllocatorCopy.Get(), nullptr, IID_PPV_ARGS(_commandListCopy.ReleaseAndGetAddressOf()));
   CheckResult(hr, "Failed to create command list");

   _commandListCopy->SetName(L"Copy Command List");

   //
   // allocate a fence as well to be able to know when a command queue finished executting
   hr = _dxDevice->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(_fenceCommandListCopy.ReleaseAndGetAddressOf()));
   CheckResult(hr, "Failed to create command list execution fence");

   _hFenceCopySignalEvent.Attach(CreateEventEx(nullptr, nullptr, 0, EVENT_MODIFY_STATE | SYNCHRONIZE));
   if (!_hFenceCopySignalEvent.IsValid())
      TF_RUNTIME_ERROR("Failed to create fence signal event");

   _fenceValueCopy = _fenceCommandListCopy->GetCompletedValue();
}

ID3D12DescriptorHeap* 
HgiDXDevice::GetRTVDescriptorHeap()
{
   return _rtvDescriptorHeap.Get();
}

ID3D12DescriptorHeap* 
HgiDXDevice::GetDSVDescriptorHeap()
{
   return _dsvDescriptorHeap.Get();
}

UINT 
HgiDXDevice::GetRTVDescriptorHeapIncrementSize()
{
   return _rtvDescriptorHeapIncrementSize;
}

UINT 
HgiDXDevice::GetDSVDescriptorHeapIncrementSize()
{
   return _dsvDescriptorHeapIncrementSize;
}

D3D12_CPU_DESCRIPTOR_HANDLE
HgiDXDevice::CreateRenderTargetView(ID3D12Resource* pRes, UINT nTexIdx)
{
   D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle;

   if (nTexIdx < s_nMaxRenderTargetDescs)
   {
      ID3D12DescriptorHeap* pHeap = GetRTVDescriptorHeap();
      UINT rtvDescriptorSize = GetRTVDescriptorHeapIncrementSize();
      rtvHandle = CD3DX12_CPU_DESCRIPTOR_HANDLE(pHeap->GetCPUDescriptorHandleForHeapStart(), nTexIdx, rtvDescriptorSize);

      //
      // TODO: is it bad that I call this mny times for the same resource?
      _dxDevice->CreateRenderTargetView(pRes, nullptr, rtvHandle);
   }

   return rtvHandle;
}

D3D12_CPU_DESCRIPTOR_HANDLE
HgiDXDevice::CreateDepthStencilView(ID3D12Resource* pRes, UINT nTexIdx)
{
   D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle;

   if (nTexIdx < s_nMaxRenderTargetDescs)
   {
      ID3D12DescriptorHeap* pHeap = GetDSVDescriptorHeap();
      UINT dsvDescriptorSize = GetDSVDescriptorHeapIncrementSize();
      dsvHandle = CD3DX12_CPU_DESCRIPTOR_HANDLE(pHeap->GetCPUDescriptorHandleForHeapStart(), nTexIdx, dsvDescriptorSize);
      _dxDevice->CreateDepthStencilView(pRes, nullptr, dsvHandle);
   }

   return dsvHandle;
}

ID3D12CommandQueue* 
HgiDXDevice::GetCommandQueue(eCommandType type)
{
   ID3D12CommandQueue* pRet = nullptr;

   std::stringstream buffer;
   buffer << "GetCommandQueue "<< type << " called on thread: " << std::this_thread::get_id();
   TF_STATUS(buffer.str().c_str());

   if (eCommandType::kGraphics == type)
   {
      if (nullptr == _commandQueueGraphics)
      {
         //
         // Create the command queue.
         D3D12_COMMAND_QUEUE_DESC queueDesc = {};
         queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
         queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;

         HRESULT hr = _dxDevice->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(_commandQueueGraphics.ReleaseAndGetAddressOf()));
         CheckResult(hr, "Failed to create command queue");

         _commandQueueGraphics->SetName(L"Graphics Command Queue");
      }

      pRet = _commandQueueGraphics.Get();
      
   }
   else if(eCommandType::kCompute == type)
   {
      if (nullptr == _commandQueueCompute)
      {
         //
         // Create the command queue.
         D3D12_COMMAND_QUEUE_DESC queueDesc = {};
         queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
         queueDesc.Type = D3D12_COMMAND_LIST_TYPE_COMPUTE;

         HRESULT hr = _dxDevice->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(_commandQueueCompute.ReleaseAndGetAddressOf()));
         CheckResult(hr, "Failed to create compute queue");

         _commandQueueCompute->SetName(L"Compute Command Queue");
      }

      pRet = _commandQueueCompute.Get();
   }
   else //eCommandType::kCopy
   {
      if (nullptr == _commandQueueCopy)
      {
         //
         // Create the command queue.
         D3D12_COMMAND_QUEUE_DESC queueDesc = {};
         queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
         queueDesc.Type = D3D12_COMMAND_LIST_TYPE_COPY;

         HRESULT hr = _dxDevice->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(_commandQueueCopy.ReleaseAndGetAddressOf()));
         CheckResult(hr, "Failed to create compute queue");

         _commandQueueCopy->SetName(L"Copy Command Queue");
      }

      pRet = _commandQueueCopy.Get();
   }

   return pRet;
}

ID3D12GraphicsCommandList* 
HgiDXDevice::GetCommandList(eCommandType type)
{
   ID3D12GraphicsCommandList* pRet = nullptr;
   if (eCommandType::kGraphics == type)
   {
      if (_bGraphicsCmdListClosed)
      {
         HRESULT hr = _commandAllocatorGraphics->Reset();
         CheckResult(hr, "Failed to reset command list allocator");

         hr = _commandListGraphics->Reset(_commandAllocatorGraphics.Get(), nullptr);
         CheckResult(hr, "Failed to reset command list");

         _bGraphicsCmdListClosed = false;
      }

      pRet = _commandListGraphics.Get();
   }
   else if(eCommandType::kCompute == type)
   {
      if (_bComputeCmdListClosed)
      {
         HRESULT hr = _commandAllocatorCompute->Reset();
         CheckResult(hr, "Failed to reset command list allocator");

         hr = _commandListCompute->Reset(_commandAllocatorCompute.Get(), nullptr);
         CheckResult(hr, "Failed to reset command list");

         _bComputeCmdListClosed = false;
      }

      pRet = _commandListCompute.Get();
   }
   else //eCommandType::kCopy
   {
      if (_bCopyCmdListClosed)
      {
         HRESULT hr = _commandAllocatorCopy->Reset();
         CheckResult(hr, "Failed to reset command list allocator");

         hr = _commandListCopy->Reset(_commandAllocatorCopy.Get(), nullptr);
         CheckResult(hr, "Failed to reset command list");

         _bCopyCmdListClosed = false;
      }

      pRet = _commandListCopy.Get();
   }
   
   return pRet;
}

void 
HgiDXDevice::SubmitCommandList(eCommandType type)
{
   if (eCommandType::kGraphics == type)
   {
      if (nullptr != _commandListGraphics)
      {
         if (!_bGraphicsCmdListClosed) // otherwise this might be a noop
         {
            _commandListGraphics->Close();
            _bGraphicsCmdListClosed = true;

            ID3D12CommandQueue* pQueue = GetCommandQueue(type);
            if (nullptr != pQueue)
            {
               //
               // debug code
               TF_STATUS("Info: Submitting graphics command list.");
               pQueue->ExecuteCommandLists(1, CommandListCast(_commandListGraphics.GetAddressOf()));
               _WaitForCommandListToExecute(type);
            }
         }
      }
   }
   else if (eCommandType::kCompute == type)
   {
      if (nullptr != _commandListCompute)
      {
         if (!_bComputeCmdListClosed) // otherwise this might be a noop
         {
            _commandListCompute->Close();
            _bComputeCmdListClosed = true;

            ID3D12CommandQueue* pQueue = GetCommandQueue(type);
            if (nullptr != pQueue)
            {
               //
               // debug code
               TF_STATUS("Info: Submitting compute command list.");
               pQueue->ExecuteCommandLists(1, CommandListCast(_commandListCompute.GetAddressOf()));
               _WaitForCommandListToExecute(type);
            }
         }
      }
   }
   else //eCommandType::kCopy
   {
      if (nullptr != _commandListCopy)
      {
         if (!_bCopyCmdListClosed) // otherwise this might be a noop
         {
            _commandListCopy->Close();
            _bCopyCmdListClosed = true;

            ID3D12CommandQueue* pQueue = GetCommandQueue(type);
            if (nullptr != pQueue)
            {
               //
               // debug code
               //TF_STATUS("Info: Submitting copy command list.");
               pQueue->ExecuteCommandLists(1, CommandListCast(_commandListCopy.GetAddressOf()));
               _WaitForCommandListToExecute(type);
            }
         }
      }
   }
}

HGIDX_API
const DXGI_ADAPTER_DESC1& 
HgiDXDevice::GetAdapterInfo()
{
   return _adapterDesc;
}


PXR_NAMESPACE_CLOSE_SCOPE
