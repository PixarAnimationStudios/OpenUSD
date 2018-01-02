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

#include "pxr/imaging/hdSt/drawItem.h"
#include "pxr/imaging/hdSt/fallbackLightingShader.h"
#include "pxr/imaging/hdSt/renderPassState.h"
#include "pxr/imaging/hdSt/renderPassShader.h"

#include "pxr/imaging/hd/bufferArrayRangeGL.h"
#include "pxr/imaging/hd/changeTracker.h"
#include "pxr/imaging/hd/conversions.h"
#include "pxr/imaging/hd/resourceRegistry.h"
#include "pxr/imaging/hd/shaderCode.h"
#include "pxr/imaging/hd/tokens.h"
#include "pxr/imaging/hd/vtBufferSource.h"

#include "pxr/base/gf/frustum.h"
#include "pxr/base/tf/staticTokens.h"
#include "pxr/base/tf/stringUtils.h"

#include <boost/functional/hash.hpp>

PXR_NAMESPACE_OPEN_SCOPE


TF_DEFINE_PRIVATE_TOKENS(
    _tokens,
    (renderPassState)
);

HdStRenderPassState::HdStRenderPassState()
    : HdRenderPassState()
    , _renderPassShader(new HdStRenderPassShader())
    , _fallbackLightingShader(new HdSt_FallbackLightingShader())
    , _clipPlanesBufferSize(0)
{
    _lightingShader = _fallbackLightingShader;
}

HdStRenderPassState::HdStRenderPassState(
    HdStRenderPassShaderSharedPtr const &renderPassShader)
    : HdRenderPassState()
    , _renderPassShader(renderPassShader)
    , _fallbackLightingShader(new HdSt_FallbackLightingShader())
    , _clipPlanesBufferSize(0)
{
    _lightingShader = _fallbackLightingShader;
}

HdStRenderPassState::~HdStRenderPassState()
{
    /*NOTHING*/
}

void
HdStRenderPassState::Sync(HdResourceRegistrySharedPtr const &resourceRegistry)
{
    HD_TRACE_FUNCTION();
    HF_MALLOC_TAG_FUNCTION();

    VtVec4fArray clipPlanes;
    TF_FOR_ALL(it, _clipPlanes) {
        clipPlanes.push_back(GfVec4f(*it));
    }
    if (clipPlanes.size() >= GL_MAX_CLIP_PLANES) {
        clipPlanes.resize(GL_MAX_CLIP_PLANES);
    }

    // allocate bar if not exists
    if (!_renderPassStateBar || (_clipPlanesBufferSize != clipPlanes.size())) {
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
        _clipPlanesBufferSize = clipPlanes.size();

        // allocate interleaved buffer
        _renderPassStateBar = resourceRegistry->AllocateUniformBufferArrayRange(
            HdTokens->drawingShader, bufferSpecs);

        HdBufferArrayRangeGLSharedPtr _renderPassStateBar_ =
            boost::static_pointer_cast<HdBufferArrayRangeGL> (_renderPassStateBar);

        // add buffer binding request
        _renderPassShader->AddBufferBinding(
            HdBindingRequest(HdBinding::UBO, _tokens->renderPassState,
                             _renderPassStateBar_, /*interleaved=*/true));
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

    // Update cull style on renderpass shader
    // XXX: Ideanlly cullstyle should stay in renderPassState.
    // However, geometric shader also sets cullstyle during batch
    // execution.
    _renderPassShader->SetCullStyle(_cullStyle);
}

void
HdStRenderPassState::SetLightingShader(HdStLightingShaderSharedPtr const &lightingShader)
{
    if (lightingShader) {
        _lightingShader = lightingShader;
    } else {
        _lightingShader = _fallbackLightingShader;
    }
}

void 
HdStRenderPassState::SetRenderPassShader(HdStRenderPassShaderSharedPtr const &renderPassShader)
{
    if (_renderPassShader == renderPassShader) return;

    _renderPassShader = renderPassShader;
    if (_renderPassStateBar) {

        HdBufferArrayRangeGLSharedPtr _renderPassStateBar_ =
            boost::static_pointer_cast<HdBufferArrayRangeGL> (_renderPassStateBar);

        _renderPassShader->AddBufferBinding(
            HdBindingRequest(HdBinding::UBO, _tokens->renderPassState,
                             _renderPassStateBar_, /*interleaved=*/true));
    }
}

void 
HdStRenderPassState::SetOverrideShader(HdShaderCodeSharedPtr const &overrideShader)
{
    _overrideShader = overrideShader;
}

HdShaderCodeSharedPtrVector
HdStRenderPassState::GetShaders() const
{
    HdShaderCodeSharedPtrVector shaders;
    shaders.reserve(2);
    shaders.push_back(_lightingShader);
    shaders.push_back(_renderPassShader);
    return shaders;
}

void
HdStRenderPassState::Bind()
{
    // XXX: this states set will be refactored as hdstream PSO.

    // XXX: viewport should be set.
    // glViewport((GLint)_viewport[0], (GLint)_viewport[1],
    //            (GLsizei)_viewport[2], (GLsizei)_viewport[3]);

    // when adding another GL state change here, please document
    // which states to be altered at the comment in the header file

    // Apply polygon offset to whole pass.
    if (!_depthBiasUseDefault) {
        if (_depthBiasEnabled) {
            glEnable(GL_POLYGON_OFFSET_FILL);
            glPolygonOffset(_depthBiasSlopeFactor, _depthBiasConstantFactor);
        } else {
            glDisable(GL_POLYGON_OFFSET_FILL);
        }
    }

    glDepthFunc(HdConversions::GetGlDepthFunc(_depthFunc));

    if (!_alphaToCoverageUseDefault) {
        if (_alphaToCoverageEnabled) {
            glEnable(GL_SAMPLE_ALPHA_TO_COVERAGE);
        } else {
            glDisable(GL_SAMPLE_ALPHA_TO_COVERAGE);
        }
    }
    glEnable(GL_PROGRAM_POINT_SIZE);
    for (size_t i = 0; i < _clipPlanes.size(); ++i) {
        if (i >= GL_MAX_CLIP_PLANES) {
            break;
        }
        glEnable(GL_CLIP_DISTANCE0 + i);
    }

    if (!_colorMaskUseDefault) {
        switch(_colorMask) {
            case HdStRenderPassState::ColorMaskNone:
                glColorMask(false, false, false, false);
                break;
            case HdStRenderPassState::ColorMaskRGB:
                glColorMask(true, true, true, false);
                break;
            case HdStRenderPassState::ColorMaskRGBA:
                glColorMask(true, true, true, true);
                break;
        }
    }
}

void
HdStRenderPassState::Unbind()
{
    // restore back to the GL defaults

    glDisable(GL_POLYGON_OFFSET_FILL);
    glDisable(GL_SAMPLE_ALPHA_TO_COVERAGE);
    glDisable(GL_PROGRAM_POINT_SIZE);
    glDepthFunc(GL_LESS);
    glPolygonOffset(0, 0);

    for (size_t i = 0; i < _clipPlanes.size(); ++i) {
        glDisable(GL_CLIP_DISTANCE0 + i);
    }

    glColorMask(true, true, true, true);
}

size_t
HdStRenderPassState::GetShaderHash() const
{
    size_t hash = 0;
    if (_lightingShader) {
        boost::hash_combine(hash, _lightingShader->ComputeHash());
    }
    if (_renderPassShader) {
        boost::hash_combine(hash, _renderPassShader->ComputeHash());
    }
    boost::hash_combine(hash, _clipPlanes.size());
    return hash;
}

PXR_NAMESPACE_CLOSE_SCOPE
