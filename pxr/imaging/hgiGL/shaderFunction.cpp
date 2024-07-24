//
// Copyright 2020 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/base/tf/diagnostic.h"
#include "pxr/base/tf/stringUtils.h"

#include "pxr/imaging/hgiGL/conversions.h"
#include "pxr/imaging/hgiGL/diagnostic.h"
#include "pxr/imaging/hgiGL/shaderFunction.h"
#include "pxr/imaging/hgiGL/shaderGenerator.h"

PXR_NAMESPACE_OPEN_SCOPE

HgiGLShaderFunction::HgiGLShaderFunction(
    Hgi const* hgi,
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

    HgiGLShaderGenerator shaderGenerator(hgi, desc);
    shaderGenerator.Execute();
    const char *shaderCode = shaderGenerator.GetGeneratedShaderCode();

    glShaderSource(_shaderId, 1, &shaderCode, nullptr);
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

    // Clear these pointers in our copy of the descriptor since we
    // have to assume they could become invalid after we return.
    _descriptor.shaderCodeDeclarations = nullptr;
    _descriptor.shaderCode = nullptr;
    _descriptor.generatedShaderCodeOut = nullptr;

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
