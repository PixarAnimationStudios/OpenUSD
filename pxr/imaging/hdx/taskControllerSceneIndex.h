//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_IMAGING_HDX_TASK_CONTROLLER_SCENE_INDEX_H
#define PXR_IMAGING_HDX_TASK_CONTROLLER_SCENE_INDEX_H

#include "pxr/pxr.h"

#include "pxr/imaging/hdx/api.h"
#include "pxr/imaging/hdx/boundingBoxTask.h"
#include "pxr/imaging/hdx/colorCorrectionTask.h"
#include "pxr/imaging/hdx/renderSetupTask.h"
#include "pxr/imaging/hdx/shadowTask.h"

#include "pxr/imaging/hd/aov.h"
#include "pxr/imaging/hd/renderIndex.h"
#include "pxr/imaging/hd/sceneIndex.h"
#include "pxr/imaging/hd/task.h"

#include "pxr/imaging/cameraUtil/framing.h"
#include "pxr/imaging/glf/simpleLightingContext.h"

#include "pxr/base/gf/matrix4d.h"

PXR_NAMESPACE_OPEN_SCOPE

HDX_API
extern TfEnvSetting<int> HDX_MSAA_SAMPLE_COUNT;

TF_DECLARE_REF_PTRS(HdRetainedSceneIndex);
TF_DECLARE_REF_PTRS(HdxTaskControllerSceneIndex);

/// \class HdxTaskControllerSceneIndex
///
/// Manages tasks necessary to render an image (or perform picking)
/// as well as the related render buffers, lights and a free camera.
///
/// Note that the set of necessary tasks is different for Storm and other
/// renderers. Thus, the c'tor needs to be given the renderer plugin name.
///
/// It is a Hydra 2.0 implementation replacing the HdxTaskController.
/// For now, the API and behavior is the same as that of the
/// HdxTaskController.
///
// XXX: This API is transitional. At the least, render/picking/selection
// APIs should be decoupled.
//
class HdxTaskControllerSceneIndex final : public HdSceneIndexBase
{
public:
    using AovDescriptorCallback =
        std::function<HdAovDescriptor(const TfToken &name)>;

    /// C'tor.
    ///
    /// All prims in this scene index are under prefix.
    /// The client needs to wrap 
    /// HdRenderDelegate::GetDefaultAovDescriptor in aovDescriptorCallback
    /// (the API on HdRenderDelegate might change).
    /// gpuEnabled decides whether the present task is run for
    /// non-Storm renderers.
    HDX_API
    static
    HdxTaskControllerSceneIndexRefPtr
    New(const SdfPath &prefix,
        const TfToken &rendererPluginName,
        const AovDescriptorCallback &aovDescriptorCallback,
        bool gpuEnabled = true);

    HDX_API
    ~HdxTaskControllerSceneIndex() override;

    HDX_API
    HdSceneIndexPrim GetPrim(const SdfPath &primPath) const override;

    HDX_API
    SdfPathVector GetChildPrimPaths(const SdfPath &primPath) const override;

    /// -------------------------------------------------------
    /// Execution API

    /// Obtain paths to the tasks managed by the task controller,
    /// for image generation. The tasks returned will be different
    /// based on current renderer state.
    HDX_API
    SdfPathVector GetRenderingTaskPaths() const;

    /// Obtain paths to tasks managed by the task controller,
    /// for picking.
    HDX_API
    SdfPathVector GetPickingTaskPaths() const;

    /// Get the path to the buffer for a rendered output.
    /// Note: the caller should call Resolve(), as HdxTaskController doesn't
    /// guarantee the buffer will be resolved.
    HDX_API
    SdfPath GetRenderBufferPath(const TfToken &aovName) const;

    /// -------------------------------------------------------
    /// Rendering API

    /// Set the collection to be rendered.
    HDX_API
    void SetCollection(const HdRprimCollection &collection);

