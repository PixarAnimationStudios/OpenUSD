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
#include "pxr/imaging/garch/glApi.h"

#include "pxr/imaging/glf/diagnostic.h"

#include "pxr/imaging/hdSt/bufferArrayRange.h"
#include "pxr/imaging/hdSt/drawItem.h"
#include "pxr/imaging/hdSt/glConversions.h"
#include "pxr/imaging/hdSt/hgiConversions.h"
#include "pxr/imaging/hdSt/fallbackLightingShader.h"
#include "pxr/imaging/hdSt/renderBuffer.h"
#include "pxr/imaging/hdSt/renderPassShader.h"
#include "pxr/imaging/hdSt/renderPassState.h"
#include "pxr/imaging/hdSt/resourceRegistry.h"
#include "pxr/imaging/hdSt/shaderCode.h"

#include "pxr/imaging/hd/aov.h"
#include "pxr/imaging/hd/changeTracker.h"
#include "pxr/imaging/hd/renderIndex.h"
#include "pxr/imaging/hd/tokens.h"
#include "pxr/imaging/hd/vtBufferSource.h"

#include "pxr/imaging/hgi/graphicsCmdsDesc.h"

#include "pxr/base/gf/frustum.h"
#include "pxr/base/tf/staticTokens.h"
#include "pxr/base/tf/stringUtils.h"
#include "pxr/base/vt/array.h"

#include <boost/functional/hash.hpp>

PXR_NAMESPACE_OPEN_SCOPE


TF_DEFINE_PRIVATE_TOKENS(
    _tokens,
    (renderPassState)
);

HdStRenderPassState::HdStRenderPassState()
    : HdStRenderPassState(std::make_shared<HdStRenderPassShader>())
{
}

HdStRenderPassState::HdStRenderPassState(
    HdStRenderPassShaderSharedPtr const &renderPassShader)
    : HdRenderPassState()
    , _renderPassShader(renderPassShader)
    , _fallbackLightingShader(std::make_shared<HdSt_FallbackLightingShader>())
    , _clipPlanesBufferSize(0)
    , _alphaThresholdCurrent(0)
    , _resolveMultiSampleAov(true)
{
    _lightingShader = _fallbackLightingShader;
}

HdStRenderPassState::~HdStRenderPassState() = default;

bool
HdStRenderPassState::_UseAlphaMask() const
{
    return (_alphaThreshold > 0.0f);
}

static
GfVec4f
_ComputeDataWindow(
    const CameraUtilFraming &framing,
    const GfVec4f &fallbackViewport)
{
    if (framing.IsValid()) {
        const GfRect2i &dataWindow = framing.dataWindow;
        return GfVec4f(
            dataWindow.GetMinX(),
            dataWindow.GetMinY(),
            dataWindow.GetWidth(),
            dataWindow.GetHeight());
    }

    return fallbackViewport;
}

