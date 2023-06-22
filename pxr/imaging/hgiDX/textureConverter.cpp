
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

#include "pxr/imaging/hgiDX/textureConverter.h"

#include "pxr/base/tf/diagnostic.h"

#include "pxr/imaging/hgiDX/buffer.h"
#include "pxr/imaging/hgiDX/conversions.h"
#include "pxr/imaging/hgiDX/device.h"
#include "pxr/imaging/hgiDX/hgi.h"
#include "pxr/imaging/hgiDX/shaderFunction.h"
#include "pxr/imaging/hgiDX/shaderProgram.h"
#include "pxr/imaging/hgiDX/texture.h"

#include "D3dCompiler.h"

//
// some useful references
//
// https://learn.microsoft.com/en-us/windows/win32/direct3d10/d3d10-graphics-programming-guide-resources-coordinates
// https://github.com/microsoft/DirectXTK12/blob/main/Src/ScreenGrab.cpp


using namespace DirectX;

PXR_NAMESPACE_OPEN_SCOPE

HgiDXTextureConverter::HgiDXTextureConverter(HgiDX* pHgi)
   :m_pHgi(pHgi)
{
   if (nullptr == m_pHgi)
      TF_FATAL_CODING_ERROR("Texture Converter cannot work with invalid Hgi");
}

HgiDXTextureConverter::~HgiDXTextureConverter()
{
}

struct VertexTex
{
   DirectX::XMFLOAT4 Position;
   DirectX::XMFLOAT2 UV;
};

//
// I want to draw entire screen via one single triangle (like ogl does)
// making one big enough to include the original screen rectangle
//  o tx(0,0) -> dc(-1,1)
//  --------.--------/ o tx(2,0) -> dc(3, 1)
//  |      |       /
//  |      |    /
//  |------| / 
//  |      /
//  |   /
//  |/
//  o tx(0,2) -> dc(-1,-3)
static VertexTex g_Vertices[3] = {
       { XMFLOAT4( -1.0f,  -3.0f, 0.0f, 1.0f), XMFLOAT2(0.0f, 2.0f) }, // 0
       { XMFLOAT4(  3.0f,   1.0f, 0.0f, 1.0f), XMFLOAT2(2.0f, 0.0f) }, // 1
       { XMFLOAT4( -1.0f,   1.0f, 0.0f, 1.0f), XMFLOAT2(0.0f, 0.0f) }, // 2
};

static UINT g_Indices[3] =
{
    0, 1, 2, 
};

static FLOAT ColorRGBA[4] = { 0.0f, 0.0f, 0.0f, 1.0f };

const static std::string codeVS =
   "struct VS_STAGE_IN {\n"
   "     float4 position : POSITION;\n"
   "     float2 uv : TEXCOORD;\n"
   "};\n"
   "\n"
   "struct VS_STAGE_OUT {\n"
   "     float4 position : SV_Position;\n"
   "     float2 uv : TEXCOORD;\n"
   "};\n"
   "\n"
   "VS_STAGE_OUT vs_main(VS_STAGE_IN IN) {\n"
   "     VS_STAGE_OUT OUT;\n"
   "     OUT.position = IN.position;\n"
   "     OUT.uv = IN.uv;\n"
   "     return OUT;\n"
   "}\n";

const static std::string codePS =
   "struct PS_STAGE_IN {\n"
   "     float4 position : SV_Position;\n"
   "     float2 uv : TEXCOORD;\n"
   "};\n"
   "\n"
   "struct PS_STAGE_OUT {\n"
   "   float4 colorOut : SV_Target;\n"
   "};\n"
   "\n"
   "Texture2D texIn : register(t0, space0);\n"
   "SamplerState MeshTextureSampler : register(s0, space0);\n"
   "\n"
   "PS_STAGE_OUT ps_main(PS_STAGE_IN IN) {\n"
   "     PS_STAGE_OUT OUT;\n"
   "     OUT.colorOut = texIn.Sample(MeshTextureSampler, IN.uv);\n"
   "     return OUT;\n"
   "}";


