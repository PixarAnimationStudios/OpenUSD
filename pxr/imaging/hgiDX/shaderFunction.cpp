
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
#include "pxr/imaging/hgiDX/device.h"
#include "pxr/imaging/hgiDX/hgi.h"
#include "pxr/imaging/hgiDX/shaderFunction.h"
#include "pxr/imaging/hgiDX/shaderGenerator.h"

#include "pxr/base/tf/diagnostic.h"
#include "pxr/base/tf/envSetting.h"
#include "pxr/base/tf/getEnv.h"

#include "D3dCompiler.h"

#include <fstream>

PXR_NAMESPACE_OPEN_SCOPE

TF_DEFINE_ENV_SETTING(HGI_ENABLE_DX_DEBUG_SHADERS, 0, "Compile DirectX shaders with debug information (for release builds).");

namespace {
   const char szEntryPoint[] = "main";
}

HgiDXShaderFunction::HgiDXShaderFunction(HgiDXDevice* device,
                                         Hgi const* hgi,
                                         HgiShaderFunctionDesc const& desc,
                                         int shaderVersion)
   : HgiShaderFunction(desc)
   , _hgi(hgi)
   , _device(device)
{
   const char* debugLbl = _descriptor.debugName.empty() ? "unknown" : _descriptor.debugName.c_str();

#if defined(_DEBUG)
   // Enable better shader debugging with the graphics debugging tools.
   UINT compileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#else
   UINT compileFlags = 0;
   int nDebug = TfGetenvInt("HGI_ENABLE_DX_DEBUG_SHADERS", 0);
   if (nDebug > 0)
      compileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
   else
   {
      //
      // D3DCOMPILE_AVOID_FLOW_CONTROL seems to be necessary in order to avoid a ton of errors
      // about potentially uninitialized variables in code that looks perfectly fine
      compileFlags = D3DCOMPILE_ALL_RESOURCES_BOUND
                     //| D3DCOMPILE_AVOID_FLOW_CONTROL // this causes test "testUsdImagingDXBasicDrawing_allPrims_3d_cam_lights_pts" to crash !
                     | D3DCOMPILE_OPTIMIZATION_LEVEL2
                     //| D3DCOMPILE_WARNINGS_ARE_ERRORS
                     ;
      //compileFlags = D3DCOMPILE_SKIP_OPTIMIZATION;
   }
#endif

   // Compile shader and capture errors
   std::string strShaderCode;
   std::string strCompileTarget;
   _GetShaderCode(strShaderCode, strCompileTarget);

   ComPtr<ID3DBlob> errorMsgs;

   HRESULT hr = D3DCompile(strShaderCode.c_str(), strShaderCode.length(),
                           debugLbl,
                           nullptr,
                           D3D_COMPILE_STANDARD_FILE_INCLUDE,
                           szEntryPoint, strCompileTarget.c_str(),
                           compileFlags, 0,
                           _shaderBlob.ReleaseAndGetAddressOf(),
                           errorMsgs.ReleaseAndGetAddressOf());

   if (FAILED(hr))
   {
      char err[20000]; // In some rare cases we can get very large errors text...
      void* pBuffPtr = errorMsgs->GetBufferPointer();
      snprintf(err, 20000, "Error %08X   %s\n", hr, (char*)pBuffPtr);
      OutputDebugString(err);
      _errors = err;
   }

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

const char*
HgiDXShaderFunction::GetShaderFunctionName() const
{
   return szEntryPoint;
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
   if(nullptr != _shaderBlob)
      ret = _shaderBlob->GetBufferSize();

   return ret;
}

uint64_t
HgiDXShaderFunction::GetRawResource() const
{
   return (uint64_t)_shaderBlob.Get();
}

ID3DBlob*
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
HgiDXShaderFunction::_GetShaderCode(std::string& strShaderCode, std::string& strShaderTarget)
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
         strShaderTarget = "vs_5_1";
         break;
      case HgiShaderStageGeometry:
         strShaderFile = "Shaders\\usd_dx_gs_1.txt";
         strShaderTarget = "gs_5_1";
         break;
      case HgiShaderStageFragment:
         strShaderFile = "Shaders\\usd_dx_ps_1.txt";
         strShaderTarget = "ps_5_1";
         break;
      case HgiShaderStageCompute:
         strShaderFile = "Shaders\\usd_dx_cs_2.txt";
         strShaderTarget = "cs_5_1";
         break;
      case HgiShaderStageTessellationEval:
         strShaderTarget = "ds_5_1";
         break;
      case HgiShaderStageTessellationControl:
         strShaderTarget = "hs_5_1";
         break;
      default:
         TF_CODING_ERROR("Compile target not implemented yet. What should we target in this case?");
         strShaderTarget = "??";
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
