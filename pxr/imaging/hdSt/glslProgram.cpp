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
#include "pxr/imaging/hdSt/resourceRegistry.h"
#include "pxr/base/arch/hash.h"
#include "pxr/base/tf/diagnostic.h"
#include "pxr/base/tf/scopeDescription.h"

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

static const char*
_GetShaderType(HgiShaderStage stage)
{
    switch(stage) {
        case HgiShaderStageCompute:
            return "COMPUTE_SHADER";
        case HgiShaderStageVertex:
            return "VERTEX_SHADER";
        case HgiShaderStageFragment:
            return "FRAGMENT_SHADER";
        case HgiShaderStageGeometry:
            return "GEOMETRY_SHADER";
        case HgiShaderStageTessellationControl:
            return "TESS_CONTROL_SHADER";
        case HgiShaderStageTessellationEval:
            return "TESS_EVALUATION_SHADER";
        case HgiShaderStagePostTessellationControl:
            return "POST_TESS_CONTROL_SHADER";
        case HgiShaderStagePostTessellationVertex:
            return "POST_TESS_VERTEX_SHADER";
        default:
            return nullptr;
    }
}

static void
_DumpShaderSource(const char *shaderType, std::string const &shaderSource)
{
    std::cout << "--------- " << shaderType << " ----------\n"
              << shaderSource
              << "---------------------------\n"
              << std::flush;
}

static void
_DumpShaderSource(HgiShaderFunctionDesc const &desc)
{
    const char *shaderType = _GetShaderType(desc.shaderStage);

    std::cout << "--------- " << shaderType << " ----------\n";

    if (desc.shaderCodeDeclarations) {
        std::cout << desc.shaderCodeDeclarations;
    } else {
        std::cout << "(shaderCodeDeclarations empty)\n"; 
    }

    if (TF_VERIFY(desc.shaderCode)) {
        std::cout << desc.shaderCode;
    } else {
        std::cout << "(shaderCode empty)\n"; 
    }

    std::cout << "---------------------------\n" << std::flush;
}

static bool
_ValidateCompilation(
    HgiShaderFunctionHandle shaderFn,
    const char *shaderType,
    std::string const &shaderSource,
    size_t debugID)
{
    std::string fname;
    if (TfDebug::IsEnabled(HDST_DUMP_SHADER_SOURCEFILE) ||
        ( TfDebug::IsEnabled(HDST_DUMP_FAILING_SHADER_SOURCEFILE) &&
          !shaderFn->IsValid())) {
        std::stringstream fnameStream;
        static size_t debugShaderID = 0;
        fnameStream << "program" << debugID << "_shader" << debugShaderID++
                << "_" << shaderType << ".glsl";
        fname = fnameStream.str();
        std::fstream output(fname.c_str(), std::ios::out);
        output << shaderSource;
        output.close();

        std::cout << "Write " << fname
                  << " (size=" << shaderSource.size() << ")\n";
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

        const char * const programName =
                fname.empty() ? shaderType : fname.c_str();
        TF_WARN("Failed to compile shader (%s): %s",
                programName, logString.c_str());

        if (TfDebug::IsEnabled(HDST_DUMP_FAILING_SHADER_SOURCE)) {
            _DumpShaderSource(shaderType, shaderSource);
        }

        return false;
    }
    return true;
}

