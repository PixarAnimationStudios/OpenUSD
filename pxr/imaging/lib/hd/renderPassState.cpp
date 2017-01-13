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
#include "pxr/imaging/glf/glew.h"

#include "pxr/imaging/hd/renderPassState.h"

#include "pxr/imaging/hd/changeTracker.h"
#include "pxr/imaging/hd/fallbackLightingShader.h"
#include "pxr/imaging/hd/drawItem.h"
#include "pxr/imaging/hd/glslProgram.h"
#include "pxr/imaging/hd/renderPassShader.h"
#include "pxr/imaging/hd/resourceRegistry.h"
#include "pxr/imaging/hd/shader.h"
#include "pxr/imaging/hd/tokens.h"
#include "pxr/imaging/hd/vtBufferSource.h"


#include "pxr/base/gf/frustum.h"
#include "pxr/base/tf/staticTokens.h"
#include "pxr/base/tf/stringUtils.h"

#include <boost/functional/hash.hpp>

TF_DEFINE_PRIVATE_TOKENS(
    _tokens,
    (renderPassState)
);

HdRenderPassState::HdRenderPassState()
    : _renderPassShader(new HdRenderPassShader())
    , _fallbackLightingShader(new Hd_FallbackLightingShader())
    , _worldToViewMatrix(1)
    , _projectionMatrix(1)
    , _viewport(0, 0, 1, 1)
    , _cullMatrix(1)
    , _overrideColor(0.0f, 0.0f, 0.0f, 0.0f)
    , _wireframeColor(0.0f, 0.0f, 0.0f, 0.0f)
    , _lightingEnabled(true)
    , _alphaThreshold(0.5f)
    , _tessLevel(32.0)
    , _drawRange(0.9, -1.0)
    , _depthBiasUseDefault(true)
    , _depthBiasEnabled(false)
    , _depthBiasConstantFactor(0.0f)
    , _depthBiasSlopeFactor(1.0f)
    , _depthFunc(HdCmpFuncLEqual)
    , _alphaToCoverageUseDefault(true)
    , _alphaToCoverageEnabled(true)
{
    _lightingShader = _fallbackLightingShader;
}

HdRenderPassState::HdRenderPassState(
    HdRenderPassShaderSharedPtr const &renderPassShader)
    : _renderPassShader(renderPassShader)
    , _fallbackLightingShader(new Hd_FallbackLightingShader())
    , _worldToViewMatrix(1)
    , _projectionMatrix(1)
    , _viewport(0, 0, 1, 1)
    , _cullMatrix(1)
    , _overrideColor(0.0f, 0.0f, 0.0f, 0.0f)
    , _wireframeColor(0.0f, 0.0f, 0.0f, 0.0f)
    , _lightingEnabled(true)
    , _alphaThreshold(0.5f)
    , _tessLevel(32.0)
    , _drawRange(0.9, -1.0)
    , _depthBiasUseDefault(true)
    , _depthBiasEnabled(false)
    , _depthBiasConstantFactor(0.0f)
    , _depthBiasSlopeFactor(1.0f)
    , _depthFunc(HdCmpFuncLEqual)
    , _alphaToCoverageUseDefault(true)
    , _alphaToCoverageEnabled(true)
{
    _lightingShader = _fallbackLightingShader;
}

HdRenderPassState::~HdRenderPassState()
{
    /*NOTHING*/
}

