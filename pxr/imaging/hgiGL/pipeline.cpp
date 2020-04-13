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
#include <vector>

#include "pxr/base/tf/diagnostic.h"

#include "pxr/imaging/hgiGL/conversions.h"
#include "pxr/imaging/hgiGL/diagnostic.h"
#include "pxr/imaging/hgiGL/pipeline.h"
#include "pxr/imaging/hgiGL/resourceBindings.h"
#include "pxr/imaging/hgiGL/shaderProgram.h"
#include "pxr/imaging/hgiGL/shaderFunction.h"

PXR_NAMESPACE_OPEN_SCOPE

HgiGLPipeline::HgiGLPipeline(
    HgiPipelineDesc const& desc)
    : HgiPipeline(desc)
    , _restoreDrawFramebuffer(0)
    , _restoreReadFramebuffer(0)
    , _restoreRenderBuffer(0)
    , _restoreVao(0)
    , _restoreDepthTest(false)
    , _restoreDepthWriteMask(false)
    , _restoreStencilWriteMask(false)
    , _restoreDepthFunc(0)
    , _restoreViewport{0,0,0,0}
    , _restoreblendEnabled(false)
    , _restoreColorOp(0)
    , _restoreAlphaOp(0)
    , _restoreAlphaToCoverage(false)
    , _vao()
{
    glCreateVertexArrays(1, &_vao);
    glObjectLabel(GL_VERTEX_ARRAY, _vao, -1, _descriptor.debugName.c_str());

    // Configure the vertex buffers in the vertex array object.
    for (HgiVertexBufferDesc const& vbo : _descriptor.vertexBuffers) {

        HgiVertexAttributeDescVector const& vas = vbo.vertexAttributes;

        // Describe each vertex attribute in the vertex buffer
        for (size_t loc=0; loc<vas.size(); loc++) {
            HgiVertexAttributeDesc const& va = vas[loc];

            uint32_t idx = va.shaderBindLocation;
            glEnableVertexArrayAttrib(_vao, idx);
            glVertexArrayAttribBinding(_vao, idx, vbo.bindingIndex);
            glVertexArrayAttribFormat(
                _vao,
                idx,
                HgiGLConversions::GetElementCount(va.format),
                HgiGLConversions::GetFormatType(va.format),
                GL_FALSE,
                va.offset);
        }
    }

    HGIGL_POST_PENDING_GL_ERRORS();
}

HgiGLPipeline::~HgiGLPipeline()
{
    glBindVertexArray(0);
    glDeleteVertexArrays(1, &_vao);
}

void
HgiGLPipeline::BindPipeline()
{
    glBindVertexArray(_vao);

    //
    // Depth Stencil State
    //
    if (_descriptor.depthState.depthTestEnabled) {
        glEnable(GL_DEPTH_TEST);
        GLenum depthFn = HgiGLConversions::GetDepthCompareFunction(
            _descriptor.depthState.depthCompareFn);
        glDepthFunc(depthFn);
    } else {
        glDisable(GL_DEPTH_TEST);
    }

    glDepthMask(_descriptor.depthState.depthWriteEnabled ? GL_TRUE : GL_FALSE);

    if (_descriptor.depthState.stencilTestEnabled) {
        TF_CODING_ERROR("Missing implementation stencil mask enabled");
    } else {
        glStencilMaskSeparate(GL_FRONT, 0);
        glStencilMaskSeparate(GL_BACK, 0);
    }

    //
    // Multi sample state
    //
    if (_descriptor.multiSampleState.alphaToCoverageEnable) {
        glEnable(GL_SAMPLE_ALPHA_TO_COVERAGE);
    } else {
        glDisable(GL_SAMPLE_ALPHA_TO_COVERAGE);
    }

    //
    // Rasterization state
    //
    GLenum cullMode = HgiGLConversions::GetCullMode(
        _descriptor.rasterizationState.cullMode);
    if (cullMode == GL_NONE) {
        glDisable(GL_CULL_FACE);
    } else {
        glEnable(GL_CULL_FACE);
        glCullFace(cullMode);
    }

    GLenum polygonMode = HgiGLConversions::GetPolygonMode(
        _descriptor.rasterizationState.polygonMode);
    glPolygonMode(GL_FRONT_AND_BACK, polygonMode);

    if (_descriptor.rasterizationState.winding == HgiWindingClockwise) {
        glFrontFace(GL_CW);
    } else {
        glFrontFace(GL_CCW);
    }

    if (_descriptor.rasterizationState.lineWidth != 1.0f) {
        glLineWidth(_descriptor.rasterizationState.lineWidth);
    }

    //
    // Shader program
    //
    HgiGLShaderProgram* glProgram = 
        static_cast<HgiGLShaderProgram*>(_descriptor.shaderProgram.Get());
    if (glProgram) {
        glUseProgram(glProgram->GetProgramId());
    }

    HGIGL_POST_PENDING_GL_ERRORS();
}

void
HgiGLPipeline::CaptureOpenGlState()
{
    glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &_restoreDrawFramebuffer);
    glGetIntegerv(GL_READ_FRAMEBUFFER_BINDING, &_restoreReadFramebuffer);
    glGetIntegerv(GL_RENDERBUFFER_BINDING, &_restoreRenderBuffer);
    glGetIntegerv(GL_VERTEX_ARRAY_BINDING, &_restoreVao);
    glGetBooleanv(GL_DEPTH_TEST, (GLboolean*)&_restoreDepthTest);
    glGetBooleanv(GL_DEPTH_WRITEMASK, (GLboolean*)&_restoreDepthWriteMask);
    glGetBooleanv(GL_STENCIL_WRITEMASK, (GLboolean*)&_restoreStencilWriteMask);
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
    HGIGL_POST_PENDING_GL_ERRORS();
}

void
HgiGLPipeline::RestoreOpenGlState()
{
    if (_restoreAlphaToCoverage) {
        glEnable(GL_SAMPLE_ALPHA_TO_COVERAGE);
    } else {
        glDisable(GL_SAMPLE_ALPHA_TO_COVERAGE);
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

    HGIGL_POST_PENDING_GL_ERRORS();
}

PXR_NAMESPACE_CLOSE_SCOPE
