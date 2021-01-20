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
#include "pxr/imaging/hio/glslfx.h"
#include "pxr/imaging/hd/perfLog.h"
#include "pxr/imaging/hd/tokens.h"
#include "pxr/imaging/hgi/tokens.h"
#include "pxr/imaging/hdSt/debugCodes.h"
#include "pxr/imaging/hdSt/package.h"
#include "pxr/imaging/hdSt/glslProgram.h"
#include "pxr/imaging/hdSt/glUtils.h"
#include "pxr/imaging/hdSt/resourceRegistry.h"
#include "pxr/base/arch/hash.h"
#include "pxr/base/tf/diagnostic.h"

#include <climits>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <string>

PXR_NAMESPACE_OPEN_SCOPE


// Get the line number from the compilation error message, and return a boolean
// indicating success/failure of parsing.
// Note: This has been tested only on nVidia.
static bool
_ParseLineNumberOfError(std::string const &error, unsigned int *lineNum)
{
    if (!lineNum) {
        return false;
    }
    // sample error on nVidia:
    // 0(279) : error C1031: swizzle mask element not present in operand "xyz"
    // 279 is the line number here.
    std::string::size_type start = error.find('(');
    std::string::size_type end = error.find(')');
    if (start != std::string::npos && end != std::string::npos) {
        std::string lineNumStr = error.substr(start+1, end-1);
        unsigned long num = strtoul(lineNumStr.c_str(), nullptr, 10);
        *lineNum = (unsigned int) num;
        if (num == ULONG_MAX || num == 0) {
            // Out of range, or no valid conversion could be performed.
            return false;
        }
        return true;
    } else {
        // Error message isn't formatted as expected.
        return false;
    }
}

// Return the substring for the inclusive range given the start and end indices.
static std::string
_GetSubstring(std::string const& str,
              std::string::size_type startPos,
              std::string::size_type endPos)
{
    if (endPos == std::string::npos) {
        return str.substr(startPos, endPos);
    }
    return str.substr(startPos, endPos - startPos + 1);
}

// It's helpful to have a few more lines around the erroring line when logging
// compiler error messages. This function returns this contextual info
// as a string.
static std::string
_GetCompileErrorCodeContext(std::string const &shader,
                            unsigned int lineNum,
                            unsigned int contextSize)
{
    unsigned int numLinesToSkip = 
        std::max<unsigned int>(0, lineNum - contextSize - 1);
    std::string::size_type i = 0;
    for (unsigned int line = 0; line < numLinesToSkip && i != std::string::npos;
         line++) {
        i = shader.find('\n', i+1); // find the next occurrance
    }

    if (i == std::string::npos) return std::string();

    // Copy context before the error line.
    std::string::size_type start = i;
    for (unsigned int line = 0; line < contextSize && i != std::string::npos; 
         line++) {
        i = shader.find('\n', i+1);
    }

    std::string context = _GetSubstring(shader, start, i);

    // Copy error line with annotation.
    start = i+1;
    i = shader.find('\n', start);
    context += _GetSubstring(shader, start, i-1) + " <<< ERROR!\n";

    // Copy context after error line.
    start = i+1;
    for (unsigned int line = 0; line < contextSize && i != std::string::npos; 
         line++) {
        i = shader.find('\n', i+1);
    }
    context += _GetSubstring(shader, start, i);

    return context;
}

static void
_DumpShaderSource(const char *shaderType, std::string const &shaderSource)
{
    std::cout << "--------- " << shaderType << " ----------\n";
    std::cout << shaderSource;
    std::cout << "---------------------------\n";
    std::cout << std::flush;
}

HdStGLSLProgram::HdStGLSLProgram(
    TfToken const &role,
    HdStResourceRegistry *const registry)
    :_registry(registry)
    , _role(role)
{
    static size_t globalDebugID = 0;
    _debugID = globalDebugID++;
}

HdStGLSLProgram::~HdStGLSLProgram()
{
    Hgi *const hgi = _registry->GetHgi();

    if (_program) {
        for (HgiShaderFunctionHandle fn : _program->GetShaderFunctions()) {
            hgi->DestroyShaderFunction(&fn);
        }
        hgi->DestroyShaderProgram(&_program);
    }
}

