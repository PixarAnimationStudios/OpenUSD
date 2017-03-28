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
#ifndef HDSTREAM_TASK_CONTROLLER_H
#define HDSTREAM_TASK_CONTROLLER_H

#include "pxr/pxr.h"
#include "pxr/imaging/hdx/taskController.h"
#include "pxr/imaging/hd/sceneDelegate.h"

#include "pxr/usd/sdf/path.h"

PXR_NAMESPACE_OPEN_SCOPE

///
/// \class HdStreamTaskController
///
/// Provide tasks and an application API for a viewer app to draw through
/// hydra (with the stream plugin).
///
class HdStreamTaskController : public HdxTaskController {
public:
    HdStreamTaskController(HdRenderIndex *renderIndex,
                           SdfPath const& controllerId);
    virtual ~HdStreamTaskController();

    // Execution API
    virtual HdTaskSharedPtrVector const &GetTasks(TfToken const& taskSet);

    // Rendering API
    virtual void SetCollection(HdRprimCollection const& collection);
    // XXX: Note: HdStreamTaskController relies on the caller to
    // correctly set GL_SAMPLE_ALPHA_TO_COVERAGE.
    virtual void SetRenderParams(HdxRenderTaskParams const& params);

    // Lighting API
    virtual void SetLightingState(GlfSimpleLightingContextPtr const& src,
            bool bypass);

    // Camera API
    virtual void SetCameraMatrices(GfMatrix4d const& viewMatrix,
                                   GfMatrix4d const& projMatrix);
    virtual void SetCameraViewport(GfVec4d const& viewport);
    virtual void SetCameraClipPlanes(std::vector<GfVec4d> const& clipPlanes);

    // Picking API
    virtual void SetPickResolution(unsigned int size);
    virtual bool TestIntersection(
            HdEngine* engine,
            GfMatrix4d const& viewMatrix,
            GfMatrix4d const& projMatrix,
            HdRprimCollection const& collection,
            float alphaThreshold,
            HdCullStyle cullStyle,
            TfToken const& intersectionMode,
            HdxIntersector::HitVector *allHits);

    // Selection API
    virtual void SetEnableSelection(bool enable);
    virtual void SetSelectionColor(GfVec4f const& color);

    // HdStreamTaskController doesn't use the progressive rendering API.
    virtual void ResetImage() {}
    virtual bool IsConverged() const { return true; }

private:
    HdTaskSharedPtrVector _tasks;
    HdxIntersector       *_intersector;

    // Create taskController objects. Since the camera is a parameter
    // to the tasks, _CreateCamera() should be called first.
    void _CreateCamera();
    void _CreateRenderTasks();
    void _CreateSelectionTask();
    void _CreateLightingTasks();

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
    //
    // _activeLightTaskId is just an alias, pointing to one of
    // _simpleLightTaskId or _simpleLightBypassTaskId, depending on which one
    // was set most recently.
    SdfPath _renderTaskId;
    SdfPath _idRenderTaskId;
    SdfPath _selectionTaskId;
    SdfPath _simpleLightTaskId;
    SdfPath _simpleLightBypassTaskId;
    SdfPath _activeLightTaskId;

    // Generated cameras
    SdfPath _cameraId;

    // Generated lights
    SdfPathVector _lightIds;

    // This class is not intended to be copied.
    HdStreamTaskController(const HdStreamTaskController&)            = delete;
    HdStreamTaskController &operator=(const HdStreamTaskController&) = delete;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // HDSTREAM_TASK_CONTROLLER_H
