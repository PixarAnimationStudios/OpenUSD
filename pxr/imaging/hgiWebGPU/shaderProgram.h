//
// Copyright 2026 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
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