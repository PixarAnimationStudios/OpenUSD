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

#include "pxr/imaging/hd/renderIndex.h"
#include "pxr/imaging/hd/sceneDelegate.h"
#include "pxr/imaging/hd/task.h"

#include "pxr/imaging/glf/simpleLightingContext.h"
#include "pxr/usd/sdf/path.h"
#include "pxr/base/tf/staticTokens.h"
#include "pxr/base/gf/matrix4d.h"

PXR_NAMESPACE_OPEN_SCOPE

// XXX: This API is transitional. At the least, render/picking/selection
// APIs should be decoupled.

/// Task set tokens:
/// - "colorRender" is the set of tasks needed to render to a color buffer.
/// - "idRender" is the set of tasks needed to render an id buffer, indicating
///              what object is at each pixel.
#define HDX_TASK_SET_TOKENS                    \
    (colorRender)                              \
    (idRender)

TF_DECLARE_PUBLIC_TOKENS(HdxTaskSetTokens, HDX_API, HDX_TASK_SET_TOKENS);

/// Intersection mode tokens, mapped to HdxIntersector API.
/// Note: "nearest" hitmode may be considerably more efficient.
/// - "nearest" returns the nearest single hit point.
/// - "unique"  returns the set of unique hit prims, keeping only the nearest
///             depth per prim.
/// - "all"     returns all hit points, possibly including multiple hits per
///             prim.
#define HDX_INTERSECTION_MODE_TOKENS           \
    (nearest)                                  \
    (unique)                                   \
    (all)

TF_DECLARE_PUBLIC_TOKENS(HdxIntersectionModeTokens, HDX_API, \
    HDX_INTERSECTION_MODE_TOKENS);

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

    /// Obtain the set of tasks managed by the task controller
    /// suitable for execution. Currently supported tasksets:
    /// HdxTaskSet->render
    /// HdxTaskSet->idRender
    ///
    /// A vector of zero length indicates the specified taskSet is unsupported.
    HDX_API
    HdTaskSharedPtrVector const &GetTasks(TfToken const& taskSet);

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
    /// Progressive Image Generation
    
    /// Reset the image render to reflect a changed scene.
    HDX_API
    void ResetImage();

    /// Return whether the image has converged.
    HDX_API
    bool IsConverged() const;

private:
    ///
    /// This class is not intended to be copied.
    ///
    HdxTaskController(HdxTaskController const&) = delete;
    HdxTaskController &operator=(HdxTaskController const&) = delete;

    HdRenderIndex *_index;
    SdfPath const _controllerId;

    HdTaskSharedPtrVector _tasks;
    std::unique_ptr<HdxIntersector> _intersector;

    // Create taskController objects. Since the camera is a parameter
    // to the tasks, _CreateCamera() should be called first.
    void _CreateCamera();
    void _CreateRenderTasks();
    void _CreateSelectionTask();
    void _CreateLightingTask();

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
        T const& GetParameter(SdfPath const& id, TfToken const& key) {
            VtValue vParams = _valueCacheMap[id][key];
            TF_VERIFY(vParams.IsHolding<T>());
            return vParams.Get<T>();
        }

        // HdSceneDelegate interface
        virtual VtValue Get(SdfPath const& id, TfToken const& key);
        virtual bool IsEnabled(TfToken const& option) const;
        virtual std::vector<GfVec4d> GetClipPlanes(SdfPath const& cameraId);

    private:
        typedef TfHashMap<TfToken, VtValue, TfToken::HashFunctor> _ValueCache;
        typedef TfHashMap<SdfPath, _ValueCache, SdfPath::Hash> _ValueCacheMap;
        _ValueCacheMap _valueCacheMap;
    };
    _Delegate _delegate;

    // Generated tasks.
    //
    // _renderTaskId and _idRenderTaskId are both of type HdxRenderTask.
    // The reason we have two around is so that they can have parallel sets of
    // HdxRenderTaskParams; if there were only one render task, we'd thrash the
    // params switching between id and color render.
    SdfPath _renderTaskId;
    SdfPath _idRenderTaskId;
    SdfPath _selectionTaskId;
    SdfPath _simpleLightTaskId;

    // Generated cameras
    SdfPath _cameraId;

    // Generated lights
    SdfPathVector _lightIds;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // HDX_TASK_CONTROLLER_H
