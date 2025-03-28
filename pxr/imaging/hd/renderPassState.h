//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_IMAGING_HD_RENDER_PASS_STATE_H
#define PXR_IMAGING_HD_RENDER_PASS_STATE_H

#include "pxr/pxr.h"
#include "pxr/imaging/hd/api.h"
#include "pxr/imaging/hd/aov.h"
#include "pxr/imaging/hd/version.h"
#include "pxr/imaging/hd/enums.h"

#include "pxr/imaging/cameraUtil/framing.h"

#include "pxr/usd/sdf/path.h"

#include "pxr/base/tf/token.h"
#include "pxr/base/vt/value.h"
#include "pxr/base/gf/matrix4d.h"
#include "pxr/base/gf/vec2f.h"
#include "pxr/base/gf/vec4d.h"
#include "pxr/base/gf/vec4f.h"

#include <memory>

#include <optional>

PXR_NAMESPACE_OPEN_SCOPE


using HdRenderPassStateSharedPtr = std::shared_ptr<class HdRenderPassState>;
using HdResourceRegistrySharedPtr = std::shared_ptr<class HdResourceRegistry>;

class HdCamera;

/// \class HdRenderPassState
///
/// A set of rendering parameters used among render passes.
///
/// Parameters are expressed as GL states, uniforms or shaders.
///
class HdRenderPassState
{
public:
    HD_API
    HdRenderPassState();
    HD_API
    virtual ~HdRenderPassState();

    /// Schedule to update renderPassState parameters.
    /// e.g. camera matrix, override color, id blend factor.
    /// Prepare, called once per frame after the sync phase, but prior to
    /// the commit phase.
    HD_API
    virtual void Prepare(HdResourceRegistrySharedPtr const &resourceRegistry);

    // ---------------------------------------------------------------------- //
    /// \name Camera and framing state
    // ---------------------------------------------------------------------- //

    using ClipPlanesVector = std::vector<GfVec4d>;

    /// Sets the camera.
    HD_API
    void SetCamera(const HdCamera *camera);

    /// Sets whether to override the window policy used to conform the camera
    /// if its aspect ratio is not matching the display window/viewport.
    /// If first value is false, the HdCamera's window policy is used.
    ///
    HD_API
    void SetOverrideWindowPolicy(
        const std::optional<CameraUtilConformWindowPolicy> &
            overrideWindowPolicy);

    /// Sets the framing to show the camera. If a valid framing is set, a
    /// viewport set earlier with SetViewport will be ignored.
    HD_API
    void SetFraming(const CameraUtilFraming &framing);

    /// Sets the viewport to show the camera. If SetViewport is called,
    /// any framing set earlier with SetFraming will be ignored.
    ///
    /// \deprecated Use the more expressive SetFraming instead.
    HD_API
    void SetViewport(const GfVec4d &viewport);

    /// Get camera
    HdCamera const *
    GetCamera() const { return _camera; }

    /// Get framing information determining how the filmback plane maps
    /// to pixels.
    const CameraUtilFraming &
    GetFraming() const { return _framing; }

    /// The override value for the window policy to conform the camera 
    /// frustum that can be specified by the application.
    const std::optional<CameraUtilConformWindowPolicy> &
    GetOverrideWindowPolicy() const { return _overrideWindowPolicy; }

    /// The resolved window policy to conform the camera frustum.
    /// This is either the override value specified by the application or
    /// the value from the scene delegate's camera.
    HD_API
    CameraUtilConformWindowPolicy
    GetWindowPolicy() const;

    /// Camera getter API
    ///
    /// Returns inverse of HdCamera's transform.
    ///
    HD_API
    virtual GfMatrix4d GetWorldToViewMatrix() const;

    /// Compute projection matrix using physical attributes of an HdCamera.
    ///
    HD_API
    virtual GfMatrix4d GetProjectionMatrix() const;

    /// Only use when clients did not specify a camera framing.
    ///
    /// \deprecated
    GfVec4f const & GetViewport() const { return _viewport; }

    /// Compute a transform from window relative coordinates (x,y,z,1) to
    /// homogeneous world coordinates (x,y,z,w), using the HdCamera's 
    /// attributes, framing, and viewport dimensions.
    /// 
    HD_API
    GfMatrix4d GetImageToWorldMatrix() const;

    /// Returns HdCamera's clip planes.
    ///
    HD_API
    virtual ClipPlanesVector const & GetClipPlanes() const;

    // ---------------------------------------------------------------------- //
    /// \name Application rendering state
    // ---------------------------------------------------------------------- //

    /// Set an override color for rendering where the R, G and B components
    /// are the color and the alpha component is the blend value
    /// The color is specified in the render color space
    HD_API
    void SetOverrideColor(GfVec4f const &color);
    const GfVec4f& GetOverrideColor() const { return _overrideColor; }

