//
// Copyright 2020 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
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
    : _restoreRenderBuffer(0)
    , _restoreVao(0)
    , _restoreDepthTest(false)
    , _restoreDepthWriteMask(false)
    , _restoreDepthFunc(0)
    , _restoreDepthBias(false)
    , _restoreDepthBiasConstantFactor(0)
    , _restoreDepthBiasSlopeFactor(0)
    , _restoreStencilTest(false)
    , _restoreStencilCompareFn{0,0}
    , _restoreStencilReferenceValue{0,0}
    , _restoreStencilReadMask{0,0}
    , _restoreStencilWriteMask{0,0}
    , _restoreViewport{0,0,0,0}
    , _restoreBlendEnabled(false)
    , _restoreColorOp(0)
    , _restoreAlphaOp(0)
    , _restoreAlphaToCoverage(false)
    , _restoreSampleAlphaToOne(false)
    , _lineWidth(1.0f)
    , _cullFace(true)
    , _cullMode(GL_BACK)
    , _frontFace(GL_CCW)
    , _rasterizerDiscard(true)
    , _restoreDepthClamp(false)
    , _depthRange{0.f,1.f}
    , _restoreFramebufferSRGB(false)
    , _restoreConservativeRaster(false)
    , _restoreMultiSample(false)
    , _restorePointSmooth(false)
{
    TRACE_FUNCTION();

    #if defined(GL_KHR_debug)
    if (GARCH_GLAPI_HAS(KHR_debug)) {
        glPushDebugGroup(GL_DEBUG_SOURCE_THIRD_PARTY, 0, -1, "Capture state");
    }
    #endif

    glGetIntegerv(GL_RENDERBUFFER_BINDING, &_restoreRenderBuffer);
    glGetIntegerv(GL_VERTEX_ARRAY_BINDING, &_restoreVao);

    glGetBooleanv(GL_DEPTH_TEST, (GLboolean*)&_restoreDepthTest);
    glGetBooleanv(GL_DEPTH_WRITEMASK, (GLboolean*)&_restoreDepthWriteMask);
    glGetIntegerv(GL_DEPTH_FUNC, &_restoreDepthFunc);

    glGetBooleanv(GL_POLYGON_OFFSET_FILL, (GLboolean*)&_restoreDepthBias);
    glGetFloatv(GL_POLYGON_OFFSET_UNITS, &_restoreDepthBiasConstantFactor);
    glGetFloatv(GL_POLYGON_OFFSET_FACTOR, &_restoreDepthBiasSlopeFactor);

    glGetIntegerv(GL_STENCIL_FUNC, &_restoreStencilCompareFn[0]);
    glGetIntegerv(GL_STENCIL_REF, &_restoreStencilReferenceValue[0]);
    glGetIntegerv(GL_STENCIL_VALUE_MASK, &_restoreStencilReadMask[0]);
    glGetIntegerv(GL_STENCIL_FAIL, &_restoreStencilFail[0]);
    glGetIntegerv(GL_STENCIL_PASS_DEPTH_FAIL, &_restoreStencilDepthFail[0]);
    glGetIntegerv(GL_STENCIL_PASS_DEPTH_PASS, &_restoreStencilDepthPass[0]);
    glGetIntegerv(GL_STENCIL_WRITEMASK, &_restoreStencilWriteMask[0]);

    glGetIntegerv(GL_STENCIL_BACK_FUNC, &_restoreStencilCompareFn[1]);
    glGetIntegerv(GL_STENCIL_BACK_REF, &_restoreStencilReferenceValue[1]);
    glGetIntegerv(GL_STENCIL_BACK_VALUE_MASK, &_restoreStencilReadMask[1]);
    glGetIntegerv(GL_STENCIL_BACK_FAIL, &_restoreStencilFail[1]);
    glGetIntegerv(GL_STENCIL_BACK_PASS_DEPTH_FAIL, &_restoreStencilDepthFail[1]);
    glGetIntegerv(GL_STENCIL_BACK_PASS_DEPTH_PASS, &_restoreStencilDepthPass[1]);
    glGetIntegerv(GL_STENCIL_BACK_WRITEMASK, &_restoreStencilWriteMask[1]);

    glGetIntegerv(GL_VIEWPORT, _restoreViewport);
    glGetBooleanv(GL_BLEND, (GLboolean*)&_restoreBlendEnabled);
    glGetIntegerv(GL_BLEND_EQUATION_RGB, &_restoreColorOp);
    glGetIntegerv(GL_BLEND_EQUATION_ALPHA, &_restoreAlphaOp);
    glGetIntegerv(GL_BLEND_SRC_RGB, &_restoreColorSrcFnOp);
    glGetIntegerv(GL_BLEND_SRC_ALPHA, &_restoreAlphaSrcFnOp);
    glGetIntegerv(GL_BLEND_DST_RGB, &_restoreColorDstFnOp);
    glGetIntegerv(GL_BLEND_DST_ALPHA, &_restoreAlphaDstFnOp);
    glGetFloatv(GL_BLEND_COLOR, _restoreBlendColor);
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
    glGetBooleanv(GL_DEPTH_CLAMP, (GLboolean*)&_restoreDepthClamp);
    glGetFloatv(GL_DEPTH_RANGE, _depthRange);
    glGetBooleanv(GL_FRAMEBUFFER_SRGB, (GLboolean*)&_restoreFramebufferSRGB);

    if (GARCH_GLAPI_HAS(NV_conservative_raster)) {
        glGetBooleanv(GL_CONSERVATIVE_RASTERIZATION_NV,
            (GLboolean*)&_restoreConservativeRaster);
    }

    GLint maxClipPlanes;
    glGetIntegerv(GL_MAX_CLIP_PLANES, &maxClipPlanes);
    _restoreClipDistances.resize(maxClipPlanes);
    for (int i = 0; i < maxClipPlanes; i++) {
        bool clipDistanceEnabled = false;
        glGetBooleanv(GL_CLIP_DISTANCE0 + i, (GLboolean*)&clipDistanceEnabled);
        _restoreClipDistances[i] = clipDistanceEnabled;
    }

    glGetBooleanv(GL_MULTISAMPLE, (GLboolean*)&_restoreMultiSample);
    glGetBooleanv(GL_POINT_SMOOTH, (GLboolean*)&_restorePointSmooth);

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

    //
    // Depth Stencil State
    //
    if (_restoreDepthTest) {
        glEnable(GL_DEPTH_TEST);
    } else {
        glDisable(GL_DEPTH_TEST);
    }
    glDepthMask(_restoreDepthWriteMask);
    glDepthFunc(_restoreDepthFunc);

    if (_restoreDepthBias) {
        glEnable(GL_POLYGON_OFFSET_FILL);
    } else {
        glDisable(GL_POLYGON_OFFSET_FILL);
    }
    glPolygonOffset(_restoreDepthBiasSlopeFactor,
                    _restoreDepthBiasConstantFactor);

    if (_restoreStencilTest) {
        glEnable(GL_STENCIL_TEST);
    } else {
        glDisable(GL_STENCIL_TEST);
    }
    glStencilFuncSeparate(GL_FRONT,
        _restoreStencilCompareFn[0],
        _restoreStencilReferenceValue[0],
        _restoreStencilReadMask[0]);
    glStencilOpSeparate(GL_FRONT,
        _restoreStencilFail[0],
        _restoreStencilDepthFail[0],
        _restoreStencilDepthPass[0]);
    glStencilMaskSeparate(GL_FRONT,
        _restoreStencilWriteMask[0]);

    glStencilFuncSeparate(GL_BACK,
        _restoreStencilCompareFn[1],
        _restoreStencilReferenceValue[1],
        _restoreStencilReadMask[1]);
    glStencilOpSeparate(GL_BACK,
        _restoreStencilFail[1],
        _restoreStencilDepthFail[1],
        _restoreStencilDepthPass[1]);
    glStencilMaskSeparate(GL_BACK,
        _restoreStencilWriteMask[1]);

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
    glBlendColor(_restoreBlendColor[0],
                 _restoreBlendColor[1],
                 _restoreBlendColor[2],
                 _restoreBlendColor[3]);

    if (_restoreBlendEnabled) {
        glEnable(GL_BLEND);
    } else {
        glDisable(GL_BLEND);
    }

    glViewport(_restoreViewport[0], _restoreViewport[1],
               _restoreViewport[2], _restoreViewport[3]);
    glBindVertexArray(_restoreVao);
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

    if (_restoreDepthClamp) {
        glEnable(GL_DEPTH_CLAMP);
    } else {
        glDisable(GL_DEPTH_CLAMP);
    }
    glDepthRangef(_depthRange[0], _depthRange[1]);

    if (_restoreFramebufferSRGB) {
       glEnable(GL_FRAMEBUFFER_SRGB);
    } else {
       glDisable(GL_FRAMEBUFFER_SRGB);
    }

    if (GARCH_GLAPI_HAS(NV_conservative_raster)) {
        if (_restoreConservativeRaster) {
            glEnable(GL_CONSERVATIVE_RASTERIZATION_NV);
        } else {
            glDisable(GL_CONSERVATIVE_RASTERIZATION_NV);
        }
    }

    for (size_t i = 0; i < _restoreClipDistances.size(); i++) {
        if (_restoreClipDistances[i]) {
            glEnable(GL_CLIP_DISTANCE0 + i);  
        } else {
            glDisable(GL_CLIP_DISTANCE0 + i);  
        }
    }

    if (_restoreMultiSample) {
        glEnable(GL_MULTISAMPLE);
    } else {
        glDisable(GL_MULTISAMPLE);
    }

    if (_restorePointSmooth) {
        glEnable(GL_POINT_SMOOTH);
    } else {
        glDisable(GL_POINT_SMOOTH);
    }

    static const GLuint samplers[8] = {0};
    glBindSamplers(0, TfArraySize(samplers), samplers);

    glUseProgram(0);

    HGIGL_POST_PENDING_GL_ERRORS();
    #if defined(GL_KHR_debug)
    if (GARCH_GLAPI_HAS(KHR_debug)) {
        glPopDebugGroup();
    }
    #endif
}


PXR_NAMESPACE_CLOSE_SCOPE
