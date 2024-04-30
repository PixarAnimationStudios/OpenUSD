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
#include "pxr/imaging/hd/renderPassState.h"

#include "pxr/imaging/hd/camera.h"
#include "pxr/imaging/hd/changeTracker.h"
#include "pxr/imaging/hd/debugCodes.h"
#include "pxr/imaging/hd/tokens.h"

#include "pxr/base/gf/camera.h"
#include "pxr/base/gf/frustum.h"
#include "pxr/base/tf/stringUtils.h"

PXR_NAMESPACE_OPEN_SCOPE

HdRenderPassState::HdRenderPassState()
    : _camera(nullptr)
    , _viewport(0, 0, 1, 1)
    , _overrideColor(0.0f, 0.0f, 0.0f, 0.0f)
    , _wireframeColor(0.0f, 0.0f, 0.0f, 0.0f)
    , _pointColor(0.0f, 0.0f, 0.0f, 1.0f)
    , _pointSize(3.0)
    , _lightingEnabled(true)
    , _clippingEnabled(true)

    , _maskColor(1.0f, 0.0f, 0.0f, 1.0f)
    , _indicatorColor(0.0f, 1.0f, 0.0f, 1.0f)
    , _pointSelectedSize(3.0)

    , _alphaThreshold(0.5f)
    , _tessLevel(32.0)
    , _drawRange(0.9, -1.0)

    , _depthBiasUseDefault(true)
    , _depthBiasEnabled(false)
    , _depthBiasConstantFactor(0.0f)
    , _depthBiasSlopeFactor(1.0f)
    , _depthFunc(HdCmpFuncLEqual)
    , _depthMaskEnabled(true)
    , _depthTestEnabled(true)
    , _depthClampEnabled(false)
    , _depthRange(GfVec2f(0, 1))
    , _cullStyle(HdCullStyleNothing)
    , _stencilFunc(HdCmpFuncAlways)
    , _stencilRef(0)
    , _stencilMask(~0)
    , _stencilFailOp(HdStencilOpKeep)
    , _stencilZFailOp(HdStencilOpKeep)
    , _stencilZPassOp(HdStencilOpKeep)
    , _stencilEnabled(false)
    , _lineWidth(1.0f)
    , _blendColorOp(HdBlendOpAdd)
    , _blendColorSrcFactor(HdBlendFactorOne)
    , _blendColorDstFactor(HdBlendFactorZero)
    , _blendAlphaOp(HdBlendOpAdd)
    , _blendAlphaSrcFactor(HdBlendFactorOne)
    , _blendAlphaDstFactor(HdBlendFactorZero)
    , _blendConstantColor(0.0f, 0.0f, 0.0f, 0.0f)
    , _blendEnabled(false)
    , _alphaToCoverageEnabled(false)
    , _colorMaskUseDefault(true)
    , _useMultiSampleAov(true)
    , _conservativeRasterizationEnabled(false)
    , _stepSize(0.f)
    , _stepSizeLighting(0.f)
    , _multiSampleEnabled(true)
{
}

HdRenderPassState::~HdRenderPassState() = default;

/* virtual */
void
HdRenderPassState::Prepare(HdResourceRegistrySharedPtr const &resourceRegistry)
{
}

void
HdRenderPassState::SetCamera(const HdCamera * const camera)
{
    _camera = camera;
}

void
HdRenderPassState::SetOverrideWindowPolicy(
    const std::optional<CameraUtilConformWindowPolicy> &overrideWindowPolicy)
{
    _overrideWindowPolicy = overrideWindowPolicy;
}

void
HdRenderPassState::SetViewport(const GfVec4d &viewport)
{
    _viewport = GfVec4f((float)viewport[0], (float)viewport[1],
                        (float)viewport[2], (float)viewport[3]);

    // Invalidate framing so that it isn't used by GetProjectionMatrix().
    _framing = CameraUtilFraming();
}    

void
HdRenderPassState::SetFraming(const CameraUtilFraming &framing)
{
    _framing = framing;
}

GfMatrix4d
HdRenderPassState::GetWorldToViewMatrix() const
{
    if (!_camera) {
        return GfMatrix4d(1.0);
    }
     
    return _camera->GetTransform().GetInverse();
}

