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
#include "pxr/base/tf/diagnostic.h"
#include <pxr/base/tf/stringUtils.h>

#include "pxr/imaging/hgiGL/conversions.h"
#include "pxr/imaging/hgiGL/diagnostic.h"
#include "pxr/imaging/hgiGL/shaderFunction.h"
#include "pxr/imaging/hgiGL/shaderGenerator.h"

PXR_NAMESPACE_OPEN_SCOPE

HgiGLShaderFunction::HgiGLShaderFunction(
    HgiShaderFunctionDesc const& desc)
    : HgiShaderFunction(desc)
    , _shaderId(0)
{
    std::vector<GLenum> stages = 
        HgiGLConversions::GetShaderStages(desc.shaderStage);

    if (!TF_VERIFY(stages.size()==1)) return;

    _shaderId = glCreateShader(stages[0]);

    if (!_descriptor.debugName.empty()) {
        glObjectLabel(GL_SHADER, _shaderId, -1, _descriptor.debugName.c_str());
    }

    HgiGLShaderGenerator shaderGenerator {desc};
    std::stringstream ss;
    shaderGenerator.Execute(ss);
    std::string shaderStr = ss.str();
    const char* src = shaderStr.c_str();
    glShaderSource(_shaderId, 1, &src, nullptr);
    glCompileShader(_shaderId);

    // Grab compile errors
    GLint status;
    glGetShaderiv(_shaderId, GL_COMPILE_STATUS, &status);
    if (status != GL_TRUE) {
        int logSize = 0;
        glGetShaderiv(_shaderId, GL_INFO_LOG_LENGTH, &logSize);
        _errors.resize(logSize+1);
        glGetShaderInfoLog(_shaderId, logSize, NULL, &_errors[0]);
        glDeleteShader(_shaderId);
        _shaderId = 0;
    }

    _descriptor.shaderCode = nullptr;
    HGIGL_POST_PENDING_GL_ERRORS();
}

HgiGLShaderFunction::~HgiGLShaderFunction()
{
    glDeleteShader(_shaderId);
    _shaderId = 0;

    HGIGL_POST_PENDING_GL_ERRORS();
}

bool
HgiGLShaderFunction::IsValid() const
{
    return _shaderId>0 && _errors.empty();
}

std::string const&
HgiGLShaderFunction::GetCompileErrors()
{
    return _errors;
}

size_t
HgiGLShaderFunction::GetByteSizeOfResource() const
{
    return 0; // Can only query program binary size, not individual shaders.
}

uint64_t
HgiGLShaderFunction::GetRawResource() const
{
    return (uint64_t) _shaderId;
}

uint32_t
HgiGLShaderFunction::GetShaderId() const
{
    return _shaderId;
}

PXR_NAMESPACE_CLOSE_SCOPE