void 
HgiDXTextureConverter::_initializeBuffers()
{
   if (nullptr == m_vertBuff)
   {
      HgiDXDevice* pDevice = m_pHgi->GetPrimaryDevice();

      HgiBufferDesc descVB;
      descVB.debugName = "TxConverterVertexInfo";
      descVB.usage = HgiBufferUsageVertex;
      descVB.byteSize = sizeof(g_Vertices);
      descVB.vertexStride = sizeof(VertexTex);
      descVB.initialData = &g_Vertices;

      m_vertBuff = std::make_unique<HgiDXBuffer>(pDevice, descVB);

      HgiBufferDesc descIdx;
      descIdx.debugName = "TxConverterIndices";
      descIdx.usage = HgiBufferUsageIndex32;
      descIdx.byteSize = sizeof(g_Indices);
      descIdx.vertexStride = sizeof(UINT);
      descIdx.initialData = &g_Indices;

      m_idxBuff = std::make_unique<HgiDXBuffer>(pDevice, descIdx);
   }
}

void 
HgiDXTextureConverter::_initialize(DXGI_FORMAT format)
{
   _initializeBuffers();

   std::map<DXGI_FORMAT, std::unique_ptr<TxConvertPipelineInfo>>::const_iterator it = m_pipelineByOutput.find(format);
   if (it == m_pipelineByOutput.end())
   {
      //
      // If I do nto set additional information, the shader generator will have nothign to do
      // The downside is that it will also be unable to auto-determine the root signature and input params 

      std::unique_ptr<TxConvertPipelineInfo> pTxPipelineInfo = std::make_unique<TxConvertPipelineInfo>();
      pTxPipelineInfo->renderTargetFormat = format;

      ComPtr<ID3DBlob> errorMsgs;

#if defined(_DEBUG)
      // Enable better shader debugging with the graphics debugging tools.
      UINT compileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#else
      UINT compileFlags = 0;
#endif

      HRESULT hr = D3DCompile(codeVS.c_str(), codeVS.length(),
                              "tx_convert_vs",
                              nullptr,
                              D3D_COMPILE_STANDARD_FILE_INCLUDE,
                              "vs_main", "vs_5_1",
                              compileFlags, 0,
                              pTxPipelineInfo->shaderBlob_VS.ReleaseAndGetAddressOf(),
                              errorMsgs.ReleaseAndGetAddressOf());

      if (FAILED(hr))
      {
         char err[20000]; // in some rare cases we can get very large errors text...
         void* pBuffPtr = errorMsgs->GetBufferPointer();
         snprintf(err, 20000, "Error %08X   %s\n", hr, (char*)pBuffPtr);
         CheckResult(hr, "Failed to compile vertex shader");
         return;
      }

      hr = D3DCompile(codePS.c_str(), codePS.length(),
                              "tx_convert_ps",
                              nullptr,
                              D3D_COMPILE_STANDARD_FILE_INCLUDE,
                              "ps_main", "ps_5_1",
                              compileFlags, 0,
                              pTxPipelineInfo->shaderBlob_PS.ReleaseAndGetAddressOf(),
                              errorMsgs.ReleaseAndGetAddressOf());

      if (FAILED(hr))
      {
         char err[20000]; // in some rare cases we can get very large errors text...
         void* pBuffPtr = errorMsgs->GetBufferPointer();
         snprintf(err, 20000, "Error %08X   %s\n", hr, (char*)pBuffPtr);
         CheckResult(hr, "Failed to compile pixel shader");
         return;
      }

      pTxPipelineInfo->inputDescs.push_back(D3D12_INPUT_ELEMENT_DESC{ "POSITION", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 });
      pTxPipelineInfo->inputDescs.push_back(D3D12_INPUT_ELEMENT_DESC{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 });

      CD3DX12_DESCRIPTOR_RANGE1 descriptorRage(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0);
      CD3DX12_ROOT_PARAMETER1 rp;
      rp.InitAsDescriptorTable(1, &descriptorRage, D3D12_SHADER_VISIBILITY_PIXEL);

      pTxPipelineInfo->rootParams.push_back(rp);
      if (_buildPSO(pTxPipelineInfo.get()))
      {
         m_pipelineByOutput[format] = std::move(pTxPipelineInfo);
      }
   }
}

