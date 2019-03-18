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
// Diagnostic.cpp
//


#include "pxr/imaging/glf/glew.h"

#include "pxr/imaging/glf/diagnostic.h"
#include "pxr/imaging/glf/debugCodes.h"
#include "pxr/imaging/glf/glContext.h"

#include "pxr/base/tf/diagnostic.h"
#include "pxr/base/tf/envSetting.h"
#include "pxr/base/tf/stackTrace.h"
#include "pxr/base/tf/stringUtils.h"

#include <sstream>

PXR_NAMESPACE_OPEN_SCOPE


TF_DEFINE_ENV_SETTING(GLF_ENABLE_DIAGNOSTIC_TRACE, 0,
                      "Enable glDebug* diagnostic tracing in Glf.");

static bool
GlfTraceEnabled()
{
#if defined(GL_KHR_debug)
    static bool _v = TfGetEnvSetting(GLF_ENABLE_DIAGNOSTIC_TRACE) == 1;
    return _v;
#else
    return false;
#endif
}

void
GlfPostPendingGLErrors(std::string const & where)
{
    bool foundError = false;
    GLenum error;
    // Protect from doing infinite looping when glGetError
    // is called from an invalid context.
    int watchDogCount = 0;
    while ((watchDogCount++ < 256) &&
            ((error = glGetError()) != GL_NO_ERROR)) {
        foundError = true;
        const GLubyte *errorString = gluErrorString(error);

        std::ostringstream errorMessage;
        errorMessage << "GL error: " << errorString;

        if (!where.empty()) {
            errorMessage << ", reported from " << where;
        }

        TF_DEBUG(GLF_DEBUG_ERROR_STACKTRACE).Msg(errorMessage.str() + "\n");

        TF_RUNTIME_ERROR(errorMessage.str());
    }
    if (foundError) {
        TF_DEBUG(GLF_DEBUG_ERROR_STACKTRACE).Msg(
            TfStringPrintf("==== GL Error Stack ====\n%s\n",
                           TfGetStackTrace().c_str()));
    }
}

void
GlfRegisterDefaultDebugOutputMessageCallback()
{
#if defined(GL_KHR_debug)
    if (glDebugMessageCallbackARB) {
        glDebugMessageCallbackARB(
            (GLDEBUGPROCARB)GlfDefaultDebugOutputMessageCallback, 0);
        // Disable push/pop group messages; we don't want to print these.
        glDebugMessageControlARB(GL_DONT_CARE, GL_DEBUG_TYPE_PUSH_GROUP,
            GL_DONT_CARE, 0, nullptr, GL_FALSE);
        glDebugMessageControlARB(GL_DONT_CARE, GL_DEBUG_TYPE_POP_GROUP,
            GL_DONT_CARE, 0, nullptr, GL_FALSE);
        glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS_ARB);
    }
#endif
}

void
GlfDefaultDebugOutputMessageCallback(
        GLenum source, GLenum type, GLuint id, GLenum severity,
        GLsizei length, GLchar const * message, GLvoid const * userParam)
{
#if defined(GL_ARB_debug_output) || defined(GL_VERSION_4_3)
    if (type == GL_DEBUG_TYPE_ERROR_ARB) {
        TF_RUNTIME_ERROR("GL debug output: "
	                 "source: %s type: %s id: %d severity: %s message: %s",
        GlfDebugEnumToString(source),
        GlfDebugEnumToString(type),
        id,
        GlfDebugEnumToString(severity),
        message);

        TF_DEBUG(GLF_DEBUG_ERROR_STACKTRACE).Msg(
            TfStringPrintf("==== GL Error Stack ====\n%s\n",
                           TfGetStackTrace().c_str()));
    } else {
        TF_WARN("GL debug output: %s", message);
    }
#endif
}

