//
// Copyright 2026 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_IMAGING_HGIWEBGPU_SHADERCOMPILER_H
#define PXR_IMAGING_HGIWEBGPU_SHADERCOMPILER_H

#include "pxr/pxr.h"
#include "pxr/imaging/hgi/enums.h"
#include "pxr/imaging/hgiWebGPU/api.h"

#include <cstdint>
#include <string>
#include <vector>


PXR_NAMESPACE_OPEN_SCOPE

/// Compiles ascii shader code (glsl) into spirv binary code (spirvOut).
/// Returns true if successful. Errors can optionally be captured.
/// numShaderCodes determines how many strings are provided via shaderCodes.
/// 'name' is purely for debugging compile errors. It can be anything.
HGIWEBGPU_API
bool HgiWebGPUCompileGLSL(
    const char* name,
    const char* shaderCodes[],
    uint8_t numShaderCodes,
    HgiShaderStage stage,
    std::vector<uint32_t>* spirvOut,
    std::string* errors = nullptr);

PXR_NAMESPACE_CLOSE_SCOPE

#endif
