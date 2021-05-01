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

#include "pxr/imaging/hgiGL/scopedStateHolder.h"
#include "pxr/imaging/hgiGL/conversions.h"
#include "pxr/imaging/hgiGL/diagnostic.h"

#include "pxr/base/trace/trace.h"
#include "pxr/base/tf/diagnostic.h"
#include "pxr/base/tf/iterator.h"

PXR_NAMESPACE_OPEN_SCOPE

HgiGL_ScopedStateHolder::HgiGL_ScopedStateHolder()
    : _restoreDrawFramebuffer(0)
    , _restoreReadFramebuffer(0)
    , _restoreRenderBuffer(0)
    , _restoreVao(0)
    , _restoreDepthTest(false)
    , _restoreDepthWriteMask(false)
    , _restoreStencilWriteMask(0)
    , _restoreDepthFunc(0)
    , _restoreViewport{0,0,0,0}
    , _restoreblendEnabled(false)
    , _restoreColorOp(0)
    , _restoreAlphaOp(0)
    , _restoreAlphaToCoverage(false)
    , _restoreSampleAlphaToOne(false)
    , _lineWidth(1.0f)
    , _cullFace(true)
    , _cullMode(GL_BACK)
    , _frontFace(GL_CCW)
    , _rasterizerDiscard(true)
    , _restoreFramebufferSRGB(false)
{
    TRACE_FUNCTION();

    #if defined(GL_KHR_debug)
    if (GARCH_GLAPI_HAS(KHR_debug)) {
        glPushDebugGroup(GL_DEBUG_SOURCE_THIRD_PARTY, 0, -1, "Capture state");
    }
    #endif

    glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &_restoreDrawFramebuffer);
    glGetIntegerv(GL_READ_FRAMEBUFFER_BINDING, &_restoreReadFramebuffer);
    glGetIntegerv(GL_RENDERBUFFER_BINDING, &_restoreRenderBuffer);
    glGetIntegerv(GL_VERTEX_ARRAY_BINDING, &_restoreVao);
    glGetBooleanv(GL_DEPTH_TEST, (GLboolean*)&_restoreDepthTest);
    glGetBooleanv(GL_DEPTH_WRITEMASK, (GLboolean*)&_restoreDepthWriteMask);
    glGetIntegerv(GL_STENCIL_WRITEMASK, &_restoreStencilWriteMask);
    glGetIntegerv(GL_DEPTH_FUNC, &_restoreDepthFunc);
    glGetIntegerv(GL_VIEWPORT, _restoreViewport);
    glGetBooleanv(GL_BLEND, (GLboolean*)&_restoreblendEnabled);
    glGetIntegerv(GL_BLEND_EQUATION_RGB, &_restoreColorOp);
    glGetIntegerv(GL_BLEND_EQUATION_ALPHA, &_restoreAlphaOp);
    glGetIntegerv(GL_BLEND_SRC_RGB, &_restoreColorSrcFnOp);
    glGetIntegerv(GL_BLEND_SRC_ALPHA, &_restoreAlphaSrcFnOp);
    glGetIntegerv(GL_BLEND_DST_RGB, &_restoreColorDstFnOp);
    glGetIntegerv(GL_BLEND_DST_ALPHA, &_restoreAlphaDstFnOp);
    glGetBooleanv(
        GL_SAMPLE_ALPHA_TO_COVERAGE, 
        (GLboolean*)&_restoreAlphaToCoverage);
    glGetBooleanv(
        GL_SAMPLE_ALPHA_TO_ONE,
        (GLboolean*)&_restoreSampleAlphaToOne);
    glGetFloatv(GL_LINE_WIDTH, &_lineWidth);
    glGetBooleanv(GL_CULL_FACE, (GLboolean*)&_cullFace);
    glGetIntegerv(GL_CULL_FACE_MODE, &_cullMode);
    glGetIntegerv(GL_FRONT_FACE, &_frontFace);
    glGetBooleanv(GL_RASTERIZER_DISCARD, (GLboolean*)&_rasterizerDiscard);
    glGetBooleanv(GL_FRAMEBUFFER_SRGB, (GLboolean*)&_restoreFramebufferSRGB);

    HGIGL_POST_PENDING_GL_ERRORS();
    #if defined(GL_KHR_debug)
    if (GARCH_GLAPI_HAS(KHR_debug)) {
        glPopDebugGroup();
    }
    #endif
}

HgiGL_ScopedStateHolder::~HgiGL_ScopedStateHolder()
{
    TRACE_FUNCTION();

    #if defined(GL_KHR_debug)
    if (GARCH_GLAPI_HAS(KHR_debug)) {
        glPushDebugGroup(GL_DEBUG_SOURCE_THIRD_PARTY, 0, -1, "Restore state");
    }
    #endif

    if (_restoreAlphaToCoverage) {
        glEnable(GL_SAMPLE_ALPHA_TO_COVERAGE);
    } else {
        glDisable(GL_SAMPLE_ALPHA_TO_COVERAGE);
    }

    if (_restoreSampleAlphaToOne) {
        glEnable(GL_SAMPLE_ALPHA_TO_ONE);
    } else {
        glDisable(GL_SAMPLE_ALPHA_TO_ONE);
    }

    glBlendFuncSeparate(_restoreColorSrcFnOp, _restoreColorDstFnOp, 
                        _restoreAlphaSrcFnOp, _restoreAlphaDstFnOp);
    glBlendEquationSeparate(_restoreColorOp, _restoreAlphaOp);

    if (_restoreblendEnabled) {
        glEnable(GL_BLEND);
    } else {
        glDisable(GL_BLEND);
    }

    glViewport(_restoreViewport[0], _restoreViewport[1],
               _restoreViewport[2], _restoreViewport[3]);
    glDepthFunc(_restoreDepthFunc);
    glDepthMask(_restoreDepthWriteMask);
    glStencilMask(_restoreStencilWriteMask);
    if (_restoreDepthTest) {
        glEnable(GL_DEPTH_TEST);
    } else {
        glDisable(GL_DEPTH_TEST);
    }
    glBindVertexArray(_restoreVao);
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, _restoreDrawFramebuffer);
    glBindFramebuffer(GL_READ_FRAMEBUFFER, _restoreReadFramebuffer);
    glBindRenderbuffer(GL_RENDERBUFFER, _restoreRenderBuffer);
    glLineWidth(_lineWidth);
    if (_cullFace) {
        glEnable(GL_CULL_FACE);
    } else {
        glDisable(GL_CULL_FACE);
    }
    glCullFace(_cullMode);
    glFrontFace(_frontFace);

    if (_rasterizerDiscard) {
        glEnable(GL_RASTERIZER_DISCARD);
    } else {
        glDisable(GL_RASTERIZER_DISCARD);
    }

    if (_restoreFramebufferSRGB) {
       glEnable(GL_FRAMEBUFFER_SRGB);
    } else {
       glDisable(GL_FRAMEBUFFER_SRGB);
    }

    static const GLuint samplers[8] = {0};
    glBindSamplers(0, TfArraySize(samplers), samplers);

    HGIGL_POST_PENDING_GL_ERRORS();
    #if defined(GL_KHR_debug)
    if (GARCH_GLAPI_HAS(KHR_debug)) {
        glPopDebugGroup();
    }
    #endif
}


PXR_NAMESPACE_CLOSE_SCOPE
