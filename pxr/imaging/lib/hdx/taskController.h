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
#ifndef HDX_TASK_CONTROLLER_H
#define HDX_TASK_CONTROLLER_H

#include "pxr/pxr.h"

#include "pxr/imaging/hdx/api.h"
#include "pxr/imaging/hdx/intersector.h"
#include "pxr/imaging/hdx/selectionTracker.h"
#include "pxr/imaging/hdx/renderSetupTask.h"
#include "pxr/imaging/hdx/shadowTask.h"
#include "pxr/imaging/hdx/colorCorrectionTask.h"

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

/// Intersection mode tokens, mapped to HdxIntersector API.
/// Note: The "nearest*" hitmodes may be considerably more efficient.
/// - "nearestToCamera" returns the single hit point closest (by depth) to the
///                     camera
/// - "nearestToCenter" returns the single hit point nearest to the center of
///                     the selection region; note that this should be faster
///                     than nearestToCamera, as it will sample outward from the
///                     center, and stop as soon as it finds any hit, while
///                     nearestToCamera will check ALL pixels in the selection
///                     region, and return the hit that has the lowest z
/// - "unique"  returns the set of unique hit prims, keeping only the nearest
///             depth per prim.
/// - "all"     returns all hit points, possibly including multiple hits per
///             prim.
#define HDX_INTERSECTION_MODE_TOKENS           \
    (nearestToCamera)                          \
    (nearestToCenter)                          \
    (unique)                                   \
    (all)

TF_DECLARE_PUBLIC_TOKENS(HdxIntersectionModeTokens, HDX_API, \
    HDX_INTERSECTION_MODE_TOKENS);

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
    SdfPath const& GetControllerId() { return _controllerId; }

    /// -------------------------------------------------------
    /// Execution API

    /// Obtain the set of tasks managed by the task controller,
    /// for execution. The tasks returned will be different based on
    /// current renderer state.
    ///
    /// A vector of zero length indicates error.
    HDX_API
    HdTaskSharedPtrVector const GetTasks();

    /// -------------------------------------------------------
    /// Rendering API

    /// Set the collection to be rendered.
    HDX_API
    void SetCollection(HdRprimCollection const& collection);

    /// Set the render params. Note: params.camera and params.viewport will
    /// be overwritten, since they come from SetCameraState.
    /// XXX: For GL renders, HdxTaskController relies on the caller to
    /// correctly set GL_SAMPLE_ALPHA_TO_COVERAGE.
    HDX_API
    void SetRenderParams(HdxRenderTaskParams const& params);

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

    /// -------------------------------------------------------
    /// Lighting API

    /// Set the lighting state for the scene.  HdxTaskController maintains
    /// a set of light sprims with data set from the lights in "src".
    /// @param src    Lighting state to implement.
    HDX_API
    void SetLightingState(GlfSimpleLightingContextPtr const& src);

    /// -------------------------------------------------------
    /// Camera API
    
    /// Set the parameters for the viewer default camera.
    HDX_API
    void SetCameraMatrices(GfMatrix4d const& viewMatrix,
                           GfMatrix4d const& projectionMatrix);

    /// Set the camera viewport.
    HDX_API
    void SetCameraViewport(GfVec4d const& viewport);

    /// Set the camera clip planes.
    HDX_API
    void SetCameraClipPlanes(std::vector<GfVec4d> const& clipPlanes);

    /// Set the camera window policy.
    HDX_API
    void SetCameraWindowPolicy(CameraUtilConformWindowPolicy windowPolicy);

    /// -------------------------------------------------------
    /// Picking API

    /// Set pick target resolution (if applicable).
    /// XXX: Is there a better place for this to live?
    HDX_API
    void SetPickResolution(unsigned int size);

    /// Test for intersection.
    /// XXX: This should be changed to not take an HdEngine*.
    HDX_API
    bool TestIntersection(
            HdEngine* engine,
            HdRprimCollection const& collection,
            HdxIntersector::Params const& qparams,
            TfToken const& intersectionMode,
            HdxIntersector::HitVector *allHits);

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
    /// be overwritten, since it comes from SetCameraState.
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

private:
    ///
    /// This class is not intended to be copied.
    ///
    HdxTaskController(HdxTaskController const&) = delete;
    HdxTaskController &operator=(HdxTaskController const&) = delete;

    HdRenderIndex *_index;
    SdfPath const _controllerId;

    std::unique_ptr<HdxIntersector> _intersector;

    // Create taskController objects. Since the camera is a parameter
    // to the tasks, _CreateCamera() should be called first.
    void _CreateCamera();
    void _CreateRenderTask();
    void _CreateSelectionTask();
    void _CreateLightingTask();
    void _CreateShadowTask();
    void _CreateColorizeTask();
    void _CreateColorCorrectionTask();

    SdfPath _GetAovPath(TfToken const& aov);

    // A private scene delegate member variable backs the tasks this
    // controller generates. To keep _Delegate simple, the containing class
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
        T const& GetParameter(SdfPath const& id, TfToken const& key) const {
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
        virtual bool IsEnabled(TfToken const& option) const;
        virtual std::vector<GfVec4d> GetClipPlanes(SdfPath const& cameraId);
        virtual HdRenderBufferDescriptor
            GetRenderBufferDescriptor(SdfPath const& id);

    private:
        typedef TfHashMap<TfToken, VtValue, TfToken::HashFunctor> _ValueCache;
        typedef TfHashMap<SdfPath, _ValueCache, SdfPath::Hash> _ValueCacheMap;
        _ValueCacheMap _valueCacheMap;
    };
    _Delegate _delegate;

    // Generated tasks.
    SdfPathVector _renderTaskIds;
    SdfPath _selectionTaskId;
    SdfPath _simpleLightTaskId;
    SdfPath _shadowTaskId;
    SdfPath _colorizeTaskId;
    SdfPath _colorCorrectionTaskId;

    // Generated cameras
    SdfPath _cameraId;

    // Generated lights
    SdfPathVector _lightIds;

    // Generated renderbuffers
    SdfPathVector _renderBufferIds;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // HDX_TASK_CONTROLLER_H
