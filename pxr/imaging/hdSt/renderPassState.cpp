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
#include "pxr/imaging/hdSt/geometricShader.h"
#include "pxr/imaging/hdSt/glConversions.h"
#include "pxr/imaging/hdSt/hgiConversions.h"
#include "pxr/imaging/hdSt/fallbackLightingShader.h"
#include "pxr/imaging/hdSt/renderBuffer.h"
#include "pxr/imaging/hdSt/renderPassShader.h"
#include "pxr/imaging/hdSt/renderPassState.h"
#include "pxr/imaging/hdSt/resourceBinder.h"
#include "pxr/imaging/hdSt/resourceRegistry.h"
#include "pxr/imaging/hdSt/shaderCode.h"

#include "pxr/imaging/hd/aov.h"
#include "pxr/imaging/hd/camera.h"
#include "pxr/imaging/hd/changeTracker.h"
#include "pxr/imaging/hd/renderIndex.h"
#include "pxr/imaging/hd/tokens.h"
#include "pxr/imaging/hd/vtBufferSource.h"

#include "pxr/imaging/hgi/graphicsCmdsDesc.h"
#include "pxr/imaging/hgi/graphicsPipeline.h"

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

static
double
_SafeDiv(const double a, const double b)
{
    if (b == 0.0) {
        return 1.0;
    }
    return a / b;
}

HdStRenderPassState::HdStRenderPassState()
    : HdStRenderPassState(std::make_shared<HdStRenderPassShader>())
{
}

HdStRenderPassState::HdStRenderPassState(
    HdStRenderPassShaderSharedPtr const &renderPassShader)
    : HdRenderPassState()
    , _worldToViewMatrix(1)
    , _projectionMatrix(1)
    , _cullMatrix(1.0)
    , _renderPassShader(renderPassShader)
    , _fallbackLightingShader(std::make_shared<HdSt_FallbackLightingShader>())
    , _clipPlanesBufferSize(0)
    , _alphaThresholdCurrent(0)
    , _resolveMultiSampleAov(true)
    , _useSceneMaterials(true)
{
    _lightingShader = _fallbackLightingShader;
}

HdStRenderPassState::~HdStRenderPassState() = default;

bool
HdStRenderPassState::_UseAlphaMask() const
{
    return (_alphaThreshold > 0.0f);
}

unsigned int
HdStRenderPassState::_GetFramebufferHeight() const
{
    for (const HdRenderPassAovBinding &aov : GetAovBindings()) {
        if (aov.renderBuffer) {
            if (aov.renderBuffer->GetHeight() > 0) {
                return aov.renderBuffer->GetHeight();
            }
        }
    }
    return 0;
}

static
float
_CameraAspectRatio(const HdCamera * const camera)
{
    if (!camera) {
        return 1.0f;
    }
    const float v = camera->GetVerticalAperture();
    const float h = camera->GetHorizontalAperture();
    return fabs(_SafeDiv(h, v));
}

static
GfRange2f
_FlipWindow(const GfRange2f &window,
          const unsigned int framebufferHeight)
{
    if (framebufferHeight > 0) {
        const GfVec2f &min = window.GetMin();
        const GfVec2f &max = window.GetMax();

        const float minX = min[0];
        const float minY = framebufferHeight - max[1];
        const float maxX = max[0];
        const float maxY = framebufferHeight - min[1];
        return GfRange2f(GfVec2f(minX, minY), GfVec2f(maxX, maxY));
    } else {
        return window;
    }

}

GfRange2f
HdStRenderPassState::_ComputeFlippedFilmbackWindow() const
{
    if (_framing.IsValid()) {
        return
            _FlipWindow(
                _framing.ComputeFilmbackWindow(
                    _CameraAspectRatio(_camera),
                    GetWindowPolicy()),
                _GetFramebufferHeight());
    } else {
        const GfVec2f origin(_viewport[0], _viewport[1]);
        const GfVec2f size(_viewport[2], _viewport[3]);
        return GfRange2f(origin, origin + size);
    };
}

