
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
#include "pxr/base/tf/iterator.h"

#include "pxr/imaging/hgiDX/capabilities.h"
#include "pxr/imaging/hgiDX/conversions.h"
#include "pxr/imaging/hgiDX/device.h"
#include "pxr/imaging/hgiDX/graphicsPipeline.h"
#include "pxr/imaging/hgiDX/shaderFunction.h"
#include "pxr/imaging/hgiDX/shaderProgram.h"
#include "pxr/imaging/hgiDX/texture.h"


PXR_NAMESPACE_OPEN_SCOPE

using HgiAttachmentDescConstPtrVector = std::vector<HgiAttachmentDesc const*>;


HgiDXGraphicsPipeline::HgiDXGraphicsPipeline(HgiDXDevice* device, HgiGraphicsPipelineDesc const& desc)
    : HgiGraphicsPipeline(desc)
    , _device(device)
{

   D3D12_GRAPHICS_PIPELINE_STATE_DESC pipelineDesc;
   ZeroMemory(&pipelineDesc, sizeof(pipelineDesc));

   // Create a root signature.
   D3D12_FEATURE_DATA_ROOT_SIGNATURE featureData = {};
   featureData.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_1;
   if (FAILED(_device->GetDevice()->CheckFeatureSupport(D3D12_FEATURE_ROOT_SIGNATURE, &featureData, sizeof(featureData))))
   {
      featureData.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_0;
   }

   //
   // I should be able to not hard-code this in the future
   // the shader functions can help me know what I need and where probably
   // Allow input layout and deny unnecessary access to certain pipeline stages.
   D3D12_ROOT_SIGNATURE_FLAGS rootSignatureFlags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT |
                                                   D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS |
                                                   D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS;

   // A single 32-bit constant root parameter that is used by the vertex shader.
   // we want to pass the combined matrices through here...
   HgiDXShaderProgram* pShaderProgram = dynamic_cast<HgiDXShaderProgram*>(desc.shaderProgram.Get());

   std::vector<D3D12_INPUT_ELEMENT_DESC> inputInfo = pShaderProgram->GetInputLayout(desc.vertexBuffers);
   pipelineDesc.InputLayout = { inputInfo.data(), (unsigned int)inputInfo.size() };

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
   hr = _device->GetDevice()->CreateRootSignature(0,
                                                 rootSignatureBlob->GetBufferPointer(),
                                                 rootSignatureBlob->GetBufferSize(),
                                                 IID_PPV_ARGS(&_rootSignature));
   CheckResult(hr, "Failed to create root signature");

   pipelineDesc.pRootSignature = _rootSignature.Get();


   

   //
   // add shaders
   HgiShaderFunctionHandleVector shaderFcs = pShaderProgram->GetShaderFunctions();
   for (HgiShaderFunctionHandle sfh : shaderFcs)
   {
      HgiDXShaderFunction* pdfsfc = dynamic_cast<HgiDXShaderFunction*>(sfh.Get());
      if (nullptr != pdfsfc)
      {
         ID3DBlob* pBlob = pdfsfc->GetShaderBlob();

         switch (pdfsfc->GetDescriptor().shaderStage)
         {
            case HgiShaderStageVertex:
               pipelineDesc.VS = CD3DX12_SHADER_BYTECODE(pBlob);
               break;
            case HgiShaderStageGeometry:
               pipelineDesc.GS = CD3DX12_SHADER_BYTECODE(pBlob);
               break;
            case HgiShaderStageFragment:
               pipelineDesc.PS = CD3DX12_SHADER_BYTECODE(pBlob);
               break;
            default:
               TF_CODING_ERROR("Shader stage not implemented yet");
               break;
         }
      }
   }

   //
   // TODO: I'll want to setup some of these settings below, properly
   // but for now the focus is elsewhere
   /*
   D3D12_RASTERIZER_DESC rasterizerDesc;
   ZeroMemory(&rasterizerDesc, sizeof(rasterizerDesc));
   rasterizerDesc.FillMode = D3D12_FILL_MODE_SOLID;
   rasterizerDesc.CullMode = D3D12_CULL_MODE_NONE;// D3D12_CULL_MODE_BACK;
   rasterizerDesc.FrontCounterClockwise = FALSE;
   rasterizerDesc.DepthBias = D3D12_DEFAULT_DEPTH_BIAS;
   rasterizerDesc.DepthBiasClamp = D3D12_DEFAULT_DEPTH_BIAS_CLAMP;
   rasterizerDesc.SlopeScaledDepthBias = D3D12_DEFAULT_SLOPE_SCALED_DEPTH_BIAS;
   rasterizerDesc.DepthClipEnable = FALSE;
   rasterizerDesc.MultisampleEnable = FALSE;
   rasterizerDesc.AntialiasedLineEnable = FALSE;
   rasterizerDesc.ForcedSampleCount = 0;
   rasterizerDesc.ConservativeRaster = D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;*/

   /*
   D3D12_RENDER_TARGET_BLEND_DESC blendRenderTargetDesc;
   ZeroMemory(&blendRenderTargetDesc, sizeof(blendRenderTargetDesc));
   blendRenderTargetDesc.BlendEnable = FALSE;

   D3D12_BLEND_DESC blendDesc;
   ZeroMemory(&blendDesc, sizeof(blendDesc));
   blendDesc.RenderTarget[0] = blendRenderTargetDesc;*/

   BOOL bDepthEnable = desc.depthState.depthTestEnabled;
   BOOL bStencilEnable = desc.depthState.stencilTestEnabled;

   if (desc.colorAttachmentDescs.size() + desc.colorResolveAttachmentDescs.size() > 8)
      TF_WARN("Too many color targets. DX seems to support max 8");

   int nIdxRT = 0;
   for (HgiAttachmentDesc const& hgiAttDesc : desc.colorAttachmentDescs)
      pipelineDesc.RTVFormats[nIdxRT++] = HgiDXConversions::GetTextureFormat(hgiAttDesc.format);

   //
   // DirectX does not allow different rendering targets, e.g. one with 4 MS and one with 1 (at the same time)
   // it also does not allow multiple depth targets, so I do not need to use the "resolve" buffers for DX 
   //  in this phase, they will be used later to "resolve" the multi-sample RTV into 
   //for (HgiAttachmentDesc const& hgiAttDesc : desc.colorResolveAttachmentDescs)
   //   pipelineDesc.RTVFormats[nIdxRT++] = HgiDXTexture::GetTextureFormat(hgiAttDesc.format);

   pipelineDesc.DSVFormat = HgiDXConversions::GetTextureFormat(desc.depthAttachmentDesc.format);

   D3D12_DEPTH_STENCIL_DESC depthStencilDesc = {};
   depthStencilDesc.DepthEnable = bDepthEnable;
   depthStencilDesc.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL; 
   depthStencilDesc.DepthFunc = _GetDepthCompareFc(desc.depthState.depthCompareFn);
   depthStencilDesc.StencilEnable = bStencilEnable;

   if (bStencilEnable)
      TF_WARN("Stencil parameters not properly setup yet.");
   //depthStencilDesc.FrontFace = desc.depthState.stencilFront;
   //depthStencilDesc.BackFace = desc.depthState.stencilBack;
   //depthStencilDesc.StencilReadMask = ;
   //depthStencilDesc.StencilWriteMask = ;


   //pipelineDesc.RasterizerState = rasterizerDesc;
   pipelineDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
   pipelineDesc.RasterizerState.FrontCounterClockwise = (desc.rasterizationState.winding == HgiWindingCounterClockwise);
   //pipelineDesc.BlendState = blendDesc;  
   pipelineDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
   pipelineDesc.SampleMask = UINT_MAX;
   //pipelineDesc.StreamOutput
   pipelineDesc.DepthStencilState = depthStencilDesc;
   //pipelineDesc.IBStripCutValue
   pipelineDesc.PrimitiveTopologyType = _GetTopologyType(); 
   pipelineDesc.NumRenderTargets = nIdxRT;
   

   pipelineDesc.SampleDesc = { (UINT)desc.multiSampleState.sampleCount, 0 };
   //pipelineDesc.NodeMask
   //pipelineDesc.CachedPSO
   //pipelineDesc.Flags

   hr = _device->GetDevice()->CreateGraphicsPipelineState(&pipelineDesc, IID_PPV_ARGS(&_pipelineState));
   CheckResult(hr, "Failed to create pipeline state object");
}

