//
// Copyright 2020 Pixar
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
#include <GL/glew.h>

#include "pxr/pxr.h"
#include "pxr/imaging/hgiGL/hgi.h"
#include "pxr/imaging/hgiGL/texture.h"
#include "pxr/imaging/hgiInterop/opengl.h"


PXR_NAMESPACE_OPEN_SCOPE

static const char* _vertexFullscreen =
    "#version 120\n"
    "attribute vec4 position;\n"
    "attribute vec2 uvIn;\n"
    "varying vec2 uv;\n"
    "void main(void)\n"
    "{\n"
    "    gl_Position = position;\n"
    "    uv = uvIn;\n"
    "}\n";

static const char* _fragmentNoDepthFullscreen =
    "#version 120\n"
    "varying vec2 uv;\n"
    "uniform sampler2D colorIn;\n"
    "void main(void)\n"
    "{\n"
    "    gl_FragColor = texture2D(colorIn, uv);\n"
    "}\n";

static const char* _fragmentDepthFullscreen =
    "#version 120\n"
    "varying vec2 uv;\n"
    "uniform sampler2D colorIn;\n"
    "uniform sampler2D depthIn;\n"
    "void main(void)\n"
    "{\n"
    "    float depth = texture2D(depthIn, uv).r;\n"
    "    gl_FragColor = texture2D(colorIn, uv);\n"
    "    gl_FragDepth = depth;\n"
    "}\n";

static uint32_t
_CompileShader(const char* src, GLenum stage)
{
    uint32_t shaderId = glCreateShader(stage);
    glShaderSource(shaderId, 1, &src, nullptr);
    glCompileShader(shaderId);
    GLint status;
    glGetShaderiv(shaderId, GL_COMPILE_STATUS, &status);
    TF_VERIFY(status == GL_TRUE);
    return shaderId;
}

static uint32_t
_LinkProgram(uint32_t vs, uint32_t fs)
{
    uint32_t programId = glCreateProgram();
    glAttachShader(programId, vs);
    glAttachShader(programId, fs);
    glLinkProgram(programId);
    GLint status;
    glGetProgramiv(programId, GL_LINK_STATUS, &status);
    TF_VERIFY(status == GL_TRUE);
    return programId;
}

static uint32_t
_CreateVertexBuffer()
{
    static const float vertices[] = { 
        /* position        uv */
        -1,  3, -1, 1,    0, 2,
        -1, -1, -1, 1,    0, 0,
         3, -1, -1, 1,    2, 0 };
    uint32_t vertexBuffer = 0;
    glGenBuffers(1, &vertexBuffer);
    glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer);
    glBufferData(GL_ARRAY_BUFFER,sizeof(vertices),&vertices[0], GL_STATIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    return vertexBuffer;
}

HgiInteropOpenGL::HgiInteropOpenGL()
    : _vs(0)
    , _fsNoDepth(0)
    , _fsDepth(0)
    , _prgNoDepth(0)
    , _prgDepth(0)
    , _vertexBuffer(0)
{
    _vs = _CompileShader(_vertexFullscreen, GL_VERTEX_SHADER);
    _fsNoDepth = _CompileShader(_fragmentNoDepthFullscreen, GL_FRAGMENT_SHADER);
    _fsDepth = _CompileShader(_fragmentDepthFullscreen, GL_FRAGMENT_SHADER);
    _prgNoDepth = _LinkProgram(_vs, _fsNoDepth);
    _prgDepth = _LinkProgram(_vs, _fsDepth);
    _vertexBuffer = _CreateVertexBuffer();
    TF_VERIFY(glGetError() == GL_NO_ERROR);
}

HgiInteropOpenGL::~HgiInteropOpenGL()
{
    glDeleteShader(_vs);
    glDeleteShader(_fsNoDepth);
    glDeleteShader(_fsDepth);
    glDeleteProgram(_prgNoDepth);
    glDeleteProgram(_prgDepth);
    glDeleteBuffers(1, &_vertexBuffer);
    TF_VERIFY(glGetError() == GL_NO_ERROR);
}

