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
#ifndef GLF_DIAGNOSTIC_H
#define GLF_DIAGNOSTIC_H

/// \file glf/diagnostic.h

#include "pxr/pxr.h"
#include "pxr/imaging/glf/api.h"
#include "pxr/imaging/garch/gl.h"
#include "pxr/base/tf/diagnostic.h"

#include <string>
#include <cstdint>

PXR_NAMESPACE_OPEN_SCOPE


/// Posts diagnostic errors for all GL errors in the current context.
/// This macro tags the diagnostic errors with the name of the calling
/// function.
#define GLF_POST_PENDING_GL_ERRORS() \
        GlfPostPendingGLErrors(__ARCH_PRETTY_FUNCTION__)

/// Posts diagnostic errors for all GL errors in the current context.
GLF_API
void GlfPostPendingGLErrors(std::string const & where = std::string());

/// Registers GlfDefaultDebugOutputMessageCallback as the 
/// debug message callback for the current GL context.
GLF_API
void GlfRegisterDefaultDebugOutputMessageCallback();

/// A GL debug output message callback method which posts diagnostic
/// errors for messages of type DEBUG_TYPE_ERROR and diagnostic warnings
/// for other message types.
GLF_API
void GlfDefaultDebugOutputMessageCallback(
        GLenum source, GLenum type, GLuint id, GLenum severity,
        GLsizei length, char const * message, GLvoid const * userParam);

/// Returns a string representation of debug output enum values.
GLF_API
char const * GlfDebugEnumToString(GLenum debugEnum);

/// \class GlfDebugGroup
///
/// Represents a GL debug group in Glf
///
class GlfDebugGroup {
    public:
    /// Pushes a new debug group onto the GL api debug trace stack
    GLF_API
    GlfDebugGroup(char const *message);
    
    /// Pops a debug group off the GL api debug trace stack
    GLF_API
    ~GlfDebugGroup();

    GlfDebugGroup() = delete;
    GlfDebugGroup(GlfDebugGroup const&) = delete;
    GlfDebugGroup& operator =(GlfDebugGroup const&) = delete;
};

/// \class GlfGLQueryObject
///
/// Represents a GL query object in Glf
///
class GlfGLQueryObject {
public:
    GLF_API
    GlfGLQueryObject();
    GLF_API
    ~GlfGLQueryObject();

    /// Begin query for the given \p target
    /// target has to be one of
    ///   GL_SAMPLES_PASSED, GL_ANY_SAMPLES_PASSED,
    ///   GL_ANY_SAMPLES_PASSED_CONSERVATIVE, GL_PRIMITIVES_GENERATED
    ///   GL_TRANSFORM_FEEDBACK_PRIMITIVES_WRITTEN
    ///   GL_TIME_ELAPSED, GL_TIMESTAMP
    GLF_API
    void Begin(GLenum target);

    /// equivalent to Begin(GL_SAMPLES_PASSED).
    /// The number of samples that pass the depth test for all drawing
    /// commands within the scope of the query will be returned.
    GLF_API
    void BeginSamplesPassed();

    /// equivalent to Begin(GL_PRIMITIVES_GENERATED).
    /// The number of primitives sent to the rasterizer by the scoped
    /// drawing command will be returned.
    GLF_API
    void BeginPrimitivesGenerated();

    /// equivalent to Begin(GL_TIME_ELAPSED).
    /// The time that it takes for the GPU to execute all of the scoped commands
    /// will be returned in nanoseconds.
    GLF_API
    void BeginTimeElapsed();

    /// End query
    GLF_API
    void End();

    /// Return the query result (synchronous)
    /// stalls CPU until the result becomes available.
    GLF_API
    int64_t GetResult();

    /// Return the query result (asynchronous)
    /// returns 0 if the result hasn't been available.
    GLF_API
    int64_t GetResultNoWait();

    GlfGLQueryObject(GlfGLQueryObject const&) = delete;
    GlfGLQueryObject& operator =(GlfGLQueryObject const&) = delete;
private:
    GLuint _id;
    GLenum _target;
};


PXR_NAMESPACE_CLOSE_SCOPE

#endif
