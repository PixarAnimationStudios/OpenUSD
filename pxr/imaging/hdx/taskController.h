//
// Copyright 2017 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_IMAGING_HDX_TASK_CONTROLLER_H
#define PXR_IMAGING_HDX_TASK_CONTROLLER_H

#include "pxr/pxr.h"

#include "pxr/imaging/hdx/api.h"
#include "pxr/imaging/hdx/selectionTracker.h"
#include "pxr/imaging/hdx/renderSetupTask.h"
#include "pxr/imaging/hdx/shadowTask.h"
#include "pxr/imaging/hdx/colorCorrectionTask.h"
#include "pxr/imaging/hdx/boundingBoxTask.h"

#include "pxr/imaging/hd/aov.h"
#include "pxr/imaging/hd/renderIndex.h"
#include "pxr/imaging/hd/sceneDelegate.h"
#include "pxr/imaging/hd/task.h"

#include "pxr/imaging/cameraUtil/framing.h"
#include "pxr/imaging/glf/simpleLightingContext.h"
#include "pxr/usd/sdf/path.h"

#include "pxr/base/gf/bbox3d.h"
#include "pxr/base/gf/matrix4d.h"

PXR_NAMESPACE_OPEN_SCOPE

// XXX: This API is transitional. At the least, render/picking/selection
// APIs should be decoupled.

class HdRenderBuffer;

class HdxTaskController final
{
public:
    HDX_API
    HdxTaskController(HdRenderIndex *renderIndex,
                      SdfPath const& controllerId,
                      bool gpuEnabled = true);
    HDX_API
    ~HdxTaskController();

    /// Return the render index this controller is bound to.
    HdRenderIndex* GetRenderIndex() { return _index; }
    HdRenderIndex const* GetRenderIndex() const { return _index; }

    /// Return the controller's scene-graph id (prefixed to any
    /// scene graph objects it creates).
    SdfPath const& GetControllerId() const { return _controllerId; }

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

    /// \deprecated Use GetRenderingTaskPaths
    ///
    /// Deprecated in preparation of changing to an
    /// HdxTaskControllerSceneIndex.
    ///
    HDX_API
    HdTaskSharedPtrVector GetRenderingTasks() const;

    /// \deprecated Use GetPickingTaskPaths
    ///
    /// Deprecated in preparation of changing to an
    /// HdxTaskControllerSceneIndex.
    ///
    HDX_API
    HdTaskSharedPtrVector GetPickingTasks() const;

    /// -------------------------------------------------------
    /// Rendering API

    /// Set the collection to be rendered.
    HDX_API
    void SetCollection(HdRprimCollection const& collection);

    /// Set the render params. Note: params.viewport will
    /// be overwritten, since it comes from SetRenderViewport.
    /// XXX: For GL renders, HdxTaskController relies on the caller to
    /// correctly set GL_SAMPLE_ALPHA_TO_COVERAGE.
    HDX_API
    void SetRenderParams(HdxRenderTaskParams const& params);

    /// Set the "view" opinion of the scenes render tags.
    /// The opinion is the base opinion for the entire scene.
    /// Individual tasks (such as the shadow task) may
    /// have a stronger opinion and override this opinion
    HDX_API
    void SetRenderTags(TfTokenVector const& renderTags);

    /// -------------------------------------------------------
    /// AOV API

    /// Set the list of outputs to be rendered. If outputs.size() == 1,
    /// this will send that output to the viewport via a colorizer task.
    /// Note: names should come from HdAovTokens.
    HDX_API
    void SetRenderOutputs(TfTokenVector const& names);

    /// Set which output should be rendered to the viewport. The empty token
    /// disables viewport rendering.
    HDX_API
    void SetViewportRenderOutput(TfToken const& name);

    /// Get the buffer for a rendered output. Note: the caller should call
    /// Resolve(), as HdxTaskController doesn't guarantee the buffer will
    /// be resolved.
    HDX_API
    HdRenderBuffer* GetRenderOutput(TfToken const& name);

    /// Set custom parameters for an AOV.
    HDX_API
    void SetRenderOutputSettings(TfToken const& name,
                                 HdAovDescriptor const& desc);

    /// Get parameters for an AOV.
    HDX_API
    HdAovDescriptor GetRenderOutputSettings(TfToken const& name) const;

