
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
#include "pxr/imaging/hgiDX/buffer.h"
#include "pxr/imaging/hgiDX/conversions.h"
#include "pxr/imaging/hgiDX/shaderProgram.h"
#include "pxr/imaging/hgiDX/shaderFunction.h"
#include "pxr/imaging/hgi/graphicsPipeline.h"

PXR_NAMESPACE_OPEN_SCOPE


using namespace DirectX;

HgiDXShaderProgram::HgiDXShaderProgram(HgiDXDevice* device, HgiShaderProgramDesc const& desc)
    : HgiShaderProgram(desc)
    , _device(device)
{
}

bool
HgiDXShaderProgram::IsValid() const
{
    return true;
}

std::string const&
HgiDXShaderProgram::GetCompileErrors()
{
    static const std::string empty;
    return empty;
}

size_t
HgiDXShaderProgram::GetByteSizeOfResource() const
{
    size_t  byteSize = 0;
    for (HgiShaderFunctionHandle const& fn : _descriptor.shaderFunctions) {
        byteSize += fn->GetByteSizeOfResource();
    }
    return byteSize;
}

uint64_t
HgiDXShaderProgram::GetRawResource() const
{
    return 0; // not sure what this is supposed to be
}

HgiShaderFunctionHandleVector const&
HgiDXShaderProgram::GetShaderFunctions() const
{
   return _descriptor.shaderFunctions;
}

HgiDXDevice*
HgiDXShaderProgram::GetDevice() const
{
    return _device;
}



