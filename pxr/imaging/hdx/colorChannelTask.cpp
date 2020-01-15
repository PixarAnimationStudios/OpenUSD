//
// Copyright 2019 Pixar
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
#include "pxr/imaging/glf/glew.h"

#include "pxr/imaging/hdx/colorChannelTask.h"
#include "pxr/imaging/hdx/package.h"
#include "pxr/imaging/hd/perfLog.h"
#include "pxr/imaging/hd/renderBuffer.h"
#include "pxr/imaging/hd/tokens.h"
#include "pxr/imaging/hdSt/glslProgram.h"
#include "pxr/imaging/hdSt/renderBuffer.h"
#include "pxr/imaging/hf/perfLog.h"
#include "pxr/imaging/glf/diagnostic.h"
#include "pxr/imaging/hio/glslfx.h"
#include "pxr/imaging/glf/glContext.h"
#include "pxr/base/tf/setenv.h"
#include "pxr/imaging/hgiGL/texture.h"

PXR_NAMESPACE_OPEN_SCOPE

TF_DEFINE_PRIVATE_TOKENS(
    _tokens,
    ((colorChannelVertex,    "ColorChannelVertex"))
    ((colorChannelFragment,  "ColorChannelFragment"))
    (colorChannelShader)
);

namespace {
    enum {
        COLOR_IN = 0,
        POSITION = 1,
        UV_IN = 2,
        CHANNEL = 3
    };
}

HdxColorChannelTask::HdxColorChannelTask(HdSceneDelegate* delegate, 
                                               SdfPath const& id)
    : HdTask(id)
    , _shaderProgram()
    , _texture(0)
    , _textureSize(0)
    , _vertexBuffer(0)
    , _copyFramebuffer(0)
    , _framebufferSize(0)
    , _channel(HdxColorChannelTokens->color)
{
}

HdxColorChannelTask::~HdxColorChannelTask()
{
    if (_texture != 0) {
        glDeleteTextures(1, &_texture);
    }

    if (_vertexBuffer != 0) {
        glDeleteBuffers(1, &_vertexBuffer);
    }

    if (_shaderProgram) {
        _shaderProgram.reset();
    }

    if (_copyFramebuffer != 0) {
        glDeleteFramebuffers(1, &_copyFramebuffer);
    }

    GLF_POST_PENDING_GL_ERRORS();
}

bool
HdxColorChannelTask::_CreateShaderResources()
{
    if (_shaderProgram) {
        return true;
    }

    _shaderProgram.reset(new HdStGLSLProgram(_tokens->colorChannelShader));

    HioGlslfx glslfx(HdxPackageColorChannelShader());

    std::string fragCode = glslfx.GetSource(_tokens->colorChannelFragment);

    if (!_shaderProgram->CompileShader(GL_VERTEX_SHADER,
            glslfx.GetSource(_tokens->colorChannelVertex)) ||
        !_shaderProgram->CompileShader(GL_FRAGMENT_SHADER, fragCode) ||
        !_shaderProgram->Link()) {
        TF_CODING_ERROR("Failed to load color channel shader");
        _shaderProgram.reset();
        return false;
    }

    GLuint programId = _shaderProgram->GetProgram().GetId();
    _locations[COLOR_IN] = glGetUniformLocation(programId, "colorIn");
    _locations[POSITION] = glGetAttribLocation(programId, "position");
    _locations[UV_IN]    = glGetAttribLocation(programId, "uvIn");
    _locations[CHANNEL]  = glGetUniformLocation(programId, "channel");
    
    GLF_POST_PENDING_GL_ERRORS();
    return true;
}

bool
HdxColorChannelTask::_CreateBufferResources()
{
    if (_vertexBuffer) {
        return true;
    }

    // A larger-than screen triangle with UVs made to fit the screen.
    //                                 positions          |   uvs
    static const float vertices[] = { -1,  3, -1, 1,        0, 2,
                                      -1, -1, -1, 1,        0, 0,
                                       3, -1, -1, 1,        2, 0 };

    glGenBuffers(1, &_vertexBuffer);
    glBindBuffer(GL_ARRAY_BUFFER, _vertexBuffer);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices),
                 &vertices[0], GL_STATIC_DRAW);

    glBindBuffer(GL_ARRAY_BUFFER, 0);

    GLF_POST_PENDING_GL_ERRORS();
    return true;
}

void
HdxColorChannelTask::_CopyTexture()
{
    GLint restoreReadFB, restoreDrawFB;
    glGetIntegerv(GL_READ_FRAMEBUFFER_BINDING, &restoreReadFB);
    glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &restoreDrawFB);

    // Make a copy of the default FB color attachment so we can read from the
    // copy and write back into it.
    glBindFramebuffer(GL_READ_FRAMEBUFFER, restoreDrawFB);
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, _copyFramebuffer);

    int width = _textureSize[0];
    int height = _textureSize[1];

    glBlitFramebuffer(0, 0, width, height,
                      0, 0, width, height,
                      GL_COLOR_BUFFER_BIT, GL_NEAREST);

    glBindFramebuffer(GL_READ_FRAMEBUFFER, restoreReadFB);
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, restoreDrawFB);

    GLF_POST_PENDING_GL_ERRORS();
}