void
HdRenderPassState::Sync()
{
    HD_TRACE_FUNCTION();
    HF_MALLOC_TAG_FUNCTION();

    HdResourceRegistry *resourceRegistry = &HdResourceRegistry::GetInstance();

    VtVec4fArray clipPlanes;
    TF_FOR_ALL(it, _clipPlanes) {
        clipPlanes.push_back(GfVec4f(*it));
    }

    // allocate bar if not exists
    if (!_renderPassStateBar) {
        HdBufferSpecVector bufferSpecs;

        // note: InterleavedMemoryManager computes the offsets in the packed
        // struct of following entries, which CodeGen generates the struct
        // definition into GLSL source in accordance with.
        GLenum matType = HdVtBufferSource::GetDefaultMatrixType();
        bufferSpecs.push_back(HdBufferSpec(HdShaderTokens->worldToViewMatrix, matType, 16));
        bufferSpecs.push_back(HdBufferSpec(HdShaderTokens->worldToViewInverseMatrix, matType, 16));
        bufferSpecs.push_back(HdBufferSpec(HdShaderTokens->projectionMatrix, matType, 16));
        bufferSpecs.push_back(HdBufferSpec(HdShaderTokens->overrideColor, GL_FLOAT, 4));
        bufferSpecs.push_back(HdBufferSpec(HdShaderTokens->wireframeColor, GL_FLOAT, 4));
        bufferSpecs.push_back(HdBufferSpec(HdShaderTokens->lightingBlendAmount, GL_FLOAT, 1));
        bufferSpecs.push_back(HdBufferSpec(HdShaderTokens->alphaThreshold, GL_FLOAT, 1));
        bufferSpecs.push_back(HdBufferSpec(HdShaderTokens->tessLevel, GL_FLOAT, 1));
        bufferSpecs.push_back(HdBufferSpec(HdShaderTokens->viewport, GL_FLOAT, 4));

        if (clipPlanes.size() > 0) {
            bufferSpecs.push_back(
                HdBufferSpec(
                    HdShaderTokens->clipPlanes, GL_FLOAT, 4, clipPlanes.size()));
        }

        // allocate interleaved buffer
        _renderPassStateBar = resourceRegistry->AllocateUniformBufferArrayRange(
            HdTokens->drawingShader, bufferSpecs);

        // add buffer binding request
        _renderPassShader->AddBufferBinding(
            HdBindingRequest(HdBinding::UBO, _tokens->renderPassState,
                             _renderPassStateBar, /*interleaved=*/true));
    }

    // Lighting hack supports different blending amounts, but we are currently
    // only using the feature to turn lighting on and off.
    float lightingBlendAmount = (_lightingEnabled ? 1.0f : 0.0f);

    HdBufferSourceVector sources;
    sources.push_back(HdBufferSourceSharedPtr(
                         new HdVtBufferSource(HdShaderTokens->worldToViewMatrix,
                                              _worldToViewMatrix)));
    sources.push_back(HdBufferSourceSharedPtr(
                  new HdVtBufferSource(HdShaderTokens->worldToViewInverseMatrix,
                                       _worldToViewMatrix.GetInverse())));
    sources.push_back(HdBufferSourceSharedPtr(
                          new HdVtBufferSource(HdShaderTokens->projectionMatrix,
                                               _projectionMatrix)));
    // Override color alpha component is used as the amount to blend in the
    // override color over the top of the regular fragment color.
    sources.push_back(HdBufferSourceSharedPtr(
                          new HdVtBufferSource(HdShaderTokens->overrideColor,
                                               VtValue(_overrideColor))));
    sources.push_back(HdBufferSourceSharedPtr(
                          new HdVtBufferSource(HdShaderTokens->wireframeColor,
                                               VtValue(_wireframeColor))));

    sources.push_back(HdBufferSourceSharedPtr(
                       new HdVtBufferSource(HdShaderTokens->lightingBlendAmount,
                                            VtValue(lightingBlendAmount))));;
    sources.push_back(HdBufferSourceSharedPtr(
                          new HdVtBufferSource(HdShaderTokens->alphaThreshold,
                                               VtValue(_alphaThreshold))));
    sources.push_back(HdBufferSourceSharedPtr(
                       new HdVtBufferSource(HdShaderTokens->tessLevel,
                                            VtValue(_tessLevel))));;
    sources.push_back(HdBufferSourceSharedPtr(
                          new HdVtBufferSource(HdShaderTokens->viewport,
                                               VtValue(_viewport))));

    if (clipPlanes.size() > 0) {
        sources.push_back(HdBufferSourceSharedPtr(
                              new HdVtBufferSource(
                                  HdShaderTokens->clipPlanes,
                                  VtValue(clipPlanes))));
    }

    resourceRegistry->AddSources(_renderPassStateBar, sources);

    // notify view-transform to the lighting shader to update its uniform block
    _lightingShader->SetCamera(_worldToViewMatrix, _projectionMatrix);
}

void
HdRenderPassState::SetCamera(GfMatrix4d const &worldToViewMatrix,
                        GfMatrix4d const &projectionMatrix,
                        GfVec4d const &viewport) {
    _worldToViewMatrix = worldToViewMatrix;
    _projectionMatrix = projectionMatrix;
    _viewport = GfVec4f((float)viewport[0], (float)viewport[1],
                        (float)viewport[2], (float)viewport[3]);

    if(!TfDebug::IsEnabled(HD_FREEZE_CULL_FRUSTUM)) {
        _cullMatrix = _worldToViewMatrix * _projectionMatrix;
    }
}

void
HdRenderPassState::SetOverrideColor(GfVec4f const &color)
{
    _overrideColor = color;
}

void
HdRenderPassState::SetWireframeColor(GfVec4f const &color)
{
    _wireframeColor = color;
}

void
HdRenderPassState::SetCullStyle(HdCullStyle cullStyle)
{
    // XXX: ideally cullstyle should be moved to renderPassState.
    //      however, geometric shader also sets cullstyle during
    //      batch execution. need some refactoring.
    _renderPassShader->SetCullStyle(cullStyle);
}

void
HdRenderPassState::SetAlphaThreshold(float alphaThreshold)
{
    _alphaThreshold = alphaThreshold;
}

void
HdRenderPassState::SetTessLevel(float tessLevel)
{
    _tessLevel = tessLevel;
}

