//
// Copyright 2017 Pixar
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
#ifndef PXR_IMAGING_HDX_TASK_CONTROLLER_H
#define PXR_IMAGING_HDX_TASK_CONTROLLER_H

#include "pxr/pxr.h"

#include "pxr/imaging/hdx/api.h"
#include "pxr/imaging/hdx/selectionTracker.h"
#include "pxr/imaging/hdx/renderSetupTask.h"
#include "pxr/imaging/hdx/shadowTask.h"
#include "pxr/imaging/hdx/colorCorrectionTask.h"
#include "pxr/imaging/hdx/colorChannelTask.h"

#include "pxr/imaging/hd/aov.h"
#include "pxr/imaging/hd/renderIndex.h"
#include "pxr/imaging/hd/sceneDelegate.h"
#include "pxr/imaging/hd/task.h"

#include "pxr/imaging/cameraUtil/conformWindow.h"

#include "pxr/imaging/glf/simpleLightingContext.h"
#include "pxr/usd/sdf/path.h"
#include "pxr/base/tf/staticTokens.h"
#include "pxr/base/gf/matrix4d.h"

PXR_NAMESPACE_OPEN_SCOPE

// XXX: This API is transitional. At the least, render/picking/selection
// APIs should be decoupled.

class HdRenderBuffer;

class HdxTaskController {
public:
    HDX_API
    HdxTaskController(HdRenderIndex *renderIndex,
                      SdfPath const& controllerId);
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

    /// Obtain the set of tasks managed by the task controller,
    /// for image generation. The tasks returned will be different
    /// based on current renderer state.
    HDX_API
    HdTaskSharedPtrVector const GetRenderingTasks() const;

    /// Obtain the set of tasks managed by the task controller,
    /// for picking.
    HDX_API
    HdTaskSharedPtrVector const GetPickingTasks() const;

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

    // Get parameters for an AOV.
    HDX_API
    HdAovDescriptor GetRenderOutputSettings(TfToken const& name) const;

    /// -------------------------------------------------------
    /// Lighting API

    /// Set the lighting state for the scene.  HdxTaskController maintains
    /// a set of light sprims with data set from the lights in "src".
    /// @param src    Lighting state to implement.
    HDX_API
    void SetLightingState(GlfSimpleLightingContextPtr const& src);

    /// -------------------------------------------------------
    /// Camera and Framing API
    
    /// Set the viewport param on tasks.
    HDX_API
    void SetRenderViewport(GfVec4d const& viewport);

    /// -- Scene camera --
    /// Set the camera param on tasks to a USD camera path.
    HDX_API
    void SetCameraPath(SdfPath const& id);
    
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
    HDX_API
    bool IsConverged() const;

    /// -------------------------------------------------------
    /// Color Correction API

    /// Configure color correction by settings params.
    HDX_API
    void SetColorCorrectionParams(HdxColorCorrectionTaskParams const& params);

    /// -------------------------------------------------------
    /// Color Channel API

    /// Configure color channel by settings params.
    HDX_API
    void SetColorChannelParams(HdxColorChannelTaskParams const& params);

    /// -------------------------------------------------------
    /// Colorize API

    /// Turns the colorize task color quantization on or off.
    /// XXX: This is a temporary function that will be soon deprecated. Please
    //       avoid calling it.
    HDX_API
    void SetColorizeQuantizationEnabled(bool enabled);

private:
    ///
    /// This class is not intended to be copied.
    ///
    HdxTaskController(HdxTaskController const&) = delete;
    HdxTaskController &operator=(HdxTaskController const&) = delete;

    HdRenderIndex *_index;
    SdfPath const _controllerId;

    // Create taskController objects. Since the camera is a parameter
    // to the tasks, _CreateCamera() should be called first.
    void _CreateRenderGraph();

    void _CreateCamera();
    void _CreateLightingTask();
    void _CreateShadowTask();
    SdfPath _CreateRenderTask(TfToken const& materialTag);
    void _CreateOitResolveTask();
    void _CreateSelectionTask();
    void _CreateColorizeTask();
    void _CreateColorizeSelectionTask();
    void _CreateColorCorrectionTask();
    void _CreateColorChannelTask();
    void _CreatePickTask();
    void _CreatePickFromRenderBufferTask();
    SdfPath _CreateAovResolveTask(TfToken const& aovName);
    void _CreatePresentTask();

    void _SetCameraParamForTasks(SdfPath const& id);

    void _SetBlendStateForMaterialTag(TfToken const& materialTag,
                                      HdxRenderTaskParams *renderParams) const;

    // Render graph topology control.
    bool _ShadowsEnabled() const;
    bool _SelectionEnabled() const;
    bool _ColorizeSelectionEnabled() const;
    bool _ColorCorrectionEnabled() const;
    bool _ColorChannelEnabled() const;
    bool _ColorizeQuantizationEnabled() const;
    bool _AovsSupported() const;

    // Helper function for renderbuffer management.
    SdfPath _GetRenderTaskPath(TfToken const& materialTag) const;
    SdfPath _GetAovPath(TfToken const& aov) const;
    SdfPathVector _GetAovEnabledTasks() const;

    // Helper function to load the default domeLight texture
    void _LoadDefaultDomeLightTexture();

    // Helper function to set the parameters of a light, get a particular light 
    // in the scene, replace and remove Sprims from the scene 
    void _SetParameters(SdfPath const& pathName, GlfSimpleLight const& light);
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
        virtual ~_Delegate() = default;

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
        virtual VtValue Get(SdfPath const& id, TfToken const& key);
        virtual GfMatrix4d GetTransform(SdfPath const& id);
        virtual VtValue GetCameraParamValue(SdfPath const& id, 
                                            TfToken const& key);
        virtual VtValue GetLightParamValue(SdfPath const& id, 
                                            TfToken const& paramName);
        virtual bool IsEnabled(TfToken const& option) const;
        virtual HdRenderBufferDescriptor
            GetRenderBufferDescriptor(SdfPath const& id);
        virtual TfTokenVector GetTaskRenderTags(SdfPath const& taskId);


    private:
        typedef TfHashMap<TfToken, VtValue, TfToken::HashFunctor> _ValueCache;
        typedef TfHashMap<SdfPath, _ValueCache, SdfPath::Hash> _ValueCacheMap;
        _ValueCacheMap _valueCacheMap;
    };
    _Delegate _delegate;

    // Generated tasks.
    SdfPath _simpleLightTaskId;
    SdfPath _shadowTaskId;
    SdfPathVector _renderTaskIds;
    SdfPath _oitResolveTaskId;
    SdfPath _selectionTaskId;
    SdfPath _colorizeSelectionTaskId;
    SdfPath _colorizeTaskId;
    SdfPath _colorCorrectionTaskId;
    SdfPath _colorChannelTaskId;
    SdfPath _pickTaskId;
    SdfPath _pickFromRenderBufferTaskId;
    SdfPath _aovColorResolveTaskId;
    SdfPath _aovDepthResolveTaskId;
    SdfPath _presentTaskId;

    // Generated camera (for the default/free cam)
    SdfPath _freeCamId;
    // Current active camera
    SdfPath _activeCameraId;
    
    // Built-in lights
    SdfPathVector _lightIds;
    HdTextureResourceSharedPtr _defaultDomeLightTextureResource;

    // Generated renderbuffers
    SdfPathVector _aovBufferIds;
    TfTokenVector _aovOutputs;
    TfToken _viewportAov;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_IMAGING_HDX_TASK_CONTROLLER_H