    /// The destination API (e.g., OpenGL, see hgiInterop for details) and
    /// framebuffer that the AOVs are presented into. The framebuffer
    /// is a VtValue that encoding a framebuffer in a destination API
    /// specific way.
    /// E.g., a uint32_t (aka GLuint) for framebuffer object for OpenGL.
    HDX_API
    void SetPresentationOutput(TfToken const &api, VtValue const &framebuffer);

    /// -------------------------------------------------------
    /// Lighting API

    /// Set the lighting state for the scene.  HdxTaskController maintains
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
    void SetCameraPath(SdfPath const& id);

    /// Set the viewport param on tasks.
    ///
    /// \deprecated Use SetFraming and SetRenderBufferSize instead.
    HDX_API
    void SetRenderViewport(GfVec4d const& viewport);

    /// -- Free camera --
    /// Set the view and projection matrices for the free camera.
    /// Note: The projection matrix must be pre-adjusted for the window policy.
    HDX_API
    void SetFreeCameraMatrices(GfMatrix4d const& viewMatrix,
                               GfMatrix4d const& projectionMatrix);
    /// Set the free camera clip planes.
    /// (Note: Scene cameras use clipping planes authored on the camera prim)
    HDX_API
    void SetFreeCameraClipPlanes(std::vector<GfVec4d> const& clipPlanes);

    /// -------------------------------------------------------
    /// Selection API

    /// Turns the selection task on or off.
    HDX_API
    void SetEnableSelection(bool enable);

    /// Set the selection color.
    HDX_API
    void SetSelectionColor(GfVec4f const& color);

    /// Set the selection locate (over) color.
    HDX_API
    void SetSelectionLocateColor(GfVec4f const& color);

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
    void SetShadowParams(HdxShadowTaskParams const& params);

    /// -------------------------------------------------------
    /// Progressive Image Generation

    /// Return whether the image has converged.
    ///
    /// \deprecated
    ///
    /// Instead, checks whether HdxTask::IsConverged for a
    /// task in the render index for each path in GetRenderingTaskPaths().
    ///
    /// Deprecated in preparation of changing to an
    /// HdxTaskControllerSceneIndex.
    ///
    HDX_API
    bool IsConverged() const;

    /// -------------------------------------------------------
    /// Color Correction API

    /// Configure color correction by settings params.
    HDX_API
    void SetColorCorrectionParams(HdxColorCorrectionTaskParams const& params);

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
    ///
    /// This class is not intended to be copied.
    ///
    HdxTaskController(HdxTaskController const&) = delete;
    HdxTaskController &operator=(HdxTaskController const&) = delete;

    HdRenderIndex *_index;
    SdfPath const _controllerId;
    bool _gpuEnabled;

    // Create taskController objects. Since the camera is a parameter
    // to the tasks, _CreateCamera() should be called first.
    void _CreateRenderGraph();

    void _CreateLightingTask();
    void _CreateShadowTask();
    SdfPath _CreateSkydomeTask();
    SdfPath _CreateRenderTask(TfToken const& materialTag);
    void _CreateOitResolveTask();
    void _CreateSelectionTask();
    void _CreateColorizeSelectionTask();
    void _CreateColorCorrectionTask();
    void _CreateVisualizeAovTask();
    void _CreatePickTask();
    void _CreatePickFromRenderBufferTask();
    void _CreateBoundingBoxTask();
    void _CreateAovInputTask();
    void _CreatePresentTask();

    void _SetCameraParamForTasks(SdfPath const& id);
    void _SetCameraFramingForTasks();
    void _UpdateAovDimensions(GfVec2i const& dimensions);

    void _SetBlendStateForMaterialTag(TfToken const& materialTag,
                                      HdxRenderTaskParams *renderParams) const;

    // Render graph topology control.
    bool _ShadowsEnabled() const;
    bool _SelectionEnabled() const;
    bool _ColorizeSelectionEnabled() const;
    bool _ColorCorrectionEnabled() const;
    bool _VisualizeAovEnabled() const;
    bool _ColorizeQuantizationEnabled() const;
    bool _AovsSupported() const;
    bool _UsingAovs() const;

    // Helper function for renderbuffer management.
    SdfPath _GetRenderTaskPath(TfToken const& materialTag) const;
    SdfPath _GetAovPath(TfToken const& aov) const;
    SdfPathVector _GetAovEnabledTasks() const;