void
HdRenderPassState::SetDrawingRange(GfVec2f const &drawRange)
{
    _drawRange = drawRange;
}

void
HdRenderPassState::SetLightingEnabled(bool enabled)
{
    _lightingEnabled = enabled;
}

void
HdRenderPassState::SetClipPlanes(ClipPlanesVector const & clipPlanes)
{
    if (_clipPlanes != clipPlanes) {
        _clipPlanes = clipPlanes;
        if (!TF_VERIFY(_clipPlanes.size() < GL_MAX_CLIP_PLANES)) {
            _clipPlanes.resize(GL_MAX_CLIP_PLANES);
        }
        _renderPassStateBar.reset();
    }
}

HdRenderPassState::ClipPlanesVector const &
HdRenderPassState::GetClipPlanes() const
{
    return _clipPlanes;
}

void
HdRenderPassState::SetLightingShader(HdLightingShaderSharedPtr const &lightingShader)
{
    if (lightingShader) {
        _lightingShader = lightingShader;
    } else {
        _lightingShader = _fallbackLightingShader;
    }
}

void 
HdRenderPassState::SetRenderPassShader(HdRenderPassShaderSharedPtr const &renderPassShader)
{
    if (_renderPassShader == renderPassShader) return;

    // XXX: cullStyle should be moved to RenderPassState.
    renderPassShader->SetCullStyle(_renderPassShader->GetCullStyle());

    _renderPassShader = renderPassShader;
    if (_renderPassStateBar) {
        _renderPassShader->AddBufferBinding(
            HdBindingRequest(HdBinding::UBO, _tokens->renderPassState,
                             _renderPassStateBar, /*interleaved=*/true));
    }
}

void 
HdRenderPassState::SetOverrideShader(HdShaderSharedPtr const &overrideShader)
{
    _overrideShader = overrideShader;
}

HdShaderSharedPtrVector
HdRenderPassState::GetShaders() const
{
    HdShaderSharedPtrVector shaders;
    shaders.reserve(2);
    shaders.push_back(_lightingShader);
    shaders.push_back(_renderPassShader);
    return shaders;
}

void
HdRenderPassState::SetDepthBiasUseDefault(bool useDefault)
{
    _depthBiasUseDefault = useDefault;
}

void
HdRenderPassState::SetDepthBiasEnabled(bool enable)
{
    _depthBiasEnabled = enable;
}

void
HdRenderPassState::SetDepthBias(float constantFactor, float slopeFactor)
{
    _depthBiasConstantFactor = constantFactor;
    _depthBiasSlopeFactor = slopeFactor;
}

void
HdRenderPassState::SetDepthFunc(HdCompareFunction depthFunc)
{
    _depthFunc = depthFunc;
}

void
HdRenderPassState::SetAlphaToCoverageUseDefault(bool useDefault)
{
    _alphaToCoverageUseDefault = useDefault;
}

void
HdRenderPassState::SetAlphaToCoverageEnabled(bool enabled)
{
    _alphaToCoverageEnabled = enabled;
}

void
HdRenderPassState::Bind()
{
    // XXX: in future, RenderPassState::Bind() and Unbind() can be a part of
    // command buffer. At that point we should not rely on GL attribute stack.

    glPushAttrib(GL_POLYGON_BIT | GL_DEPTH_BUFFER_BIT | GL_ENABLE_BIT);

    // Apply polygon offset to whole pass.
    // Restored by GL_POLYGON_BIT|GL_ENABLE_BIT
    if (!_depthBiasUseDefault) {
        if (_depthBiasEnabled) {
            glEnable(GL_POLYGON_OFFSET_FILL);
            glPolygonOffset(_depthBiasSlopeFactor, _depthBiasConstantFactor);
        } else {
            glDisable(GL_POLYGON_OFFSET_FILL);
        }
    }

    // Restored by GL_DEPTH_BUFFER_BIT
    glDepthFunc(HdConversions::GetGlDepthFunc(_depthFunc));

    // Restored by GL_ENABLE_BIT pop attrib
    if (!_alphaToCoverageUseDefault) {
        if (_alphaToCoverageEnabled) {
            glEnable(GL_SAMPLE_ALPHA_TO_COVERAGE);
        } else {
            glDisable(GL_SAMPLE_ALPHA_TO_COVERAGE);
        }
    }
    glEnable(GL_PROGRAM_POINT_SIZE);
    for (size_t i = 0; i < _clipPlanes.size(); ++i) {
        glEnable(GL_CLIP_DISTANCE0 + i);
    }
}

void
HdRenderPassState::Unbind()
{
    glPopAttrib();
}

size_t
HdRenderPassState::GetShaderHash() const
{
    size_t hash = _lightingShader->ComputeHash();
    boost::hash_combine(hash, _renderPassShader->ComputeHash());
    boost::hash_combine(hash, _clipPlanes.size());
    return hash;
}