bool  
HgiDXTextureConverter::_buildPSO(TxConvertPipelineInfo* pInfo)
{
   D3D12_GRAPHICS_PIPELINE_STATE_DESC pipelineDesc;
   ZeroMemory(&pipelineDesc, sizeof(pipelineDesc));

   HgiDXDevice* pDevice = m_pHgi->GetPrimaryDevice();

   // Create a root signature.
   D3D12_FEATURE_DATA_ROOT_SIGNATURE featureData = {};
   featureData.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_1;
   if (FAILED(pDevice->GetDevice()->CheckFeatureSupport(D3D12_FEATURE_ROOT_SIGNATURE, &featureData, sizeof(featureData))))
   {
      featureData.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_0;
   }

   D3D12_ROOT_SIGNATURE_FLAGS rootSignatureFlags =
      D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT |
      D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS |
      D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS;

   // A single 32-bit constant root parameter that is used by the vertex shader.
   // we want to pass the combined matrices through here...
   pipelineDesc.InputLayout = { pInfo->inputDescs.data(), (unsigned int)pInfo->inputDescs.size() };

   CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC rootSignatureDescription;
   std::vector<CD3DX12_ROOT_PARAMETER1> rootParams = pInfo->rootParams;
   CD3DX12_STATIC_SAMPLER_DESC txSampler(0, D3D12_FILTER_COMPARISON_MIN_MAG_MIP_LINEAR);
   rootSignatureDescription.Init_1_1((int)rootParams.size(), rootParams.data(), 1, &txSampler, rootSignatureFlags);

   // Serialize the root signature.
   ComPtr<ID3DBlob> rootSignatureBlob;
   ComPtr<ID3DBlob> errorBlob;
   HRESULT hr = D3DX12SerializeVersionedRootSignature(&rootSignatureDescription,
                                                      featureData.HighestVersion,
                                                      &rootSignatureBlob,
                                                      &errorBlob);
   CheckResult(hr, "Failed to serialize root signature");

   // Create the root signature.
   hr = pDevice->GetDevice()->CreateRootSignature(0,
                                                  rootSignatureBlob->GetBufferPointer(),
                                                  rootSignatureBlob->GetBufferSize(),
                                                  IID_PPV_ARGS(&pInfo->rootSignature));
   CheckResult(hr, "Failed to create root signature");
   if (FAILED(hr))
      return false;

   pipelineDesc.pRootSignature = pInfo->rootSignature.Get();

   //
   // add shaders
   pipelineDesc.VS = CD3DX12_SHADER_BYTECODE(pInfo->shaderBlob_VS.Get());
   pipelineDesc.PS = CD3DX12_SHADER_BYTECODE(pInfo->shaderBlob_PS.Get());
   

   D3D12_DEPTH_STENCIL_DESC depthStencilDesc = {};
   depthStencilDesc.DepthEnable = FALSE;
   depthStencilDesc.StencilEnable = FALSE;

   pipelineDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
   pipelineDesc.RasterizerState.FrontCounterClockwise = TRUE;
   pipelineDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
   pipelineDesc.SampleMask = UINT_MAX;
   pipelineDesc.DepthStencilState = depthStencilDesc;
   pipelineDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
   pipelineDesc.NumRenderTargets = 1;

   //pipelineDesc.DSVFormat = DXGI_FORMAT_D32_FLOAT; // should not matter, depth is disabled
   pipelineDesc.RTVFormats[0] = pInfo->renderTargetFormat;

   pipelineDesc.SampleDesc = { 1, 0 };
   
   hr = pDevice->GetDevice()->CreateGraphicsPipelineState(&pipelineDesc, IID_PPV_ARGS(&pInfo->pso));
   CheckResult(hr, "Texture converter: Failed to create pipeline state object");

   if (FAILED(hr))
      return false;

   return true;
}

