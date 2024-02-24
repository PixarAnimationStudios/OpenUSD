
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
#include "pxr/imaging/hgiDX/shaderFunction.h"

#include "pxr/imaging/hgiDX/device.h"
#include "pxr/imaging/hgiDX/hgi.h"
#include "pxr/imaging/hgiDX/shaderGenerator.h"

#include "pxr/base/tf/diagnostic.h"
#include "pxr/base/tf/envSetting.h"
#include "pxr/base/tf/getenv.h"

#include <fstream>

PXR_NAMESPACE_OPEN_SCOPE
static const bool bShadersModel6 = TfGetenvBool("HGI_DX_SHADERS_MODEL_6", false);

HgiDXShaderFunction::HgiDXShaderFunction(HgiDXDevice* device,
                                         Hgi const* hgi,
                                         HgiShaderFunctionDesc const& desc,
                                         int shaderVersion)
   : HgiShaderFunction(desc)
   , _hgi(hgi)
   , _device(device)
{
   const char* debugLbl = _descriptor.debugName.empty() ? "unknown" : _descriptor.debugName.c_str();

   // get source code and target
   std::string strShaderCode;
   HgiDXShaderCompiler::CompileTarget ct;
   _GetShaderCode(strShaderCode, ct);

   bool bDebugTest = false;
   if (bDebugTest)
   {
      std::ifstream fileStream("D:\\_work\\USD\\linetypes\\experiment.cpp");
      std::stringstream buffer;
      buffer << fileStream.rdbuf();
      strShaderCode = buffer.str();

      ct = HgiDXShaderCompiler::CompileTarget::kVS;
   }

   _shaderBlob = HgiDXShaderCompiler::Compile(strShaderCode, ct, _errors);

   //
   // Not 100% sure about the code below, this is what they did for OpenGL and vulkan... 
   // makes a bit of sense... leaving it here for now.
   // 
   // Clear these pointers in our copy of the descriptor since we
   // have to assume they could become invalid after we return.
   _descriptor.shaderCodeDeclarations = "";
   _descriptor.shaderCode = "";
   _descriptor.generatedShaderCodeOut = nullptr;
}

HgiDXShaderFunction::~HgiDXShaderFunction()
{
}

bool
HgiDXShaderFunction::IsValid() const
{
   return _errors.empty();
}

std::string const&
HgiDXShaderFunction::GetCompileErrors()
{
   return _errors;
}

size_t
HgiDXShaderFunction::GetByteSizeOfResource() const
{
   size_t ret = 0;

   if (nullptr != _shaderBlob)
   {
      if (bShadersModel6) {
         ret = ((IDxcBlob*)_shaderBlob.Get())->GetBufferSize();
      }
      else {
         ret = ((ID3DBlob*)_shaderBlob.Get())->GetBufferSize();
      }
   }
 
   return ret;
}

uint64_t
HgiDXShaderFunction::GetRawResource() const
{
   return (uint64_t)GetShaderBlob();
}

IUnknown*
HgiDXShaderFunction::GetShaderBlob() const
{
   return _shaderBlob.Get();
}

const std::vector<DXShaderInfo::StageParamInfo>&
HgiDXShaderFunction::GetStageInputInfo() const
{
   return _inputInfo;
}

const std::vector<DXShaderInfo::RootParamInfo>& 
HgiDXShaderFunction::GetStageRootParamInfo() const
{
   return _rootParamInfo;
}

HgiDXDevice*
HgiDXShaderFunction::GetDevice() const
{
   return _device;
}

void 
HgiDXShaderFunction::_GetShaderCode(std::string& strShaderCode, HgiDXShaderCompiler::CompileTarget& ct)
{
   //
   // The code & paths below are something that work in my test environment (project) only
   // They are sometimes useful for debugging purposes 
   // (quick check that a certain small change in a shader is good, 
   // e.g. before going into codegen.cpp or shaderGenerator.cpp and making changes there)
   std::string strShaderFile;
   switch (_descriptor.shaderStage) {
      case HgiShaderStageVertex:
         strShaderFile = "Shaders\\usd_dx_vs_1.txt"; 
         ct = HgiDXShaderCompiler::CompileTarget::kVS;
         break;
      case HgiShaderStageGeometry:
         strShaderFile = "Shaders\\usd_dx_gs_1.txt";
         ct = HgiDXShaderCompiler::CompileTarget::kGS;
         break;
      case HgiShaderStageFragment:
         strShaderFile = "Shaders\\usd_dx_ps_1.txt";
         ct = HgiDXShaderCompiler::CompileTarget::kPS;
         break;
      case HgiShaderStageCompute:
         strShaderFile = "Shaders\\usd_dx_cs_2.txt";
         ct = HgiDXShaderCompiler::CompileTarget::kCS;
         break;
      case HgiShaderStageTessellationEval:
         ct = HgiDXShaderCompiler::CompileTarget::kDS;
         break;
      case HgiShaderStageTessellationControl:
         ct = HgiDXShaderCompiler::CompileTarget::kHS;
         break;
      default:
         TF_CODING_ERROR("Compile target not implemented yet. What should we target in this case?");
         ct = HgiDXShaderCompiler::CompileTarget::kUnknown;
         break;
   }

   HgiDXShaderGenerator shaderGenerator(_hgi, _descriptor);
   shaderGenerator.Execute();
   _inputInfo = shaderGenerator.GetStageInputInfo();
   _rootParamInfo = shaderGenerator.GetStageRootParamInfo();

   strShaderCode = shaderGenerator.GetGeneratedShaderCode();

   //
   // Debug code that replaces generated shader with
   // pre-existing HDD file (in case we want to test something...
   // compare generated results with some manual modifications...)
   bool bReadHDDFile = false;
   if (bReadHDDFile)
   {
      std::stringstream buffer;
      if (!strShaderFile.empty())
      {
         std::ifstream fileStream(strShaderFile.c_str());
         buffer << fileStream.rdbuf();
         strShaderCode = buffer.str();
      }
   }
}


PXR_NAMESPACE_CLOSE_SCOPE