    // Helper functions to set up the lighting state for the built-in lights
    bool _SupportBuiltInLightTypes();
    void _SetBuiltInLightingState(GlfSimpleLightingContextPtr const& src);

    // Helper function to get the built-in Camera light type SimpleLight for
    // Storm, and DistantLight otherwise
    TfToken _GetCameraLightType();
    
    // Helper functions to set the parameters of a light, get a particular light 
    // in the scene, replace and remove Sprims from the scene 
    VtValue _GetDomeLightTexture(GlfSimpleLight const& light);
    void _SetParameters(SdfPath const& pathName, GlfSimpleLight const& light);
    void _SetMaterialNetwork(SdfPath const& pathName, 
                             GlfSimpleLight const& light);
    GlfSimpleLight _GetLightAtId(size_t const& pathIdx);
    void _RemoveLightSprim(size_t const& pathIdx);
    void _ReplaceLightSprim(size_t const& pathIdx, GlfSimpleLight const& light, 
                        SdfPath const& pathName);

    // A private scene delegate member variable backs the tasks and the free cam
    // this controller generates. To keep _Delegate simple, the containing class
    // is responsible for marking things dirty.
    class _Delegate : public HdSceneDelegate
    {
    public:
        _Delegate(HdRenderIndex *parentIndex,
                  SdfPath const& delegateID)
            : HdSceneDelegate(parentIndex, delegateID)
            {}
        ~_Delegate() override = default;

        // HdxTaskController set/get interface
        template <typename T>
        void SetParameter(SdfPath const& id, TfToken const& key,
                          T const& value) {
            _valueCacheMap[id][key] = value;
        }
        template <typename T>
        T GetParameter(SdfPath const& id, TfToken const& key) const {
            VtValue vParams;
            _ValueCache vCache;
            TF_VERIFY(
                TfMapLookup(_valueCacheMap, id, &vCache) &&
                TfMapLookup(vCache, key, &vParams) &&
                vParams.IsHolding<T>());
            return vParams.Get<T>();
        }
        bool HasParameter(SdfPath const& id, TfToken const& key) const {
            _ValueCache vCache;
            if (TfMapLookup(_valueCacheMap, id, &vCache) &&
                vCache.count(key) > 0) {
                return true;
            }
            return false;
        }

        // HdSceneDelegate interface
        VtValue Get(SdfPath const& id, TfToken const& key) override;
        GfMatrix4d GetTransform(SdfPath const& id) override;
        VtValue GetLightParamValue(SdfPath const& id, 
                                   TfToken const& paramName) override;
        VtValue GetMaterialResource(SdfPath const& id) override;
        bool IsEnabled(TfToken const& option) const override;
        HdRenderBufferDescriptor
            GetRenderBufferDescriptor(SdfPath const& id) override;
        TfTokenVector GetTaskRenderTags(SdfPath const& taskId) override;


    private:
        using _ValueCache = TfHashMap<TfToken, VtValue, TfToken::HashFunctor>;
        using _ValueCacheMap = TfHashMap<SdfPath, _ValueCache, SdfPath::Hash>;
        _ValueCacheMap _valueCacheMap;
    };
    _Delegate _delegate;
    std::unique_ptr<class HdxFreeCameraSceneDelegate> _freeCameraSceneDelegate;

    // Generated tasks.
    SdfPath _simpleLightTaskId;
    SdfPath _shadowTaskId;
    SdfPathVector _renderTaskIds;
    SdfPath _aovInputTaskId;
    SdfPath _oitResolveTaskId;
    SdfPath _selectionTaskId;
    SdfPath _colorizeSelectionTaskId;
    SdfPath _colorCorrectionTaskId;
    SdfPath _visualizeAovTaskId;
    SdfPath _pickTaskId;
    SdfPath _pickFromRenderBufferTaskId;
    SdfPath _boundingBoxTaskId;
    SdfPath _presentTaskId;

    // Current active camera
    SdfPath _activeCameraId;
    
    // Built-in lights
    SdfPathVector _lightIds;

    // Generated renderbuffers
    SdfPathVector _aovBufferIds;
    TfTokenVector _aovOutputs;
    TfToken _viewportAov;

    GfVec2i _renderBufferSize;
    CameraUtilFraming _framing;
    std::optional<CameraUtilConformWindowPolicy> _overrideWindowPolicy;

    GfVec4d _viewport;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_IMAGING_HDX_TASK_CONTROLLER_H
