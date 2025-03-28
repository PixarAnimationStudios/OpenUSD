//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_IMAGING_HDX_DRAW_TARGET_TASK_H
#define PXR_IMAGING_HDX_DRAW_TARGET_TASK_H

#include "pxr/pxr.h"
#include "pxr/imaging/hdx/api.h"
#include "pxr/imaging/hdx/version.h"

#include "pxr/imaging/hd/task.h"

#include "pxr/base/gf/vec4f.h"
#include "pxr/base/tf/declarePtrs.h"

PXR_NAMESPACE_OPEN_SCOPE

class HdStDrawTarget;
class HdStDrawTargetRenderPassState;
struct HdxDrawTargetTaskParams;
using HdStRenderPassStateSharedPtr
    = std::shared_ptr<class HdStRenderPassState>;
using HdStSimpleLightingShaderSharedPtr
    = std::shared_ptr<class HdStSimpleLightingShader>;
TF_DECLARE_REF_PTRS(GlfSimpleLightingContext);

// Not strictly necessary here.
// But without it, would require users of the class to include it anyway

class HdxDrawTargetTask  : public HdTask
{
public:
    using TaskParams = HdxDrawTargetTaskParams;

    HDX_API
    HdxDrawTargetTask(HdSceneDelegate* delegate, SdfPath const& id);

    HDX_API
    ~HdxDrawTargetTask() override;

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
    struct _RenderPassInfo;
    struct _CameraInfo;
    using _RenderPassInfoVector = std::vector<_RenderPassInfo>;

    static _RenderPassInfoVector _ComputeRenderPassInfos(
        HdRenderIndex * renderIndex);

    static _CameraInfo _ComputeCameraInfo(
        const HdRenderIndex &renderIndex,
        const HdStDrawTarget * drawTarget);
    static void _UpdateLightingContext(
        const _CameraInfo &cameraInfo,
        GlfSimpleLightingContextConstRefPtr const &srcContext,
        GlfSimpleLightingContextRefPtr const &ctx);
    void _UpdateRenderPassState(
        const HdRenderIndex &renderIndex,
        const _CameraInfo &cameraInfo,
        HdStSimpleLightingShaderSharedPtr const &lightingShader,
        const HdStDrawTargetRenderPassState *srcState,
        HdStRenderPassStateSharedPtr const &state) const;
    static void _UpdateRenderPass(
        _RenderPassInfo *info);

    unsigned _currentDrawTargetSetVersion;
    _RenderPassInfoVector _renderPassesInfo;

    // Raster State - close match to render task
    // but doesn't have enableHardwareShading
    // as that has to be enabled for draw targets.
//    typedef std::vector<GfVec4d> ClipPlanesVector;
//    ClipPlanesVector _clipPlanes;
    GfVec4f _overrideColor;
    GfVec4f _wireframeColor;
    bool _enableLighting;
    float _alphaThreshold;

    /// Polygon Offset State
    bool _depthBiasUseDefault;
    bool _depthBiasEnable;
    float _depthBiasConstantFactor;
    float _depthBiasSlopeFactor;

    HdCompareFunction _depthFunc;

    // Viewer's Render Style
    HdCullStyle _cullStyle;

    // Alpha sample alpha to coverage
    bool _enableSampleAlphaToCoverage;
    TfTokenVector _renderTags;

    HdxDrawTargetTask() = delete;
    HdxDrawTargetTask(const HdxDrawTargetTask &) = delete;
    HdxDrawTargetTask &operator =(const HdxDrawTargetTask &) = delete;
};

struct HdxDrawTargetTaskParams
{
    HdxDrawTargetTaskParams()
        : overrideColor(0.0)
        , wireframeColor(0.0)
        , enableLighting(false)
        , alphaThreshold(0.0)
        , depthBiasUseDefault(true)
        , depthBiasEnable(false)
        , depthBiasConstantFactor(0.0f)
        , depthBiasSlopeFactor(1.0f)
        , depthFunc(HdCmpFuncLEqual)
        // XXX: When rendering draw targets we need alpha to coverage
        // at least until we support a transparency pass
        , enableAlphaToCoverage(true)
        , cullStyle(HdCullStyleBackUnlessDoubleSided)
        {}

//    ClipPlanesVector clipPlanes;
    GfVec4f overrideColor;
    GfVec4f wireframeColor;
    bool enableLighting;
    float alphaThreshold;

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

    bool enableAlphaToCoverage;

    // Viewer's Render Style
    HdCullStyle cullStyle;
};

// VtValue requirements
HDX_API
std::ostream& operator<<(std::ostream& out, const HdxDrawTargetTaskParams& pv);
HDX_API
bool operator==(
    const HdxDrawTargetTaskParams& lhs, 
    const HdxDrawTargetTaskParams& rhs);
HDX_API
bool operator!=(
    const HdxDrawTargetTaskParams& lhs, 
    const HdxDrawTargetTaskParams& rhs);

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_IMAGING_HDX_DRAW_TARGET_TASK_H