    /// Set the render params. Note: params.viewport will
    /// be overwritten, since it comes from SetRenderViewport.
    /// XXX: For GL renders, HdxTaskControllerSceneIndex relies on the caller to
    /// correctly set GL_SAMPLE_ALPHA_TO_COVERAGE.
    HDX_API
    void SetRenderParams(const HdxRenderTaskParams &params);

    /// Set the "view" opinion of the scenes render tags.
    /// The opinion is the base opinion for the entire scene.
    /// Individual tasks (such as the shadow task) may
    /// have a stronger opinion and override this opinion
    HDX_API
    void SetRenderTags(const TfTokenVector &renderTags);

    /// -------------------------------------------------------
    /// AOV API

    /// Set the list of outputs to be rendered. If outputs.size() == 1,
    /// this will send that output to the viewport via a colorizer task.
    /// Note: names should come from HdAovTokens.
    HDX_API
    void SetRenderOutputs(const TfTokenVector &aovNames);

    /// Set which output should be rendered to the viewport. The empty token
    /// disables viewport rendering.
    HDX_API
    void SetViewportRenderOutput(TfToken const &aovName);

    /// Set custom parameters for an AOV.
    HDX_API
    void SetRenderOutputSettings(TfToken const& aovName,
                                 const HdAovDescriptor &desc);

    /// Get parameters for an AOV.
    HDX_API
    HdAovDescriptor GetRenderOutputSettings(const TfToken &aovName) const;

    /// The destination API (e.g., OpenGL, see hgiInterop for details) and
    /// framebuffer that the AOVs are presented into. The framebuffer
    /// is a VtValue that encoding a framebuffer in a destination API
    /// specific way.
    /// E.g., a uint32_t (aka GLuint) for framebuffer object for OpenGL.
    HDX_API
    void SetPresentationOutput(const TfToken &api, const VtValue &framebuffer);

    /// -------------------------------------------------------
    /// Lighting API

    /// Set the lighting state for the scene.  HdxTaskControllerSceneIndex maintains
    /// a set of light sprims with data set from the lights in "src".
    /// @param src    Lighting state to implement.
    HDX_API
    void SetLightingState(GlfSimpleLightingContextPtr const& src);

    /// -------------------------------------------------------
    /// Camera and Framing API

    /// Set the size of the render buffers backing the AOVs.
    /// GUI applications should set this to the size of the window.
    ///
    HDX_API
    void SetRenderBufferSize(const GfVec2i &size);

    /// Determines how the filmback of the camera is mapped into
    /// the pixels of the render buffer and what pixels of the render
    /// buffer will be rendered into.
    HDX_API
    void SetFraming(const CameraUtilFraming &framing);

    /// Specifies whether to force a window policy when conforming
    /// the frustum of the camera to match the display window of
    /// the camera framing.
    HDX_API
    void SetOverrideWindowPolicy(
        const std::optional<CameraUtilConformWindowPolicy> &policy);

    /// -- Scene camera --
    /// Set the camera param on tasks to a USD camera path.
    HDX_API
    void SetCameraPath(const SdfPath &path);

    /// Set the viewport param on tasks.
    ///
    /// \deprecated Use SetFraming and SetRenderBufferSize instead.
    HDX_API
    void SetRenderViewport(const GfVec4d &viewport);

    /// -- Free camera --
    /// Set the view and projection matrices for the free camera.
    /// Note: The projection matrix must be pre-adjusted for the window policy.
    HDX_API
    void SetFreeCameraMatrices(const GfMatrix4d &viewMatrix,
                               const GfMatrix4d &projectionMatrix);
    /// Set the free camera clip planes.
    /// (Note: Scene cameras use clipping planes authored on the camera prim)
    HDX_API
    void SetFreeCameraClipPlanes(const std::vector<GfVec4d> &clipPlanes);

    /// -------------------------------------------------------
    /// Selection API

    /// Turns the selection task on or off.
    HDX_API
    void SetEnableSelection(bool enable);

    /// Set the selection color.
    HDX_API
    void SetSelectionColor(const GfVec4f &color);

    /// Set the selection locate (over) color.
    HDX_API
    void SetSelectionLocateColor(const GfVec4f &color);

