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
#ifndef HDX_DRAW_TARGET_TASK_H
#define HDX_DRAW_TARGET_TASK_H

#include "pxr/imaging/hdx/version.h"

#include "pxr/imaging/hd/task.h"
#include "pxr/imaging/hd/drawTargetRenderPass.h"
#include "pxr/imaging/hd/simpleLightingShader.h"

#include "pxr/base/gf/vec2f.h"
#include "pxr/base/gf/vec4f.h"

typedef boost::weak_ptr<class HdDrawTarget> HdDrawTargetWeakPtr;
typedef std::unique_ptr<HdDrawTargetRenderPass> HdDrawTargetRenderPassUniquePtr;
typedef boost::shared_ptr<class HdxSimpleLightingShader> HdxSimpleLightingShaderSharedPtr;

// Not strictly necessary here.
// But without it, would require users of the class to include it anyway

class HdxDrawTargetTask  : public HdSceneTask {
public:
    HdxDrawTargetTask(HdSceneDelegate* delegate, SdfPath const& id);
    virtual ~HdxDrawTargetTask() = default;

protected:
    /// Sync the render pass resources
    virtual void _Sync(HdTaskContext* ctx);

    /// Execute render pass task
    virtual void _Execute(HdTaskContext* ctx);

private:
    // Uses unique_ptr for pass to avoid copy constructor
    // use by std::vector::reserve().

    struct RenderPassInfo {
        HdDrawTargetRenderPassUniquePtr  pass;
        HdRenderPassStateSharedPtr       renderPassState;
        HdxSimpleLightingShaderSharedPtr simpleLightingShader;
        HdDrawTargetWeakPtr              target;
        unsigned int                     version;
    };

    bool     _enableDrawTargets;
    unsigned _currentDrawTargetSetVersion;


    typedef std::vector< RenderPassInfo > RenderPassArray;
    RenderPassArray _renderPasses;

    // Raster State - close match to render task
    // but doesn't have enableHardwareShading
    // as that has to be enabled for draw targets.
//    typedef std::vector<GfVec4d> ClipPlanesVector;
//    ClipPlanesVector _clipPlanes;
    GfVec4f _overrideColor;
    GfVec4f _wireframeColor;
    bool _enableLighting;
    float _alphaThreshold;
    float _tessLevel;
    GfVec2f _drawingRange;


    /// Polygon Offset State
    bool _depthBiasUseDefault;
    bool _depthBiasEnable;
    float _depthBiasConstantFactor;
    float _depthBiasSlopeFactor;

    HdCompareFunction _depthFunc;

    // Viewer's Render Style
    HdCullStyle _cullStyle;
    HdGeomStyle _geomStyle;
    HdComplexity _complexity;
    bool _hullVisibility;
    bool _surfaceVisibility;


    HdxDrawTargetTask()                                      = delete;
    HdxDrawTargetTask(const HdxDrawTargetTask &)             = delete;
    HdxDrawTargetTask &operator =(const HdxDrawTargetTask &) = delete;
};

struct HdxDrawTargetTaskParams
{
    HdxDrawTargetTaskParams()
        : enableDrawTargets(false)
        , overrideColor(0.0)
        , wireframeColor(0.0)
        , enableLighting(false)
        , alphaThreshold(0.0)
        , tessLevel(1.0)
        , drawingRange(0.0)
        , depthBiasUseDefault(true)
        , depthBiasEnable(false)
        , depthBiasConstantFactor(0.0f)
        , depthBiasSlopeFactor(1.0f)
        , depthFunc(HdCmpFuncLEqual)
        , cullStyle(HdCullStyleBackUnlessDoubleSided)
        , geomStyle(HdGeomStylePolygons)
        , complexity(HdComplexityLow)
        , hullVisibility(false)
        , surfaceVisibility(true)
        {}

    bool enableDrawTargets;

//    ClipPlanesVector clipPlanes;
    GfVec4f overrideColor;
    GfVec4f wireframeColor;
    bool enableLighting;
    float alphaThreshold;
    float tessLevel;
    GfVec2f drawingRange;

    // Depth Bias Raster State
    // When use default is true - state
    // is inherited and onther values are
    // ignored.  Otherwise the raster state
    // is set using the values specified.

    bool depthBiasUseDefault;
    bool depthBiasEnable;
    float depthBiasConstantFactor;
    float depthBiasSlopeFactor;

    HdCompareFunction depthFunc;

    // Viewer's Render Style
    HdCullStyle cullStyle;
    HdGeomStyle geomStyle;
    HdComplexity complexity;
    bool hullVisibility;
    bool surfaceVisibility;

};

// VtValue requirements
std::ostream& operator<<(std::ostream& out, const HdxDrawTargetTaskParams& pv);
bool operator==(const HdxDrawTargetTaskParams& lhs, const HdxDrawTargetTaskParams& rhs);
bool operator!=(const HdxDrawTargetTaskParams& lhs, const HdxDrawTargetTaskParams& rhs);

#endif // HDX_DRAW_TARGET_TASK_H