void
HdStRenderPassState::Prepare(
    HdResourceRegistrySharedPtr const &resourceRegistry)
{
    HD_TRACE_FUNCTION();
    HF_MALLOC_TAG_FUNCTION();
    GLF_GROUP_FUNCTION();

    HdRenderPassState::Prepare(resourceRegistry);

    HdStResourceRegistrySharedPtr const& hdStResourceRegistry =
        std::static_pointer_cast<HdStResourceRegistry>(resourceRegistry);

    VtVec4fArray clipPlanes;
    TF_FOR_ALL(it, GetClipPlanes()) {
        clipPlanes.push_back(GfVec4f(*it));
    }
    GLint glMaxClipPlanes;
    glGetIntegerv(GL_MAX_CLIP_PLANES, &glMaxClipPlanes);
    size_t maxClipPlanes = (size_t)glMaxClipPlanes;
    if (clipPlanes.size() >= maxClipPlanes) {
        clipPlanes.resize(maxClipPlanes);
    }

    // allocate bar if not exists
    if (!_renderPassStateBar || 
        (_clipPlanesBufferSize != clipPlanes.size()) ||
        _alphaThresholdCurrent != _alphaThreshold) {
        HdBufferSpecVector bufferSpecs;

        // note: InterleavedMemoryManager computes the offsets in the packed
        // struct of following entries, which CodeGen generates the struct
        // definition into GLSL source in accordance with.
        HdType matType = HdVtBufferSource::GetDefaultMatrixType();

        bufferSpecs.emplace_back(
            HdShaderTokens->worldToViewMatrix,
            HdTupleType{matType, 1});
        bufferSpecs.emplace_back(
            HdShaderTokens->worldToViewInverseMatrix,
            HdTupleType{matType, 1});
        bufferSpecs.emplace_back(
            HdShaderTokens->projectionMatrix,
            HdTupleType{matType, 1});
        bufferSpecs.emplace_back(
            HdShaderTokens->overrideColor,
            HdTupleType{HdTypeFloatVec4, 1});
        bufferSpecs.emplace_back(
            HdShaderTokens->wireframeColor,
            HdTupleType{HdTypeFloatVec4, 1});
        bufferSpecs.emplace_back(
            HdShaderTokens->maskColor,
            HdTupleType{HdTypeFloatVec4, 1});
        bufferSpecs.emplace_back(
            HdShaderTokens->indicatorColor,
            HdTupleType{HdTypeFloatVec4, 1});
        bufferSpecs.emplace_back(
            HdShaderTokens->pointColor,
            HdTupleType{HdTypeFloatVec4, 1});
        bufferSpecs.emplace_back(
            HdShaderTokens->pointSize,
            HdTupleType{HdTypeFloat, 1});
        bufferSpecs.emplace_back(
            HdShaderTokens->pointSelectedSize,
            HdTupleType{HdTypeFloat, 1});
        bufferSpecs.emplace_back(
            HdShaderTokens->lightingBlendAmount,
            HdTupleType{HdTypeFloat, 1});

        if (_UseAlphaMask()) {
            bufferSpecs.emplace_back(
                HdShaderTokens->alphaThreshold,
                HdTupleType{HdTypeFloat, 1});
        }
        _alphaThresholdCurrent = _alphaThreshold;

        bufferSpecs.emplace_back(
            HdShaderTokens->tessLevel,
            HdTupleType{HdTypeFloat, 1});
        bufferSpecs.emplace_back(
            HdShaderTokens->viewport,
            HdTupleType{HdTypeFloatVec4, 1});

        if (clipPlanes.size() > 0) {
            bufferSpecs.emplace_back(
                HdShaderTokens->clipPlanes,
                HdTupleType{HdTypeFloatVec4, clipPlanes.size()});
        }
        _clipPlanesBufferSize = clipPlanes.size();

        // allocate interleaved buffer
        _renderPassStateBar = 
            hdStResourceRegistry->AllocateUniformBufferArrayRange(
                HdTokens->drawingShader, bufferSpecs, HdBufferArrayUsageHint());

        HdStBufferArrayRangeSharedPtr _renderPassStateBar_ =
            std::static_pointer_cast<HdStBufferArrayRange> (_renderPassStateBar);

        // add buffer binding request
        _renderPassShader->AddBufferBinding(
            HdBindingRequest(HdBinding::UBO, _tokens->renderPassState,
                             _renderPassStateBar_, /*interleaved=*/true));
    }

    // Lighting hack supports different blending amounts, but we are currently
    // only using the feature to turn lighting on and off.
    float lightingBlendAmount = (_lightingEnabled ? 1.0f : 0.0f);

    GfMatrix4d const& worldToViewMatrix = GetWorldToViewMatrix();
    GfMatrix4d projMatrix = GetProjectionMatrix();

    HdBufferSourceSharedPtrVector sources = {
        std::make_shared<HdVtBufferSource>(
            HdShaderTokens->worldToViewMatrix,
            worldToViewMatrix),
        std::make_shared<HdVtBufferSource>(
            HdShaderTokens->worldToViewInverseMatrix,
            worldToViewMatrix.GetInverse()),
        std::make_shared<HdVtBufferSource>(
            HdShaderTokens->projectionMatrix,
            projMatrix),
        // Override color alpha component is used as the amount to blend in the
        // override color over the top of the regular fragment color.
        std::make_shared<HdVtBufferSource>(
            HdShaderTokens->overrideColor,
            VtValue(_overrideColor)),
        std::make_shared<HdVtBufferSource>(
            HdShaderTokens->wireframeColor,
            VtValue(_wireframeColor)),
        std::make_shared<HdVtBufferSource>(
            HdShaderTokens->maskColor,
            VtValue(_maskColor)),
        std::make_shared<HdVtBufferSource>(
            HdShaderTokens->indicatorColor,
            VtValue(_indicatorColor)),
        std::make_shared<HdVtBufferSource>(
            HdShaderTokens->pointColor,
            VtValue(_pointColor)),
        std::make_shared<HdVtBufferSource>(
            HdShaderTokens->pointSize,
            VtValue(_pointSize)),
        std::make_shared<HdVtBufferSource>(
            HdShaderTokens->pointSelectedSize,
            VtValue(_pointSelectedSize)),
        std::make_shared<HdVtBufferSource>(
            HdShaderTokens->lightingBlendAmount,
            VtValue(lightingBlendAmount))
    };

    if (_UseAlphaMask()) {
        sources.push_back(
            std::make_shared<HdVtBufferSource>(
                HdShaderTokens->alphaThreshold,
                VtValue(_alphaThreshold)));
    }

    sources.push_back(
        std::make_shared<HdVtBufferSource>(
            HdShaderTokens->tessLevel,
            VtValue(_tessLevel)));
    sources.push_back(
        std::make_shared<HdVtBufferSource>(
            HdShaderTokens->viewport,
            VtValue(
                _ComputeDataWindow(
                    _framing, _viewport))));

    if (clipPlanes.size() > 0) {
        sources.push_back(
            std::make_shared<HdVtBufferSource>(
                HdShaderTokens->clipPlanes,
                VtValue(clipPlanes),
                clipPlanes.size()));
    }

    hdStResourceRegistry->AddSources(_renderPassStateBar, std::move(sources));

    // notify view-transform to the lighting shader to update its uniform block
    _lightingShader->SetCamera(worldToViewMatrix, projMatrix);

    // Update cull style on renderpass shader
    // (Note that the geometric shader overrides the render pass's cullStyle 
    // opinion if the prim has an opinion).
    _renderPassShader->SetCullStyle(_cullStyle);
}

