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
#ifndef PXR_IMAGING_HGIWEBGPU_SHADERCOMPILER_H
#define PXR_IMAGING_HGIWEBGPU_SHADERCOMPILER_H

#include "pxr/pxr.h"
#include "pxr/imaging/hgi/enums.h"
#include "pxr/imaging/hgiWebGPU/api.h"

#include <stdint.h>
#include <stdlib.h>
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
    std::vector<unsigned int>* spirvOUT,
    std::string* errors = nullptr);

PXR_NAMESPACE_CLOSE_SCOPE

#endif
