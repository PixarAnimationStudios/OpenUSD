
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
#include "pxr/base/tf/envSetting.h"
#include "pxr/base/tf/getenv.h"

#include "pxr/imaging/hgiDX/shaderCompiler.h"

// hopefully these 2 can live together in the same process
#pragma comment (lib, "dxcompiler.lib")
#pragma comment (lib, "d3dcompiler.lib")

const char* shader_model_6 = "6_1";
const wchar_t* shader_model_6w = L"6_1";
const char* shader_model_5 = "5_1";
const wchar_t* shader_model_5w = L"5_1";


#include <fstream>

PXR_NAMESPACE_OPEN_SCOPE

TF_DEFINE_ENV_SETTING(HGI_ENABLE_DX_DEBUG_SHADERS, 0, "Compile DirectX shaders with debug information (for release builds).");
static const bool bShadersModel6 = TfGetenvBool("HGI_DX_SHADERS_MODEL_6", false);

ComPtr<IUnknown>
HgiDXShaderCompiler::Compile(const std::string& strShaderSource, HgiDXShaderCompiler::CompileTarget ct, std::string& errors)
{
   ComPtr<IUnknown> result;
   
   HRESULT hr = E_FAIL;

   if (bShadersModel6) {

      ComPtr<IDxcUtils> pUtils;
      DxcCreateInstance(CLSID_DxcUtils, IID_PPV_ARGS(pUtils.GetAddressOf()));
      ComPtr<IDxcBlobEncoding> pSource;
      pUtils->CreateBlob(strShaderSource.c_str(), strShaderSource.length(), CP_UTF8, pSource.GetAddressOf());

      std::vector<LPCWSTR> arguments;
      // command line reference: https://github.com/microsoft/DirectXShaderCompiler/wiki/Using-dxc.exe-and-dxcompiler.dll
      // more options: https://simoncoenen.com/blog/programming/graphics/DxcCompiling

      // -E for the entry point (eg. 'main')
      arguments.push_back(L"-E");
      arguments.push_back(L"mainDX");

      // -T for the target profile (eg. 'ps_6_6')
      arguments.push_back(L"-T");
      arguments.push_back(GetTargetNameW(ct));

#if defined(_DEBUG)
      arguments.push_back(L"-Zi"); // could use DXC_ARG_DEBUG
      arguments.push_back(L"-Qembed_debug");
      arguments.push_back(L"-Od"); // disable optimizations
#else 
      int nDebug = TfGetenvInt("HGI_ENABLE_DX_DEBUG_SHADERS", 0);
      if (nDebug > 0) {
         arguments.push_back(L"-Zi"); // could use DXC_ARG_DEBUG
         arguments.push_back(L"-Qembed_debug");
         arguments.push_back(L"-Od"); // disable optimizations
      }
      else {
         // make sure we strip reflection data and pdbs
         //arguments.push_back(L"-Qstrip_debug");
         //arguments.push_back(L"-Qstrip_reflect");
      }
#endif

      // -Zpc                    Pack matrices in column-major order.
      // -Zpr                    Pack matrices in row - major order.
      // -HV <value>             HLSL version(2016, 2017, 2018, 2021).Default is 2018.

      //arguments.push_back(DXC_ARG_WARNINGS_ARE_ERRORS); //-WX

      std::vector<LPWSTR> defines;
      for (const std::wstring& define : defines)
      {
         arguments.push_back(L"-D");
         arguments.push_back(define.c_str());
      }

      DxcBuffer sourceBuffer;
      sourceBuffer.Ptr = pSource->GetBufferPointer();
      sourceBuffer.Size = pSource->GetBufferSize();
      sourceBuffer.Encoding = 0;

      ComPtr<IDxcResult> pCompileResult;

      ComPtr<IDxcCompiler3> pCompiler;
      DxcCreateInstance(CLSID_DxcCompiler, IID_PPV_ARGS(pCompiler.GetAddressOf()));
      hr = pCompiler->Compile(&sourceBuffer, arguments.data(), (UINT32)arguments.size(), nullptr, IID_PPV_ARGS(pCompileResult.GetAddressOf()));

      if (nullptr != pCompileResult) {
         pCompileResult->GetStatus(&hr); // this hr here is the important compile result value

         //
         // I want to try and get errors and warnings in all cases
         // and at least dump them in the command prompt
         ComPtr<IDxcBlobEncoding> pErrors;
         HRESULT hrGetErrors = pCompileResult->GetErrorBuffer(&pErrors);
         if (SUCCEEDED(hrGetErrors) && pErrors && pErrors->GetBufferSize() > 0)
         {
            errors = (char*)pErrors->GetBufferPointer();
         }
         if(SUCCEEDED(hr)) {
            ComPtr<IDxcBlob> pDxResult;
            pCompileResult->GetResult(&pDxResult);
            result = pDxResult;
         }
      }
   }
   else {
#if defined(_DEBUG)
      // Enable better shader debugging with the graphics debugging tools.
      UINT compileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#else
      UINT compileFlags = D3DCOMPILE_PACK_MATRIX_COLUMN_MAJOR;
   ;
      int nDebug = TfGetenvInt("HGI_ENABLE_DX_DEBUG_SHADERS", 0);
      if (nDebug > 0)
         compileFlags |= (D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION);
      else
      {
         //
         // D3DCOMPILE_AVOID_FLOW_CONTROL seems to be necessary in order to avoid a ton of errors
         // about potentially uninitialized variables in code that looks perfectly fine
         compileFlags |= (D3DCOMPILE_ALL_RESOURCES_BOUND
            //| D3DCOMPILE_AVOID_FLOW_CONTROL // this causes test "testUsdImagingDXBasicDrawing_allPrims_3d_cam_lights_pts" to crash !
            | D3DCOMPILE_OPTIMIZATION_LEVEL2
            //| D3DCOMPILE_WARNINGS_ARE_ERRORS
            );
         //compileFlags = D3DCOMPILE_SKIP_OPTIMIZATION;
      }
#endif

      const char* shaderTarget = GetTargetName(ct); 
      ComPtr<ID3DBlob> errorMsgs;

      ComPtr<ID3DBlob> pDxResult;
      hr = D3DCompile(strShaderSource.c_str(), strShaderSource.length(),
                      shaderTarget, // the shaders desc did not give us a good name for this anyway
                      nullptr,
                      D3D_COMPILE_STANDARD_FILE_INCLUDE,
                      "mainDX", shaderTarget,
                      compileFlags, 0,
                      pDxResult.ReleaseAndGetAddressOf(),
                      errorMsgs.ReleaseAndGetAddressOf());
      result = pDxResult;
      if (FAILED(hr) || (nullptr != errorMsgs)) // I want to get warnings even when build succeeded 
         errors = (char*)errorMsgs->GetBufferPointer();
   }

   if (errors.length() > 0) {
      TF_STATUS(errors.c_str());

      // If build succeeded I do not want to pass forward the warnings because 
      // their presence is a (false) indication compilation failed 
      if (SUCCEEDED(hr))
         errors = "";
   }

   return result;
}