HgiDXGraphicsPipeline::~HgiDXGraphicsPipeline()
{
}

void
HgiDXGraphicsPipeline::BindPipeline()
{
   ID3D12GraphicsCommandList* pCmdList = _device->GetCommandList(HgiDXDevice::eCommandType::kGraphics);

   if (nullptr != pCmdList)
   {
      pCmdList->SetPipelineState(_pipelineState.Get());
      pCmdList->SetGraphicsRootSignature(_rootSignature.Get());

      pCmdList->IASetPrimitiveTopology(_GetTopology());
   }
   else
      TF_WARN("Cannot get command list. Failed to bind pipeline.");
}

ID3D12CommandSignature* 
HgiDXGraphicsPipeline::GetIndirectCommandSignature(uint32_t stride)
{
   if (_indirectArgumentStride != stride)
   {
      //
      // Build or re-build it now. 
      // My current theory is that I only need to rebuild it if I get here a different stride for some reason.

      //
      // What I observed experimentally, the way I usually get called to draw indirect is with an argument
      // built like this (HdSt_PipelineDrawBatch::_CompileBatch):
      // 
      // *cmdIt++ = indexCount;
      // *cmdIt++ = instanceCount;
      // *cmdIt++ = baseIndex;
      // *cmdIt++ = baseVertex;
      // *cmdIt++ = baseInstance;
      // +
      // *cmdIt++ = drawingCoord0, drawingCoord1, drawingCoord2 data, potentially plus more
      // At this time, I do not understand what would be a robust way for me to know what kind of arguments I am going to get
      // Seems some idea was started, that might help with this (IndirectCommandEncoder) although at this time
      // one the one hand that is not called, and on the other it still does not provide information about 
      // the indirect arguments structure

      //
      // another interesting observation is that the drawing0, 1, 2 arguments seem to also be passed normally 
      // via BindVertexBuffers, so I will hope the information is redundant and try to ignore it

      //
      // "luckily" the structure of the data passed for each indirect draw call matches perfectly with 
      // D3D12_INDIRECT_ARGUMENT_TYPE_DRAW_INDEXED, therefore:
      //struct IndirectCommand
      //{
      //   D3D12_DRAW_INDEXED_ARGUMENTS drawArguments;
      //};

      // Each command consists of a DrawInstanced call.
      D3D12_INDIRECT_ARGUMENT_DESC argumentDescs[1] = {};
      argumentDescs[0].Type = D3D12_INDIRECT_ARGUMENT_TYPE_DRAW_INDEXED;
      

      D3D12_COMMAND_SIGNATURE_DESC commandSignatureDesc = {};
      commandSignatureDesc.pArgumentDescs = argumentDescs;
      commandSignatureDesc.NumArgumentDescs = _countof(argumentDescs);
      commandSignatureDesc.ByteStride = stride; // sizeof(IndirectCommand); // I hope that will make Dx read the information I understand and ignore the rest

      //
      // A root signature should be specified if and only if the command signature indicates that root arguments will be changed
      _indirectCommandSignature.Reset();
      HRESULT hr = _device->GetDevice()->CreateCommandSignature(&commandSignatureDesc, nullptr, IID_PPV_ARGS(&_indirectCommandSignature));
      CheckResult(hr, "Failed to setup indirect command signature.");
   }

   return _indirectCommandSignature.Get();
}