void
HdStRenderPassState::SetResolveAovMultiSample(bool state)
{
    _resolveMultiSampleAov = state;
}

bool
HdStRenderPassState::GetResolveAovMultiSample() const
{
    return _resolveMultiSampleAov;
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

        HdStBufferArrayRangeSharedPtr _renderPassStateBar_ =
            std::static_pointer_cast<HdStBufferArrayRange> (_renderPassStateBar);

        _renderPassShader->AddBufferBinding(
            HdBindingRequest(HdBinding::UBO, _tokens->renderPassState,
                             _renderPassStateBar_, /*interleaved=*/true));
    }
}

void 
HdStRenderPassState::SetOverrideShader(HdStShaderCodeSharedPtr const &overrideShader)
{
    _overrideShader = overrideShader;
}

HdStShaderCodeSharedPtrVector
HdStRenderPassState::GetShaders() const
{
    HdStShaderCodeSharedPtrVector shaders;
    shaders.reserve(2);
    shaders.push_back(_lightingShader);
    shaders.push_back(_renderPassShader);
    return shaders;
}

// Note: The geometric shader may override the state set below if necessary,
// including disabling h/w culling altogether. 
// Disabling h/w culling is required to handle instancing wherein 
// instanceScale/instanceTransform can flip the xform handedness.
namespace {

void
_SetGLCullState(HdCullStyle cullstyle)
{
    switch (cullstyle) {
        case HdCullStyleFront:
        case HdCullStyleFrontUnlessDoubleSided:
            glEnable(GL_CULL_FACE);
            glCullFace(GL_FRONT);
            break;
        case HdCullStyleBack:
        case HdCullStyleBackUnlessDoubleSided:
            glEnable(GL_CULL_FACE);
            glCullFace(GL_BACK);
            break;
        case HdCullStyleNothing:
        case HdCullStyleDontCare:
        default:
            // disable culling
            glDisable(GL_CULL_FACE);
            break;
    }
}

void
_SetColorMask(int drawBufferIndex, HdRenderPassState::ColorMask const& mask)
{
    bool colorMask[4] = {true, true, true, true};
    switch (mask)
    {
        case HdStRenderPassState::ColorMaskNone:
            colorMask[0] = colorMask[1] = colorMask[2] = colorMask[3] = false;
            break;
        case HdStRenderPassState::ColorMaskRGB:
            colorMask[3] = false;
            break;
        default:
            ; // no-op
    }

    if (drawBufferIndex == -1) {
        glColorMask(colorMask[0], colorMask[1], colorMask[2], colorMask[3]);
    } else {
        glColorMaski((uint32_t) drawBufferIndex,
                     colorMask[0], colorMask[1], colorMask[2], colorMask[3]);
    }
}

} // anonymous namespace 

