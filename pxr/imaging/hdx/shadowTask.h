//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_IMAGING_HDX_SHADOW_TASK_H
#define PXR_IMAGING_HDX_SHADOW_TASK_H

#include "pxr/pxr.h"
#include "pxr/imaging/hdx/api.h"
#include "pxr/imaging/hdx/version.h"
#include "pxr/imaging/hd/enums.h"
#include "pxr/imaging/hd/task.h"

#include "pxr/base/gf/vec4f.h"
#include "pxr/base/gf/vec4d.h"
#include "pxr/base/tf/declarePtrs.h"

#include <memory>

PXR_NAMESPACE_OPEN_SCOPE

class GlfSimpleLight;
class HdRenderIndex;
class HdSceneDelegate;
class HdStRenderPassState;

using HdStRenderPassShaderSharedPtr =
    std::shared_ptr<class HdStRenderPassShader>;
using HdStShaderCodeSharedPtr =
    std::shared_ptr<class HdStShaderCode>;

using HdRenderPassSharedPtr = std::shared_ptr<class HdRenderPass>;
using HdRenderPassSharedPtrVector = std::vector<HdRenderPassSharedPtr>;
using HdStRenderPassStateSharedPtr = std::shared_ptr<class HdStRenderPassState>;
using HdStRenderPassStateSharedPtrVector = 
    std::vector<HdStRenderPassStateSharedPtr>;

TF_DECLARE_WEAK_AND_REF_PTRS(GlfSimpleShadowArray);

struct HdxShadowTaskParams
{
    HdxShadowTaskParams()
        : overrideColor(0.0)
        , wireframeColor(0.0)
        , enableLighting(false)
        , enableSceneMaterials(true)
        , alphaThreshold(0.0)
        , depthBiasEnable(false)
        , depthBiasConstantFactor(0.0f)
        , depthBiasSlopeFactor(1.0f)
        , depthFunc(HdCmpFuncLEqual)
        , cullStyle(HdCullStyleBackUnlessDoubleSided)
        {}

    // RenderPassState
    GfVec4f overrideColor;
    GfVec4f wireframeColor;
    bool enableLighting;
    bool enableSceneMaterials;
    float alphaThreshold;
    bool  depthBiasEnable;
    float depthBiasConstantFactor;
    float depthBiasSlopeFactor;
    HdCompareFunction depthFunc;
    HdCullStyle cullStyle;
};

/// \class HdxShadowTask
///
/// A task for generating shadow maps.
///
class HdxShadowTask : public HdTask
{
public:
    using TaskParams = HdxShadowTaskParams;

    HDX_API
    HdxShadowTask(HdSceneDelegate* delegate, SdfPath const& id);

    HDX_API
    ~HdxShadowTask() override;

    /// Sync the render pass resources
    HDX_API
    void Sync(HdSceneDelegate* delegate,
              HdTaskContext* ctx,
              HdDirtyBits* dirtyBits) override;
    
    /// Prepare the tasks resources
    HDX_API
    void Prepare(HdTaskContext* ctx,
                 HdRenderIndex* renderIndex) override;

    /// Execute render pass task
    HDX_API
    void Execute(HdTaskContext* ctx) override;

    /// Collect Render Tags used by the task.
    HDX_API
    const TfTokenVector &GetRenderTags() const override;


private:
    void _UpdateDirtyParams(HdStRenderPassStateSharedPtr &renderPassState, 
        HdxShadowTaskParams const &params);

    HdRenderPassSharedPtrVector _passes;
    HdStRenderPassStateSharedPtrVector _renderPassStates;
    HdxShadowTaskParams _params;
    TfTokenVector       _renderTags;


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

#endif //PXR_IMAGING_HDX_SHADOW_TASK_H