void 
HgiDXTextureConverter::convert(HgiDXTexture* pTxSource, 
                               const D3D12_CPU_DESCRIPTOR_HANDLE& rtvHandle,
                               DXGI_FORMAT targetFormat,
                               int nWidth,
                               int nHeight)
{
   if (nullptr != pTxSource)
   {
      std::map<DXGI_FORMAT, std::unique_ptr<TxConvertPipelineInfo>>::const_iterator it = m_pipelineByOutput.find(targetFormat);
      if (it == m_pipelineByOutput.end())
      {
         _initialize(targetFormat);
         it = m_pipelineByOutput.find(targetFormat);
      }

      if (it != m_pipelineByOutput.end())
      {
         TxConvertPipelineInfo* pPipelineInfo = it->second.get();
         HgiDXDevice* pDevice = m_pHgi->GetPrimaryDevice();
         if (nullptr != pDevice)
         {
            ID3D12GraphicsCommandList* pCmdList = pDevice->GetCommandList(HgiDXDevice::kGraphics);

            if (nullptr != pCmdList)
            {
               //
               // bind pipeline
               pCmdList->SetPipelineState(pPipelineInfo->pso.Get());
               pCmdList->SetGraphicsRootSignature(pPipelineInfo->rootSignature.Get());
               pCmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

               //
               // viewport
               CD3DX12_VIEWPORT vp (0.0f, 0.0f, nWidth, nHeight);
               pCmdList->RSSetViewports(1, &vp);

               CD3DX12_RECT sr (0.0f, 0.0f, nWidth, nHeight);
               pCmdList->RSSetScissorRects(1, &sr);

               //
               // update data
               
               pCmdList->OMSetRenderTargets(1, &rtvHandle, TRUE, nullptr);
               
               pCmdList->ClearRenderTargetView(rtvHandle, ColorRGBA, 0, nullptr);

               pTxSource->UpdateResourceState(pCmdList, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
               ID3D12DescriptorHeap* pTxHeap = pTxSource->GetGPUDescHeap();
               ID3D12DescriptorHeap* descriptorHeaps[] = { pTxHeap };
               pCmdList->SetDescriptorHeaps(_countof(descriptorHeaps), descriptorHeaps);

               pCmdList->SetGraphicsRootDescriptorTable(0, pTxHeap->GetGPUDescriptorHandleForHeapStart());
               
               m_vertBuff->UpdateResourceState(pCmdList, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);
               D3D12_VERTEX_BUFFER_VIEW vbv;
               vbv.BufferLocation = m_vertBuff->GetGPUVirtualAddress();
               vbv.SizeInBytes = m_vertBuff->GetByteSizeOfResource();
               vbv.StrideInBytes = sizeof(VertexTex);

               pCmdList->IASetVertexBuffers(0, 1, &vbv);

               m_idxBuff->UpdateResourceState(pCmdList, D3D12_RESOURCE_STATE_INDEX_BUFFER);
               D3D12_INDEX_BUFFER_VIEW ibv;
               ibv.BufferLocation = m_idxBuff->GetGPUVirtualAddress();
               ibv.SizeInBytes = m_idxBuff->GetByteSizeOfResource();
               ibv.Format = DXGI_FORMAT_R32_UINT;

               pCmdList->IASetIndexBuffer(&ibv);

               //
               // execute
               UINT nIndices = sizeof(g_Indices) / sizeof(UINT);
               pCmdList->DrawIndexedInstanced(nIndices, 1, 0, 0, 0);

               pDevice->SubmitCommandList(HgiDXDevice::kGraphics);

            }
            else
               CheckResult(E_FAIL, "Cannot get command list. Failed to bind textures convert pipeline.");
         }
      }
   }
   else
      TF_WARN("Invalid operation. Both source and destination textures must be valid");
}

PXR_NAMESPACE_CLOSE_SCOPE