    /// Set a wireframe color for rendering where the R, G and B components
    /// are the color and the alpha component is the blend value
    /// The color is specified in the render color space
    HD_API
    void SetWireframeColor(GfVec4f const &color);
    const GfVec4f& GetWireframeColor() const { return _wireframeColor; }

    /// The color is specified in the render color space
    HD_API
    void SetMaskColor(GfVec4f const &color);
    const GfVec4f& GetMaskColor() const { return _maskColor; }

    /// The color is specified in the render color space
    HD_API
    void SetIndicatorColor(GfVec4f const &color);
    const GfVec4f& GetIndicatorColor() const { return _indicatorColor; }

    /// Set a point color for rendering where the R, G and B components
    /// are the color and the alpha component is the blend value
    /// The color is specified in the render color space
    HD_API
    void SetPointColor(GfVec4f const &color);
    const GfVec4f& GetPointColor() const { return _pointColor; }

    /// Set the point size for unselected points.
    HD_API
    void SetPointSize(float size);
    float GetPointSize() const { return _pointSize; }

    /// Set the point size for selected points.
    HD_API
    void SetPointSelectedSize(float size);
    float GetPointSelectedSize() const { return _pointSelectedSize; }

    /// XXX: Hacky way of disabling lighting
    HD_API
    void SetLightingEnabled(bool enabled);
    bool GetLightingEnabled() const { return _lightingEnabled; }

    HD_API
    void SetClippingEnabled(bool enabled);
    bool GetClippingEnabled() const { return _clippingEnabled; }

    // ---------------------------------------------------------------------- //
    /// \name Render pipeline state
    // ---------------------------------------------------------------------- //

    /// Set the attachments for this renderpass to render into.
    HD_API
    void SetAovBindings(HdRenderPassAovBindingVector const &aovBindings);
    HD_API
    HdRenderPassAovBindingVector const& GetAovBindings() const;

    /// Set the AOVs that this renderpass needs to read from.
    HD_API
    void SetAovInputBindings(HdRenderPassAovBindingVector const &aovBindings);
    HD_API
    HdRenderPassAovBindingVector const& GetAovInputBindings() const;

    /// Returns true if the render pass wants to render into the multi-sample
    /// aovs. Returns false if the render wants to render into the resolve aovs.
    HD_API
    void SetUseAovMultiSample(bool state);
    HD_API
    bool GetUseAovMultiSample() const;

    HD_API
    void SetCullStyle(HdCullStyle cullStyle);
    HD_API
    HdCullStyle GetCullStyle() const { return _cullStyle; }

    HD_API
    void SetAlphaThreshold(float alphaThreshold);
    float GetAlphaThreshold() const { return _alphaThreshold; }

    HD_API
    void SetTessLevel(float level);
    float GetTessLevel() const { return _tessLevel; }

    HD_API
    void SetDrawingRange(GfVec2f const &drawRange);
    GfVec2f GetDrawingRange() const { return _drawRange; } // in pixel
    HD_API
    GfVec2f GetDrawingRangeNDC() const; // in ndc

    HD_API
    void SetDepthBiasUseDefault(bool useDefault);
    bool GetDepthBiasUseDefault() const { return _depthBiasUseDefault; }

    HD_API
    void SetDepthBiasEnabled(bool enabled);
    bool GetDepthBiasEnabled() const { return _depthBiasEnabled; }

    HD_API
    void SetDepthBias(float constantFactor, float slopeFactor);

    HD_API
    void SetDepthFunc(HdCompareFunction depthFunc);
    HdCompareFunction GetDepthFunc() const { return _depthFunc; }

    HD_API
    void SetEnableDepthMask(bool state);
    HD_API
    bool GetEnableDepthMask() const;

    HD_API
    void SetEnableDepthTest(bool enabled);
    HD_API
    bool GetEnableDepthTest() const;

    HD_API
    void SetEnableDepthClamp(bool enabled);
    HD_API
    bool GetEnableDepthClamp() const;

    HD_API
    void SetDepthRange(GfVec2f const &depthRange);
    HD_API
    const GfVec2f& GetDepthRange() const;

    HD_API
    void SetStencil(HdCompareFunction func, int ref, int mask,
                    HdStencilOp fail, HdStencilOp zfail, HdStencilOp zpass);
    HdCompareFunction GetStencilFunc() const { return _stencilFunc; }
    int GetStencilRef() const { return _stencilRef; }
    int GetStencilMask() const { return _stencilMask; }
    HdStencilOp GetStencilFailOp() const { return _stencilFailOp; }
    HdStencilOp GetStencilDepthFailOp() const { return _stencilZFailOp; }
    HdStencilOp GetStencilDepthPassOp() const { return _stencilZPassOp; }
    HD_API
    void SetStencilEnabled(bool enabled);
    HD_API
    bool GetStencilEnabled() const;
    
    HD_API
    void SetLineWidth(float width);
    float GetLineWidth() const { return _lineWidth; }
    