std::vector<D3D12_INPUT_ELEMENT_DESC> 
HgiDXShaderProgram::GetInputLayout(const std::vector<HgiVertexBufferDesc>& vbdv) const
{
   std::vector<D3D12_INPUT_ELEMENT_DESC> ret;
   
   //
   // just setting the slot info correctly does not seem to be enough, 
   // apparently the order of parameters declarations matters more for DirectX
   HgiShaderFunctionHandleVector shaderFcs = GetShaderFunctions();
   for (HgiShaderFunctionHandle sfh : shaderFcs)
   {
      HgiDXShaderFunction* pDxSfc = dynamic_cast<HgiDXShaderFunction*>(sfh.Get());
      if ((nullptr != pDxSfc) && (pDxSfc->GetDescriptor().shaderStage == HgiShaderStageVertex))
      {
         const std::vector<DXShaderInfo::StageParamInfo>& sii = pDxSfc->GetStageInputInfo();

         //
         // for directx the bind location needs to translate into the order of parameters declaration
         // but I do not trust the data I'm getting here without a bit of sanity check
         
         for (const DXShaderInfo::StageParamInfo& siiCurr : sii)
         {
            if (siiCurr.nSuggestedBindingIdx != UINT_MAX)
            {
               if (_inputBindIdx2ShaderData.find(siiCurr.nSuggestedBindingIdx) != _inputBindIdx2ShaderData.end())
                  TF_WARN("Error. Overlapping binding of input parameters.");

               _inputBindIdx2ShaderData.insert(std::pair<uint32_t, DXShaderInfo::StageParamInfo>(siiCurr.nSuggestedBindingIdx, siiCurr));
            }
         }

         break;
      }
   }

   for (const HgiVertexBufferDesc& vbd : vbdv)
   {
      uint32_t nInputSlot = vbd.bindingIndex;
      for (const HgiVertexAttributeDesc& vad : vbd.vertexAttributes)
      {
         D3D12_INPUT_ELEMENT_DESC iedCurr;
         iedCurr.InputSlot = nInputSlot;
         iedCurr.AlignedByteOffset = vad.offset;
         iedCurr.Format = HgiDXConversions::GetTextureFormat(vad.format);

         //
         // TODO: the thing is Dx actually knows how to auto-expand R10G10B10A2 into some type of vec4
         // but we still need to tell it this was a R10G10B10A2.
         // 
         // Unfortunately, the code at "resourceBinder.cpp" / ln ~338
         // if it converts the encoding it does it so that I no longer have a chance to know and tell DX that data
         // will be passed as "packed" (or I need to dig more and see if that information is still accessible somewhere)
         // 
         // The reverse, if I allow the code to flow normally, not force convert data in "resourceBinder.cpp" / ln ~338
         // I also generate shader code using an int instead of a vec4, 
         // but becasue we tell Dx we are using a R10G10B10A2, it does some sort of auto unpack for us
         // and puts inside that int only a part of the entire data, making us loose 2/3rds of the "smooth normals" data
         // 
         // what I am (temporarily) doing here is throw Dx off not leting it know we are using a packed format
         // because we have custom code which unpacks data 
         //
         // I guess the much better solution migth be to allow the generated code to expand the data in the shader to vec4
         // (rollback my dx special case in "resourceBinder.cpp") but also be able to tell DX here that the input is packed.
         if (iedCurr.Format == DXGI_FORMAT_R10G10B10A2_UINT)
            iedCurr.Format = DXGI_FORMAT_R32_SINT;

         // D32_float not accepted by input assembler
         if (iedCurr.Format == DXGI_FORMAT_D32_FLOAT)
            iedCurr.Format = DXGI_FORMAT_R32_FLOAT;
         
         auto itSD = _inputBindIdx2ShaderData.find(vad.shaderBindLocation);
         if (itSD != _inputBindIdx2ShaderData.end())
         {
            iedCurr.SemanticName = itSD->second.strSemanticPipelineName.c_str();
            iedCurr.SemanticIndex = itSD->second.nSemanticPipelineIndex; // this is for when we have several semantics with same name and different idx, e.g. drawinCoord 0, 1, 2
            
            if(iedCurr.Format != itSD->second.Format)
               TF_WARN("Inconsistent vertex input binding information between HDSt and shaders provided data.");
         }
         else
            TF_WARN("Failed to acquire vertex input binding information from shaders.");
         
         
         if (vbd.vertexStepFunction != HgiVertexBufferStepFunction::HgiVertexBufferStepFunctionPerVertex)
         {
            UINT nStepRate = 0;
            if (vbd.vertexStepFunction == HgiVertexBufferStepFunction::HgiVertexBufferStepFunctionPerInstance)
               nStepRate = 1; // hopefully this tells the program to move to the next data for each instance
            else if (vbd.vertexStepFunction == HgiVertexBufferStepFunction::HgiVertexBufferStepFunctionPerDrawCommand)
               nStepRate = MAXUINT; // hopefully this tells the program to never move to the next data (except in a new draw command)
            else
               TF_WARN("This type of vertex info is not implemented yet (is it supported by DirectX?).");

            iedCurr.InputSlotClass = D3D12_INPUT_CLASSIFICATION_PER_INSTANCE_DATA;
            iedCurr.InstanceDataStepRate = nStepRate;
         }
         else
         {
            iedCurr.InputSlotClass = D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA;
            iedCurr.InstanceDataStepRate = 0;
         }

         ret.push_back(iedCurr);
      }
   }
   
   return ret;
}


//
// I need to compensate for the fact that currently I have my shaders hard-coded
// and the code generation apparently does not always generate the shaders buffers in the same order
// and with the same binding id request
uint32_t
_GetHardCodedBindingIdx(const std::string& strBufferName)
{
   int nRet = -1;
   if (strBufferName == "lightingContext")
      nRet = 0;
   else if (strBufferName == "lightSource")
      nRet = 1;
   else if (strBufferName == "material")
      nRet = 2;
   else if (strBufferName == "renderPassState")
      nRet = 3;

   else if (strBufferName == "constantPrimvars")
      nRet = 4;
   else if (strBufferName == "primitiveParam")
      nRet = 5;
   else if (strBufferName == "edgeIndices")
      nRet = 6;

   //
   // compute shader
   else if (strBufferName == "params")
      nRet = 0;
   else if (strBufferName == "points")
      nRet = 1;
   else if (strBufferName == "normals")
      nRet = 2;
   else if (strBufferName == "entry")
      nRet = 3;

   if (nRet < 0)
      TF_WARN("Unexpected buffer binding.");

   return nRet;
}


