//
// Copyright 2022 Pixar
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
#ifndef PXR_IMAGING_HGIWEBGPU_SHADERPROGRAM_H
#define PXR_IMAGING_HGIWEBGPU_SHADERPROGRAM_H

#include <vector>

#include "pxr/imaging/hgi/shaderProgram.h"

#include "pxr/imaging/hgiWebGPU/api.h"
#include "pxr/imaging/hgiWebGPU/shaderFunction.h"

PXR_NAMESPACE_OPEN_SCOPE

///
/// \class HgiWebGPUShaderProgram
///
/// WebGPU implementation of HgiShaderProgram
///
class HgiWebGPUShaderProgram final : public HgiShaderProgram
{
public:
    HGIWEBGPU_API
    ~HgiWebGPUShaderProgram() override = default;

    HGIWEBGPU_API
    bool IsValid() const override;

    HGIWEBGPU_API
    std::string const& GetCompileErrors() override;

    HGIWEBGPU_API
    size_t GetByteSizeOfResource() const override;

    HGIWEBGPU_API
    uint64_t GetRawResource() const override;

    /// Returns the shader functions that are part of this program.
    HGIWEBGPU_API
    HgiShaderFunctionHandleVector const& GetShaderFunctions() const override;

protected:
    friend class HgiWebGPU;

    HGIWEBGPU_API
    HgiWebGPUShaderProgram(HgiShaderProgramDesc const& desc);

private:
    HgiWebGPUShaderProgram() = delete;
    HgiWebGPUShaderProgram & operator=(const HgiWebGPUShaderProgram&) = delete;
    HgiWebGPUShaderProgram(const HgiWebGPUShaderProgram&) = delete;
};


PXR_NAMESPACE_CLOSE_SCOPE

#endif