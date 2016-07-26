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
///
/// \file glf/diagnostic.h

#ifndef GLF_DIAGNOSTIC_H
#define GLF_DIAGNOSTIC_H

#include "pxr/imaging/garch/gl.h"
#include "pxr/base/tf/diagnostic.h"

#include <boost/noncopyable.hpp>
#include <string>
#include <cstdint>

///
/// \brief Posts diagnostic errors for all GL errors in the current context.
/// This macro tags the diagnostic errors with the name of the calling
/// function.
#define GLF_POST_PENDING_GL_ERRORS() \
        GlfPostPendingGLErrors(__ARCH_PRETTY_FUNCTION__)

///
/// \brief Posts diagnostic errors for all GL errors in the current context.
void GlfPostPendingGLErrors(std::string const & where = std::string());

///
/// \brief Registers GlfDefaultDebugOutputMessageCallback as the 
/// debug message callback for the current GL context.
void GlfRegisterDefaultDebugOutputMessageCallback();

///
/// \brief A GL debug output message callback method which posts diagnostic
/// errors for messages of type DEBUG_TYPE_ERROR and diagnostic warnings
/// for other message types.
void GlfDefaultDebugOutputMessageCallback(
        GLenum source, GLenum type, GLuint id, GLenum severity,
        GLsizei length, GLchar const * message, GLvoid * userParam);

///
/// \brief Returns a string representation of debug output enum values.
char const * GlfDebugEnumToString(GLenum debugEnum);

///
/// \class GlfGLQueryObject
/// \brief Represents a GL query object in Glf
///
class GlfGLQueryObject : public boost::noncopyable {
public:
    GlfGLQueryObject();
    ~GlfGLQueryObject();

    /// Begin query for the given \p target
    /// target has to be one of
    ///   GL_SAMPLES_PASSED, GL_ANY_SAMPLES_PASSED,
    ///   GL_ANY_SAMPLES_PASSED_CONSERVATIVE, GL_PRIMITIVES_GENERATED
    ///   GL_TRANSFORM_FEEDBACK_PRIMITIVES_WRITTEN
    ///   GL_TIME_ELAPSED, GL_TIMESTAMP
    void Begin(GLenum target);

    /// equivalent to Begin(GL_SAMPLES_PASSED).
    /// The number of samples that pass the depth test for all drawing
    /// commands within the scope of the query will be returned.
    void BeginSamplesPassed();

    /// equivalent to Begin(GL_PRIMITIVES_GENERATED).
    /// The number of primitives sent to the rasterizer by the scoped
    /// drawing command will be returned.
    void BeginPrimitivesGenerated();

    /// equivalent to Begin(GL_TIME_ELAPSED).
    /// The time that it takes for the GPU to execute all of the scoped commands
    /// will be returned in nanoseconds.
    void BeginTimeElapsed();

    /// End query
    void End();

    /// Return the query result (synchronous)
    /// stalls CPU until the result becomes available.
    int64_t GetResult();

    /// Return the query result (asynchronous)
    /// returns 0 if the result hasn't been available.
    int64_t GetResultNoWait();

private:
    GLuint _id;
    GLenum _target;
};

#endif
