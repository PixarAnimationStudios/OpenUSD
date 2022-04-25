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
