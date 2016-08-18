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

#include "pxr/imaging/hd/version.h"
#include "pxr/imaging/hd/resource.h"

#include <boost/shared_ptr.hpp>

typedef boost::shared_ptr<class HdGLSLProgram> HdGLSLProgramSharedPtr;

/// \class HdGLSLProgram
///
/// An instance of glsl program.
///
/// This is almost a stripped version of GlfGLSLProgram, intended
/// not to disturb Glf API during Hd development.
/// It will be refactored and integrated back to Glf at some point.
///
/// This class assumes every glsl program has a single uniform block.
/// Although this class generate ID of the buffer object, the allocation
/// and update of the uniform block is caller's responsibility.
///
// XXX: this design is transitional and will be revised soon.
class HdGLSLProgram
{
public:
    typedef size_t ID;

    HdGLSLProgram(TfToken const &role);
    ~HdGLSLProgram();

    /// Returns the hash value of the program for \a sourceFile
    static ID ComputeHash(TfToken const & sourceFile);

    /// Compile shader source of type
    bool CompileShader(GLenum type, std::string const & source);

    /// Link the compiled shaders together.
    bool Link();

    /// Validate if this program is a valid progam in the current context.
    bool Validate() const;

    /// Returns HdResource of the program object.
    HdResource const &GetProgram() const { return _program; };

    /// Returns HdResource of the global uniform buffer object for this program.
    HdResource const &GetGlobalUniformBuffer() const {
        return _uniformBuffer;
    };

    /// Convenience method to get a shared compute shader program
    static HdGLSLProgramSharedPtr GetComputeProgram(TfToken const &shaderToken);

private:
    HdResource _program;
    HdResource _uniformBuffer;
};


#endif  // HD_COMPUTE_SHADER_H