    HD_API
    void SetBlend(HdBlendOp colorOp,
                  HdBlendFactor colorSrcFactor,
                  HdBlendFactor colorDstFactor,
                  HdBlendOp alphaOp,
                  HdBlendFactor alphaSrcFactor,
                  HdBlendFactor alphaDstFactor);
    HdBlendOp GetBlendColorOp() { return _blendColorOp; }
    HdBlendFactor GetBlendColorSrcFactor() { return _blendColorSrcFactor; }
    HdBlendFactor GetBlendColorDstFactor() { return _blendColorDstFactor; }
    HdBlendOp GetBlendAlphaOp() { return _blendAlphaOp; }
    HdBlendFactor GetBlendAlphaSrcFactor() { return _blendAlphaSrcFactor; }
    HdBlendFactor GetBlendAlphaDstFactor() { return _blendAlphaDstFactor; }

    // Blend constant color is specified in the render color space
    HD_API
    void SetBlendConstantColor(GfVec4f const & color);
    const GfVec4f& GetBlendConstantColor() const { return _blendConstantColor; }
    HD_API
    void SetBlendEnabled(bool enabled);

    HD_API
    void SetAlphaToCoverageEnabled(bool enabled);
    bool GetAlphaToCoverageEnabled() const { return _alphaToCoverageEnabled; }

    HD_API
    void SetColorMaskUseDefault(bool useDefault);
    bool GetColorMaskUseDefault() const { return _colorMaskUseDefault;}

    HD_API
    void SetConservativeRasterizationEnabled(bool enabled);
    bool GetConservativeRasterizationEnabled() const {
        return _conservativeRasterizationEnabled;
    }

    HD_API
    void SetVolumeRenderingConstants(float stepSize, float stepSizeLighting);

    enum ColorMask {
        ColorMaskNone,
        ColorMaskRGB,
        ColorMaskRGBA
    };

    HD_API
    void SetColorMasks(std::vector<ColorMask> const& masks);
    std::vector<ColorMask> const& GetColorMasks() const { return _colorMasks; }

    HD_API
    void SetMultiSampleEnabled(bool enabled);
    bool GetMultiSampleEnabled() const { return _multiSampleEnabled; }

protected:
    // ---------------------------------------------------------------------- //
    // Camera and framing state 
    // ---------------------------------------------------------------------- //
    HdCamera const *_camera;
    GfVec4f _viewport;
    CameraUtilFraming _framing;
    std::optional<CameraUtilConformWindowPolicy> _overrideWindowPolicy;

    // ---------------------------------------------------------------------- //
    // Application rendering state
    // ---------------------------------------------------------------------- //
    GfVec4f _overrideColor;
    GfVec4f _wireframeColor;
    GfVec4f _pointColor;
    float _pointSize;
    bool _lightingEnabled;
    bool _clippingEnabled;

    GfVec4f _maskColor;
    GfVec4f _indicatorColor;
    float _pointSelectedSize;

    // ---------------------------------------------------------------------- //
    // Render pipeline state
    // ---------------------------------------------------------------------- //
    float _alphaThreshold;
    float _tessLevel;
    GfVec2f _drawRange;

    bool _depthBiasUseDefault; // inherit existing state, ignore values below.
    bool _depthBiasEnabled;
    float _depthBiasConstantFactor;
    float _depthBiasSlopeFactor;
    HdCompareFunction _depthFunc;
    bool _depthMaskEnabled;
    bool _depthTestEnabled;
    bool _depthClampEnabled;
    GfVec2f _depthRange;

    HdCullStyle _cullStyle;

    // Stencil RenderPassState
    HdCompareFunction _stencilFunc;
    int _stencilRef;
    int _stencilMask;
    HdStencilOp _stencilFailOp;
    HdStencilOp _stencilZFailOp;
    HdStencilOp _stencilZPassOp;
    bool _stencilEnabled;
    
    // Line width
    float _lineWidth;
    
    // Blending
    HdBlendOp _blendColorOp;
    HdBlendFactor _blendColorSrcFactor;
    HdBlendFactor _blendColorDstFactor;
    HdBlendOp _blendAlphaOp;
    HdBlendFactor _blendAlphaSrcFactor;
    HdBlendFactor _blendAlphaDstFactor;
    GfVec4f _blendConstantColor;
    bool _blendEnabled;

    // alpha to coverage
    bool _alphaToCoverageEnabled;

    bool _colorMaskUseDefault;
    std::vector<ColorMask> _colorMasks;

    HdRenderPassAovBindingVector _aovBindings;
    HdRenderPassAovBindingVector _aovInputBindings;
    bool _useMultiSampleAov;

    bool _conservativeRasterizationEnabled;

    float _stepSize;
    float _stepSizeLighting;

    bool _multiSampleEnabled;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif  // PXR_IMAGING_HD_RENDER_PASS_STATE_H