D3D12_PRIMITIVE_TOPOLOGY_TYPE 
HgiDXGraphicsPipeline::_GetTopologyType()
{
   D3D12_PRIMITIVE_TOPOLOGY_TYPE ret = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;

   switch (_descriptor.primitiveType)
   {
   case HgiPrimitiveTypePointList:
      ret = D3D12_PRIMITIVE_TOPOLOGY_TYPE_POINT;
      break;
   case HgiPrimitiveTypeLineList:
      ret = D3D12_PRIMITIVE_TOPOLOGY_TYPE_LINE;
      break;
   case HgiPrimitiveTypeTriangleList:
      ret = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
      break;
   case HgiPrimitiveTypePatchList:
      ret = D3D12_PRIMITIVE_TOPOLOGY_TYPE_PATCH;
      break;
   case HgiPrimitiveTypeLineListWithAdjacency:
      ret = D3D12_PRIMITIVE_TOPOLOGY_TYPE_LINE;
      break;
   case HgiPrimitiveTypeLineStrip:
      ret = D3D12_PRIMITIVE_TOPOLOGY_TYPE_LINE;
      break;
   default:
      TF_WARN("Unknown topology type mapping to DirectX");
      break;
   }

   return ret;
}

D3D12_PRIMITIVE_TOPOLOGY 
HgiDXGraphicsPipeline::_GetTopology()
{
   D3D12_PRIMITIVE_TOPOLOGY ret = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;

   switch (_descriptor.primitiveType)
   {
   case HgiPrimitiveTypePointList:
      ret = D3D_PRIMITIVE_TOPOLOGY_POINTLIST;
      break;
   case HgiPrimitiveTypeLineList:
      ret = D3D_PRIMITIVE_TOPOLOGY_LINELIST;
      break;
   case HgiPrimitiveTypeLineStrip:
      ret = D3D_PRIMITIVE_TOPOLOGY_LINESTRIP;
      break;
   case HgiPrimitiveTypeTriangleList:
      ret = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
      break;
   case HgiPrimitiveTypeLineListWithAdjacency:
      ret = D3D_PRIMITIVE_TOPOLOGY_LINELIST_ADJ;
      break;
   case HgiPrimitiveTypePatchList:
   default:
      TF_WARN("Unknown topology type mapping to DirectX");
      break;
   }

   return ret;
}