HdStRenderPassState::_AxisAlignedAffineTransform
HdStRenderPassState::_ComputeImageToHorizontallyNormalizedFilmback() const
{
    const GfRange2f window = _ComputeFlippedFilmbackWindow();

    // Recall the documentation of
    // _ComputeImageToHorizontallyNormalizedFilmback.
    //
    // To achieve 1., we need x to change by 2 when moving from the
    // left to the right edge of window.
    const float xScale = _SafeDiv(2.0, window.GetSize()[0]);

    // To achieve 3., we need to take the pixel aspect ratio into account.
    const float yScale =
        _framing.IsValid()
            ? _SafeDiv(xScale, _framing.pixelAspectRatio)
            : xScale;

    // We need the midpoint of window to go to (0,0) for 2.
    const GfVec2f midPoint = window.GetMidpoint();

    return GfVec4f(
        xScale, yScale,
        -midPoint[0] * xScale, -midPoint[1] * yScale);
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

static
GfVec4i
_ToVec4i(const GfVec4f &v)
{
    return GfVec4i(int(v[0]), int(v[1]), int(v[2]), int(v[3]));
}

static
GfVec4i
_ComputeViewport(const GfRect2i &dataWindow, unsigned int framebufferHeight)
{
    if (framebufferHeight > 0) {
        return GfVec4i(
            dataWindow.GetMinX(),
            framebufferHeight - (dataWindow.GetMinY() + dataWindow.GetHeight()),
            dataWindow.GetWidth(),
            dataWindow.GetHeight());
    } else {
        return GfVec4i(dataWindow.GetMinX(),  dataWindow.GetMinY(),
                       dataWindow.GetWidth(), dataWindow.GetHeight());
    }
}

GfVec4i
HdStRenderPassState::ComputeViewport() const
{
    const CameraUtilFraming &framing = GetFraming();
    // Use data window for clients using the new camera framing API.
    if (framing.IsValid()) {
        return _ComputeViewport(framing.dataWindow, _GetFramebufferHeight());
    }

    // For clients not using the new camera framing API, fallback
    // to the viewport they specified.
    return _ToVec4i(GetViewport());
}

void
HdStRenderPassState::Prepare(
    HdResourceRegistrySharedPtr const &resourceRegistry)
{
    HD_TRACE_FUNCTION();
    HF_MALLOC_TAG_FUNCTION();
    GLF_GROUP_FUNCTION();

    HdRenderPassState::Prepare(resourceRegistry);

    if(!TfDebug::IsEnabled(HD_FREEZE_CULL_FRUSTUM)) {
        _cullMatrix = GetWorldToViewMatrix() * GetProjectionMatrix();
    }

    HdStResourceRegistrySharedPtr const& hdStResourceRegistry =
        std::static_pointer_cast<HdStResourceRegistry>(resourceRegistry);

    VtVec4fArray clipPlanes;
    TF_FOR_ALL(it, GetClipPlanes()) {
        clipPlanes.push_back(GfVec4f(*it));
    }
    const size_t maxClipPlanes = (size_t)hdStResourceRegistry->GetHgi()->
        GetCapabilities()->GetMaxClipDistances();
    if (clipPlanes.size() >= maxClipPlanes) {
        clipPlanes.resize(maxClipPlanes);
    }

    // allocate bar if it does not exist
    if (!_renderPassStateBar || 
        (_clipPlanesBufferSize != clipPlanes.size()) ||
        _alphaThresholdCurrent != _alphaThreshold) {
        HdBufferSpecVector bufferSpecs;

        // note: InterleavedMemoryManager computes the offsets in the packed
        // struct of following entries, which CodeGen generates the struct
        // definition into GLSL source in accordance with.
        const HdType matType = HdVtBufferSource::GetDefaultMatrixType();

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
            HdShaderTokens->imageToWorldMatrix,
            HdTupleType{matType, 1});
        bufferSpecs.emplace_back(
            HdShaderTokens->imageToHorizontallyNormalizedFilmback,
            HdTupleType{HdTypeFloatVec4, 1});
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
        bufferSpecs.emplace_back(
            HdShaderTokens->stepSize,
            HdTupleType{HdTypeFloat, 1});
        bufferSpecs.emplace_back(
            HdShaderTokens->stepSizeLighting,
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
            HdStBindingRequest(HdStBinding::UBO, _tokens->renderPassState,
                               _renderPassStateBar_, /*interleaved=*/true));
    }

    // Lighting hack supports different blending amounts, but we are currently
    // only using the feature to turn lighting on and off.
    float lightingBlendAmount = (_lightingEnabled ? 1.0f : 0.0f);

    GfMatrix4d const& worldToViewMatrix = GetWorldToViewMatrix();
    GfMatrix4d projMatrix = GetProjectionMatrix();

    HgiCapabilities const * capabilities =
        hdStResourceRegistry->GetHgi()->GetCapabilities();
    if (!capabilities->IsSet(
        HgiDeviceCapabilitiesBitsDepthRangeMinusOnetoOne)) {
        // Different backends use different clip space depth ranges. The
        // codebase generally assumes an OpenGL-style depth of [-1, 1] when
        // computing projection matrices, so we must add an additional
        // conversion when the Hgi backend expects a [0, 1] depth range.
        GfMatrix4d depthAdjustmentMat = GfMatrix4d(1);
        depthAdjustmentMat[2][2] = 0.5;
        depthAdjustmentMat[3][2] = 0.5;
        projMatrix = projMatrix * depthAdjustmentMat;
    }
    bool const doublesSupported = capabilities->IsSet(
        HgiDeviceCapabilitiesBitsShaderDoublePrecision);

    HdBufferSourceSharedPtrVector sources = {
        std::make_shared<HdVtBufferSource>(
            HdShaderTokens->worldToViewMatrix,
            worldToViewMatrix,
            doublesSupported),
        std::make_shared<HdVtBufferSource>(
            HdShaderTokens->worldToViewInverseMatrix,
            worldToViewMatrix.GetInverse(),
            doublesSupported),
        std::make_shared<HdVtBufferSource>(
            HdShaderTokens->projectionMatrix,
            projMatrix,
            doublesSupported),
        std::make_shared<HdVtBufferSource>(
            HdShaderTokens->imageToWorldMatrix,
            GetImageToWorldMatrix(),
            doublesSupported),
        std::make_shared<HdVtBufferSource>(
            HdShaderTokens->imageToHorizontallyNormalizedFilmback,
            VtValue(_ComputeImageToHorizontallyNormalizedFilmback())),
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
            VtValue(lightingBlendAmount)),
        std::make_shared<HdVtBufferSource>(
            HdShaderTokens->stepSize,
            VtValue(_stepSize)),
        std::make_shared<HdVtBufferSource>(
            HdShaderTokens->stepSizeLighting,
            VtValue(_stepSizeLighting))
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
    _renderPassShader = renderPassShader;
    if (_renderPassStateBar) {

        HdStBufferArrayRangeSharedPtr _renderPassStateBar_ =
            std::static_pointer_cast<HdStBufferArrayRange> (_renderPassStateBar);

        _renderPassShader->AddBufferBinding(
            HdStBindingRequest(HdStBinding::UBO, _tokens->renderPassState,
                               _renderPassStateBar_, /*interleaved=*/true));
    }
}