/* static */
HdStGLSLProgram::ID
HdStGLSLProgram::ComputeHash(TfToken const &sourceFile)
{
    HD_TRACE_FUNCTION();

    uint32_t hash = 0;
    std::string const &filename = sourceFile.GetString();
    hash = ArchHash(filename.c_str(), filename.size(), hash);

    return hash;
}

bool
HdStGLSLProgram::CompileShader(
    HgiShaderStage stage,
    std::string const &shaderSource)
{
    HD_TRACE_FUNCTION();
    HF_MALLOC_TAG_FUNCTION();

    // early out for empty source.
    // this may not be an error, since glslfx gives empty string
    // for undefined shader stages (i.e. null geometry shader)
    if (shaderSource.empty()) {
        return false;
    }

    const char *shaderType = nullptr;

    switch (stage) {
        case HgiShaderStageVertex:
            shaderType = "GL_VERTEX_SHADER";
            break;
        case HgiShaderStageTessellationControl:
            shaderType = "GL_TESS_CONTROL_SHADER";
            break;
        case HgiShaderStageTessellationEval:
            shaderType = "GL_TESS_EVALUATION_SHADER";
            break;
        case HgiShaderStageGeometry:
            shaderType = "GL_GEOMETRY_SHADER";
            break;
        case HgiShaderStageFragment:
            shaderType = "GL_FRAGMENT_SHADER";
            break;
        case HgiShaderStageCompute:
            shaderType = "GL_COMPUTE_SHADER";
            break;
    }

    if (!shaderType) {
        TF_CODING_ERROR("Invalid shader type %d\n", stage);
        return false;
    }

    if (TfDebug::IsEnabled(HDST_DUMP_SHADER_SOURCE)) {
        _DumpShaderSource(shaderType, shaderSource);
    }

    Hgi *const hgi = _registry->GetHgi();

    // Create a shader, compile it
    HgiShaderFunctionDesc shaderFnDesc;
    shaderFnDesc.shaderCode = shaderSource.c_str();
    shaderFnDesc.shaderStage = stage;
    HgiShaderFunctionHandle shaderFn = hgi->CreateShaderFunction(shaderFnDesc);

    std::string fname;
    if (TfDebug::IsEnabled(HDST_DUMP_SHADER_SOURCEFILE) ||
            ( TfDebug::IsEnabled(HDST_DUMP_FAILING_SHADER_SOURCEFILE) &&
              !shaderFn->IsValid())) {
        std::stringstream fnameStream;
        static size_t debugShaderID = 0;
        fnameStream << "program" << _debugID << "_shader" << debugShaderID++
                << "_" << shaderType << ".glsl";
        fname = fnameStream.str();
        std::fstream output(fname.c_str(), std::ios::out);
        output << shaderSource;
        output.close();

        std::cout << "Write " << fname << " (size=" << shaderSource.size() << ")\n";
    }

    if (!shaderFn->IsValid()) {
        std::string logString = shaderFn->GetCompileErrors();
        unsigned int lineNum = 0;
        if (_ParseLineNumberOfError(logString, &lineNum)) {
            // Get lines surrounding the erroring line for context.
            std::string errorContext =
                _GetCompileErrorCodeContext(shaderSource, lineNum, 3);
            if (!errorContext.empty()) {
                // erase the \0 if present.
                if (logString.back() == '\0') {
                    logString.erase(logString.end() - 1, logString.end());
                }
                logString.append("\nError Context:\n");
                logString.append(errorContext);
            }
        }
        
        const char* programName = fname.empty() ? shaderType : fname.c_str();
        TF_WARN("Failed to compile shader (%s): %s",
                programName, logString.c_str());

        if (TfDebug::IsEnabled(HDST_DUMP_FAILING_SHADER_SOURCE)) {
            _DumpShaderSource(shaderType, shaderSource);
        }

        // shader is no longer needed.
        hgi->DestroyShaderFunction(&shaderFn);

        return false;
    }

    // Store the shader function in the program descriptor so it can be used
    // during Link time.
    _programDesc.shaderFunctions.push_back(shaderFn);

    return true;
}