bool
HdxColorChannelTask::_CreateFramebufferResources()
{
    // If framebufferSize is not provided we use the viewport size.
    // This can be incorrect if the client/app has changed the viewport to
    // be different then the render window size. (E.g. UsdView CameraMask mode)
    GfVec2i fboSize = _framebufferSize;
    if (fboSize[0] <= 0 || fboSize[1] <= 0) {
        GLint res[4] = {0};
        glGetIntegerv(GL_VIEWPORT, res);
        fboSize = GfVec2i(res[2], res[3]);
        _framebufferSize = fboSize;
    }

    bool createTexture = (_texture == 0 || fboSize != _textureSize);

    if (createTexture) {
        if (_texture != 0) {
            glDeleteTextures(1, &_texture);
            _texture = 0;
        }

        _textureSize = fboSize;

        GLint restoreTexture;
        glGetIntegerv(GL_TEXTURE_BINDING_2D, &restoreTexture);

        glGenTextures(1, &_texture);
        glBindTexture(GL_TEXTURE_2D, _texture);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

        // XXX For now we assume we always want R16F. We could perhaps expose
        //     this as client-API in HdxColorChannelTaskParams.
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, _textureSize[0], 
                     _textureSize[1], 0, GL_RGBA, GL_FLOAT, 0);

        glBindTexture(GL_TEXTURE_2D, restoreTexture);
    }

    // XXX: Removed due to slowness in the IsCurrent() call when multiple
    //      gl contexts are registered in GlfGLContextRegistry. This code is
    //      relevant only when there is a possibility of having context
    //      switching between the creation of the render pass and the execution
    //      of this task on each frame.
    // bool switchedGLContext = !_owningContext || !_owningContext->IsCurrent();
    // 
    // if (switchedGLContext) {
    //     // If we're rendering with a different context than the render pass
    //     // was created with, recreate the FBO because FB is not shared.
    //     // XXX we need this since we use a FBO in _CopyTexture(). Ideally we
    //     // use HdxFullscreenShader to do the copy, but for that we need to know
    //     // the textureId currently bound to the default framebuffer. However
    //     // glGetFramebufferAttachmentParameteriv will return and error when
    //     // trying to query the texture name bound to GL_BACK_LEFT.
    //     if (_owningContext && _owningContext->IsValid()) {
    //         GlfGLContextScopeHolder contextHolder(_owningContext);
    //         glDeleteFramebuffers(1, &_copyFramebuffer);
    //     }
    // 
    //     _owningContext = GlfGLContext::GetCurrentGLContext();
    //     if (!TF_VERIFY(_owningContext, "No valid GL context")) {
    //         return false;
    //     }
    // 

        if (_copyFramebuffer == 0) {
            glGenFramebuffers(1, &_copyFramebuffer);
        }

    // }
    // 
    // if (createTexture || switchedGLContext) {

    if (createTexture) {
        GLint restoreReadFB, restoreDrawFB;
        glGetIntegerv(GL_READ_FRAMEBUFFER_BINDING, &restoreReadFB);
        glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &restoreDrawFB);
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, _copyFramebuffer);

        glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, 
                               GL_TEXTURE_2D, _texture, 0);

        glBindFramebuffer(GL_READ_FRAMEBUFFER, restoreReadFB);
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, restoreDrawFB);
    }

    GLF_POST_PENDING_GL_ERRORS();
    return true;
}