void
HdStRenderPassState::Bind()
{
    GLF_GROUP_FUNCTION();

    if (!glBlendColor) {
        return;
    }
    
    // notify view-transform to the lighting shader to update its uniform block
    // this needs to be done in execute as a multi camera setup may have been 
    // synced with a different view matrix baked in for shadows.
    // SetCamera will no-op if the transforms are the same as before.
    _lightingShader->SetCamera(GetWorldToViewMatrix(),
                               GetProjectionMatrix());

    // when adding another GL state change here, please document
    // which states to be altered at the comment in the header file

    // Apply polygon offset to whole pass.
    if (!GetDepthBiasUseDefault()) {
        if (GetDepthBiasEnabled()) {
            glEnable(GL_POLYGON_OFFSET_FILL);
            glPolygonOffset(_depthBiasSlopeFactor, _depthBiasConstantFactor);
        } else {
            glDisable(GL_POLYGON_OFFSET_FILL);
        }
    }

    if (GetEnableDepthTest()) {
        glEnable(GL_DEPTH_TEST);
        glDepthFunc(HdStGLConversions::GetGlDepthFunc(_depthFunc));
        glDepthMask(GetEnableDepthMask()); // depth writes are enabled only
                                           // when the test is enabled.
    } else {
        glDisable(GL_DEPTH_TEST);
    }

    // Stencil
    if (GetStencilEnabled()) {
        glEnable(GL_STENCIL_TEST);
        glStencilFunc(HdStGLConversions::GetGlStencilFunc(_stencilFunc),
                _stencilRef, _stencilMask);
        glStencilOp(HdStGLConversions::GetGlStencilOp(_stencilFailOp),
                HdStGLConversions::GetGlStencilOp(_stencilZFailOp),
                HdStGLConversions::GetGlStencilOp(_stencilZPassOp));
    } else {
        glDisable(GL_STENCIL_TEST);
    }
    
    // Face culling
    _SetGLCullState(_cullStyle);

    // Line width
    if (_lineWidth > 0) {
        glLineWidth(_lineWidth);
    }

    // Blending
    if (_blendEnabled) {
        glEnable(GL_BLEND);
        glBlendEquationSeparate(
                HdStGLConversions::GetGlBlendOp(_blendColorOp),
                HdStGLConversions::GetGlBlendOp(_blendAlphaOp));
        glBlendFuncSeparate(
                HdStGLConversions::GetGlBlendFactor(_blendColorSrcFactor),
                HdStGLConversions::GetGlBlendFactor(_blendColorDstFactor),
                HdStGLConversions::GetGlBlendFactor(_blendAlphaSrcFactor),
                HdStGLConversions::GetGlBlendFactor(_blendAlphaDstFactor));
        glBlendColor(_blendConstantColor[0],
                     _blendConstantColor[1],
                     _blendConstantColor[2],
                     _blendConstantColor[3]);
    } else {
        glDisable(GL_BLEND);
    }

    if (_alphaToCoverageEnabled) {
        glEnable(GL_SAMPLE_ALPHA_TO_COVERAGE);
        glEnable(GL_SAMPLE_ALPHA_TO_ONE);
    } else {
        glDisable(GL_SAMPLE_ALPHA_TO_COVERAGE);
    }
    
    glEnable(GL_PROGRAM_POINT_SIZE);
    GLint glMaxClipPlanes;
    glGetIntegerv(GL_MAX_CLIP_PLANES, &glMaxClipPlanes);
    for (size_t i = 0; i < GetClipPlanes().size(); ++i) {
        if (i >= (size_t)glMaxClipPlanes) {
            break;
        }
        glEnable(GL_CLIP_DISTANCE0 + i);
    }

    if (_colorMaskUseDefault) {
        // Enable color writes for all components for all attachments.
        _SetColorMask(-1, ColorMaskRGBA);
    } else {
        if (_colorMasks.size() == 1) {
            // Use the same color mask for all attachments.
            _SetColorMask(-1, _colorMasks[0]);
        } else {
            for (size_t i = 0; i < _colorMasks.size(); i++) {
                _SetColorMask(i, _colorMasks[i]);
            }
        }
    }
}

