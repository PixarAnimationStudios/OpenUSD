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

#include "shaderGenerator.h"

#include <sstream>

PXR_NAMESPACE_OPEN_SCOPE


static void
_ExtractVersionString(
    const std::string &originalShader, std::string *version)
{
    if (originalShader.rfind("#version", 0) == 0) {
        std::stringstream ss;
        int ind = 0;

        char curr = '\0';
        while(curr != '\n') {
            curr = originalShader[ind];
            ss << curr;
            ind++;
        }
        *version = ss.str();
    }
}

HgiShaderGenerator::HgiShaderGenerator(const HgiShaderFunctionDesc &descriptor)
    : _version()
    , _originalShader(descriptor.shaderCode)
    , _stage(descriptor.shaderStage)
{
    //As we append to the top of complete GLSLFX files, the version
    //string has to be hoisted for OpenGL and removed for Metal
    _ExtractVersionString(_originalShader, &_version);
}

void
HgiShaderGenerator::Execute(std::ostream &ss)
{
    //Use the protected version which can be overridden
    _Execute(ss, _originalShader);
}

const std::string&
HgiShaderGenerator::GetOriginalShader() const
{
    return _originalShader;
}

HgiShaderStage
HgiShaderGenerator::GetShaderStage() const
{
    return _stage;
}

PXR_NAMESPACE_CLOSE_SCOPE
