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

#include "pxr/pxr.h"
#include "pxr/imaging/hdx/api.h"
#include "pxr/imaging/hdx/version.h"
#include "pxr/imaging/hd/enums.h"
#include "pxr/imaging/hd/rprimCollection.h"
#include "pxr/imaging/hd/task.h"

#include "pxr/base/gf/vec2f.h"
#include "pxr/base/gf/vec4f.h"
#include "pxr/base/gf/vec4d.h"
#include "pxr/base/tf/declarePtrs.h"

#include <boost/shared_ptr.hpp>

PXR_NAMESPACE_OPEN_SCOPE

class GlfSimpleLight;
class HdRenderIndex;
class HdSceneDelegate;
class HdStRenderPassState;

typedef boost::shared_ptr<class HdStRenderPassShader> HdStRenderPassShaderSharedPtr;
typedef boost::shared_ptr<class HdStShaderCode> HdStShaderCodeSharedPtr;
typedef boost::shared_ptr<class HdRenderPassState> HdRenderPassStateSharedPtr;
typedef boost::shared_ptr<class HdRenderPass> HdRenderPassSharedPtr;
typedef std::vector<HdRenderPassStateSharedPtr> HdRenderPassStateSharedPtrVector;
typedef std::vector<HdRenderPassSharedPtr> HdRenderPassSharedPtrVector;

TF_DECLARE_WEAK_AND_REF_PTRS(GlfSimpleShadowArray);

struct HdxShadowTaskParams {
    HdxShadowTaskParams()
        : overrideColor(0.0)
        , wireframeColor(0.0)
        , enableLighting(false)
        , enableIdRender(false)
        , enableSceneMaterials(true)
        , alphaThreshold(0.0)
        , depthBiasEnable(false)
        , depthBiasConstantFactor(0.0f)
        , depthBiasSlopeFactor(1.0f)
        , depthFunc(HdCmpFuncLEqual)
        , cullStyle(HdCullStyleBackUnlessDoubleSided)
        , camera()
        , viewport(0.0)
        , lightIncludePaths(1, SdfPath::AbsoluteRootPath())
        , lightExcludePaths()
        {}

    // RenderPassState
    GfVec4f overrideColor;
    GfVec4f wireframeColor;
    bool enableLighting;
    bool enableIdRender;
    bool enableSceneMaterials;
    float alphaThreshold;
    bool  depthBiasEnable;
    float depthBiasConstantFactor;
    float depthBiasSlopeFactor;
    HdCompareFunction depthFunc;
    HdCullStyle cullStyle;

    // RenderPassState index objects
    SdfPath camera;
    GfVec4d viewport;

    // Lights/Shadows specific parameters
    SdfPathVector lightIncludePaths;
    SdfPathVector lightExcludePaths;
};

/// \class HdxShadowTask
///
/// A task for generating shadow maps.
///
class HdxShadowTask : public HdSceneTask {
public:
    HDX_API
    HdxShadowTask(HdSceneDelegate* delegate, SdfPath const& id);

    HDX_API
    virtual ~HdxShadowTask();

    /// Sync the render pass resources
    HDX_API
    virtual void Sync(HdTaskContext*   ctx) override;

    /// Execute render pass task
    HDX_API
    virtual void Execute(HdTaskContext* ctx) override;

private:
    void _SetHdStRenderPassState(HdxShadowTaskParams const &params,
        HdStRenderPassState *renderPassState);

    void _UpdateDirtyParams(HdRenderPassStateSharedPtr &renderPassState, 
        HdxShadowTaskParams const &params);

    static HdStShaderCodeSharedPtr _overrideShader;
    static void _CreateOverrideShader();

    HdRenderPassSharedPtrVector _passes;
    HdRenderPassStateSharedPtrVector _renderPassStates;
    HdxShadowTaskParams _params;

    HdxShadowTask() = delete;
    HdxShadowTask(const HdxShadowTask &) = delete;
    HdxShadowTask &operator =(const HdxShadowTask &) = delete;
};

// VtValue requirements
HDX_API
std::ostream& operator<<(std::ostream& out, const HdxShadowTaskParams& pv);
HDX_API
bool operator==(const HdxShadowTaskParams& lhs, const HdxShadowTaskParams& rhs);
HDX_API
bool operator!=(const HdxShadowTaskParams& lhs, const HdxShadowTaskParams& rhs);


PXR_NAMESPACE_CLOSE_SCOPE

#endif //HDX_SHADOW_TASK_H
