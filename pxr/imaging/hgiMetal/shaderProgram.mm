//
// Copyright 2020 Pixar
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
#include "pxr/imaging/hgiMetal/shaderProgram.h"
#include "pxr/imaging/hgiMetal/shaderFunction.h"

PXR_NAMESPACE_OPEN_SCOPE


HgiMetalShaderProgram::HgiMetalShaderProgram(HgiShaderProgramDesc const& desc)
    : HgiShaderProgram(desc)
    , _vertexFunction(nil)
    , _fragmentFunction(nil)
    , _computeFunction(nil)
{
    HgiShaderFunctionHandleVector const &shaderFuncs = desc.shaderFunctions;
    for (auto const& func : shaderFuncs) {
        HgiMetalShaderFunction const *metalFunction =
            static_cast<HgiMetalShaderFunction*>(func.Get());
        switch (metalFunction->GetDescriptor().shaderStage) {
            case HgiShaderStageVertex:
                _vertexFunction = metalFunction->GetShaderId();
                break;
            case HgiShaderStageFragment:
                _fragmentFunction = metalFunction->GetShaderId();
                break;
            case HgiShaderStageCompute:
                _computeFunction = metalFunction->GetShaderId();
                break;
        }
    }
}

HgiMetalShaderProgram::~HgiMetalShaderProgram() = default;

HgiShaderFunctionHandleVector const&
HgiMetalShaderProgram::GetShaderFunctions() const
{
    return _descriptor.shaderFunctions;
}

bool
HgiMetalShaderProgram::IsValid() const
{
    return true;
}

std::string const&
HgiMetalShaderProgram::GetCompileErrors()
{
    return _errors;
}

size_t
HgiMetalShaderProgram::GetByteSizeOfResource() const
{
    size_t  byteSize = 0;
    for (HgiShaderFunctionHandle const& fn : _descriptor.shaderFunctions) {
        byteSize += fn->GetByteSizeOfResource();
    }
    return byteSize;
}

uint64_t
HgiMetalShaderProgram::GetRawResource() const
{
    return 0;
}

PXR_NAMESPACE_CLOSE_SCOPE
