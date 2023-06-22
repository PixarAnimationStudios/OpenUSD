
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
#include "pxr/imaging/hgiDX/presentation.h"
#include "pxr/base/tf/diagnostic.h"
#include "pxr/imaging/hgiDX/buffer.h"
#include "pxr/imaging/hgiDX/conversions.h"
#include "pxr/imaging/hgiDX/device.h"
#include "pxr/imaging/hgiDX/texture.h"
#include "pxr/imaging/hgiDX/textureConverter.h"

#include "pxr/imaging/hgi/graphicsPipeline.h"

static const bool c_bIndepWndSwapchainFormat = true;
static const DXGI_FORMAT c_scFormat = DXGI_FORMAT_B8G8R8A8_UNORM;
static const uint32_t c_idxRTVPresent = 4; // TODO: this is super ugly...


PXR_NAMESPACE_OPEN_SCOPE

HgiDXPresentation::HgiDXPresentation(HgiDXDevice* pDXDevice, HgiDXTextureConverter* pTxConverter)
   :m_pDevice(pDXDevice),
    m_pTxConverter(pTxConverter)
{

}

void 
HgiDXPresentation::TransferToApp(HgiTextureHandle const& srcColor,
                                 HgiTextureHandle const& srcDepth,
                                 VtValue const& dstFramebuffer,
                                 GfVec4i const& dstRegion)
{
   HgiDXTexture* pRTTx = dynamic_cast<HgiDXTexture*>(srcColor.Get());
   if (nullptr != pRTTx)
   {
      if (m_hWnd != 0)
      {
         Initialize(pRTTx, dstRegion);
         _Present2Wnd(pRTTx);
      }
      else if (m_offscreenTxHandle.Get() != nullptr)
      {
         _PresentOffscreen(pRTTx);
      }
   }
}

void 
HgiDXPresentation::SetTargetWnd(HWND hWnd)
{
   m_hWnd = hWnd;
   m_offscreenTxHandle = HgiTextureHandle();
}

void 
HgiDXPresentation::SetTargetOffscreen(HgiTextureHandle offscreenTxHandle)
{
   m_hWnd = 0;
   m_offscreenTxHandle = offscreenTxHandle;
}

void 
HgiDXPresentation::Initialize(HgiDXTexture* pRenderTargetColor, GfVec4i const& dstRegion)
{
   if (0 == m_hWnd)
   {
      TF_WARN("Target window is not initialized. Not much can be done here without that information.");
      return;
   }

   //
   // TODO: This check does not feel safe enough anymore
   //if (m_renderTargetTx == pRenderTargetColor)
   //   return;

   m_renderTargetTx = pRenderTargetColor;

   if (!c_bIndepWndSwapchainFormat && (nullptr == pRenderTargetColor))
      return; // I need it to setup my swapChain with compatible data

   //
   // TODO: ignoring corner info for the moment...
   bool bResizeSwapChain = (m_nWidth != dstRegion[2]) || (m_nHeight != dstRegion[3]);
   m_nWidth = dstRegion[2];
   m_nHeight = dstRegion[3];

   HgiTextureDesc desc = pRenderTargetColor->GetDescriptor();
   DXGI_FORMAT newFormat;
   if (c_bIndepWndSwapchainFormat)
      newFormat = c_scFormat;
   else
      newFormat = HgiDXConversions::GetTextureFormat(desc.format);

   bResizeSwapChain |= (m_RenderTargetBufferFormat != newFormat);
   m_RenderTargetBufferFormat = newFormat;

   if ((nullptr != m_swapChain) && (!bResizeSwapChain))
      return;

   HRESULT hr = S_OK;

   m_pDevice->WaitForIdle();

   // If the swap chain already exists, resize it, otherwise create one.
   if (m_swapChain)
   {
      // Release resources that are tied to the swap chain and update fence values.
      for (UINT n = 0; n < c_swapBufferCount; n++)
      {
         m_renderTargets[n].Reset();
         m_fenceValues[n] = 0;
      }

      hr = m_swapChain->ResizeBuffers(c_swapBufferCount, m_nWidth, m_nHeight, m_RenderTargetBufferFormat, 0);

      if (hr == DXGI_ERROR_DEVICE_REMOVED || hr == DXGI_ERROR_DEVICE_RESET)
      {
         // If the device was removed for any reason, a new device and swap chain will need to be created.
         //TODO: OnDeviceLost();
         CheckResult(E_FAIL, "Device lost detected but proper reaction not implemented yet.");

         // Everything is set up now. Do not continue execution of this method. OnDeviceLost will reenter this method
         // and correctly set up the new device.
         return;
      }
      else
         CheckResult(E_FAIL, "Failed to resize buffers.");
   }
   else
   {
      // Create the swap chain and init fence values.
      for (UINT n = 0; n < c_swapBufferCount; n++)
         m_fenceValues[n] = 0;

      // Create a descriptor for the swap chain.
      DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};
      swapChainDesc.Width = m_nWidth;
      swapChainDesc.Height = m_nHeight;
      swapChainDesc.Format = m_RenderTargetBufferFormat;
      swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
      swapChainDesc.BufferCount = c_swapBufferCount;
      swapChainDesc.SampleDesc.Count = desc.sampleCount;
      swapChainDesc.SampleDesc.Quality = 0;
      swapChainDesc.Scaling = DXGI_SCALING_NONE;
      swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD; //this one requires more than one back buffer
      swapChainDesc.AlphaMode = DXGI_ALPHA_MODE_IGNORE;

      DXGI_SWAP_CHAIN_FULLSCREEN_DESC fsSwapChainDesc = {};
      fsSwapChainDesc.Windowed = TRUE;

      // Create a swap chain for the window.
      ComPtr<IDXGISwapChain1> swapChain;

      ID3D12CommandQueue* pCmdQueue = m_pDevice->GetCommandQueue(HgiDXDevice::eCommandType::kGraphics);
      IDXGIFactory4* pFactory = m_pDevice->GetFactory();
      HRESULT hr = pFactory->CreateSwapChainForHwnd(pCmdQueue,
                                                    m_hWnd,
                                                    &swapChainDesc,
                                                    &fsSwapChainDesc,
                                                    nullptr,
                                                    swapChain.GetAddressOf());
      CheckResult(hr, "Failed to create swap chain.");

      hr = swapChain.As(&m_swapChain);
      CheckResult(hr, "Failed to move?? swap chain.");

      //
      // TODO: add fullscreen support here (and test it somehow).
      // This template does not support exclusive fullscreen mode and prevents DXGI from responding to the ALT+ENTER shortcut.
      hr = pFactory->MakeWindowAssociation(m_hWnd, DXGI_MWA_NO_ALT_ENTER);
      CheckResult(hr, "Failed associate window.");

      //
      // Allocate a fence to wait for presentation to finish.
      hr = m_pDevice->GetDevice()->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(m_presentationFence.ReleaseAndGetAddressOf()));
      CheckResult(hr, "Failed to create present fence");
   }

   // Obtain the back buffers for this window which will be the present targets.
   for (UINT n = 0; n < c_swapBufferCount; n++)
   {
      hr = m_swapChain->GetBuffer(n, IID_PPV_ARGS(m_renderTargets[n].ReleaseAndGetAddressOf()));
      CheckResult(hr, "Failed to get swap chain render targets");
      m_rtvHandles[n] = m_pDevice->CreateRenderTargetView(m_renderTargets[n].Get(), c_idxRTVPresent + n);
   }

   m_backBufferIndex = m_swapChain->GetCurrentBackBufferIndex();
}