static std::string
_GetScopeDescriptionLabel(HgiShaderProgramDesc const &desc)
{
    if (desc.debugName.empty()) {
        return {};
    } else {
        return TfStringPrintf(" (%s)", desc.debugName.c_str());
    }
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
static
HdStGLSLProgram::ID
_ComputeHash(TfToken const &sourceFile,
             std::string const &defines = std::string())
{
    HD_TRACE_FUNCTION();

    uint32_t hash = 0;
    std::string const &filename = sourceFile.GetString();
    hash = ArchHash(filename.c_str(), filename.size(), hash);
    hash = ArchHash(defines.c_str(), defines.size(), hash);

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

    const char *shaderType = _GetShaderType(stage);
    if (!shaderType) {
        TF_CODING_ERROR("Invalid shader type %d\n", stage);
        return false;
    }

    TF_DESCRIBE_SCOPE(
        "Compiling GLSL shader" + _GetScopeDescriptionLabel(_programDesc));

    if (TfDebug::IsEnabled(HDST_DUMP_SHADER_SOURCE)) {
        _DumpShaderSource(shaderType, shaderSource);
    }

    Hgi *const hgi = _registry->GetHgi();

    // Create a shader, compile it
    HgiShaderFunctionDesc shaderFnDesc;
    shaderFnDesc.shaderCode = shaderSource.c_str();
    shaderFnDesc.shaderStage = stage;

    std::string generatedCode;
    shaderFnDesc.generatedShaderCodeOut = &generatedCode;

    HgiShaderFunctionHandle shaderFn = hgi->CreateShaderFunction(shaderFnDesc);

    if (!_ValidateCompilation(shaderFn, shaderType, generatedCode, _debugID)) {
        // shader is no longer needed.
        hgi->DestroyShaderFunction(&shaderFn);

        return false;
    }

    // Store the shader function in the program descriptor so it can be used
    // during Link time.
    _programDesc.shaderFunctions.push_back(shaderFn);

    return true;
}

bool
HdStGLSLProgram::CompileShader(HgiShaderFunctionDesc const &desc)
{
    HD_TRACE_FUNCTION();
    HF_MALLOC_TAG_FUNCTION();

    // early out for empty source.
    // this may not be an error, since glslfx gives empty string
    // for undefined shader stages (i.e. null geometry shader)
    if (!desc.shaderCode) return false;

    const char *shaderType = _GetShaderType(desc.shaderStage);
    if (!shaderType) {
        TF_CODING_ERROR("Invalid shader type %d\n", desc.shaderStage);
        return false;
    }

    TF_DESCRIBE_SCOPE(
        "Compiling GLSL shader" + _GetScopeDescriptionLabel(_programDesc));

    if (TfDebug::IsEnabled(HDST_DUMP_SHADER_SOURCE)) {
        _DumpShaderSource(desc);
    }

    // Create a shader, compile it
    Hgi *const hgi = _registry->GetHgi();

    // Optionally, capture generated shader code for diagnostic output.
    std::string *generatedCode = desc.generatedShaderCodeOut;

    HgiShaderFunctionHandle shaderFn = hgi->CreateShaderFunction(desc);

    if (!_ValidateCompilation(shaderFn, shaderType, *generatedCode, _debugID)) {
        // shader is no longer needed.
        hgi->DestroyShaderFunction(&shaderFn);

        return false;
    }

    // Store the shader function in the program descriptor so it can be used
    // during Link time.
    _programDesc.shaderFunctions.push_back(shaderFn);

    return true;
}

static std::string
_DebugLinkSource(HgiShaderProgramHandle const& program)
{
    std::string result;
    result = TfStringPrintf("==== Source Program ID=%p\nBEGIN_DUMP\n",
                            program.Get());

    for (HgiShaderFunctionHandle fn : program->GetShaderFunctions()) {
        HgiShaderFunctionDesc const& desc = fn->GetDescriptor();
        result.append("--------");
        result.append(_GetShaderType(desc.shaderStage));
        result.append("--------\n");
        if (TF_VERIFY(desc.shaderCode)) {
            result.append(desc.shaderCode);
        } else {
            result.append("(shaderCode empty)\n");
        }
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

    TF_DESCRIBE_SCOPE(
        "Linking GLSL shader" + _GetScopeDescriptionLabel(_programDesc));

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
    const HdStGLSLProgram::ID hash = _ComputeHash(shaderToken);

    // Find the program from registry
    HdInstance<HdStGLSLProgramSharedPtr> programInstance =
                resourceRegistry->RegisterGLSLProgram(hash);

    if (programInstance.IsFirstInstance()) {

        TF_DEBUG(HDST_LOG_COMPUTE_SHADER_PROGRAM_MISSES).Msg(
            "(MISS) First compute program instance for %s (hash = %zu)\n",
            shaderFileName.GetText(), hash);

        // if not exists, create new one
        HdStGLSLProgramSharedPtr newProgram =
            std::make_shared<HdStGLSLProgram>(
                HdTokens->computeShader, resourceRegistry);

        HioGlslfx glslfx(shaderFileName);
        std::string errorString;
        if (!glslfx.IsValid(&errorString)){
            TF_CODING_ERROR("Failed to parse " + shaderFileName.GetString() 
                            + ": " + errorString);
            return nullptr;
        }
        if (!newProgram->CompileShader(
                HgiShaderStageCompute, glslfx.GetSource(shaderToken))) {
            TF_CODING_ERROR("Fail to compile " + shaderToken.GetString());
            return nullptr;
        }
        if (!newProgram->Link()) {
            TF_CODING_ERROR("Fail to link " + shaderToken.GetString());
            return nullptr;
        }
        programInstance.SetValue(newProgram);

    } else {
        TF_DEBUG(HDST_LOG_COMPUTE_SHADER_PROGRAM_HITS).Msg(
            "(HIT) Found compute program instance for %s (hash = %zu)\n",
            shaderFileName.GetText(), hash);
    }
    return programInstance.GetValue();
}

HdStGLSLProgramSharedPtr
HdStGLSLProgram::GetComputeProgram(
    TfToken const &shaderToken,
    HdStResourceRegistry *resourceRegistry,
    PopulateDescriptorCallback populateDescriptor)
{
    return GetComputeProgram(shaderToken,
                             std::string(),
                             resourceRegistry,
                             populateDescriptor);
}

HdStGLSLProgramSharedPtr
HdStGLSLProgram::GetComputeProgram(
    TfToken const &shaderToken,
    std::string const &defines,
    HdStResourceRegistry *resourceRegistry,
    PopulateDescriptorCallback populateDescriptor)
{
    return GetComputeProgram(HdStPackageComputeShader(),
                             shaderToken,
                             defines,
                             resourceRegistry,
                             populateDescriptor);
}

HdStGLSLProgramSharedPtr
HdStGLSLProgram::GetComputeProgram(
    TfToken const &shaderFileName,
    TfToken const &shaderToken,
    std::string const &defines,
    HdStResourceRegistry *resourceRegistry,
    PopulateDescriptorCallback populateDescriptor)
{
    const HdStGLSLProgram::ID hash = _ComputeHash(shaderToken, defines);
    // Find the program from registry
    HdInstance<HdStGLSLProgramSharedPtr> programInstance =
                resourceRegistry->RegisterGLSLProgram(hash);

    if (programInstance.IsFirstInstance()) {

        TF_DEBUG(HDST_LOG_COMPUTE_SHADER_PROGRAM_MISSES).Msg(
            "(MISS) First compute program instance for %s (hash = %zu)\n",
            shaderFileName.GetText(), hash);

        // If program does not exist, create new one
        const HioGlslfx glslfx(shaderFileName, HioGlslfxTokens->defVal);
        std::string errorString;
        if (!glslfx.IsValid(&errorString)){
            TF_CODING_ERROR("Failed to parse " + shaderFileName.GetString()
                            + ": " + errorString);
            return nullptr;
        }

        Hgi *hgi = resourceRegistry->GetHgi();

        HgiShaderFunctionDesc computeDesc;
        populateDescriptor(computeDesc);

        const std::string sourceCode = defines + glslfx.GetSource(shaderToken);
        computeDesc.shaderCode = sourceCode.c_str();

        std::string generatedCode;
        computeDesc.generatedShaderCodeOut = &generatedCode;

        HgiShaderFunctionHandle computeFn =
            hgi->CreateShaderFunction(computeDesc);

        static const char *shaderType = "GL_COMPUTE_SHADER";

        if (!_ValidateCompilation(computeFn, shaderType, generatedCode, 0)) {
            // shader is no longer needed.
            hgi->DestroyShaderFunction(&computeFn);
            return nullptr;
        }

        HdStGLSLProgramSharedPtr newProgram =
            std::make_shared<HdStGLSLProgram>(
                HdTokens->computeShader, resourceRegistry);

        newProgram->_programDesc.shaderFunctions.push_back(computeFn);
        if (!newProgram->Link()) {
            TF_CODING_ERROR("Fail to link " + shaderToken.GetString());
            return nullptr;
        }
        programInstance.SetValue(newProgram);
    } else {
        TF_DEBUG(HDST_LOG_COMPUTE_SHADER_PROGRAM_HITS).Msg(
            "(HIT) Found compute program instance for %s (hash = %zu)\n",
            shaderFileName.GetText(), hash);
    }
    return programInstance.GetValue();
}


PXR_NAMESPACE_CLOSE_SCOPE

