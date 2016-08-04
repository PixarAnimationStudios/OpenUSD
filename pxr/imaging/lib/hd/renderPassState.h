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
#ifndef HD_RENDER_PASS_STATE_H
#define HD_RENDER_PASS_STATE_H

#include "pxr/imaging/hd/api.h"
#include "pxr/imaging/hd/version.h"

#include "pxr/imaging/hd/bufferArrayRange.h"
#include "pxr/imaging/hd/enums.h"

#include "pxr/base/gf/matrix4d.h"
#include "pxr/base/gf/vec2f.h"
#include "pxr/base/gf/vec4d.h"

#include <boost/shared_ptr.hpp>

typedef boost::shared_ptr<class HdRenderPassState> HdRenderPassStateSharedPtr;
typedef boost::shared_ptr<class HdShader> HdShaderSharedPtr;
typedef boost::shared_ptr<class HdLightingShader> HdLightingShaderSharedPtr;
typedef boost::shared_ptr<class HdRenderPassShader>
                HdRenderPassShaderSharedPtr;
typedef boost::shared_ptr<class Hd_DefaultLightingShader>
                Hd_DefaultLightingShaderSharedPtr;
typedef std::vector<HdShaderSharedPtr> HdShaderSharedPtrVector;

/// HdRenderPassState is a set of rendering parameters used among
/// render passes. Parameters are expressed as GL states, uniforms
/// or shaders.
///
class HdRenderPassState {
public:
	HDLIB_API
    HdRenderPassState();
	HDLIB_API
    HdRenderPassState(HdRenderPassShaderSharedPtr const &shader);
	HDLIB_API
    virtual ~HdRenderPassState();

    /// Schedule to update renderPassState parameters.
    /// e.g. camera matrix, override color, id blend factor.
	HDLIB_API
    void Sync();

    /// temp.
    /// Set camera framing of this render pass state.
	HDLIB_API
    void SetCamera(GfMatrix4d const &worldToViewMatrix,
                   GfMatrix4d const &projectionMatrix,
                   GfVec4d const &viewport);

    /// Set additional clipping planes (defined in camera/view space).
    typedef std::vector<GfVec4d> ClipPlanesVector;
	HDLIB_API
    void SetClipPlanes(ClipPlanesVector const & clipPlanes);
	HDLIB_API
    ClipPlanesVector const & GetClipPlanes() const;

    /// Set an override color for rendering where the R, G and B components
    /// are the color and the alpha component is the blend value
	HDLIB_API
    void SetOverrideColor(GfVec4f const &color);
    const GfVec4f& GetOverrideColor() const { return _overrideColor; }

    /// Set a wireframe color for rendering where the R, G and B components
    /// are the color and the alpha component is the blend value
	HDLIB_API
    void SetWireframeColor(GfVec4f const &color);
    const GfVec4f& GetWireframeColor() const { return _wireframeColor; }

    /// temp.
    /// Hacky way of disabling lighting
	HDLIB_API
    void SetLightingEnabled(bool enabled);
    bool GetLightingEnabled() const { return _lightingEnabled; }

	HDLIB_API
    void SetCullStyle(HdCullStyle cullStyle);

	HDLIB_API
    void SetAlphaThreshold(float alphaThreshold);
    float GetAlphaThreshold() const { return _alphaThreshold; }

	HDLIB_API
    void SetTessLevel(float level);
    float GetTessLevel() const { return _tessLevel; }

	HDLIB_API
    void SetDrawingRange(GfVec2f const &drawRange);
    GfVec2f GetDrawingRange() const { return _drawRange; } // in pixel
    GfVec2f GetDrawingRangeNDC() const { // in ndc
        return GfVec2f(2*_drawRange[0]/_viewport[2],
                       2*_drawRange[1]/_viewport[3]);
    }

    /// Set lighting shader
	HDLIB_API
    void SetLightingShader(HdLightingShaderSharedPtr const &lightingShader);
    HdLightingShaderSharedPtr const & GetLightingShader() const {
        return _lightingShader;
    }

    /// renderpass shader
	HDLIB_API
    void SetRenderPassShader(HdRenderPassShaderSharedPtr const &renderPassShader);
    HdRenderPassShaderSharedPtr const &GetRenderPassShader() const {
        return _renderPassShader;
    }

    /// override shader
	HDLIB_API
    void SetOverrideShader(HdShaderSharedPtr const &overrideShader);
    HdShaderSharedPtr const &GetOverrideShader() const {
        return _overrideShader;
    }

    /// returns composed shader vectors. surfaceShader may be overriden.
	HDLIB_API
    HdShaderSharedPtrVector GetShaders(
        HdShaderSharedPtr const &surfaceShader) const;

    GfMatrix4d const &GetCullMatrix() const {
        return _cullMatrix;
    }

	HDLIB_API
    void SetDepthBiasUseDefault(bool useDefault);
    bool GetDepthBiasUseDefault() const { return _depthBiasUseDefault; }

	HDLIB_API
    void SetDepthBiasEnabled(bool enabled);
    bool GetDepthBiasEnabled() const { return _depthBiasEnabled; }

	HDLIB_API
    void SetDepthBias(float constantFactor, float slopeFactor);

	HDLIB_API
    void SetDepthFunc(HdCompareFunction depthFunc);
    HdCompareFunction GetDepthFunc() const { return _depthFunc; }

	HDLIB_API
    void SetAlphaToCoverageUseDefault(bool useDefault);
    bool GetAlphaToCoverageUseDefault() const { return _alphaToCoverageUseDefault; }

	HDLIB_API
    void SetAlphaToCoverageEnabled(bool enabled);
    bool GetAlphaToCoverageEnabled() const { return _alphaToCoverageEnabled; }

	HDLIB_API
    void Bind();
	HDLIB_API
    void Unbind();

	HDLIB_API
    size_t GetShaderHash() const;

private:
    // ---------------------------------------------------------------------- //
    // Shader Objects
    // ---------------------------------------------------------------------- //
    HdRenderPassShaderSharedPtr _renderPassShader;
    Hd_DefaultLightingShaderSharedPtr _defaultLightingShader;
    HdLightingShaderSharedPtr _lightingShader;
    HdShaderSharedPtr _overrideShader;

    // ---------------------------------------------------------------------- //
    // Camera State 
    // ---------------------------------------------------------------------- //
    GfMatrix4d _worldToViewMatrix;
    GfMatrix4d _projectionMatrix;
    GfVec4f _viewport;

    // TODO: This is only used for CPU culling, should compute it on the fly.
    GfMatrix4d _cullMatrix; 

    // ---------------------------------------------------------------------- //
    // Shader Uniform Inputs (should be generalized)
    // ---------------------------------------------------------------------- //
    HdBufferArrayRangeSharedPtr _renderPassStateBar;
    GfVec4f _overrideColor;
    GfVec4f _wireframeColor;
    bool _lightingEnabled;
    float _alphaThreshold;
    float _tessLevel;
    GfVec2f _drawRange;

    // Depth Bias RenderPassState
    // When use default is true - state
    // is inherited and onther values are
    // ignored.  Otherwise the raster state
    // is set using the values specified.
    bool _depthBiasUseDefault;
    bool _depthBiasEnabled;
    float _depthBiasConstantFactor;
    float _depthBiasSlopeFactor;
    HdCompareFunction _depthFunc;

    // alpha to coverage
    bool _alphaToCoverageUseDefault;
    bool _alphaToCoverageEnabled;

    ClipPlanesVector _clipPlanes;

};

#endif  // HD_RENDER_PASS_STATE_H
