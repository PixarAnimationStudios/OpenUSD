
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

#include "pxr/imaging/hgiDX/computePipeline.h"
#include "pxr/imaging/hgiDX/device.h"
#include "pxr/imaging/hgiDX/shaderFunction.h"
#include "pxr/imaging/hgiDX/shaderProgram.h"

PXR_NAMESPACE_OPEN_SCOPE

HgiDXComputePipeline::HgiDXComputePipeline(HgiDXDevice* device, HgiComputePipelineDesc const& desc)
    : HgiComputePipeline(desc)
    , _device(device)
{
   D3D12_COMPUTE_PIPELINE_STATE_DESC pipelineDesc;
   ZeroMemory(&pipelineDesc, sizeof(pipelineDesc));

   // Create a root signature.
   D3D12_FEATURE_DATA_ROOT_SIGNATURE featureData = {};
   featureData.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_1;
   if (FAILED(device->GetDevice()->CheckFeatureSupport(D3D12_FEATURE_ROOT_SIGNATURE, &featureData, sizeof(featureData))))
   {
      featureData.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_0;
   }

   //
   // TODO: I should be able to not hard-code this in the future:
   // The shader functions can help me know what I need and where probably
   // so I can allow or deny unnecessary access of pipeline stages to resources.
   D3D12_ROOT_SIGNATURE_FLAGS rootSignatureFlags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT |
                                                   D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS |
                                                   D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS;

   HgiDXShaderProgram* pShaderProgram = dynamic_cast<HgiDXShaderProgram*>(desc.shaderProgram.Get());

   CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC rootSignatureDescription;
   std::vector<CD3DX12_ROOT_PARAMETER1> rootParams = pShaderProgram->GetRootParameters();
   rootSignatureDescription.Init_1_1((int)rootParams.size(), rootParams.data(), 0, nullptr, rootSignatureFlags);

   // Serialize the root signature.
   ComPtr<ID3DBlob> rootSignatureBlob;
   ComPtr<ID3DBlob> errorBlob;
   HRESULT hr = D3DX12SerializeVersionedRootSignature(&rootSignatureDescription,
                                                      featureData.HighestVersion,
                                                      &rootSignatureBlob,
                                                      &errorBlob);
   CheckResult(hr, "Failed to serialize root signature");

   // Create the root signature.
   hr = device->GetDevice()->CreateRootSignature(0,
                                                 rootSignatureBlob->GetBufferPointer(),
                                                 rootSignatureBlob->GetBufferSize(),
                                                 IID_PPV_ARGS(&_rootSignature));
   CheckResult(hr, "Failed to create root signature");

   pipelineDesc.pRootSignature = _rootSignature.Get();

   //
   // add shaders
   HgiShaderFunctionHandleVector shaderFcs = pShaderProgram->GetShaderFunctions();
   if ((shaderFcs.size() > 0) && (shaderFcs.size() < 2))
   {
      HgiDXShaderFunction* pdfsfc = dynamic_cast<HgiDXShaderFunction*>(shaderFcs[0].Get());
      if (nullptr != pdfsfc)
      {
         if (pdfsfc->GetDescriptor().shaderStage != HgiShaderStageCompute)
            TF_WARN("Unexpected shader function type for compute pipeline.");

         ID3DBlob* pBlob = pdfsfc->GetShaderBlob();
         pipelineDesc.CS = CD3DX12_SHADER_BYTECODE(pBlob);
      }
   }
   else
      TF_WARN("Unexpected number of shaedr functions for compute pipeline.");
   
   //
   // TODO: what else could we / should we set for this PSO?
   //pipelineDesc.NodeMask
   //pipelineDesc.CachedPSO
   //pipelineDesc.Flags

   hr = device->GetDevice()->CreateComputePipelineState(&pipelineDesc, IID_PPV_ARGS(&_pipelineState));
   CheckResult(hr, "Failed to create pipeline state object");
}

void 
HgiDXComputePipeline::BindPipeline()
{
   ID3D12GraphicsCommandList* pCmdList = _device->GetCommandList(HgiDXDevice::eCommandType::kCompute);

   if (nullptr != pCmdList)
   {
      pCmdList->SetPipelineState(_pipelineState.Get());
      pCmdList->SetComputeRootSignature(_rootSignature.Get());
   }
   else
      TF_WARN("Cannot get command list. Failed to bind pipeline.");
}

HgiDXComputePipeline::~HgiDXComputePipeline()
{
}

PXR_NAMESPACE_CLOSE_SCOPE