void
HdStRenderPassState::Unbind()
{
    GLF_GROUP_FUNCTION();
    // restore back to the GL defaults

    if (!glBlendColor) {
        return;
    }

    glDisable(GL_CULL_FACE);
    glDisable(GL_POLYGON_OFFSET_FILL);
    glDisable(GL_SAMPLE_ALPHA_TO_COVERAGE);
    glDisable(GL_SAMPLE_ALPHA_TO_ONE);
    glDisable(GL_PROGRAM_POINT_SIZE);
    glEnable(GL_DEPTH_TEST);
    glDisable(GL_STENCIL_TEST);
    glDepthFunc(GL_LESS);
    glPolygonOffset(0, 0);
    glLineWidth(1.0f);

    glDisable(GL_BLEND);
    glBlendEquationSeparate(GL_FUNC_ADD, GL_FUNC_ADD);
    glBlendFuncSeparate(GL_ONE, GL_ZERO, GL_ONE, GL_ZERO);
    glBlendColor(0.0f, 0.0f, 0.0f, 0.0f);

    for (size_t i = 0; i < GetClipPlanes().size(); ++i) {
        glDisable(GL_CLIP_DISTANCE0 + i);
    }

    glColorMask(true, true, true, true);
    glDepthMask(true);
}

void
HdStRenderPassState::SetCameraFramingState(GfMatrix4d const &worldToViewMatrix,
                                           GfMatrix4d const &projectionMatrix,
                                           GfVec4d const &viewport,
                                           ClipPlanesVector const & clipPlanes)
{
    if (_camera) {
        // If a camera handle was set, reset it.
        _camera = nullptr;
    }

    _worldToViewMatrix = worldToViewMatrix;
    _projectionMatrix = projectionMatrix;
    _viewport = GfVec4f((float)viewport[0], (float)viewport[1],
                        (float)viewport[2], (float)viewport[3]);
    _clipPlanes = clipPlanes;
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
    boost::hash_combine(hash, GetClipPlanes().size());
    boost::hash_combine(hash, _UseAlphaMask());
    return hash;
}

static
HdRenderBuffer *
_GetRenderBuffer(const HdRenderPassAovBinding& aov,
                 const HdRenderIndex * const renderIndex)
{
    if (aov.renderBuffer) {
        return aov.renderBuffer;
    }

    return 
        dynamic_cast<HdRenderBuffer*>(
            renderIndex->GetBprim(
                HdPrimTypeTokens->renderBuffer,
                aov.renderBufferId));
}

// Clear values are always vec4f in HgiGraphicsCmdDesc.
static
GfVec4f _ToVec4f(const VtValue &v)
{
    if (v.IsHolding<float>()) {
        const float depth = v.UncheckedGet<float>();
        return GfVec4f(depth,0,0,0);
    }
    if (v.IsHolding<double>()) {
        const double val = v.UncheckedGet<double>();
        return GfVec4f(val);
    }
    if (v.IsHolding<GfVec2f>()) {
        const GfVec2f val = v.UncheckedGet<GfVec2f>();
        return GfVec4f(val[0], val[1], 0.0, 1.0);
    }
    if (v.IsHolding<GfVec2d>()) {
        const GfVec2d val = v.UncheckedGet<GfVec2d>();
        return GfVec4f(val[0], val[1], 0.0, 1.0);
    }
    if (v.IsHolding<GfVec3f>()) {
        const GfVec3f val = v.UncheckedGet<GfVec3f>();
        return GfVec4f(val[0], val[1], val[2], 1.0);
    }
    if (v.IsHolding<GfVec3d>()) {
        const GfVec3d val = v.UncheckedGet<GfVec3d>();
        return GfVec4f(val[0], val[1], val[2], 1.0);
    }
    if (v.IsHolding<GfVec4f>()) {
        return v.UncheckedGet<GfVec4f>();
    }
    if (v.IsHolding<GfVec4d>()) {
        return GfVec4f(v.UncheckedGet<GfVec4d>());
    }

    TF_CODING_ERROR("Unsupported clear value for draw target attachment.");
    return GfVec4f(0.0);
}

