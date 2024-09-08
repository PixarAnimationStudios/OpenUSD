//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_IMAGING_HD_ST_GLSL_PROGRAM_H
#define PXR_IMAGING_HD_ST_GLSL_PROGRAM_H

#include "pxr/pxr.h"
#include "pxr/imaging/hd/version.h"
#include "pxr/imaging/hdSt/api.h"
#include "pxr/imaging/hgi/buffer.h"
#include "pxr/imaging/hgi/shaderProgram.h"
#include "pxr/imaging/hgi/enums.h"

#include <functional>

PXR_NAMESPACE_OPEN_SCOPE

class HdStResourceRegistry;
using HdStGLSLProgramSharedPtr = std::shared_ptr<class HdStGLSLProgram>;

using HgiShaderProgramHandle = HgiHandle<class HgiShaderProgram>;

/// \class HdStGLSLProgram
///
/// An instance of a glsl program.
///
class HdStGLSLProgram final
{
public:
    typedef size_t ID;

    HDST_API
    HdStGLSLProgram(TfToken const &role, HdStResourceRegistry*const registry);
    HDST_API
    ~HdStGLSLProgram();

    /// Compile shader source for a shader stage.
    HDST_API
    bool CompileShader(HgiShaderStage stage, std::string const & source);

    /// Compile shader source for a shader stage from an HgiShaderFunctionDesc.
    HDST_API
    bool CompileShader(HgiShaderFunctionDesc const &desc);

    /// Link the compiled shaders together.
    HDST_API
    bool Link();

    /// Validate if this program is a valid progam in the current context.
    HDST_API
    bool Validate() const;

    /// Returns HgiShaderProgramHandle for the shader program.
    HgiShaderProgramHandle const &GetProgram() const { return _program; }

    /// Convenience method to get a shared compute shader program
    HDST_API
    static HdStGLSLProgramSharedPtr GetComputeProgram(
        TfToken const &shaderToken,
        HdStResourceRegistry *resourceRegistry);

    HDST_API
    static HdStGLSLProgramSharedPtr GetComputeProgram(
        TfToken const &shaderFileName,
        TfToken const &shaderToken,
        HdStResourceRegistry *resourceRegistry);

    using PopulateDescriptorCallback =
        std::function<void(HgiShaderFunctionDesc &computeDesc)>;

    HDST_API
    static HdStGLSLProgramSharedPtr GetComputeProgram(
        TfToken const &shaderToken,
        HdStResourceRegistry *resourceRegistry,
        PopulateDescriptorCallback populateDescriptor);

    HDST_API
    static HdStGLSLProgramSharedPtr GetComputeProgram(
        TfToken const &shaderToken,
        std::string const &defines,
        HdStResourceRegistry *resourceRegistry,
        PopulateDescriptorCallback populateDescriptor);

    HDST_API
    static HdStGLSLProgramSharedPtr GetComputeProgram(
        TfToken const &shaderFileName,
        TfToken const &shaderToken,
        std::string const &defines,
        HdStResourceRegistry *resourceRegistry,
        PopulateDescriptorCallback populateDescriptor);

    /// Returns the role of the GPU data in this resource.
    TfToken const & GetRole() const {return _role;}

private:
    HdStResourceRegistry *const _registry;
    TfToken _role;

    HgiShaderProgramDesc _programDesc;
    HgiShaderProgramHandle _program;

    // An identifier for uniquely identifying the program, for debugging
    // purposes - programs that fail to compile for one reason or another
    // will get deleted, and their GL program IDs reused, so we can't use
    // that to identify it uniquely
    size_t _debugID;
};


PXR_NAMESPACE_CLOSE_SCOPE

#endif  // PXR_IMAGING_HD_ST_GLSL_PROGRAM_H