bool 
HgiDXShaderCompiler::UsingShaderModel6()
{
   if(bShadersModel6)
      return true;
   else 
      return false;
}

LPCSTR 
HgiDXShaderCompiler::GetTargetName(CompileTarget ct)
{
   switch (ct) {
      case CompileTarget::kVS:
         return GetVSTargetName();
         break;
      case CompileTarget::kPS:
         return GetPSTargetName();
         break;
      case CompileTarget::kGS:
         return GetGSTargetName();
         break;
      case CompileTarget::kCS:
         return GetCSTargetName();
         break;
      case CompileTarget::kDS:
         return GetDSTargetName();
         break;
      case CompileTarget::kHS:
         return GetHSTargetName();
         break;
      default:
         return "";
   }
}

LPCWSTR
HgiDXShaderCompiler::GetTargetNameW(CompileTarget ct)
{
   switch (ct) {
   case CompileTarget::kVS:
      return GetVSTargetNameW();
      break;
   case CompileTarget::kPS:
      return GetPSTargetNameW();
      break;
   case CompileTarget::kGS:
      return GetGSTargetNameW();
      break;
   case CompileTarget::kCS:
      return GetCSTargetNameW();
      break;
   case CompileTarget::kDS:
      return GetDSTargetNameW();
      break;
   case CompileTarget::kHS:
      return GetHSTargetNameW();
      break;
   default:
      return L"";
   }
}

LPCSTR 
HgiDXShaderCompiler::GetVSTargetName()
{
   static const std::string target = std::string("vs_") + (bShadersModel6? shader_model_6 : shader_model_5);
   return target.c_str();
}

LPCSTR 
HgiDXShaderCompiler::GetPSTargetName()
{
   static const std::string target = std::string("ps_") + (bShadersModel6 ? shader_model_6 : shader_model_5);
   return target.c_str();
}

LPCSTR 
HgiDXShaderCompiler::GetGSTargetName()
{
   static const std::string target = std::string("gs_") + (bShadersModel6 ? shader_model_6 : shader_model_5);
   return target.c_str();
}

LPCSTR 
HgiDXShaderCompiler::GetCSTargetName()
{
   static const std::string target = std::string("cs_") + (bShadersModel6 ? shader_model_6 : shader_model_5);
   return target.c_str();
}

LPCSTR 
HgiDXShaderCompiler::GetDSTargetName()
{
   static const std::string target = std::string("ds_") + (bShadersModel6 ? shader_model_6 : shader_model_5);
   return target.c_str();
}

LPCSTR 
HgiDXShaderCompiler::GetHSTargetName()
{
   static const std::string target = std::string("hs_") + (bShadersModel6 ? shader_model_6 : shader_model_5);
   return target.c_str();
}

LPCWSTR
HgiDXShaderCompiler::GetVSTargetNameW()
{
   static const std::wstring target = std::wstring(L"vs_") + (bShadersModel6? shader_model_6w : shader_model_5w);
   return target.c_str();
}

LPCWSTR
HgiDXShaderCompiler::GetPSTargetNameW()
{
   static const std::wstring target = std::wstring(L"ps_") + (bShadersModel6 ? shader_model_6w : shader_model_5w);
   return target.c_str();
}

LPCWSTR
HgiDXShaderCompiler::GetGSTargetNameW()
{
   static const std::wstring target = std::wstring(L"gs_") + (bShadersModel6 ? shader_model_6w : shader_model_5w);
   return target.c_str();
}

LPCWSTR
HgiDXShaderCompiler::GetCSTargetNameW()
{
   static const std::wstring target = std::wstring(L"cs_") + (bShadersModel6 ? shader_model_6w : shader_model_5w);
   return target.c_str();
}

LPCWSTR
HgiDXShaderCompiler::GetDSTargetNameW()
{
   static const std::wstring target = std::wstring(L"ds_") + (bShadersModel6 ? shader_model_6w : shader_model_5w);
   return target.c_str();
}

LPCWSTR
HgiDXShaderCompiler::GetHSTargetNameW()
{
   static const std::wstring target = std::wstring(L"hs_") + (bShadersModel6 ? shader_model_6w : shader_model_5w);
   return target.c_str();
}

PXR_NAMESPACE_CLOSE_SCOPE