void
HgiInteropOpenGL::CopyToInterop(
    HgiTextureHandle const &color,
    HgiTextureHandle const &depth)
{
    if (!ARCH_UNLIKELY(color)) {
        TF_WARN("No valid color texture provided");
        return;
    }

#if defined(GL_KHR_debug)
    if (GLEW_KHR_debug) {
        glPushDebugGroup(GL_DEBUG_SOURCE_THIRD_PARTY, 0, -1, "Interop");
    }
#endif

    // Verify there were no gl errors coming in.
    TF_VERIFY(glGetError() == GL_NO_ERROR);

    // Setup shader program
    uint32_t prg = color && depth ? _prgDepth : _prgNoDepth;
    glUseProgram(prg);

    // Bind textures
    HgiGLTexture *glColor = static_cast<HgiGLTexture*>(color.Get());
    HgiGLTexture *glDepth = static_cast<HgiGLTexture*>(depth.Get());

    if (glColor) {
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, glColor->GetTextureId());
        GLint loc = glGetUniformLocation(prg, "colorIn");
        glUniform1i(loc, 0);
    }

    if (glDepth) {
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, glDepth->GetTextureId());
        GLint loc = glGetUniformLocation(prg, "depthIn");
        glUniform1i(loc, 1);
    }

    // Vertex attributes
    GLint locPosition = glGetAttribLocation(prg, "position");
    glBindBuffer(GL_ARRAY_BUFFER, _vertexBuffer);
    glVertexAttribPointer(locPosition, 4, GL_FLOAT, GL_FALSE,
            sizeof(float)*6, 0);
    glEnableVertexAttribArray(locPosition);

    GLint locUv = glGetAttribLocation(prg, "uvIn");
    glVertexAttribPointer(locUv, 2, GL_FLOAT, GL_FALSE,
            sizeof(float)*6, reinterpret_cast<void*>(sizeof(float)*4));
    glEnableVertexAttribArray(locUv);

    // Depth test must be ALWAYS instead of disabling the depth_test because
    // we want to transfer the depth pixels. Disabling depth_test 
    // disables depth writes and we need to copy depth to screen FB.
    GLboolean restoreDepthEnabled = glIsEnabled(GL_DEPTH_TEST);
    glEnable(GL_DEPTH_TEST);
    GLint restoreDepthFunc;
    glGetIntegerv(GL_DEPTH_FUNC, &restoreDepthFunc);
    glDepthFunc(GL_ALWAYS);

    // Any alpha blending the client wanted should have happened into the AOV. 
    // When copying back to client buffer disable blending.
    GLboolean blendEnabled;
    glGetBooleanv(GL_BLEND, &blendEnabled);
    glDisable(GL_BLEND);

    // Disable alpha to coverage (we want to transfer all pixels as-is)
    GLboolean restoreAlphaToCoverage;
    glGetBooleanv(GL_SAMPLE_ALPHA_TO_COVERAGE, &restoreAlphaToCoverage);
    glDisable(GL_SAMPLE_ALPHA_TO_COVERAGE);

    // The application may have set a custom glViewport (e.g. camera mask), we 
    // instead want to blit the entire aov texture to the screen, because the 
    // aov already contains the masked result.
    int32_t restoreVp[4];
    glGetIntegerv(GL_VIEWPORT, restoreVp);
    const GfVec3i dimensions = glColor ? 
        glColor->GetDescriptor().dimensions :
        glDepth->GetDescriptor().dimensions;
    glViewport(0,0, dimensions[0], dimensions[1]);

    // Draw fullscreen triangle
    glDrawArrays(GL_TRIANGLES, 0, 3);

    // Restore state and verify gl errors
    if (blendEnabled) {
        glEnable(GL_BLEND);
    }
    glDepthFunc(restoreDepthFunc);
    if (!restoreDepthEnabled) {
        glDisable(GL_DEPTH_TEST);
    }
    if (restoreAlphaToCoverage) {
        glEnable(GL_SAMPLE_ALPHA_TO_COVERAGE);
    }
    glViewport(restoreVp[0], restoreVp[1], restoreVp[2], restoreVp[3]);

    glUseProgram(0);

#if defined(GL_KHR_debug)
    if (GLEW_KHR_debug) {
        glPopDebugGroup();
    }
#endif

    TF_VERIFY(glGetError() == GL_NO_ERROR);
}

PXR_NAMESPACE_CLOSE_SCOPE
