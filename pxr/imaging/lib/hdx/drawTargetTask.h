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

#include "pxr/pxr.h"
#include "pxr/imaging/hdx/api.h"
#include "pxr/imaging/hdx/version.h"
#include "pxr/imaging/hdx/drawTargetRenderPass.h"

#include "pxr/imaging/hd/task.h"

#include "pxr/base/gf/vec2f.h"
#include "pxr/base/gf/vec4f.h"

PXR_NAMESPACE_OPEN_SCOPE

class HdStDrawTarget;


typedef std::unique_ptr<HdxDrawTargetRenderPass> HdxDrawTargetRenderPassUniquePtr;
typedef std::shared_ptr<class HdxSimpleLightingShader> HdxSimpleLightingShaderSharedPtr;

// Not strictly necessary here.
// But without it, would require users of the class to include it anyway

class HdxDrawTargetTask  : public HdSceneTask {
public:
    HDX_API
    HdxDrawTargetTask(HdSceneDelegate* delegate, SdfPath const& id);
    virtual ~HdxDrawTargetTask() = default;

protected:
    /// Sync the render pass resources
    HDX_API
    virtual void _Sync(HdTaskContext* ctx);

    /// Execute render pass task
    HDX_API
    virtual void _Execute(HdTaskContext* ctx);

private:
    struct RenderPassInfo {
        HdStRenderPassStateSharedPtr      renderPassState;
        HdxSimpleLightingShaderSharedPtr  simpleLightingShader;
        const HdStDrawTarget             *target;
        unsigned int                      version;
    };
    unsigned _currentDrawTargetSetVersion;

    typedef std::vector< RenderPassInfo > RenderPassInfoArray;
    RenderPassInfoArray _renderPassesInfo;
    std::vector< HdxDrawTargetRenderPassUniquePtr > _renderPasses;

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
        : overrideColor(0.0)
        , wireframeColor(0.0)
        , enableLighting(false)
        , alphaThreshold(0.0)
        , tessLevel(1.0)
        , drawingRange(0.9, -1.0)
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
HDX_API
std::ostream& operator<<(std::ostream& out, const HdxDrawTargetTaskParams& pv);
HDX_API
bool operator==(const HdxDrawTargetTaskParams& lhs, const HdxDrawTargetTaskParams& rhs);
HDX_API
bool operator!=(const HdxDrawTargetTaskParams& lhs, const HdxDrawTargetTaskParams& rhs);


PXR_NAMESPACE_CLOSE_SCOPE

#endif // HDX_DRAW_TARGET_TASK_H
