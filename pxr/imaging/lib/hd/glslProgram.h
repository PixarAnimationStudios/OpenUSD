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
#ifndef HD_GLSL_PROGRAM_H
#define HD_GLSL_PROGRAM_H

#include "pxr/pxr.h"
#include "pxr/imaging/hd/api.h"
#include "pxr/imaging/hd/version.h"
#include "pxr/imaging/hd/resourceGL.h"

#include <boost/shared_ptr.hpp>

PXR_NAMESPACE_OPEN_SCOPE

class HdResourceRegistry;
typedef boost::shared_ptr<class HdGLSLProgram> HdGLSLProgramSharedPtr;

/// \class HdGLSLProgram
///
/// An instance of a glsl program.
///
// XXX: this design is transitional and will be revised soon.
class HdGLSLProgram
{
public:
    typedef size_t ID;

    HD_API
    HdGLSLProgram(TfToken const &role);
    HD_API
    ~HdGLSLProgram();

    /// Returns the hash value of the program for \a sourceFile
    HD_API
    static ID ComputeHash(TfToken const & sourceFile);

    /// Compile shader source of type
    HD_API
    bool CompileShader(GLenum type, std::string const & source);

    /// Link the compiled shaders together.
    HD_API
    bool Link();

    /// Validate if this program is a valid progam in the current context.
    HD_API
    bool Validate() const;

    /// Returns HdResource of the program object.
    HdResourceGL const &GetProgram() const { return _program; }

    /// Returns HdResource of the global uniform buffer object for this program.
    HdResourceGL const &GetGlobalUniformBuffer() const {
        return _uniformBuffer;
    }

    /// Convenience method to get a shared compute shader program
    HD_API
    static HdGLSLProgramSharedPtr GetComputeProgram(TfToken const &shaderToken,
        HdResourceRegistry *resourceRegistry);

private:
    HdResourceGL _program;
    HdResourceGL _uniformBuffer;
};


PXR_NAMESPACE_CLOSE_SCOPE

#endif  // HD_COMPUTE_SHADER_H
