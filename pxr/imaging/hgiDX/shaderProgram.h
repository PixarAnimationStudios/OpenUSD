
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

#include <vector>

#include "pxr/imaging/hgi/shaderProgram.h"

#include "pxr/imaging/hgiDX/api.h"
#include "pxr/imaging/hgiDX/buffer.h"
#include "pxr/imaging/hgiDX/shaderFunction.h"
#include "pxr/imaging/hgiDX/shaderInfo.h"

PXR_NAMESPACE_OPEN_SCOPE

class HgiDXDevice;
struct HgiVertexBufferDesc;


///
/// \class HgiDXShaderProgram
///
/// DirectX implementation of HgiShaderProgram
///
class HgiDXShaderProgram final : public HgiShaderProgram
{
public:
    HGIDX_API
    ~HgiDXShaderProgram() override = default;

    HGIDX_API
    bool IsValid() const override;

    HGIDX_API
    std::string const& GetCompileErrors() override;

    HGIDX_API
    size_t GetByteSizeOfResource() const override;

    HGIDX_API
    uint64_t GetRawResource() const override;

    /// Returns the shader functions that are part of this program.
    HGIDX_API
    HgiShaderFunctionHandleVector const& GetShaderFunctions() const;

    /// Returns the device used to create this object.
    HGIDX_API
    HgiDXDevice* GetDevice() const;

    std::vector<D3D12_INPUT_ELEMENT_DESC> GetInputLayout(const std::vector<HgiVertexBufferDesc>& vbdv) const;
    std::vector<CD3DX12_ROOT_PARAMETER1> GetRootParameters() const;
    bool GetInfo(UINT nSuggestedBindIdx, DXShaderInfo::RootParamInfo& rpi, bool bMovedParam) const;

protected:
    friend class HgiDX;

    HGIDX_API
    HgiDXShaderProgram(HgiDXDevice* device, HgiShaderProgramDesc const& desc);

private:
    HgiDXShaderProgram() = delete;
    HgiDXShaderProgram & operator=(const HgiDXShaderProgram&) = delete;
    HgiDXShaderProgram(const HgiDXShaderProgram&) = delete;

    HgiDXDevice* _device;
    mutable std::map<uint32_t, DXShaderInfo::StageParamInfo> _inputBindIdx2ShaderData;
    mutable std::map<UINT, DXShaderInfo::RootParamInfo> _rootParamsBySuggestedBindIdx;
};


PXR_NAMESPACE_CLOSE_SCOPE