CameraUtilConformWindowPolicy
HdRenderPassState::GetWindowPolicy() const
{
    if (_overrideWindowPolicy) {
        return *_overrideWindowPolicy;
    }
    if (_camera) {
        return _camera->GetWindowPolicy();
    }

    return CameraUtilFit;
}

GfMatrix4d
HdRenderPassState::GetProjectionMatrix() const
{
    if (!_camera) {
        return GfMatrix4d(1.0);
    }

    if (_framing.IsValid()) {
        return
            _framing.ApplyToProjectionMatrix(
                _camera->ComputeProjectionMatrix(),
                GetWindowPolicy());
    }

    const double aspect =
        (_viewport[3] != 0.0 ? _viewport[2] / _viewport[3] : 1.0);

    // Adjust the camera frustum based on the window policy.
    return CameraUtilConformedWindow(
        _camera->ComputeProjectionMatrix(), GetWindowPolicy(), aspect);
}

GfMatrix4d
HdRenderPassState::GetImageToWorldMatrix() const
{
    // Resolve the user-specified framing over the fallback viewport.
    GfRect2i vpRect;
    if (_framing.IsValid()) {
        vpRect = GfRect2i(
            GfVec2i(_framing.dataWindow.GetMinX(),
                    _framing.dataWindow.GetMinY()),
            _framing.dataWindow.GetWidth(),
            _framing.dataWindow.GetHeight());
    } else {
        vpRect = GfRect2i(GfVec2i(_viewport[0],_viewport[1]),
            _viewport[2], _viewport[3]);
    }

    // Tranform that maps NDC [-1,+1]x[-1,+1] to viewport
    // Note that z-coordinate is also transformed to map from [-1,+1]
    // and [0,+1]

    const GfVec3d viewportScale(
        vpRect.GetWidth()  / 2.0,
        vpRect.GetHeight() / 2.0,
        0.5);
    
    const GfVec3d viewportTranslate(
        vpRect.GetMinX() + vpRect.GetWidth()  / 2.0,
        vpRect.GetMinY()  + vpRect.GetHeight() / 2.0,
        0.5);
        
    const GfMatrix4d viewportTransform = 
        GfMatrix4d().SetScale(viewportScale) *
        GfMatrix4d().SetTranslate(viewportTranslate);

    GfMatrix4d worldToImage = GetWorldToViewMatrix() * GetProjectionMatrix() *
        viewportTransform;

    return worldToImage.GetInverse();
}

