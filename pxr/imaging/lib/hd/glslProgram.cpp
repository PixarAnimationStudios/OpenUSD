//
// Copyright 2016 Pixar
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
#include "pxr/imaging/glf/glew.h"
#include "pxr/imaging/hd/glslProgram.h"
#include "pxr/imaging/hd/glUtils.h"
#include "pxr/imaging/hd/package.h"
#include "pxr/imaging/hd/perfLog.h"
#include "pxr/imaging/hd/resourceRegistry.h"
#include "pxr/imaging/hd/tokens.h"
#include "pxr/imaging/glf/glslfx.h"
#include "pxr/base/tf/diagnostic.h"
#include "pxr/base/tf/envSetting.h"
#include <string>
#include <fstream>
#include <iostream>

TF_DEFINE_ENV_SETTING(HD_ENABLE_SHARED_CONTEXT_CHECK, 0,
    "Enable GL context sharing validation");

HdGLSLProgram::HdGLSLProgram(TfToken const &role)
    : _program(role), _uniformBuffer(role)
{
}

HdGLSLProgram::~HdGLSLProgram()
{
    GLuint program = _program.GetId();
    if (program != 0) {
        if (glDeleteProgram)
            glDeleteProgram(program);
        _program.SetAllocation(0, 0);
    }
    GLuint uniformBuffer = _uniformBuffer.GetId();
    if (uniformBuffer) {
        if (glDeleteBuffers)
            glDeleteBuffers(1, &uniformBuffer);
        _uniformBuffer.SetAllocation(0, 0);
    }
}

/* static */
HdGLSLProgram::ID
HdGLSLProgram::ComputeHash(TfToken const &sourceFile)
{
    HD_TRACE_FUNCTION();

    uint32_t hash = 0;
    std::string const &filename = sourceFile.GetString();
    hash = ArchHash(filename.c_str(), filename.size(), hash);

    return hash;
}

bool
HdGLSLProgram::CompileShader(GLenum type,
                             std::string const &shaderSource)
{
    HD_TRACE_FUNCTION();
    HD_MALLOC_TAG_FUNCTION();

    // early out for empty source.
    // this may not be an error, since glslfx gives empty string
    // for undefined shader stages (i.e. null geometry shader)
    if (shaderSource.empty()) return false;

    const char *shaderType = NULL;
    if (type == GL_VERTEX_SHADER) {
        shaderType = "GL_VERTEX_SHADER";
    } else if (type == GL_TESS_CONTROL_SHADER) {
        shaderType = "GL_TESS_CONTROL_SHADER";
    } else if (type == GL_TESS_EVALUATION_SHADER) {
        shaderType = "GL_TESS_EVALUATION_SHADER";
    } else if (type == GL_GEOMETRY_SHADER) {
        shaderType = "GL_GEOMETRY_SHADER";
    } else if (type == GL_FRAGMENT_SHADER) {
        shaderType = "GL_FRAGMENT_SHADER";
    } else if (type == GL_COMPUTE_SHADER) {
        shaderType = "GL_COMPUTE_SHADER";
    } else {
        TF_CODING_ERROR("Invalid shader type %d\n", type);
        return false;
    }

    if (TfDebug::IsEnabled(HD_DUMP_SHADER_SOURCE)) {
        std::cout << "--------- " << shaderType << " ----------\n";
        std::cout << shaderSource;
        std::cout << "---------------------------\n";
        std::cout << std::flush;
    }

    // glew has to be intialized
    if (!glCreateProgram)
        return false;

    // create a program if not exists
    GLuint program = _program.GetId();
    if (program == 0) {
        program = glCreateProgram();
        _program.SetAllocation(program, 0);
    }

    // create a shader, compile it
    const char *shaderSources[1];
    shaderSources[0] = shaderSource.c_str();
    GLuint shader = glCreateShader(type);
    glShaderSource(shader, sizeof(shaderSources)/sizeof(const char *), shaderSources, NULL);
    glCompileShader(shader);

    std::string logString;
    if (!HdGLUtils::GetShaderCompileStatus(shader, &logString)) {
        // XXX:validation
        TF_WARN("Failed to compile shader (%s): \n%s",
                shaderType, logString.c_str());
    }

    // attach the shader to the program
    glAttachShader(program, shader);

    // shader is no longer needed.
    glDeleteShader(shader);

    return true;
}

