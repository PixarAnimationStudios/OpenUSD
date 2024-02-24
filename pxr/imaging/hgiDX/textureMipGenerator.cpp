
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

#include "pxr/imaging/hgiDX/textureMipGenerator.h"

#include "pxr/base/tf/diagnostic.h"
#include "pxr/base/tf/getenv.h"

#include "pxr/imaging/hgiDX/buffer.h"
#include "pxr/imaging/hgiDX/conversions.h"
#include "pxr/imaging/hgiDX/device.h"
#include "pxr/imaging/hgiDX/hgi.h"
#include "pxr/imaging/hgiDX/shaderFunction.h"
#include "pxr/imaging/hgiDX/shaderProgram.h"
#include "pxr/imaging/hgiDX/texture.h"

//
// some useful references
//
// https://learn.microsoft.com/en-us/windows/win32/direct3d10/d3d10-graphics-programming-guide-resources-coordinates
// https://github.com/microsoft/DirectXTK12/blob/main/Src/ScreenGrab.cpp


using namespace DirectX;

PXR_NAMESPACE_OPEN_SCOPE
static const bool bShadersModel6 = TfGetenvBool("HGI_DX_SHADERS_MODEL_6", false);


HgiDXTextureMipGenerator::HgiDXTextureMipGenerator(HgiDX* pHgi)
   :_pHgi(pHgi)
{
   if (nullptr == _pHgi)
      TF_FATAL_CODING_ERROR("Texture Converter cannot work with invalid Hgi");
}

HgiDXTextureMipGenerator::~HgiDXTextureMipGenerator()
{
}

struct VertexTex
{
   DirectX::XMFLOAT4 Position;
   DirectX::XMFLOAT2 UV;
};

struct alignas(16) GenerateMipsCB
{
   uint32_t SrcMipLevel;           // Texture level of source mip
   uint32_t NumMipLevels;          // Number of OutMips to write: [1-4]
   uint32_t SrcDimension;          // Width and height of the source texture are even or odd.
   uint32_t IsSRGB;                // Must apply gamma correction to sRGB textures.
   DirectX::XMFLOAT2 TexelSize;    // 1.0 / OutMip1.Dimensions
};



static FLOAT ClearColor[4] = { 0.0f, 0.0f, 0.0f, 1.0f };

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
   "VS_STAGE_OUT mainDX(VS_STAGE_IN IN) {\n"
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
   "PS_STAGE_OUT mainDX(PS_STAGE_IN IN) {\n"
   "     PS_STAGE_OUT OUT;\n"
   "     OUT.colorOut = texIn.Sample(MeshTextureSampler, IN.uv);\n"
   "     return OUT;\n"
   "}";


void 
HgiDXTextureMipGenerator::_initialize(DXGI_FORMAT format)
{
   ComPtr<ID3DBlob> errorMsgs;

   std::string errors;
   ComPtr<IUnknown> vsBlob = HgiDXShaderCompiler::Compile(codeVS, HgiDXShaderCompiler::CompileTarget::kVS, errors);
   errors = "";
   ComPtr<IUnknown> psBlob = HgiDXShaderCompiler::Compile(codePS, HgiDXShaderCompiler::CompileTarget::kPS, errors);


   //pTxPipelineInfo->inputDescs.push_back(D3D12_INPUT_ELEMENT_DESC{ "POSITION", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 });
   //pTxPipelineInfo->inputDescs.push_back(D3D12_INPUT_ELEMENT_DESC{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 });

   CD3DX12_DESCRIPTOR_RANGE1 descriptorRage(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0);
   CD3DX12_ROOT_PARAMETER1 rp;
   rp.InitAsDescriptorTable(1, &descriptorRage, D3D12_SHADER_VISIBILITY_PIXEL);

   //pTxPipelineInfo->rootParams.push_back(rp);
   _buildPSO();
}

bool  
HgiDXTextureMipGenerator::_buildPSO()
{
   D3D12_GRAPHICS_PIPELINE_STATE_DESC pipelineDesc;
   ZeroMemory(&pipelineDesc, sizeof(pipelineDesc));

   HgiDXDevice* pDevice = _pHgi->GetPrimaryDevice();

   // Create a root signature.
   D3D12_FEATURE_DATA_ROOT_SIGNATURE featureData = {};
   featureData.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_1;
   if (FAILED(pDevice->GetDevice()->CheckFeatureSupport(D3D12_FEATURE_ROOT_SIGNATURE, &featureData, sizeof(featureData))))
   {
      featureData.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_0;
   }

   //
   // The other implementation(s) that I am drawing inspiration from seem to think
   // it's a good idea to calculate up to 4 mips at the same time, but no more
   // I have yet to understand the reasoning behind all that (maybe performance?)
   // but for the moment even if this makes the code more awkward and complicated I'll stick to their example

   CD3DX12_DESCRIPTOR_RANGE1 srcMip(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0, 0, D3D12_DESCRIPTOR_RANGE_FLAG_DESCRIPTORS_VOLATILE);
   CD3DX12_DESCRIPTOR_RANGE1 outMip(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 4, 0, 0, D3D12_DESCRIPTOR_RANGE_FLAG_DESCRIPTORS_VOLATILE);

   std::vector<CD3DX12_ROOT_PARAMETER1> rootParameters(3);
   rootParameters[0].InitAsConstants(sizeof(GenerateMipsCB) / 4, 0);
   rootParameters[1].InitAsDescriptorTable(1, &srcMip);
   rootParameters[2].InitAsDescriptorTable(1, &outMip);

   CD3DX12_STATIC_SAMPLER_DESC linearClampSampler(
      0,
      D3D12_FILTER_MIN_MAG_MIP_LINEAR,
      D3D12_TEXTURE_ADDRESS_MODE_CLAMP,
      D3D12_TEXTURE_ADDRESS_MODE_CLAMP,
      D3D12_TEXTURE_ADDRESS_MODE_CLAMP
   );

   CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC rootSignatureDescription;
   rootSignatureDescription.Init_1_1((int)rootParameters.size(), rootParameters.data(), 1, &linearClampSampler);

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
                                                  IID_PPV_ARGS(&_rootSignature));
   CheckResult(hr, "Failed to create root signature");
   if (FAILED(hr))
      return false;

   ID3D12DescriptorHeap* pHeap = pDevice->GetCbvSrvUavDescriptorHeap();
   UINT nHeapDescSize = pDevice->GetCbvSrvUavDescriptorHeapIncrementSize();

   //
   // generate some default MIPS UAV to have someting to map to the pipeline even when we need to generate less than 4 mips
   for (UINT i = 0; i < 4; ++i)
   {
      D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
      uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
      uavDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
      uavDesc.Texture2D.MipSlice = i;
      uavDesc.Texture2D.PlaneSlice = 0;

      D3D12_CPU_DESCRIPTOR_HANDLE cdh = CD3DX12_CPU_DESCRIPTOR_HANDLE(pHeap->GetCPUDescriptorHandleForHeapStart(), i, nHeapDescSize);

      pDevice->GetDevice()->CreateUnorderedAccessView(nullptr, nullptr, &uavDesc, cdh);
   }

   return true;
}

void 
HgiDXTextureMipGenerator::generate(HgiDXTexture* pTxSource)
{
   if (nullptr != pTxSource)
   {
      //
      // I want to (setup once) + activate the pipeline

      //
      // for each mip
      //    set the input / output textures parameters
      //    execute (num threads?!)



   }
   else
      TF_WARN("Invalid generate mips operation. Texture must be valid");
}

PXR_NAMESPACE_CLOSE_SCOPE