D3D12_COMPARISON_FUNC 
HgiDXGraphicsPipeline::_GetDepthCompareFc(HgiCompareFunction fc)
{
   D3D12_COMPARISON_FUNC ret = D3D12_COMPARISON_FUNC_LESS_EQUAL;
   switch (fc)
   {
   case HgiCompareFunctionNever:
      ret = D3D12_COMPARISON_FUNC_NEVER;
      break;
   case HgiCompareFunctionLess:
      ret = D3D12_COMPARISON_FUNC_LESS;
      break;
   case HgiCompareFunctionEqual:
      ret = D3D12_COMPARISON_FUNC_EQUAL;
      break;
   case HgiCompareFunctionLEqual:
      ret = D3D12_COMPARISON_FUNC_LESS_EQUAL;
      break;
   case HgiCompareFunctionGreater:
      ret = D3D12_COMPARISON_FUNC_GREATER;
      break;
   case HgiCompareFunctionNotEqual:
      ret = D3D12_COMPARISON_FUNC_NOT_EQUAL;
      break;
   case HgiCompareFunctionGEqual:
      ret = D3D12_COMPARISON_FUNC_GREATER_EQUAL;
      break;
   case HgiCompareFunctionAlways:
      ret = D3D12_COMPARISON_FUNC_ALWAYS;
      break;
   default:
      TF_WARN("Unexpected value for depth compare function.");
      break;
   }

   return ret;
}

PXR_NAMESPACE_CLOSE_SCOPE
