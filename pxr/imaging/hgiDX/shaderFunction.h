
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

#include "pxr/imaging/hgi/shaderFunction.h"
#include "pxr/imaging/hgiDX/api.h"
#include "pxr/imaging/hgiDX/shaderInfo.h"

PXR_NAMESPACE_OPEN_SCOPE

class Hgi;
class HgiDX;
class HgiDXDevice;


///
/// \class HgiDXShaderFunction
///
/// DirectX implementation of HgiShaderFunction
///
class HgiDXShaderFunction final : public HgiShaderFunction
{
public:

   HGIDX_API
   ~HgiDXShaderFunction() override;

   HGIDX_API
   bool IsValid() const override;

   HGIDX_API
   std::string const& GetCompileErrors() override;

   HGIDX_API
   size_t GetByteSizeOfResource() const override;

   HGIDX_API
   uint64_t GetRawResource() const override;

   /// Returns the shader entry function name (usually "main").
   HGIDX_API
   const char* GetShaderFunctionName() const;

   /// Returns the device used to create this object.
   HGIDX_API
   HgiDXDevice* GetDevice() const;

   HGIDX_API
   ID3DBlob* GetShaderBlob() const;

   HGIDX_API
   const std::vector<DXShaderInfo::StageParamInfo>& GetStageInputInfo() const;
   HGIDX_API
   const std::vector<DXShaderInfo::RootParamInfo>& GetStageRootParamInfo() const;

protected:
   friend class HgiDX;

   HGIDX_API
   HgiDXShaderFunction(HgiDXDevice* device,
                       Hgi const* hgi,
                       HgiShaderFunctionDesc const& desc,
                       int shaderVersion);


private:
   HgiDXShaderFunction() = delete;
   HgiDXShaderFunction& operator=(const HgiDXShaderFunction&) = delete;
   HgiDXShaderFunction(const HgiDXShaderFunction&) = delete;
   void _GetShaderCode(std::string& strShaderCode, std::string& strShaderTarget);


private:
   Hgi const* _hgi;
   HgiDXDevice* _device;
   std::string _errors;
   ComPtr<ID3DBlob> _shaderBlob;
   
   std::vector<DXShaderInfo::StageParamInfo> _inputInfo;
   std::vector<DXShaderInfo::RootParamInfo> _rootParamInfo;
};


PXR_NAMESPACE_CLOSE_SCOPE
