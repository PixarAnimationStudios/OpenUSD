//
// Copyright 2021 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef HDX_SKYDOME_TASK_H
#define HDX_SKYDOME_TASK_H

#include "pxr/pxr.h"
#include "pxr/imaging/hdx/api.h"
#include "pxr/imaging/hdx/renderSetupTask.h"
#include "pxr/imaging/hdx/task.h"

#include "pxr/base/gf/matrix4f.h"
#include "pxr/base/gf/vec2f.h"
#include "pxr/usd/sdf/path.h"
#include "pxr/imaging/hgi/graphicsCmds.h"

#include "pxr/imaging/glf/simpleLightingContext.h"

PXR_NAMESPACE_OPEN_SCOPE

/// \class HdxSkydomeTask
///
/// If we have a domelight present in the lighting context the SkydomeTask
/// will render the associated environment map as a Skydome. Otherwise, it
/// will simply clear the AOVs.
///
/// Note that this task is intended to be the first "Render Task" in the
/// HdxTaskController so that the AOV's are properly cleared, however it 
/// does not spawn a HdRenderPass. 
///
class HdxSkydomeTask : public HdxTask
{
public:
    using TaskParams = HdxRenderTaskParams;
    
    HDX_API
    HdxSkydomeTask(HdSceneDelegate* delegate, SdfPath const& id);

    HDX_API
    ~HdxSkydomeTask() override;

    /// Prepare the tasks resources
    HDX_API
    void Prepare(HdTaskContext* ctx,
                 HdRenderIndex* renderIndex) override;

    /// Execute render pass task
    HDX_API
    void Execute(HdTaskContext* ctx) override;

protected:
    /// Sync the render pass resources
    HDX_API
    void _Sync(HdSceneDelegate* delegate,
              HdTaskContext* ctx,
              HdDirtyBits* dirtyBits) override;

private:
    HdRenderIndex* _renderIndex;
    HgiTextureHandle _skydomeTexture;
    HgiSamplerHandle _skydomeSampler;
    // Optional internal render setup task, for params unpacking.
    // This is used for aov bindings, camera matrices and framing
    HdxRenderSetupTaskSharedPtr _setupTask;
    unsigned int _settingsVersion;
    bool _skydomeVisibility;

    HdxSkydomeTask() = delete;
    HdxSkydomeTask(const HdxSkydomeTask &) = delete;
    HdxSkydomeTask &operator =(const HdxSkydomeTask &) = delete;

    HdRenderPassStateSharedPtr _GetRenderPassState(HdTaskContext *ctx) const;
    bool _GetSkydomeTexture(HdTaskContext* ctx);
    void _SetFragmentShader();

    // Utility function to update the shader uniform parameters.
    // Returns true if the values were updated. False if unchanged.
    bool _UpdateParameterBuffer(
        const GfMatrix4f& invProjMatrix,
        const GfMatrix4f& viewToWorldMatrix,
        const GfMatrix4f& lightTransform);

    // This struct must match ParameterBuffer in Skydome.glslfx.
    // Be careful to remember the std430 rules.
    struct _ParameterBuffer 
    {
        GfMatrix4f invProjMatrix;
        GfMatrix4f viewToWorldMatrix;
        GfMatrix4f lightTransform;
    };

    std::unique_ptr<class HdxFullscreenShader> _compositor;
    _ParameterBuffer _parameterData;
};


PXR_NAMESPACE_CLOSE_SCOPE

#endif
