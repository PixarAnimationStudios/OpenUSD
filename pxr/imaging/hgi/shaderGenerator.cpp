//
// Copyright 2020 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/imaging/hgi/shaderGenerator.h"

#include "pxr/imaging/hgi/shaderFunctionDesc.h"

#include "pxr/base/tf/stringUtils.h"

PXR_NAMESPACE_OPEN_SCOPE

HgiShaderGenerator::HgiShaderGenerator(const HgiShaderFunctionDesc &descriptor)
    : _descriptor(descriptor)
{
}

HgiShaderGenerator::~HgiShaderGenerator() = default;

void
HgiShaderGenerator::Execute()
{
    std::stringstream ss;

    // Use the protected version which can be overridden
    _Execute(ss);

    // Capture the result as specified by the descriptor or locally.
    if (_descriptor.generatedShaderCodeOut) {
       *_descriptor.generatedShaderCodeOut = ss.str();
    } else {
       _localGeneratedShaderCode = ss.str();
    }
}

const char *
HgiShaderGenerator::_GetShaderCodeDeclarations() const
{
    static const char *emptyString = "";
    return _descriptor.shaderCodeDeclarations
                ? _descriptor.shaderCodeDeclarations : emptyString;
}

const char *
HgiShaderGenerator::_GetShaderCode() const
{
    static const char *emptyString = "";
    return _descriptor.shaderCode
                ? _descriptor.shaderCode : emptyString;
}

HgiShaderStage
HgiShaderGenerator::_GetShaderStage() const
{
    return _descriptor.shaderStage;
}

const char *
HgiShaderGenerator::GetGeneratedShaderCode() const
{
    // Return the result as specified by the descriptor or locally.
    if (_descriptor.generatedShaderCodeOut) {
       return _descriptor.generatedShaderCodeOut->c_str();
    } else {
       return _localGeneratedShaderCode.c_str();
    }
}

PXR_NAMESPACE_CLOSE_SCOPE
