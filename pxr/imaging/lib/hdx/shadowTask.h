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
#ifndef HDX_SHADOW_TASK_H
#define HDX_SHADOW_TASK_H

#include "pxr/imaging/hdx/version.h"
#include "pxr/imaging/hd/enums.h"
#include "pxr/imaging/hd/rprimCollection.h"
#include "pxr/imaging/hd/task.h"

#include "pxr/base/gf/vec2f.h"
#include "pxr/base/gf/vec4f.h"
#include "pxr/base/gf/vec4d.h"

#include <boost/shared_ptr.hpp>

class HdRenderIndex;
class HdSceneDelegate;
class GlfSimpleLight;

typedef boost::shared_ptr<class HdRenderPassState> HdRenderPassStateSharedPtr;
typedef boost::shared_ptr<class HdRenderPass> HdRenderPassSharedPtr;
typedef boost::shared_ptr<class HdSimpleLightingShader> HdSimpleLightingShaderSharedPtr;
typedef std::vector<HdRenderPassStateSharedPtr> HdRenderPassStateSharedPtrVector;
typedef std::vector<HdRenderPassSharedPtr> HdRenderPassSharedPtrVector;

TF_DECLARE_WEAK_AND_REF_PTRS(GlfSimpleShadowArray);

/// A task for generating shadow maps.
///
class HdxShadowTask : public HdSceneTask {
public:
    HdxShadowTask(HdSceneDelegate* delegate, SdfPath const& id);

protected:
    /// Execute render pass task
    virtual void _Execute(HdTaskContext* ctx);

    /// Sync the render pass resources
    virtual void _Sync(HdTaskContext* ctx);

private:
    HdRenderPassSharedPtrVector _passes;
    HdRenderPassStateSharedPtrVector _renderPassStates;
    int _collectionVersion;
    bool _enableShadows;

    /// Polygon Offset State
    bool _depthBiasEnable;
    float _depthBiasConstantFactor;
    float _depthBiasSlopeFactor;

    HdCompareFunction _depthFunc;
};

struct HdxShadowTaskParams : public HdTaskParams
{
    HdxShadowTaskParams()
        : overrideColor(0.0)
        , wireframeColor(0.0)
        , enableLighting(false)
        , enableIdRender(false)
        , alphaThreshold(0.0)
        , tessLevel(1.0)
        , drawingRange(0.0, -1.0)
        , depthBiasEnable(false)
        , depthBiasConstantFactor(0.0f)
        , depthBiasSlopeFactor(1.0f)
        , depthFunc(HdCmpFuncLEqual)
        , cullStyle(HdCullStyleBackUnlessDoubleSided)
        , complexity(HdComplexityLow)
        , camera()
        , viewport(0.0)
        , lightIncludePaths(1, SdfPath::AbsoluteRootPath())
        , lightExcludePaths()
        , enableShadows(false)
        {}

    // RenderPassState
    GfVec4f overrideColor;
    GfVec4f wireframeColor;
    bool enableLighting;
    bool enableIdRender;
    float alphaThreshold;
    float tessLevel;
    GfVec2f drawingRange;
    bool  depthBiasEnable;
    float depthBiasConstantFactor;
    float depthBiasSlopeFactor;
    HdCompareFunction depthFunc;
    HdCullStyle cullStyle;
    HdComplexity complexity;

    // RenderPassState index objects
    SdfPath camera;
    GfVec4d viewport;

    // Lights/Shadows specific paramenters
    SdfPathVector lightIncludePaths;
    SdfPathVector lightExcludePaths;
    bool enableShadows;
};

// VtValue requirements
std::ostream& operator<<(std::ostream& out, const HdxShadowTaskParams& pv);
bool operator==(const HdxShadowTaskParams& lhs, const HdxShadowTaskParams& rhs);
bool operator!=(const HdxShadowTaskParams& lhs, const HdxShadowTaskParams& rhs);

#endif //HDX_SHADOW_TASK_H
