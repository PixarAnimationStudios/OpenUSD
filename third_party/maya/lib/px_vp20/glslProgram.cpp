//
// Copyright 2017 Pixar
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

// glew must be included before any other GL header.
#include "pxr/imaging/glf/glew.h"

#include "px_vp20/glslProgram.h"

#include "pxr/pxr.h"
#include "pxr/base/tf/diagnostic.h"
#include "pxr/imaging/garch/gl.h"

#include <string>


PXR_NAMESPACE_OPEN_SCOPE


PxrMayaGLSLProgram::PxrMayaGLSLProgram() : mProgramId(0)
{
}

/* virtual */
PxrMayaGLSLProgram::~PxrMayaGLSLProgram()
{
    if (!glDeleteProgram) {
        return;
    }

    if (mProgramId != 0) {
        glDeleteProgram(mProgramId);
    }
}

bool
PxrMayaGLSLProgram::CompileShader(
        const GLenum type,
        const std::string& shaderSource)
{
    if (!glCreateProgram) {
        return false;
    }

    if (shaderSource.empty()) {
        return false;
    }

    const char* shaderType = NULL;
    switch (type) {
        case GL_COMPUTE_SHADER:
            shaderType = "GL_COMPUTE_SHADER";
            break;
        case GL_VERTEX_SHADER:
            shaderType = "GL_VERTEX_SHADER";
            break;
        case GL_TESS_CONTROL_SHADER:
            shaderType = "GL_TESS_CONTROL_SHADER";
            break;
        case GL_TESS_EVALUATION_SHADER:
            shaderType = "GL_TESS_EVALUATION_SHADER";
            break;
        case GL_GEOMETRY_SHADER:
            shaderType = "GL_GEOMETRY_SHADER";
            break;
        case GL_FRAGMENT_SHADER:
            shaderType = "GL_FRAGMENT_SHADER";
            break;
        default:
            TF_CODING_ERROR("Unsupported shader type %d\n", type);
            return false;
    }

    // Create a program if one does not already exist.
    if (mProgramId == 0) {
        mProgramId = glCreateProgram();
    }

    // Create a shader from shaderSource and compile it.
    const GLchar* shaderSources[1];
    shaderSources[0] = shaderSource.c_str();
    GLuint shader = glCreateShader(type);
    glShaderSource(shader,
                   sizeof(shaderSources) / sizeof(const GLchar*),
                   shaderSources,
                   NULL);
    glCompileShader(shader);

    // Verify that the shader compiled successfully.
    GLint success = 0;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        std::string compileInfo;
        GLint infoLogLength = 0;
        glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &infoLogLength);
        if (infoLogLength > 0) {
            char* infoLog = new char[infoLogLength];
            glGetShaderInfoLog(shader, infoLogLength, NULL, infoLog);
            compileInfo.assign(infoLog, infoLogLength);
            delete[] infoLog;
        }

        TF_WARN("Failed to compile shader type %s: %s",
                shaderType,
                compileInfo.c_str());
        return false;
    }

    // Attach the shader to the program.
    glAttachShader(mProgramId, shader);

    // The shader is no longer needed.
    glDeleteShader(shader);

    return true;
}

bool
PxrMayaGLSLProgram::Link()
{
    if (!glLinkProgram) {
        return false;
    }

    if (mProgramId == 0) {
        TF_CODING_ERROR("At least one shader must be compiled before linking.");
        return false;
    }

    glLinkProgram(mProgramId);

    // Verify that the program linked successfully.
    GLint success = 0;
    glGetProgramiv(mProgramId, GL_LINK_STATUS, &success);
    if (!success) {
        std::string linkInfo;
        GLint infoLogLength = 0;
        glGetProgramiv(mProgramId, GL_INFO_LOG_LENGTH, &infoLogLength);
        if (infoLogLength > 0) {
            char* infoLog = new char[infoLogLength];
            glGetProgramInfoLog(mProgramId, infoLogLength, NULL, infoLog);
            linkInfo.assign(infoLog, infoLogLength);
            delete[] infoLog;
        }

        TF_WARN("Failed to link shader program: %s", linkInfo.c_str());
        return false;
    }

    return true;
}

bool
PxrMayaGLSLProgram::Validate() const
{
    if (!glValidateProgram) {
        return false;
    }

    if (mProgramId == 0) {
        return false;
    }

    glValidateProgram(mProgramId);

    GLint success = 0;
    glGetProgramiv(mProgramId, GL_VALIDATE_STATUS, &success);
    if (!success) {
        std::string validateInfo;
        GLint infoLogLength = 0;
        glGetProgramiv(mProgramId, GL_INFO_LOG_LENGTH, &infoLogLength);
        if (infoLogLength > 0) {
            char* infoLog = new char[infoLogLength];
            glGetProgramInfoLog(mProgramId, infoLogLength, NULL, infoLog);
            validateInfo.assign(infoLog, infoLogLength);
            delete[] infoLog;
        }

        TF_WARN("Validation failed for shader program: %s",
                validateInfo.c_str());
        return false;
    }

    return true;
}


PXR_NAMESPACE_CLOSE_SCOPE