void 
HdStRenderPassState::SetUseSceneMaterials(bool state)
{
    _useSceneMaterials = state;
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

namespace {

void
_SetGLCullState(HgiCullMode const resolvedCullMode)
{
    switch (resolvedCullMode) {
        default:
        case HgiCullModeNone:
            glDisable(GL_CULL_FACE);
            break;
        case HgiCullModeFront:
            glEnable(GL_CULL_FACE);
            glCullFace(GL_FRONT);
            break;
        case HgiCullModeBack:
            glEnable(GL_CULL_FACE);
            glCullFace(GL_BACK);
            break;
        case HgiCullModeFrontAndBack:
            glEnable(GL_CULL_FACE);
            glCullFace(GL_FRONT_AND_BACK);
            break;
    }
}

void
_SetGLPolygonMode(float rsLineWidth,
                  HdSt_GeometricShaderSharedPtr const &geometricShader)
{
    if (geometricShader->GetPolygonMode() == HdPolygonModeLine) {
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
        float gsLineWidth = geometricShader->GetLineWidth();
        if (gsLineWidth > 0) {
            glLineWidth(gsLineWidth);
        }
    } else {
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
        if (rsLineWidth > 0) {
            glLineWidth(rsLineWidth);
        }
    }
}

void
_SetGLColorMask(int drawBufferIndex, HdRenderPassState::ColorMask const& mask)
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

HgiColorMask
_GetColorMask(HdRenderPassState::ColorMask const & mask)
{
    switch (mask) {
        default:
        case HdStRenderPassState::ColorMaskNone:
            return HgiColorMask(0);

        case HdStRenderPassState::ColorMaskRGB:
            return HgiColorMaskRed |
                   HgiColorMaskGreen |
                   HgiColorMaskBlue;

        case HdStRenderPassState::ColorMaskRGBA:
            return HgiColorMaskRed |
                   HgiColorMaskGreen |
                   HgiColorMaskBlue |
                   HgiColorMaskAlpha;
    }
}

} // anonymous namespace

void
HdStRenderPassState::ApplyStateFromGeometricShader(
        HdSt_ResourceBinder const &binder,
        HdSt_GeometricShaderSharedPtr const &geometricShader)
{
    _SetGLCullState(geometricShader->ResolveCullMode(_cullStyle));
    _SetGLPolygonMode(_lineWidth, geometricShader);
}

void
HdStRenderPassState::ApplyStateFromCamera()
{
    // notify view-transform to the lighting shader to update its uniform block
    // this needs to be done in execute as a multi camera setup may have been 
    // synced with a different view matrix baked in for shadows.
    // SetCamera will no-op if the transforms are the same as before.
    _lightingShader->SetCamera(GetWorldToViewMatrix(),
                               GetProjectionMatrix());
}

void
HdStRenderPassState::Bind(HgiCapabilities const &hgiCapabilities)
{
    GLF_GROUP_FUNCTION();

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
    if (GetEnableDepthClamp()) {
        glEnable(GL_DEPTH_CLAMP);
    }
    glDepthRange(GetDepthRange()[0], GetDepthRange()[1]);

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
        _SetGLColorMask(-1, ColorMaskRGBA);
    } else {
        if (_colorMasks.size() == 1) {
            // Use the same color mask for all attachments.
            _SetGLColorMask(-1, _colorMasks[0]);
        } else {
            for (size_t i = 0; i < _colorMasks.size(); i++) {
                _SetGLColorMask(i, _colorMasks[i]);
            }
        }
    }
    
    if (hgiCapabilities.IsSet(HgiDeviceCapabilitiesBitsConservativeRaster)) {
        if (_conservativeRasterizationEnabled) {
            glEnable(GL_CONSERVATIVE_RASTERIZATION_NV);
        } else {
            glDisable(GL_CONSERVATIVE_RASTERIZATION_NV);
        }
    }

    if (_multiSampleEnabled) {
        glEnable(GL_MULTISAMPLE);
    } else {
        glDisable(GL_MULTISAMPLE);
        // If not using GL_MULTISAMPLE, use GL_POINT_SMOOTH to render points as 
        // circles instead of square.
        // XXX Switch points rendering to emit quad with FS that draws circle.
        glEnable(GL_POINT_SMOOTH);
    }
}

