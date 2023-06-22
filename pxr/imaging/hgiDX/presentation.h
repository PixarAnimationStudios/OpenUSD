
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
#include "pxr/imaging/hgiInterop/hgiInterop.h"

PXR_NAMESPACE_OPEN_SCOPE

class HgiDX;
class HgiDXDevice;
class HgiDXTexture;
class HgiDXTextureConverter;


class HgiDXPresentation final : public HgiCustomInterop
{
public:
   HGIDX_API
   void Initialize(HgiDXTexture* pRenderTargetColor, GfVec4i const& dstRegion);

   //
   // The host Application is responsible to tell the hgi what to do with the rendered image.
   // If no target is set, there will be no "handover", "HdxPresentTask" or whoever calls this
   // will have no effect.
   // 
   // Call this method if the image is to be displayed in a window.
   HGIDX_API
   void SetTargetWnd(HWND hWnd); 

   //
   // Host Application is responsible to tell the hgi what to do with the rendered image.
   // If no target is set, there will be no "handover", "HdxPresentTask" or whoever calls this
   // will have no effect.
   // 
   // Call this method if the image is to be transferred to an offscreen texture.
   // 
   // The transfer will take care of the potential format differences 
   // between the internally produced image and the target specifications.
   HGIDX_API
   void SetTargetOffscreen(HgiTextureHandle offscreenTxHandle); 

   //
   // This is called by the HdxPresentTask to "handover" the image produced internally
   void TransferToApp(HgiTextureHandle const& srcColor,
                      HgiTextureHandle const& srcDepth,
                      VtValue const& dstFramebuffer,
                      GfVec4i const& dstRegion) override; //(left, BOTTOM, width, height)

private:
   friend class HgiDX;
   HgiDXPresentation(HgiDXDevice*, HgiDXTextureConverter*);

   void _Present2Wnd(HgiDXTexture* pRTTx);
   void _PresentOffscreen(HgiDXTexture* pRTTx);


   void _CopyRenderTarget2SwapChain();
   void _MoveToNextFrame();


private:
   HgiDXDevice* m_pDevice;
   HgiDXTextureConverter* m_pTxConverter;

   DXGI_FORMAT m_RenderTargetBufferFormat = DXGI_FORMAT::DXGI_FORMAT_R8G8B8A8_UNORM;
   DXGI_FORMAT m_DepthBufferFormat = DXGI_FORMAT::DXGI_FORMAT_D32_FLOAT;

   PXR_INTERNAL_NS::HgiDXTexture* m_renderTargetTx = nullptr;

   static const UINT c_swapBufferCount = 2;
   Microsoft::WRL::ComPtr<IDXGISwapChain3> m_swapChain;
   Microsoft::WRL::ComPtr<ID3D12Resource> m_renderTargets[c_swapBufferCount];
   D3D12_CPU_DESCRIPTOR_HANDLE m_rtvHandles[c_swapBufferCount];
   Microsoft::WRL::ComPtr<ID3D12Resource> m_depthStencil;

   UINT64 m_fenceValues[c_swapBufferCount];
   
   Microsoft::WRL::ComPtr<ID3D12Fence> m_presentationFence;
   Microsoft::WRL::Wrappers::Event m_presentationFenceEvent;

   UINT m_backBufferIndex = 0;

   int m_nWidth;
   int m_nHeight;
   HWND m_hWnd = 0;

   HgiTextureHandle m_offscreenTxHandle;
};



PXR_NAMESPACE_CLOSE_SCOPE