HgiGraphicsCmdsDesc
HdStRenderPassState::MakeGraphicsCmdsDesc(
    const HdRenderIndex * const renderIndex) const
{
    const HdRenderPassAovBindingVector& aovBindings = GetAovBindings();

    static const size_t maxColorTex = 8;
    const bool useMultiSample = GetUseAovMultiSample();
    const bool resolveMultiSample = GetResolveAovMultiSample();

    HgiGraphicsCmdsDesc desc;

    // If the AOV bindings have not changed that does NOT mean the
    // graphicsCmdsDescriptor will not change. The HdRenderBuffer may be
    // resized at any time, which will destroy and recreate the HgiTextureHandle
    // that backs the render buffer and was attached for graphics encoding.

    for (const HdRenderPassAovBinding& aov : aovBindings) {
        HdRenderBuffer * const renderBuffer =
            _GetRenderBuffer(aov, renderIndex);


        if (!TF_VERIFY(renderBuffer, "Invalid render buffer")) {
            continue;
        }

        const bool multiSampled =
            useMultiSample && renderBuffer->IsMultiSampled();
        const VtValue rv = renderBuffer->GetResource(multiSampled);

        if (!TF_VERIFY(rv.IsHolding<HgiTextureHandle>(), 
            "Invalid render buffer texture")) {
            continue;
        }

        // Get render target texture
        HgiTextureHandle hgiTexHandle = rv.UncheckedGet<HgiTextureHandle>();

        // Get resolve texture target.
        HgiTextureHandle hgiResolveHandle;
        if (multiSampled && resolveMultiSample) {
            VtValue resolveRes = renderBuffer->GetResource(/*ms*/false);
            if (!TF_VERIFY(resolveRes.IsHolding<HgiTextureHandle>())) {
                continue;
            }
            hgiResolveHandle = resolveRes.UncheckedGet<HgiTextureHandle>();
        }

        HgiAttachmentDesc attachmentDesc;

        attachmentDesc.format = hgiTexHandle->GetDescriptor().format;
        attachmentDesc.usage = hgiTexHandle->GetDescriptor().usage;

        // We need to use LoadOpLoad instead of DontCare because we can have
        // multiple render passes that use the same attachments.
        // For example, translucent renders after opaque so we must load the
        // opaque results before rendering translucent objects.
        HgiAttachmentLoadOp loadOp = aov.clearValue.IsEmpty() ?
            HgiAttachmentLoadOpLoad :
            HgiAttachmentLoadOpClear;

        attachmentDesc.loadOp = loadOp;

        // Don't store multisample images. Only store the resolved versions.
        // This saves a bunch of bandwith (especially on tiled gpu's).
        attachmentDesc.storeOp = (multiSampled && resolveMultiSample) ?
            HgiAttachmentStoreOpDontCare :
            HgiAttachmentStoreOpStore;

        if (!aov.clearValue.IsEmpty()) {
            attachmentDesc.clearValue = _ToVec4f(aov.clearValue);
        }

        // HdSt expresses blending per RenderPassState, where Hgi expresses
        // blending per-attachment. Transfer pass blend state to attachments.
        attachmentDesc.blendEnabled = _blendEnabled;
        attachmentDesc.srcColorBlendFactor=HgiBlendFactor(_blendColorSrcFactor);
        attachmentDesc.dstColorBlendFactor=HgiBlendFactor(_blendColorDstFactor);
        attachmentDesc.colorBlendOp = HgiBlendOp(_blendColorOp);
        attachmentDesc.srcAlphaBlendFactor=HgiBlendFactor(_blendAlphaSrcFactor);
        attachmentDesc.dstAlphaBlendFactor=HgiBlendFactor(_blendAlphaDstFactor);
        attachmentDesc.alphaBlendOp = HgiBlendOp(_blendAlphaOp);

        if (HdAovHasDepthSemantic(aov.aovName)) {
            desc.depthAttachmentDesc = std::move(attachmentDesc);
            desc.depthTexture = hgiTexHandle;
            if (hgiResolveHandle) {
                desc.depthResolveTexture = hgiResolveHandle;
            }
        } else if (TF_VERIFY(desc.colorAttachmentDescs.size() < maxColorTex,
                   "Too many aov bindings for color attachments"))
        {
            desc.colorAttachmentDescs.push_back(std::move(attachmentDesc));
            desc.colorTextures.push_back(hgiTexHandle);
            if (hgiResolveHandle) {
                desc.colorResolveTextures.push_back(hgiResolveHandle);
            }
        }
    }

    return desc;
}

PXR_NAMESPACE_CLOSE_SCOPE