void 
HgiDXPresentation::_Present2Wnd(HgiDXTexture* pRTTx)
{
   if (nullptr == pRTTx)
      return;

   if (c_bIndepWndSwapchainFormat)
   {
      ID3D12GraphicsCommandList* pCmdList = m_pDevice->GetCommandList(HgiDXDevice::eCommandType::kGraphics);

      if (nullptr != pCmdList)
      {
         const D3D12_RESOURCE_BARRIER barrier = 
            CD3DX12_RESOURCE_BARRIER::Transition(m_renderTargets[m_backBufferIndex].Get(),
                                                 D3D12_RESOURCE_STATE_PRESENT, 
                                                 D3D12_RESOURCE_STATE_RENDER_TARGET);

         pCmdList->ResourceBarrier(1, &barrier);

         m_pTxConverter->convert(pRTTx, m_rtvHandles[m_backBufferIndex], c_scFormat, m_nWidth, m_nHeight);

         //
         // get command list again because "convert" will submit and close the command list
         pCmdList = m_pDevice->GetCommandList(HgiDXDevice::eCommandType::kGraphics);

         const D3D12_RESOURCE_BARRIER barrier2 =
            CD3DX12_RESOURCE_BARRIER::Transition(m_renderTargets[m_backBufferIndex].Get(),
                                                 D3D12_RESOURCE_STATE_RENDER_TARGET,
                                                 D3D12_RESOURCE_STATE_PRESENT);
         pCmdList->ResourceBarrier(1, &barrier2);

         // Send the command list off to the GPU for processing.
         m_pDevice->SubmitCommandList(HgiDXDevice::eCommandType::kGraphics);
      }
   }
   else
   {
      //
      // I know this results in corrupted final color (unclear why yet)
      _CopyRenderTarget2SwapChain();
   }

   // The first argument instructs DXGI to block until VSync, putting the application
   // to sleep until the next VSync. This ensures we don't waste any cycles rendering
   // frames that will never be displayed to the screen.
   HRESULT hr = m_swapChain->Present(1, 0);

   // If the device was reset we must completely reinitialize the renderer.
   if (hr == DXGI_ERROR_DEVICE_REMOVED || hr == DXGI_ERROR_DEVICE_RESET)
   {
      //TODO: OnDeviceLost();
      CheckResult(E_FAIL, "device lost detected but proper reaction not implemented yet");
   }
   else
   {
      CheckResult(hr, "Failed to resize buffers.");
      _MoveToNextFrame();
   }
}