void
HdxColorChannelTask::_ApplyColorChannel()
{
    // A note here: colorChannel is used for all of our plugins and has to be
    // robust to poor GL support.  OSX compatibility profile provides a
    // GL 2.1 API, slightly restricting our choice of API and heavily
    // restricting our shader syntax. See also HdxFullscreenShader.

    // Read from the texture-copy we made of the clients FBO and output the
    // color-corrected pixels into the clients FBO.

    GLuint programId = _shaderProgram->GetProgram().GetId();
    glUseProgram(programId);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, _texture);
    glUniform1i(_locations[COLOR_IN], 0);

    glUniform1i(_locations[CHANNEL], _GetChannelAsGLint());

    glBindBuffer(GL_ARRAY_BUFFER, _vertexBuffer);
    glVertexAttribPointer(_locations[POSITION], 4, GL_FLOAT, GL_FALSE,
                          sizeof(float)*6, 0);
    glEnableVertexAttribArray(_locations[POSITION]);
    glVertexAttribPointer(_locations[UV_IN], 2, GL_FLOAT, GL_FALSE,
            sizeof(float)*6, reinterpret_cast<void*>(sizeof(float)*4));
    glEnableVertexAttribArray(_locations[UV_IN]);

    // We are rendering a full-screen triangle, which would render to depth.
    // Instead we want to preserve the original depth, so disable depth writes.
    GLboolean restoreDepthWriteMask;
    GLboolean restoreStencilWriteMask;
    glGetBooleanv(GL_DEPTH_WRITEMASK, &restoreDepthWriteMask);
    glGetBooleanv(GL_STENCIL_WRITEMASK, &restoreStencilWriteMask);
    glDepthMask(GL_FALSE);
    glStencilMask(GL_FALSE);

    // Depth test must be ALWAYS instead of disabling the depth_test because
    // we still want to write to the depth buffer. Disabling depth_test disables
    // depth_buffer writes.
    GLint restoreDepthFunc;
    glGetIntegerv(GL_DEPTH_FUNC, &restoreDepthFunc);
    glDepthFunc(GL_ALWAYS);

    GLint restoreViewport[4] = {0};
    glGetIntegerv(GL_VIEWPORT, restoreViewport);
    glViewport(0, 0, _framebufferSize[0], _framebufferSize[1]);

    // The app may have alpha blending enabled.
    // We want to pass-through the alpha values, not alpha-blend on top of dest.
    GLboolean restoreblendEnabled;
    glGetBooleanv(GL_BLEND, &restoreblendEnabled);
    glDisable(GL_BLEND);

    // Alpha to coverage would prevent any pixels that have an alpha of 0.0 from
    // being written. We want to color correct all pixels. Even background
    // pixels that were set with a clearColor alpha of 0.0
    GLboolean restoreAlphaToCoverage;
    glGetBooleanv(GL_SAMPLE_ALPHA_TO_COVERAGE, &restoreAlphaToCoverage);
    glDisable(GL_SAMPLE_ALPHA_TO_COVERAGE);

    glDrawArrays(GL_TRIANGLES, 0, 3);

    if (restoreAlphaToCoverage) {
        glEnable(GL_SAMPLE_ALPHA_TO_COVERAGE);
    }

    if (restoreblendEnabled) {
        glEnable(GL_BLEND);
    }

    glViewport(restoreViewport[0], restoreViewport[1],
               restoreViewport[2], restoreViewport[3]);

    glDepthFunc(restoreDepthFunc);
    glDepthMask(restoreDepthWriteMask);
    glStencilMask(restoreStencilWriteMask);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glDisableVertexAttribArray(_locations[POSITION]);
    glDisableVertexAttribArray(_locations[UV_IN]);

    glUseProgram(0);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, 0);

    GLF_POST_PENDING_GL_ERRORS();
}

GLint
HdxColorChannelTask::_GetChannelAsGLint()
{
    // Return the index of the current channel in HdxColorChannelTokens
    int i = 0;
    for(const TfToken& channelToken : HdxColorChannelTokens->allTokens) {
        if (channelToken == _channel) return i;
        ++i;
    }

    // Default to 'color' (index 0)
    return 0;
}

void
HdxColorChannelTask::Sync(HdSceneDelegate* delegate,
                             HdTaskContext* ctx,
                             HdDirtyBits* dirtyBits)
{
    HD_TRACE_FUNCTION();
    HF_MALLOC_TAG_FUNCTION();

    if ((*dirtyBits) & HdChangeTracker::DirtyParams) {
        HdxColorChannelTaskParams params;

        if (_GetTaskParams(delegate, &params)) {
            _framebufferSize = params.framebufferSize;
            _channel = params.channel;
        }
    }

    *dirtyBits = HdChangeTracker::Clean;
}

void
HdxColorChannelTask::Prepare(HdTaskContext* ctx,
                                HdRenderIndex* renderIndex)
{
}

void
HdxColorChannelTask::Execute(HdTaskContext* ctx)
{
    HD_TRACE_FUNCTION();
    HF_MALLOC_TAG_FUNCTION();
    GLF_GROUP_FUNCTION();

    if (!_CreateBufferResources()) {
        return;
    }

    if (!_CreateShaderResources()) {
        return;
    }

    // Disable the sRGB transformation if it is enabled to avoid a double sRGB
    // color transformation
    GLboolean srgbEnabled = 0;
    glGetBooleanv(GL_FRAMEBUFFER_SRGB_EXT, &srgbEnabled);
    if (srgbEnabled) {
        glDisable(GL_FRAMEBUFFER_SRGB_EXT);
    }

    _CreateFramebufferResources();
    _CopyTexture();
    _ApplyColorChannel();

    // Restore the sRGB transformation if it was enabled
    if (srgbEnabled) {
        glEnable(GL_FRAMEBUFFER_SRGB_EXT);
    }
}


// -------------------------------------------------------------------------- //
// VtValue Requirements
// -------------------------------------------------------------------------- //

std::ostream& operator<<(
    std::ostream& out, 
    const HdxColorChannelTaskParams& pv)
{
    out << "ColorChannelTask Params: (...) "
        << pv.framebufferSize << " "
        << pv.channel << " "
    ;
    return out;
}

bool operator==(const HdxColorChannelTaskParams& lhs,
                const HdxColorChannelTaskParams& rhs)
{
    return lhs.framebufferSize == rhs. framebufferSize &&
           lhs.channel == rhs.channel;
}

bool operator!=(const HdxColorChannelTaskParams& lhs,
                const HdxColorChannelTaskParams& rhs)
{
    return !(lhs == rhs);
}

PXR_NAMESPACE_CLOSE_SCOPE