std::vector<CD3DX12_ROOT_PARAMETER1> 
HgiDXShaderProgram::GetRootParameters() const
{
   std::vector<CD3DX12_ROOT_PARAMETER1> ret;

   //
   // I will collect these from all the stages
   HgiShaderFunctionHandleVector shaderFcs = GetShaderFunctions();
   for (HgiShaderFunctionHandle sfh : shaderFcs)
   {
      HgiDXShaderFunction* pDxSfc = dynamic_cast<HgiDXShaderFunction*>(sfh.Get());
      if (nullptr != pDxSfc)
      {
         const std::vector<DXShaderInfo::RootParamInfo>& rootParams = pDxSfc->GetStageRootParamInfo();
         for (DXShaderInfo::RootParamInfo rp : rootParams)
         {
            std::map<UINT, DXShaderInfo::RootParamInfo>::const_iterator it = _rootParamsBySuggestedBindIdx.find(rp.nSuggestedBindingIdx);
            if (it != _rootParamsBySuggestedBindIdx.end())
            {
               if (it->second.strName != rp.strName)
               {
                  //
                  // From what I could tell, this does happen, but if it does,
                  // it can be handled by using different register spaces for the params
                  TF_WARN("Overlapping root params definitions binding");
               }
            }
            else
            {
               //
               // I should be able to drop the hard-coded bindings by now
               //rp.nBindingIdx = rp.nShaderRegister = _GetHardCodedBindingIdx(rp.strName);
               rp.nBindingIdx = rp.nShaderRegister = rp.nShaderRegister;
               rp.nRegisterSpace = 0;
               _rootParamsBySuggestedBindIdx[rp.nSuggestedBindingIdx] = rp;
            }   
         }

         break;
      }
   }

   //
   // For DirectX the order of root parameters matters more than the bind register
   // and since I know HDSt defines input with gaps, I'll declare them in order
   // and note the new position (for later when I bind the buffers)
   
   ret.resize(_rootParamsBySuggestedBindIdx.size());

   int nIdx = 0;
   std::map<UINT, DXShaderInfo::RootParamInfo>::iterator it = _rootParamsBySuggestedBindIdx.begin();
   while (it != _rootParamsBySuggestedBindIdx.end())
   {
      //CD3DX12_ROOT_PARAMETER1& rp = ret[it->second.nBindingIdx];
      CD3DX12_ROOT_PARAMETER1& rp = ret[nIdx];
      if (it->second.bConst)
         rp.InitAsConstantBufferView(it->second.nShaderRegister, it->second.nRegisterSpace);
      else if(it->second.bWritable)
         rp.InitAsUnorderedAccessView(it->second.nShaderRegister, it->second.nRegisterSpace);
      else
         rp.InitAsShaderResourceView(it->second.nShaderRegister, it->second.nRegisterSpace);
      
      it->second.nBindingIdx = nIdx;
      
      nIdx++;
      it++;
   }

   return ret;
}

bool
HgiDXShaderProgram::GetInfo(UINT nSuggestedBindIdx, DXShaderInfo::RootParamInfo& rpi, bool bMovedParam) const
{
   bool bRet = false;

   std::map<UINT, DXShaderInfo::RootParamInfo>::const_iterator it = _rootParamsBySuggestedBindIdx.find(nSuggestedBindIdx);
   if (it != _rootParamsBySuggestedBindIdx.end())
   {
      bRet = true;
      rpi = it->second;
   }
   
   return bRet;
}

PXR_NAMESPACE_CLOSE_SCOPE
