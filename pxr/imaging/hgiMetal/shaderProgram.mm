//
// Copyright 2020 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/imaging/hgiMetal/shaderProgram.h"
#include "pxr/imaging/hgiMetal/shaderFunction.h"

PXR_NAMESPACE_OPEN_SCOPE


HgiMetalShaderProgram::HgiMetalShaderProgram(HgiShaderProgramDesc const& desc)
    : HgiShaderProgram(desc)
    , _vertexFunction(nil)
    , _fragmentFunction(nil)
    , _computeFunction(nil)
    , _postTessVertexFunction(nil)
    , _postTessControlFunction(nil)
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
            case HgiShaderStagePostTessellationVertex:
                _postTessVertexFunction = metalFunction->GetShaderId();
                break;
            case HgiShaderStagePostTessellationControl:
                _postTessControlFunction = metalFunction->GetShaderId();
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