bool
HdGLSLProgram::Link()
{
    HD_TRACE_FUNCTION();
    HD_MALLOC_TAG_FUNCTION();

    if (!glLinkProgram) return false; // glew initialized

    GLuint program = _program.GetId();
    if (program == 0) {
        TF_CODING_ERROR("At least one shader has to be compiled before linking.");
        return false;
    }

    // set RETRIEVABLE_HINT to true for getting program binary length.
    // note: Actually the GL driver may recompile the program dynamically on
    // some state changes, so the size of program could be inaccurate.
    glProgramParameteri(program, GL_PROGRAM_BINARY_RETRIEVABLE_HINT, GL_TRUE);

    // link
    glLinkProgram(program);

    std::string logString;
    bool success = true;
    if (!HdGLUtils::GetProgramLinkStatus(program, &logString)) {
        // XXX:validation
        TF_WARN("Failed to link shader: \n%s", logString.c_str());
        success = false;
    }

    // initial program size
    GLint size;
    glGetProgramiv(program, GL_PROGRAM_BINARY_LENGTH, &size);

    // update the program resource allocation
    _program.SetAllocation(program, size);

    // create an uniform buffer
    GLuint uniformBuffer = _uniformBuffer.GetId();
    if (uniformBuffer == 0) {
        glGenBuffers(1, &uniformBuffer);
        _uniformBuffer.SetAllocation(uniformBuffer, 0);
    }

    // binary dump out
    if (TfDebug::IsEnabled(HD_DUMP_SHADER_BINARY)) {
        std::vector<char> bin(size);
        GLsizei len;
        GLenum format;
        glGetProgramBinary(program, size, &len, &format, &bin[0]);
        static int id = 0;
        std::stringstream fname;
        fname << "program" << id++ << ".bin";

        std::fstream output(fname.str().c_str(), std::ios::out|std::ios::binary);
        output.write(&bin[0], size);
        output.close();

        std::cout << "Write " << fname.str() << " (size=" << size << ")\n";
    }

    return success;
}

bool
HdGLSLProgram::Validate() const
{
    GLuint program = _program.GetId();
    if (program == 0) return false;

    if (TfDebug::IsEnabled(HD_SAFE_MODE) ||
        TfGetEnvSetting(HD_ENABLE_SHARED_CONTEXT_CHECK)) {

        HD_TRACE_FUNCTION();

        // make sure the binary size is same as when it's created.
        if (glIsProgram(program) == GL_FALSE) return false;
        GLint size = 0;
        glGetProgramiv(program, GL_PROGRAM_BINARY_LENGTH, &size);
        if (size == 0) {
            return false;
        }
        if (size != _program.GetSize()) {
            return false;
        }
    }
    return true;
}

HdGLSLProgramSharedPtr
HdGLSLProgram::GetComputeProgram(TfToken const &shaderToken)
{
    HdResourceRegistry *resourceRegistry = &HdResourceRegistry::GetInstance();

    {
        // Find the program from registry
        HdInstance<HdGLSLProgram::ID, HdGLSLProgramSharedPtr> programInstance;

        std::unique_lock<std::mutex> regLock = 
            resourceRegistry->RegisterGLSLProgram(HdGLSLProgram::ComputeHash(shaderToken), &programInstance);

        if (programInstance.IsFirstInstance()) {
            // if not exists, create new one
            HdGLSLProgramSharedPtr newProgram(
                new HdGLSLProgram(HdTokens->computeShader));

            GlfGLSLFX glslfx(HdPackageComputeShader());
            std::string version = "#version 430\n";
            if (!newProgram->CompileShader(
                    GL_COMPUTE_SHADER, version + glslfx.GetSource(shaderToken))) {
                TF_CODING_ERROR("Fail to compile " + shaderToken.GetString());
                return HdGLSLProgramSharedPtr();
            }
            if (!newProgram->Link()) {
                TF_CODING_ERROR("Fail to link " + shaderToken.GetString());
                return HdGLSLProgramSharedPtr();
            }
            programInstance.SetValue(newProgram);
        }
        return programInstance.GetValue();
    }
}

