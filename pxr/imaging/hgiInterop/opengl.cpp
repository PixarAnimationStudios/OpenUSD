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
#include "pxr/imaging/garch/glApi.h"

#include "pxr/pxr.h"
#include "pxr/imaging/hgi/hgi.h"
#include "pxr/imaging/hgi/texture.h"
#include "pxr/imaging/hgiInterop/opengl.h"
#include "pxr/base/vt/value.h"

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
HgiInteropOpenGL::CompositeToInterop(
    HgiTextureHandle const &color,
    HgiTextureHandle const &depth,
    VtValue const &framebuffer,
    GfVec4i const &compRegion)
{
    if (!ARCH_UNLIKELY(color)) {
        TF_WARN("No valid color texture provided");
        return;
    }

    // Verify there were no gl errors coming in.
    TF_VERIFY(glGetError() == GL_NO_ERROR);

    if (!ARCH_UNLIKELY(color)) {
        TF_CODING_ERROR("A valid color texture handle is required.\n");
        return;
    }

    const GLuint colorName = static_cast<GLuint>(color->GetRawResource());
    if (!glIsTexture(colorName)) {
        TF_CODING_ERROR(
            "Hgi color texture handle is not holding a valid OpenGL "
            "texture.");
        return;
    }

#if defined(GL_KHR_debug)
    if (GARCH_GLAPI_HAS(KHR_debug)) {
        glPushDebugGroup(GL_DEBUG_SOURCE_THIRD_PARTY, 0, -1, "Interop");
    }
#endif
    
    GLint restoreDrawFramebuffer = 0;
    bool doRestoreDrawFramebuffer = false;

    if (!framebuffer.IsEmpty()) {
        if (framebuffer.IsHolding<uint32_t>()) {
            glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING,
                          &restoreDrawFramebuffer);
            doRestoreDrawFramebuffer = true;
            glBindFramebuffer(GL_DRAW_FRAMEBUFFER,
                              framebuffer.UncheckedGet<uint32_t>());
        } else {
            TF_CODING_ERROR(
                "dstFramebuffer must hold uint32_t when targeting OpenGL");
        }
    }

    GLint restoreActiveTexture = 0;
    glGetIntegerv(GL_ACTIVE_TEXTURE, &restoreActiveTexture);

    // Setup shader program
    const uint32_t prg = color && depth ? _prgDepth : _prgNoDepth;
    glUseProgram(prg);

    {
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, colorName);
        const GLint loc = glGetUniformLocation(prg, "colorIn");
        glUniform1i(loc, 0);
    }

    // Depth is optional
    if (depth) {
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D,
                      static_cast<GLuint>(depth->GetRawResource()));
        const GLint loc = glGetUniformLocation(prg, "depthIn");
        glUniform1i(loc, 1);
    }

    // Get the current array buffer binding state
    GLint restoreArrayBuffer = 0;
    glGetIntegerv(GL_ARRAY_BUFFER_BINDING, &restoreArrayBuffer);

    // Vertex attributes
    const GLint locPosition = glGetAttribLocation(prg, "position");
    glBindBuffer(GL_ARRAY_BUFFER, _vertexBuffer);
    glVertexAttribPointer(locPosition, 4, GL_FLOAT, GL_FALSE,
            sizeof(float)*6, 0);
    glEnableVertexAttribArray(locPosition);

    const GLint locUv = glGetAttribLocation(prg, "uvIn");
    glVertexAttribPointer(locUv, 2, GL_FLOAT, GL_FALSE,
            sizeof(float)*6, reinterpret_cast<void*>(sizeof(float)*4));
    glEnableVertexAttribArray(locUv);

    // Since we want to composite over the application's framebuffer contents,
    // we need to honor depth testing if we have a valid depth texture.
    const GLboolean restoreDepthEnabled = glIsEnabled(GL_DEPTH_TEST);
    GLboolean restoreDepthMask;
    glGetBooleanv(GL_DEPTH_WRITEMASK, &restoreDepthMask);
    GLint restoreDepthFunc;
    glGetIntegerv(GL_DEPTH_FUNC, &restoreDepthFunc);
    if (depth) {
        glEnable(GL_DEPTH_TEST);
        glDepthMask(GL_TRUE);
        // Note: Use LEQUAL and not LESS to ensure that fragments with only
        // translucent contribution (that don't update depth) are composited.
        glDepthFunc(GL_LEQUAL);
    } else {
        glDisable(GL_DEPTH_TEST);
        glDepthMask(GL_FALSE);
    }

    // Enable blending to composite correctly over framebuffer contents.
    // Use pre-multiplied alpha scaling factors.
    GLboolean blendEnabled;
    glGetBooleanv(GL_BLEND, &blendEnabled);
    glEnable(GL_BLEND);
    GLint restoreColorSrcFnOp, restoreAlphaSrcFnOp;
    GLint restoreColorDstFnOp, restoreAlphaDstFnOp;
    glGetIntegerv(GL_BLEND_SRC_RGB, &restoreColorSrcFnOp);
    glGetIntegerv(GL_BLEND_SRC_ALPHA, &restoreAlphaSrcFnOp);
    glGetIntegerv(GL_BLEND_DST_RGB, &restoreColorDstFnOp);
    glGetIntegerv(GL_BLEND_DST_ALPHA, &restoreAlphaDstFnOp);
    glBlendFuncSeparate(/*srcColor*/GL_ONE,
                        /*dstColor*/GL_ONE_MINUS_SRC_ALPHA,
                        /*srcAlpha*/GL_ONE,
                        /*dstAlpha*/GL_ONE);
    GLint restoreColorOp, restoreAlphaOp;
    glGetIntegerv(GL_BLEND_EQUATION_RGB, &restoreColorOp);
    glGetIntegerv(GL_BLEND_EQUATION_ALPHA, &restoreAlphaOp);
    glBlendEquationSeparate(GL_FUNC_ADD, GL_FUNC_ADD);

    // Disable alpha to coverage (we want to composite the pixels as-is)
    GLboolean restoreAlphaToCoverage;
    glGetBooleanv(GL_SAMPLE_ALPHA_TO_COVERAGE, &restoreAlphaToCoverage);
    glDisable(GL_SAMPLE_ALPHA_TO_COVERAGE);

    int32_t restoreVp[4];
    glGetIntegerv(GL_VIEWPORT, restoreVp);
    glViewport(compRegion[0], compRegion[1], compRegion[2], compRegion[3]);

    // Draw fullscreen triangle
    glDrawArrays(GL_TRIANGLES, 0, 3);

    // Restore state and verify gl errors
    glDisableVertexAttribArray(locPosition);
    glDisableVertexAttribArray(locUv);
    glBindBuffer(GL_ARRAY_BUFFER, restoreArrayBuffer);
    
    if (!blendEnabled) {
        glDisable(GL_BLEND);
    }
    glBlendFuncSeparate(restoreColorSrcFnOp, restoreColorDstFnOp, 
                        restoreAlphaSrcFnOp, restoreAlphaDstFnOp);
    glBlendEquationSeparate(restoreColorOp, restoreAlphaOp);

    if (!restoreDepthEnabled) {
        glDisable(GL_DEPTH_TEST);
    } else {
        glEnable(GL_DEPTH_TEST);
    }
    glDepthMask(restoreDepthMask);
    glDepthFunc(restoreDepthFunc);
    
    if (restoreAlphaToCoverage) {
        glEnable(GL_SAMPLE_ALPHA_TO_COVERAGE);
    }
    glViewport(restoreVp[0], restoreVp[1], restoreVp[2], restoreVp[3]);

    glUseProgram(0);

    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, 0);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, 0);

#if defined(GL_KHR_debug)
    if (GARCH_GLAPI_HAS(KHR_debug)) {
        glPopDebugGroup();
    }
#endif

    glActiveTexture(restoreActiveTexture);

    if (doRestoreDrawFramebuffer) {
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER,
                          restoreDrawFramebuffer);
    }

    TF_VERIFY(glGetError() == GL_NO_ERROR);
}

PXR_NAMESPACE_CLOSE_SCOPE