void
HdStRenderPassState::Unbind(HgiCapabilities const &hgiCapabilities)
{
    GLF_GROUP_FUNCTION();
    // restore back to the GL defaults

    if (!GetDepthBiasUseDefault()) {
        glDisable(GL_POLYGON_OFFSET_FILL);
        glPolygonOffset(0, 0);
    }

    glDisable(GL_CULL_FACE);
    glDisable(GL_SAMPLE_ALPHA_TO_COVERAGE);
    glDisable(GL_SAMPLE_ALPHA_TO_ONE);
    glDisable(GL_PROGRAM_POINT_SIZE);
    glEnable(GL_DEPTH_TEST);
    glDisable(GL_STENCIL_TEST);
    glDepthFunc(GL_LESS);
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    glLineWidth(1.0f);
    glDisable(GL_DEPTH_CLAMP);
    glDepthRange(0, 1);

    glDisable(GL_BLEND);
    glBlendEquationSeparate(GL_FUNC_ADD, GL_FUNC_ADD);
    glBlendFuncSeparate(GL_ONE, GL_ZERO, GL_ONE, GL_ZERO);
    glBlendColor(0.0f, 0.0f, 0.0f, 0.0f);

    for (size_t i = 0; i < GetClipPlanes().size(); ++i) {
        glDisable(GL_CLIP_DISTANCE0 + i);
    }

    glColorMask(true, true, true, true);
    glDepthMask(true);

    if (hgiCapabilities.IsSet(HgiDeviceCapabilitiesBitsConservativeRaster)) {
        glDisable(GL_CONSERVATIVE_RASTERIZATION_NV);
    }

    glEnable(GL_MULTISAMPLE);
    glDisable(GL_POINT_SMOOTH);
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

void
HdStRenderPassState::_InitAttachmentDesc(
    HgiAttachmentDesc &attachmentDesc,
    int aovIndex) const
{
    // HdSt expresses blending per RenderPassState, where Hgi expresses
    // blending per-attachment. Transfer pass blend state to attachments.
    attachmentDesc.blendEnabled = _blendEnabled;
    attachmentDesc.srcColorBlendFactor = HgiBlendFactor(_blendColorSrcFactor);
    attachmentDesc.dstColorBlendFactor = HgiBlendFactor(_blendColorDstFactor);
    attachmentDesc.colorBlendOp = HgiBlendOp(_blendColorOp);
    attachmentDesc.srcAlphaBlendFactor = HgiBlendFactor(_blendAlphaSrcFactor);
    attachmentDesc.dstAlphaBlendFactor = HgiBlendFactor(_blendAlphaDstFactor);
    attachmentDesc.alphaBlendOp = HgiBlendOp(_blendAlphaOp);
    attachmentDesc.blendConstantColor = _blendConstantColor;

    if (!_colorMaskUseDefault) {
        if (aovIndex > 0 && aovIndex < (int)_colorMasks.size()) {
            attachmentDesc.colorMask = _GetColorMask(_colorMasks[aovIndex]);
        } else if (_colorMasks.size() == 1) {
            attachmentDesc.colorMask = _GetColorMask(_colorMasks[0]);
        }
    }
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

    for (size_t aovIndex = 0; aovIndex < aovBindings.size(); ++aovIndex) {
        HdRenderPassAovBinding const & aov = aovBindings[aovIndex];
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
        
        // APPLE METAL: The logic above needs revisiting!
        attachmentDesc.storeOp = HgiAttachmentStoreOpStore;

        if (!aov.clearValue.IsEmpty()) {
            attachmentDesc.clearValue = _ToVec4f(aov.clearValue);
        }

        _InitAttachmentDesc(attachmentDesc, aovIndex);

        if (HdAovHasDepthSemantic(aov.aovName) ||
            HdAovHasDepthStencilSemantic(aov.aovName)) {
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

GfMatrix4d
HdStRenderPassState::GetWorldToViewMatrix() const
{
    if (_camera) {
        return HdRenderPassState::GetWorldToViewMatrix();
    }

    return _worldToViewMatrix;
}

GfMatrix4d
HdStRenderPassState::GetProjectionMatrix() const
{
    if (_camera) {
        return HdRenderPassState::GetProjectionMatrix();
    }
    return _projectionMatrix;
}

HdRenderPassState::ClipPlanesVector const &
HdStRenderPassState::GetClipPlanes() const
{
    if (!_camera) {
        if (_clippingEnabled) {
            return _clipPlanes;
        } else {
            const static HdRenderPassState::ClipPlanesVector empty;
            return empty;
        }
    }

    return HdRenderPassState::GetClipPlanes();
}

void
HdStRenderPassState::_InitPrimitiveState(
    HgiGraphicsPipelineDesc * pipeDesc,
    HdSt_GeometricShaderSharedPtr const & geometricShader) const
{
    pipeDesc->primitiveType = geometricShader->GetHgiPrimitiveType();

    if (pipeDesc->primitiveType == HgiPrimitiveTypePatchList) {
        pipeDesc->tessellationState.primitiveIndexSize =
                            geometricShader->GetPrimitiveIndexSize();

        if (geometricShader->GetUseMetalTessellation()) {
            pipeDesc->tessellationState.patchType =
                geometricShader->IsPrimTypeTriangles()
                    ? HgiTessellationState::PatchType::Triangle
                    : HgiTessellationState::PatchType::Quad;
            pipeDesc->tessellationState.tessFactorMode =
                geometricShader->IsPrimTypePatches()
                    ? HgiTessellationState::TessVertex
                    : HgiTessellationState::Constant;
        }
    }
}

void
HdStRenderPassState::_InitAttachmentState(
    HgiGraphicsPipelineDesc * pipeDesc) const
{
    // For Metal we have to pass the color and depth descriptors down so
    // that they are available when creating the Render Pipeline State for
    // the fragment shaders.
    HdRenderPassAovBindingVector const& aovBindings = GetAovBindings();

    for (size_t aovIndex = 0; aovIndex < aovBindings.size(); ++aovIndex) {
        HdRenderPassAovBinding const & binding = aovBindings[aovIndex];
        if (HdAovHasDepthSemantic(binding.aovName) ||
            HdAovHasDepthStencilSemantic(binding.aovName)) {
            HdFormat const hdFormat = binding.renderBuffer->GetFormat();
            HgiFormat const format = HdStHgiConversions::GetHgiFormat(hdFormat);
            pipeDesc->depthAttachmentDesc.format = format;
            pipeDesc->depthAttachmentDesc.usage =
                HgiTextureUsageBitsDepthTarget;

            if (HdAovHasDepthStencilSemantic(binding.aovName)) {
                pipeDesc->depthAttachmentDesc.usage |=
                    HgiTextureUsageBitsStencilTarget;
            }
        } else {
            HdFormat const hdFormat = binding.renderBuffer->GetFormat();
            HgiFormat const format = HdStHgiConversions::GetHgiFormat(hdFormat);
            HgiAttachmentDesc attachment;
            attachment.format = format;
            _InitAttachmentDesc(attachment, aovIndex);
            pipeDesc->colorAttachmentDescs.push_back(attachment);
        }
    }

    // Assume all the aovs have the same multisample settings.
    HgiSampleCount sampleCount = HgiSampleCount1;
    if (!aovBindings.empty() && GetUseAovMultiSample()) {
        HdStRenderBuffer *firstRenderBuffer =
            static_cast<HdStRenderBuffer*>(aovBindings.front().renderBuffer);

        if (firstRenderBuffer->IsMultiSampled()) {
            sampleCount = HgiSampleCount(
                firstRenderBuffer->GetMSAASampleCount());
        }
    }

    pipeDesc->multiSampleState.sampleCount = sampleCount;
}

void
HdStRenderPassState::_InitDepthStencilState(
    HgiDepthStencilState * depthState) const
{
    if (GetEnableDepthTest()) {
        depthState->depthTestEnabled = true;
        depthState->depthCompareFn =
            HdStHgiConversions::GetHgiCompareFunction(_depthFunc);
        depthState->depthWriteEnabled = GetEnableDepthMask();
    } else {
        depthState->depthTestEnabled = false;
        depthState->depthWriteEnabled = false;
    }

    if (!GetDepthBiasUseDefault() && GetDepthBiasEnabled()) {
        depthState->depthBiasEnabled = true;
        depthState->depthBiasConstantFactor = _depthBiasConstantFactor;
        depthState->depthBiasSlopeFactor = _depthBiasSlopeFactor;
    }

    if (GetStencilEnabled()) {
        depthState->stencilTestEnabled = true;
        depthState->stencilFront.compareFn =
            HdStHgiConversions::GetHgiCompareFunction(_stencilFunc);
        depthState->stencilFront.referenceValue = _stencilRef;
        depthState->stencilFront.stencilFailOp =
            HdStHgiConversions::GetHgiStencilOp(_stencilFailOp);
        depthState->stencilFront.depthFailOp =
            HdStHgiConversions::GetHgiStencilOp(_stencilZFailOp);
        depthState->stencilFront.depthStencilPassOp =
            HdStHgiConversions::GetHgiStencilOp(_stencilZPassOp);
        depthState->stencilFront.readMask = _stencilMask;
        depthState->stencilBack = depthState->stencilFront;
    }
}

void
HdStRenderPassState::_InitMultiSampleState(
    HgiMultiSampleState * multiSampleState) const
{
    multiSampleState->multiSampleEnable = _multiSampleEnabled;

    if (_alphaToCoverageEnabled) {
        multiSampleState->alphaToCoverageEnable = true;
        multiSampleState->alphaToOneEnable = true;
    }
}

void
HdStRenderPassState::_InitRasterizationState(
    HgiRasterizationState * rasterizationState,
    HdSt_GeometricShaderSharedPtr const & geometricShader) const
{
    if (geometricShader->GetPolygonMode() == HdPolygonModeLine) {
        rasterizationState->polygonMode = HgiPolygonModeLine;
        float const gsLineWidth = geometricShader->GetLineWidth();
        if (gsLineWidth > 0) {
            rasterizationState->lineWidth = gsLineWidth;
        }
    } else {
        rasterizationState->polygonMode = HgiPolygonModeFill;
    }

    rasterizationState->cullMode =
        geometricShader->ResolveCullMode(_cullStyle);

    if (GetEnableDepthClamp()) {
        rasterizationState->depthClampEnabled = true;
    }
    rasterizationState->depthRange = GetDepthRange();

    rasterizationState->conservativeRaster = _conservativeRasterizationEnabled;

    rasterizationState->numClipDistances = GetClipPlanes().size();
}

void
HdStRenderPassState::InitGraphicsPipelineDesc(
    HgiGraphicsPipelineDesc * pipeDesc,
    HdSt_GeometricShaderSharedPtr const & geometricShader) const
{
    _InitPrimitiveState(pipeDesc, geometricShader);
    _InitDepthStencilState(&pipeDesc->depthState);
    _InitMultiSampleState(&pipeDesc->multiSampleState);
    _InitRasterizationState(&pipeDesc->rasterizationState, geometricShader);
    _InitAttachmentState(pipeDesc);
}

uint64_t
HdStRenderPassState::GetGraphicsPipelineHash() const
{
    // Hash all of the state that is captured in the pipeline state object.
    uint64_t hash = TfHash::Combine(
        _depthBiasUseDefault,
        _depthBiasEnabled,
        _depthBiasConstantFactor,
        _depthBiasSlopeFactor,
        _depthFunc,
        _depthMaskEnabled,
        _depthTestEnabled,
        _depthClampEnabled,
        _depthRange,
        _cullStyle,
        _stencilFunc,
        _stencilRef,
        _stencilMask,
        _stencilFailOp,
        _stencilZFailOp,
        _stencilZPassOp,
        _stencilEnabled,
        _lineWidth,
        _blendColorOp,
        _blendColorSrcFactor,
        _blendColorDstFactor,
        _blendAlphaOp,
        _blendAlphaSrcFactor,
        _blendAlphaDstFactor,
        _blendAlphaDstFactor,
        _blendConstantColor,
        _blendEnabled,
        _alphaToCoverageEnabled,
        _colorMaskUseDefault,
        _useMultiSampleAov,
        _conservativeRasterizationEnabled,
        GetClipPlanes().size(),
        _multiSampleEnabled);
    
    // Hash the aov bindings by name and format.
    for (HdRenderPassAovBinding const& binding : GetAovBindings()) {
        HdStRenderBuffer *renderBuffer =
            static_cast<HdStRenderBuffer*>(binding.renderBuffer);
        
        const uint32_t msaaCount = renderBuffer->IsMultiSampled() ?
            renderBuffer->GetMSAASampleCount() : 1;

        hash = TfHash::Combine(hash,
                               binding.aovName,
                               renderBuffer->GetFormat(),
                               msaaCount);
    }
    
    return hash;
}


PXR_NAMESPACE_CLOSE_SCOPE