char const *
GlfDebugEnumToString(GLenum debugEnum)
{
#if defined(GL_ARB_debug_output) || defined(GL_VERSION_4_3)
    switch (debugEnum) {
    case GL_DEBUG_SOURCE_API_ARB:
        return "GL_DEBUG_SOURCE_API";
    case GL_DEBUG_SOURCE_WINDOW_SYSTEM_ARB:
        return "GL_DEBUG_SOURCE_WINDOW_SYSTEM";
    case GL_DEBUG_SOURCE_SHADER_COMPILER_ARB:
        return "GL_DEBUG_SOURCE_SHADER_COMPILER";
    case GL_DEBUG_SOURCE_THIRD_PARTY_ARB:
        return "GL_DEBUG_SOURCE_THIRD_PARTY";
    case GL_DEBUG_SOURCE_APPLICATION_ARB:
        return "GL_DEBUG_SOURCE_APPLICATION";
    case GL_DEBUG_SOURCE_OTHER_ARB:
        return "GL_DEBUG_SOURCE_OTHER";

    case GL_DEBUG_TYPE_ERROR_ARB:
        return "GL_DEBUG_TYPE_ERROR";
    case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR_ARB:
        return "GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR";
    case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR_ARB:
        return "GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR";
    case GL_DEBUG_TYPE_PORTABILITY_ARB:
        return "GL_DEBUG_TYPE_PORTABILITY";
    case GL_DEBUG_TYPE_PERFORMANCE_ARB:
        return "GL_DEBUG_TYPE_PERFORMANCE";
    case GL_DEBUG_TYPE_OTHER_ARB:
        return "GL_DEBUG_TYPE_OTHER";
#if defined(GL_VERSION_4_3)
    case GL_DEBUG_TYPE_MARKER:
        return "GL_DEBUG_TYPE_MARKER";
    case GL_DEBUG_TYPE_PUSH_GROUP:
        return "GL_DEBUG_TYPE_PUSH_GROUP";
    case GL_DEBUG_TYPE_POP_GROUP:
        return "GL_DEBUG_TYPE_POP_GROUP";
#endif

#if defined(GL_VERSION_4_3)
    case GL_DEBUG_SEVERITY_NOTIFICATION:
        return "GL_DEBUG_SEVERITY_NOTIFICATION";
#endif
    case GL_DEBUG_SEVERITY_HIGH_ARB:
        return "GL_DEBUG_SEVERITY_HIGH";
    case GL_DEBUG_SEVERITY_MEDIUM_ARB:
        return "GL_DEBUG_SEVERITY_MEDIUM";
    case GL_DEBUG_SEVERITY_LOW_ARB:
        return "GL_DEBUG_SEVERITY_LOW";
    }
#endif
    TF_CODING_ERROR("unknown debug enum");
    return "unknown";
}

static void
_GlfPushDebugGroup(char const * message)
{
#if defined(GL_KHR_debug)
    if (GLEW_KHR_debug) {
        glPushDebugGroup(GL_DEBUG_SOURCE_THIRD_PARTY, 0, -1, message);
    }
#endif
}

static void
_GlfPopDebugGroup()
{
#if defined(GL_KHR_debug)
    if (GLEW_KHR_debug) {
        glPopDebugGroup();
    }
#endif
}

GlfDebugGroup::GlfDebugGroup(char const *message)
{
    if (GlfTraceEnabled()) {
        _GlfPushDebugGroup(message);
    }
}

GlfDebugGroup::~GlfDebugGroup()
{
    if (GlfTraceEnabled()) {
        _GlfPopDebugGroup();
    }
}

void
GlfDebugLabelBuffer(GLuint id, char const *label)
{
#if defined(GL_KHR_debug)
    if (GlfTraceEnabled()) {
        if (GLEW_KHR_debug) {
            glObjectLabel(GL_BUFFER, id, -1, label);
        }
    }
#endif
}

void
GlfDebugLabelShader(GLuint id, char const *label)
{
#if defined(GL_KHR_debug)
    if (GlfTraceEnabled()) {
        if (GLEW_KHR_debug) {
            glObjectLabel(GL_SHADER, id, -1, label);
        }
    }
#endif
}

void
GlfDebugLabelProgram(GLuint id, char const *label)
{
#if defined(GL_KHR_debug)
    if (GlfTraceEnabled()) {
        if (GLEW_KHR_debug) {
            glObjectLabel(GL_PROGRAM, id, -1, label);
        }
    }
#endif
}

GlfGLQueryObject::GlfGLQueryObject()
    : _id(0), _target(0)
{
    GlfGlewInit();
    if (glGenQueries) {
        glGenQueries(1, &_id);
    }
}

GlfGLQueryObject::~GlfGLQueryObject()
{
    GlfSharedGLContextScopeHolder sharedGLContextScopeHolder;
    if (glDeleteQueries && _id) {
        glDeleteQueries(1, &_id);
    }
}

void
GlfGLQueryObject::BeginSamplesPassed()
{
    Begin(GL_SAMPLES_PASSED);
}

void
GlfGLQueryObject::BeginPrimitivesGenerated()
{
    Begin(GL_PRIMITIVES_GENERATED);
}
void
GlfGLQueryObject::BeginTimeElapsed()
{
    Begin(GL_TIME_ELAPSED);
}

void
GlfGLQueryObject::Begin(GLenum target)
{
    _target = target;
    if (glBeginQuery && _id) {
        glBeginQuery(_target, _id);
    }
}

void
GlfGLQueryObject::End()
{
    if (glEndQuery && _target) {
        glEndQuery(_target);
    }
    _target = 0;
}

GLint64
GlfGLQueryObject::GetResult()
{
    GLint64 value = 0;
    if (glGetQueryObjecti64v && _id) {
        glGetQueryObjecti64v(_id, GL_QUERY_RESULT, &value);
    }
    return value;
}

GLint64
GlfGLQueryObject::GetResultNoWait()
{
    GLint64 value = 0;
    if (glGetQueryObjecti64v && _id) {
        glGetQueryObjecti64v(_id, GL_QUERY_RESULT_AVAILABLE, &value);
        if (value == GL_TRUE) {
            glGetQueryObjecti64v(_id, GL_QUERY_RESULT, &value);
        }
    }
    return value;
}

PXR_NAMESPACE_CLOSE_SCOPE