    /// Set if the selection highlight should be rendered as an outline around
    /// the selected objects or as a solid color overlaid on top of them.
    HDX_API
    void SetSelectionEnableOutline(bool enableOutline);

    /// Set the selection outline radius (thickness) in pixels. This is only
    /// relevant if the highlight is meant to be rendered as an outline (if
    /// SetSelectionRenderOutline(true) is called).
    HDX_API
    void SetSelectionOutlineRadius(unsigned int radius);

    /// -------------------------------------------------------
    /// Shadow API

    /// Turns the shadow task on or off.
    HDX_API
    void SetEnableShadows(bool enable);

    /// Set the shadow params. Note: params.camera will
    /// be overwritten, since it comes from SetCameraPath/SetCameraState.
    HDX_API
    void SetShadowParams(const HdxShadowTaskParams &params);

    /// -------------------------------------------------------
    /// Color Correction API

    /// Configure color correction by settings params.
    HDX_API
    void SetColorCorrectionParams(const HdxColorCorrectionTaskParams &params);

    /// -------------------------------------------------------
    /// Bounding Box API

    /// Set the bounding box params.
    HDX_API
    void SetBBoxParams(const HdxBoundingBoxTaskParams& params);

    /// -------------------------------------------------------
    /// Present API

    /// Enable / disable presenting the render to bound framebuffer.
    /// An application may choose to manage the AOVs that are rendered into
    /// itself and skip the task controller's presentation.
    HDX_API
    void SetEnablePresentation(bool enabled);

private:
    HdxTaskControllerSceneIndex(
        const SdfPath &prefix,
        const TfToken &rendererPluginName,
        const AovDescriptorCallback &aovDescriptorCallback,
        bool gpuEnabled);

    bool _IsForStorm() const;

    void _CreateStormTasks();
    void _CreateGenericTasks();

    SdfPathVector _GetRenderingTaskPathsForStorm() const;
    SdfPathVector _GetRenderingTaskPathsForGenericRenderer() const;

    GfVec3i _RenderBufferDimensions() const;

    void _SetRenderOutputs(const TfTokenVector &aovNames);
    void _SetCameraFramingForTasks();
    void _SetRenderBufferSize();
    void _SetSimpleLightTaskParams(GlfSimpleLightingContextPtr const& src);
    void _SetLights(const GlfSimpleLightVector &lights);

    const SdfPath _prefix;
    const bool _isForStorm;
    const AovDescriptorCallback _aovDescriptorCallback;
    const bool _runGpuAovTasks;

    HdRetainedSceneIndexRefPtr const _retainedSceneIndex;

    // All tasks using HdxRenderTaskParams.
    SdfPathVector _renderTaskPaths;
    SdfPath _activeCameraId;

    // Generated renderbuffers
    TfTokenVector _aovNames;
    TfToken _viewportAov;

    GfVec2i _renderBufferSize;
    CameraUtilFraming _framing;
    std::optional<CameraUtilConformWindowPolicy> _overrideWindowPolicy;

    GfVec4d _viewport;

    friend class _Observer;
    class _Observer : public HdSceneIndexObserver
    {
    public:
        _Observer(HdxTaskControllerSceneIndex * const owner)
         : _owner(owner) {}

          HDX_API
          void PrimsAdded(
                  const HdSceneIndexBase &sender,
                  const AddedPrimEntries &entries) override;

          HDX_API
          void PrimsRemoved(
                  const HdSceneIndexBase &sender,
                  const RemovedPrimEntries &entries) override;

          HDX_API
          void PrimsDirtied(
                  const HdSceneIndexBase &sender,
                  const DirtiedPrimEntries &entries) override;

          HDX_API
          void PrimsRenamed(
                  const HdSceneIndexBase &sender,
                  const RenamedPrimEntries &entries) override;
    private:
        HdxTaskControllerSceneIndex * const _owner;
    };
    _Observer _observer;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_IMAGING_HDX_TASK_CONTROLLER_SCENE_INDEX_H