HdRenderPassState::ClipPlanesVector const &
HdRenderPassState::GetClipPlanes() const
{
    if (!(_clippingEnabled && _camera)) {
        const static HdRenderPassState::ClipPlanesVector empty;
        return empty;
    }

    return _camera->GetClipPlanes();
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
HdRenderPassState::SetMaskColor(GfVec4f const &color)
{
    _maskColor = color;
}

void
HdRenderPassState::SetIndicatorColor(GfVec4f const &color)
{
    _indicatorColor = color;
}

void
HdRenderPassState::SetPointColor(GfVec4f const &color)
{
    _pointColor = color;
}

void
HdRenderPassState::SetPointSize(float size)
{
    _pointSize = size;
}

void
HdRenderPassState::SetPointSelectedSize(float size)
{
    _pointSelectedSize = size;
}

void
HdRenderPassState::SetCullStyle(HdCullStyle cullStyle)
{
    _cullStyle = cullStyle;
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
HdRenderPassState::SetClippingEnabled(bool enabled)
{
    _clippingEnabled = enabled;
}

void
HdRenderPassState::SetAovBindings(
        HdRenderPassAovBindingVector const& aovBindings)
{
    _aovBindings= aovBindings;
}

HdRenderPassAovBindingVector const&
HdRenderPassState::GetAovBindings() const
{
    return _aovBindings;
}

void
HdRenderPassState::SetAovInputBindings(
        HdRenderPassAovBindingVector const& aovBindings)
{
    _aovInputBindings= aovBindings;
}

HdRenderPassAovBindingVector const&
HdRenderPassState::GetAovInputBindings() const
{
    return _aovInputBindings;
}

void
HdRenderPassState::SetUseAovMultiSample(bool state)
{
    _useMultiSampleAov = state;
}

bool
HdRenderPassState::GetUseAovMultiSample() const
{
    return _useMultiSampleAov;
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
HdRenderPassState::SetEnableDepthMask(bool state)
{
    _depthMaskEnabled = state;
}

bool
HdRenderPassState::GetEnableDepthMask() const
{
    return _depthMaskEnabled;
}

void
HdRenderPassState::SetEnableDepthTest(bool enabled)
{
    _depthTestEnabled = enabled;
}

bool
HdRenderPassState::GetEnableDepthTest() const
{
    return _depthTestEnabled;
}

void
HdRenderPassState::SetEnableDepthClamp(bool enabled)
{
    _depthClampEnabled = enabled;
}

bool
HdRenderPassState::GetEnableDepthClamp() const
{
    return _depthClampEnabled;
}

void
HdRenderPassState::SetDepthRange(GfVec2f const &depthRange)
{
    _depthRange = depthRange;
}

const GfVec2f&
HdRenderPassState::GetDepthRange() const
{
    return _depthRange;
}

void
HdRenderPassState::SetStencil(HdCompareFunction func,
        int ref, int mask,
        HdStencilOp fail, HdStencilOp zfail, HdStencilOp zpass)
{
    _stencilFunc = func;
    _stencilRef = ref;
    _stencilMask = mask;
    _stencilFailOp = fail;
    _stencilZFailOp = zfail;
    _stencilZPassOp = zpass;
}

void
HdRenderPassState::SetStencilEnabled(bool enabled)
{
    _stencilEnabled = enabled;
}

bool
HdRenderPassState::GetStencilEnabled() const
{
    return _stencilEnabled;
}

void
HdRenderPassState::SetLineWidth(float width)
{
    _lineWidth = width;
}

void
HdRenderPassState::SetBlend(HdBlendOp colorOp,
                            HdBlendFactor colorSrcFactor,
                            HdBlendFactor colorDstFactor,
                            HdBlendOp alphaOp,
                            HdBlendFactor alphaSrcFactor,
                            HdBlendFactor alphaDstFactor)
{
    _blendColorOp = colorOp;
    _blendColorSrcFactor = colorSrcFactor;
    _blendColorDstFactor = colorDstFactor;
    _blendAlphaOp = alphaOp;
    _blendAlphaSrcFactor = alphaSrcFactor;
    _blendAlphaDstFactor = alphaDstFactor;
}

void
HdRenderPassState::SetBlendConstantColor(GfVec4f const & color)
{
    _blendConstantColor = color;
}

void
HdRenderPassState::SetBlendEnabled(bool enabled)
{
    _blendEnabled = enabled;
}

void
HdRenderPassState::SetAlphaToCoverageEnabled(bool enabled)
{
    _alphaToCoverageEnabled = enabled;
}

void
HdRenderPassState::SetColorMaskUseDefault(bool useDefault)
{
    _colorMaskUseDefault = useDefault;
}

void
HdRenderPassState::SetConservativeRasterizationEnabled(bool enabled)
{
    _conservativeRasterizationEnabled = enabled;
}

void
HdRenderPassState::SetVolumeRenderingConstants(
    float stepSize, float stepSizeLighting)
{
    _stepSize = stepSize;
    _stepSizeLighting = stepSizeLighting;
}

void
HdRenderPassState::SetColorMasks(
    std::vector<HdRenderPassState::ColorMask> const& masks)
{
    _colorMasks = masks;
}

void
HdRenderPassState::SetMultiSampleEnabled(bool enabled)
{
    _multiSampleEnabled = enabled;
}

GfVec2f
HdRenderPassState::GetDrawingRangeNDC() const
{
    int width;
    int height;
    if (_framing.IsValid()) {
        width  = _framing.dataWindow.GetWidth();
        height = _framing.dataWindow.GetHeight();
    } else {
        width  = _viewport[2];
        height = _viewport[3];
    }
   
    return GfVec2f(2*_drawRange[0]/width,
                   2*_drawRange[1]/height);
}

PXR_NAMESPACE_CLOSE_SCOPE