static const char*
_GetShaderType(HgiShaderStage stage)
{
    switch(stage) {
        case HgiShaderStageVertex:
            return "--------GL_VERTEX_SHADER--------\n";
        case HgiShaderStageFragment:
            return "--------GL_FRAGMENT_SHADER--------\n";
        case HgiShaderStageGeometry:
            return "--------GL_GEOMETRY_SHADER--------\n";
        case HgiShaderStageTessellationControl:
            return "--------GL_TESS_CONTROL_SHADER--------\n";
        case HgiShaderStageTessellationEval:
            return "--------GL_TESS_EVALUATION_SHADER--------\n";

        default:
            return "--------UNKNOWN_SHADER_STAGE--------\n";
    }
}

static std::string
_DebugLinkSource(HgiShaderProgramHandle const& program)
{
    std::string result;
    result = TfStringPrintf("==== Source Program ID=%p\nBEGIN_DUMP\n",
                            program.Get());

    for (HgiShaderFunctionHandle fn : program->GetShaderFunctions()) {
        HgiShaderFunctionDesc const& desc = fn->GetDescriptor();
        result.append(_GetShaderType(desc.shaderStage));
        result.append(desc.shaderCode);
    }

    result += "END DUMP\n";

    return result;
}

bool
HdStGLSLProgram::Link()
{
    HD_TRACE_FUNCTION();
    HF_MALLOC_TAG_FUNCTION();

    if (_programDesc.shaderFunctions.empty()) {
        TF_CODING_ERROR("At least one shader has to be compiled before linking.");
        return false;
    }

    Hgi *const hgi = _registry->GetHgi();

    // Create the shader program.
    if (_program) {
        hgi->DestroyShaderProgram(&_program);
    }
    _program = hgi->CreateShaderProgram(_programDesc);

    bool success = _program->IsValid();
    if (!success) {
        std::string const& logString = _program->GetCompileErrors();
        TF_WARN("Failed to link shader: %s", logString.c_str());

        if (TfDebug::IsEnabled(HDST_DUMP_FAILING_SHADER_SOURCE)) {
            std::cout << _DebugLinkSource(_program) << std::flush;
        }
    }

    return success;
}

bool
HdStGLSLProgram::Validate() const
{
    if (!_program || !_program->IsValid()) {
        return false;
    }

    return true;
}

HdStGLSLProgramSharedPtr
HdStGLSLProgram::GetComputeProgram(
        TfToken const &shaderToken,
        HdStResourceRegistry *resourceRegistry)
{
    return GetComputeProgram(HdStPackageComputeShader(), shaderToken, 
                            resourceRegistry);
}

HdStGLSLProgramSharedPtr
HdStGLSLProgram::GetComputeProgram(
        TfToken const &shaderFileName,
        TfToken const &shaderToken,
        HdStResourceRegistry *resourceRegistry)
{
    // Find the program from registry
    HdInstance<HdStGLSLProgramSharedPtr> programInstance =
                resourceRegistry->RegisterGLSLProgram(
                        HdStGLSLProgram::ComputeHash(shaderToken));

    if (programInstance.IsFirstInstance()) {
        // if not exists, create new one
        HdStGLSLProgramSharedPtr newProgram(
            new HdStGLSLProgram(HdTokens->computeShader, resourceRegistry));

        HioGlslfx glslfx(shaderFileName);
        std::string errorString;
        if (!glslfx.IsValid(&errorString)){
            TF_CODING_ERROR("Failed to parse " + shaderFileName.GetString() 
                            + ": " + errorString);
            return HdStGLSLProgramSharedPtr();
        }
        std::string version = "#version 430\n";
        if (!newProgram->CompileShader(
                HgiShaderStageCompute,
                version + glslfx.GetSource(shaderToken))) {
            TF_CODING_ERROR("Fail to compile " + shaderToken.GetString());
            return HdStGLSLProgramSharedPtr();
        }
        if (!newProgram->Link()) {
            TF_CODING_ERROR("Fail to link " + shaderToken.GetString());
            return HdStGLSLProgramSharedPtr();
        }
        programInstance.SetValue(newProgram);
    }
    return programInstance.GetValue();
}


PXR_NAMESPACE_CLOSE_SCOPE