void 
HgiDXPresentation::_CopyRenderTarget2SwapChain()
{
   if (nullptr == m_renderTargetTx)
      return;

   ID3D12GraphicsCommandList* pCmdList = m_pDevice->GetCommandList(HgiDXDevice::eCommandType::kGraphics);

   if (nullptr != pCmdList)
   {
      //
      // transition source resource from renderTarget into "copy from" mode
      m_renderTargetTx->UpdateResourceState(pCmdList, D3D12_RESOURCE_STATE_PRESENT);

      //
      // transition destination resource into "copy to" mode
      const D3D12_RESOURCE_BARRIER barrier2 =
         CD3DX12_RESOURCE_BARRIER::Transition(m_renderTargets[m_backBufferIndex].Get(),
                                              D3D12_RESOURCE_STATE_PRESENT,
                                              D3D12_RESOURCE_STATE_COPY_DEST);

      pCmdList->ResourceBarrier(1, &barrier2);

      pCmdList->CopyResource(m_renderTargets[m_backBufferIndex].Get(), m_renderTargetTx->GetResource());

      const D3D12_RESOURCE_BARRIER barrier3 =
         CD3DX12_RESOURCE_BARRIER::Transition(m_renderTargets[m_backBufferIndex].Get(),
                                              D3D12_RESOURCE_STATE_COPY_DEST,
                                              D3D12_RESOURCE_STATE_PRESENT);
      pCmdList->ResourceBarrier(1, &barrier3);

      // Send the command list off to the GPU for processing.
      m_pDevice->SubmitCommandList(HgiDXDevice::eCommandType::kGraphics);
   }
   else
      CheckResult(E_FAIL, "Cannot get valid command list. Failed to set buffer data.");
}

void 
HgiDXPresentation::_MoveToNextFrame()
{
   //
   // TODO: I need to think this through some more
   // it feels inefficient to wait here for presentation when I could continue to 
   // render in parallel... I should only wait for myself...
   // maybe use a different command queue for this?
   // Send the command list off to the GPU for processing.
   ID3D12CommandQueue* pCmdQueue = m_pDevice->GetCommandQueue(HgiDXDevice::eCommandType::kGraphics);
   
   // Schedule a Signal command in the queue.
   const UINT64 nextFenceValue = m_fenceValues[m_backBufferIndex] + 1;
   HRESULT hr = pCmdQueue->Signal(m_presentationFence.Get(), nextFenceValue);
   CheckResult(hr, "Failed to signal queue for \"next\" frame.");

   // Update the back buffer index.
   m_backBufferIndex = m_swapChain->GetCurrentBackBufferIndex();

   // If the next frame is not ready to be rendered yet, wait until it is ready.
   if (m_presentationFence->GetCompletedValue() < nextFenceValue)
   {
      hr = m_presentationFence->SetEventOnCompletion(nextFenceValue, m_presentationFenceEvent.Get());
      CheckResult(hr, "Failed to set completion event for next frame signal.");
      std::ignore = WaitForSingleObjectEx(m_presentationFenceEvent.Get(), INFINITE, FALSE);
   }

   // Set the fence value for the next frame.
   m_fenceValues[m_backBufferIndex] = nextFenceValue;
}

void 
HgiDXPresentation::_PresentOffscreen(HgiDXTexture* pRTTx)
{
   if (nullptr == pRTTx)
      return;

   HgiDXTexture* pOffscreenTx = dynamic_cast<HgiDXTexture*>(m_offscreenTxHandle.Get());
   if (nullptr == pOffscreenTx)
      return;

   //
   // TODO: I assume (and do not check for now) that 
   // the size is the same for the render target and the final destination texture 

   ID3D12GraphicsCommandList* pCmdList = m_pDevice->GetCommandList(HgiDXDevice::eCommandType::kGraphics);
   if (nullptr != pCmdList)
   {
      const HgiTextureDesc targetDesc = pOffscreenTx->GetDescriptor();
      DXGI_FORMAT targetFormat = HgiDXConversions::GetTextureFormat(targetDesc.format);

      GfVec3i dims = targetDesc.dimensions;

      pOffscreenTx->UpdateResourceState(pCmdList, D3D12_RESOURCE_STATE_RENDER_TARGET);
      D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = m_pDevice->CreateRenderTargetView(pOffscreenTx->GetResource(), c_idxRTVPresent);

      //
      // Looks like the best course of action here as well is to execute a dedicated program 
      // to do the format conversion, similar to what GL does in "$\pxr\imaging\hgiInterop\opengl.cpp".
      m_pTxConverter->convert(pRTTx, rtvHandle, targetFormat, dims[0], dims[1]);
   }
}


PXR_NAMESPACE_CLOSE_SCOPE