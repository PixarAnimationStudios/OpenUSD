//
// Copyright 2018 Pixar
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

#include "pxr/imaging/hdx/compositor.h"
#include "pxr/imaging/hdx/package.h"
#include "pxr/imaging/hdSt/glslProgram.h"
#include "pxr/imaging/hf/perfLog.h"
#include "pxr/imaging/hd/perfLog.h"
#include "pxr/imaging/glf/diagnostic.h"
#include "pxr/imaging/glf/glslfx.h"
#include "pxr/base/tf/staticTokens.h"

PXR_NAMESPACE_OPEN_SCOPE

TF_DEFINE_PRIVATE_TOKENS(
    _tokens,
    ((fullscreenVertex,              "FullscreenVertex"))
    ((compositeFragmentNoDepth,      "CompositeFragmentNoDepth"))
    ((compositeFragmentWithDepth,    "CompositeFragmentWithDepth"))
    (fullscreenShader)
);

HdxCompositor::HdxCompositor()
    : _colorTexture(0), _colorSize(0)
    , _depthTexture(0), _depthSize(0)
    , _compositorProgram()
    , _useDepthProgram(false)
{
}

HdxCompositor::~HdxCompositor()
{
    if (_colorTexture != 0) {
        glDeleteTextures(1, &_colorTexture);
    }
    if (_depthTexture != 0) {
        glDeleteTextures(1, &_depthTexture);
    }
    GLF_POST_PENDING_GL_ERRORS();

    if (_compositorProgram) {
        _compositorProgram.reset();
    }
}

void
HdxCompositor::_CreateShaderResources(bool useDepthProgram)
{

    _compositorProgram.reset(new HdStGLSLProgram(_tokens->fullscreenShader));
    GlfGLSLFX glslfx(HdxPackageFullscreenShader());
    std::string version = "#version 430\n";
    TfToken fsToken = useDepthProgram ? _tokens->compositeFragmentWithDepth
                                      : _tokens->compositeFragmentNoDepth;
    if (!_compositorProgram->CompileShader(GL_VERTEX_SHADER,
            version + glslfx.GetSource(_tokens->fullscreenVertex)) ||
        !_compositorProgram->CompileShader(GL_FRAGMENT_SHADER,
            version + glslfx.GetSource(fsToken)) ||
        !_compositorProgram->Link()) {
        TF_CODING_ERROR("Failed to load compositing shader");
        _compositorProgram.reset();
        return;
    }
}

void
HdxCompositor::_CreateTextureResources(GLuint *texture)
{
    glGenTextures(1, texture);
    glBindTexture(GL_TEXTURE_2D, *texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
}

void
HdxCompositor::UpdateColor(int width, int height, uint8_t *data)
{
    HD_TRACE_FUNCTION();
    HF_MALLOC_TAG_FUNCTION();

    if (width == 0 && height == 0) {
        if (_colorTexture != 0) {
            glDeleteTextures(1, &_colorTexture);
            _colorTexture = 0;
        }
        _colorSize = GfVec2i(0,0);
        return;
    }

    if (_colorTexture == 0) {
        _CreateTextureResources(&_colorTexture);
    }
    glBindTexture(GL_TEXTURE_2D, _colorTexture);

    GfVec2i size(width, height);
    if (size != _colorSize) {
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA,
                     GL_UNSIGNED_BYTE, data);
    } else {
        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, width, height, GL_RGBA,
                        GL_UNSIGNED_BYTE, data);
    }
    _colorSize = size;
    glBindTexture(GL_TEXTURE_2D, 0);

    GLF_POST_PENDING_GL_ERRORS();
}

void
HdxCompositor::UpdateDepth(int width, int height, uint8_t *data)
{
    HD_TRACE_FUNCTION();
    HF_MALLOC_TAG_FUNCTION();

    if (width == 0 && height == 0) {
        if (_depthTexture != 0) {
            glDeleteTextures(1, &_depthTexture);
            _depthTexture = 0;
        }
        _depthSize = GfVec2i(0,0);
        return;
    }

    if (_depthTexture == 0) {
        _CreateTextureResources(&_depthTexture);
    }
    glBindTexture(GL_TEXTURE_2D, _depthTexture);

    GfVec2i size(width, height);
    if (size != _depthSize) {
        glTexImage2D(GL_TEXTURE_2D, 0, GL_R32F, width, height, 0, GL_RED,
                     GL_FLOAT, data);
    } else {
        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, width, height, GL_RED,
                        GL_FLOAT, data);
    }
    _depthSize = size;

    glBindTexture(GL_TEXTURE_2D, 0);

    GLF_POST_PENDING_GL_ERRORS();
}

void
HdxCompositor::Draw()
{
    // No-op if no color data was specified.
    if (_colorTexture == 0) {
        return;
    }

    bool useDepthProgram = (_depthTexture != 0);

    if (!_compositorProgram || _useDepthProgram != useDepthProgram) {
        _CreateShaderResources(useDepthProgram);
        _useDepthProgram = useDepthProgram;
    }

    // No-op if the shader failed to compile.
    if (!_compositorProgram) {
        return;
    }

    GLuint programId = _compositorProgram->GetProgram().GetId();

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, _colorTexture);
    glProgramUniform1i(programId, 0, 0);

    if (_depthTexture != 0) {
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, _depthTexture);
        glProgramUniform1i(programId, 1, 1);
    }

    glUseProgram(programId);
    glDrawArrays(GL_TRIANGLES, 0, 3);
    glUseProgram(0);

    if (_depthTexture != 0) {
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, 0);
    }

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, 0);

    GLF_POST_PENDING_GL_ERRORS();
}

PXR_NAMESPACE_CLOSE_SCOPE
