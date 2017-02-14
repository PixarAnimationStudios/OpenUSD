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

/// \file glslProgram.h


#ifndef __PX_VP20_GLSL_PROGRAM_H__
#define __PX_VP20_GLSL_PROGRAM_H__

#include "pxr/pxr.h"

#include <GL/gl.h>

#include <string>


PXR_NAMESPACE_OPEN_SCOPE


/// \class PxrMayaGLSLProgram
///
/// A convenience class that abstracts away the OpenGL API details of
/// compiling and linking GLSL shaders into a program.
///
class PxrMayaGLSLProgram
{
    public:
        PxrMayaGLSLProgram();
        virtual ~PxrMayaGLSLProgram();

        /// Compile a shader of type \p type with the given \p source.
        bool CompileShader(const GLenum type, const std::string& source);

        /// Link the compiled shaders together.
        bool Link();

        /// Validate whether this program is valid in the current context.
        bool Validate() const;

        /// Get the ID of the OpenGL program object.
        GLuint GetProgramId() const {
            return mProgramId;
        };

    private:
        GLuint mProgramId;
};


PXR_NAMESPACE_CLOSE_SCOPE


#endif  // __PX_VP20_GLSL_PROGRAM_H__
