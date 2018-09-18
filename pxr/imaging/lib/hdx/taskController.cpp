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
#include "pxr/imaging/hdx/taskController.h"

#include "pxr/imaging/hd/camera.h"
#include "pxr/imaging/hdSt/light.h"
#include "pxr/imaging/hdx/intersector.h"
#include "pxr/imaging/hdx/renderTask.h"
#include "pxr/imaging/hdx/selectionTask.h"
#include "pxr/imaging/hdx/simpleLightTask.h"
#include "pxr/imaging/hdx/shadowTask.h"
#include "pxr/imaging/hdx/tokens.h"

#include "pxr/imaging/glf/simpleLight.h"
#include "pxr/imaging/glf/simpleLightingContext.h"

PXR_NAMESPACE_OPEN_SCOPE

TF_DEFINE_PUBLIC_TOKENS(HdxTaskSetTokens, HDX_TASK_SET_TOKENS);
TF_DEFINE_PUBLIC_TOKENS(HdxIntersectionModeTokens, \
    HDX_INTERSECTION_MODE_TOKENS);

// ---------------------------------------------------------------------------
// Delegate implementation.

/* virtual */
VtValue
HdxTaskController::_Delegate::Get(SdfPath const& id, TfToken const& key)
{
    _ValueCache *vcache = TfMapLookupPtr(_valueCacheMap, id);
    VtValue ret;
    if (vcache && TfMapLookup(*vcache, key, &ret)) {
        return ret;
    }
    TF_CODING_ERROR("%s:%s doesn't exist in the value cache\n",
            id.GetText(), key.GetText());
    return VtValue();
}

/* virtual */
bool
HdxTaskController::_Delegate::IsEnabled(TfToken const& option) const
{
    // The client using this task controller is responsible for setting
    // GL_SAMPLE_ALPHA_TO_COVERAGE.
    if (option == HdxOptionTokens->taskSetAlphaToCoverage) {
        return true;
    }
    return HdSceneDelegate::IsEnabled(option);
}

/* virtual */
std::vector<GfVec4d>
HdxTaskController::_Delegate::GetClipPlanes(SdfPath const& cameraId)
{
    return GetParameter<std::vector<GfVec4d>>(cameraId,
                HdCameraTokens->clipPlanes);
}

// ---------------------------------------------------------------------------
// Task controller implementation.

TF_DEFINE_PRIVATE_TOKENS(
    _tokens,
    (idRenderTask)
    (renderTask)
    (selectionTask)
    (simpleLightTask)
    (shadowTask)
    (camera)
);

HdxTaskController::HdxTaskController(HdRenderIndex *renderIndex,
                                     SdfPath const& controllerId)
    : _index(renderIndex)
    , _controllerId(controllerId)
    , _intersector(new HdxIntersector(renderIndex))
    , _delegate(renderIndex, controllerId)
{
    // We create camera and tasks here, but lights are created lazily by
    // SetLightingState. Camera needs to be created first, since it's a
    // parameter of most tasks.

    _CreateCamera();
    _CreateRenderTasks();
    _CreateSelectionTask();
    _CreateLightingTask();
    _CreateShadowTask();
}

void
HdxTaskController::_CreateCamera()
{
    // Create a default camera, driven by SetCameraMatrices.
    _cameraId = GetControllerId().AppendChild(_tokens->camera);
    GetRenderIndex()->InsertSprim(HdPrimTypeTokens->camera,
        &_delegate, _cameraId);

    _delegate.SetParameter(_cameraId, HdCameraTokens->windowPolicy,
        VtValue(CameraUtilFit));
    _delegate.SetParameter(_cameraId, HdCameraTokens->worldToViewMatrix,
        VtValue(GfMatrix4d(1.0)));
    _delegate.SetParameter(_cameraId, HdCameraTokens->projectionMatrix,
        VtValue(GfMatrix4d(1.0)));
    _delegate.SetParameter(_cameraId, HdCameraTokens->clipPlanes,
        VtValue(std::vector<GfVec4d>()));
}

void
HdxTaskController::_CreateRenderTasks()
{
    // Create two render tasks, one to create a color render, the other
    // to create an id render (so we don't need to thrash params).
    _renderTaskId = GetControllerId().AppendChild(_tokens->renderTask);
    _idRenderTaskId = GetControllerId().AppendChild(_tokens->idRenderTask);

    HdxRenderTaskParams renderParams;
    renderParams.camera = _cameraId;
    renderParams.viewport = GfVec4d(0,0,1,1);

    HdRprimCollection collection(HdTokens->geometry,
                                 HdReprSelector(HdReprTokens->smoothHull));
    collection.SetRootPath(SdfPath::AbsoluteRootPath());

    SdfPath const renderTasks[] = {
        _renderTaskId,
        _idRenderTaskId,
    };
    for (size_t i = 0; i < sizeof(renderTasks)/sizeof(renderTasks[0]); ++i) {
        GetRenderIndex()->InsertTask<HdxRenderTask>(&_delegate,
            renderTasks[i]);

        _delegate.SetParameter(renderTasks[i], HdTokens->params,
            renderParams);
        _delegate.SetParameter(renderTasks[i], HdTokens->children,
            SdfPathVector());
        _delegate.SetParameter(renderTasks[i], HdTokens->collection,
            collection);
    }
}

void
HdxTaskController::_CreateSelectionTask()
{
    // Create a selection highlighting task.
    _selectionTaskId = GetControllerId().AppendChild(_tokens->selectionTask);

    HdxSelectionTaskParams selectionParams;
    selectionParams.enableSelection = true;
    selectionParams.selectionColor = GfVec4f(1,1,0,1);
    selectionParams.locateColor = GfVec4f(0,0,1,1);

    GetRenderIndex()->InsertTask<HdxSelectionTask>(&_delegate,
        _selectionTaskId);

    _delegate.SetParameter(_selectionTaskId, HdTokens->params,
        selectionParams);
    _delegate.SetParameter(_selectionTaskId, HdTokens->children,
        SdfPathVector());
}

void
HdxTaskController::_CreateLightingTask()
{
    // Simple lighting task uses lighting state from Sprims.
    _simpleLightTaskId = GetControllerId().AppendChild(
        _tokens->simpleLightTask);

    HdxSimpleLightTaskParams simpleLightParams;
    simpleLightParams.cameraPath = _cameraId;

    GetRenderIndex()->InsertTask<HdxSimpleLightTask>(&_delegate,
        _simpleLightTaskId);

    _delegate.SetParameter(_simpleLightTaskId, HdTokens->params,
        simpleLightParams);
    _delegate.SetParameter(_simpleLightTaskId, HdTokens->children,
        SdfPathVector());
}

void
HdxTaskController::_CreateShadowTask() 
{
    _shadowTaskId = GetControllerId().AppendChild(_tokens->shadowTask);

    HdxShadowTaskParams shadowParams;
    shadowParams.camera = _cameraId;

    GetRenderIndex()->InsertTask<HdxShadowTask>(&_delegate, _shadowTaskId);

    _delegate.SetParameter(_shadowTaskId, HdTokens->params, shadowParams);
    _delegate.SetParameter(_shadowTaskId, HdTokens->children, SdfPathVector());
}

HdxTaskController::~HdxTaskController()
{
    GetRenderIndex()->RemoveSprim(HdPrimTypeTokens->camera, _cameraId);
    SdfPath const tasks[] = {
        _renderTaskId,
        _idRenderTaskId,
        _selectionTaskId,
        _simpleLightTaskId,
        _shadowTaskId,
    };
    for (size_t i = 0; i < sizeof(tasks)/sizeof(tasks[0]); ++i) {
        GetRenderIndex()->RemoveTask(tasks[i]);
    }
    TF_FOR_ALL (id, _lightIds) {
        GetRenderIndex()->RemoveSprim(HdPrimTypeTokens->simpleLight, *id);
    }
}

HdTaskSharedPtrVector const& 
HdxTaskController::GetTasks(TfToken const& taskSet)
{
    _tasks.clear();

    // Light - Only run simpleLightTask if the backend supports simpleLight...
    if (GetRenderIndex()->IsSprimTypeSupported(HdPrimTypeTokens->simpleLight)) {
        const HdxSimpleLightTaskParams& simpleLightParams =
            _delegate.GetParameter<HdxSimpleLightTaskParams>(
                _simpleLightTaskId, HdTokens->params);
        _tasks.push_back(GetRenderIndex()->GetTask(_simpleLightTaskId));

        // If shadows are enabled then we add the task to generate the 
        // shadow maps.
        if (simpleLightParams.enableShadows) {
            _tasks.push_back(GetRenderIndex()->GetTask(_shadowTaskId));
        }
    }

    // Render
    if (taskSet == HdxTaskSetTokens->idRender) {
        _tasks.push_back(GetRenderIndex()->GetTask(_idRenderTaskId));
    } else if (taskSet == HdxTaskSetTokens->colorRender) {
        _tasks.push_back(GetRenderIndex()->GetTask(_renderTaskId));
    }

    // Selection highlighting (overlay on color render).
    if (taskSet == HdxTaskSetTokens->colorRender) {
        _tasks.push_back(GetRenderIndex()->GetTask(_selectionTaskId));
    }

    return _tasks;
}

void
HdxTaskController::SetCollection(HdRprimCollection const& collection)
{
    SdfPath const tasks[] = {
        _renderTaskId,
        _idRenderTaskId,
    };
    // Update the collection for each task. Check only the first task
    // to see if the value changed; if so, update all tasks.
    HdRprimCollection oldCollection =
        _delegate.GetParameter<HdRprimCollection>(
            tasks[0], HdTokens->collection);
    if (oldCollection == collection) {
        return;
    }

    for(size_t i = 0; i < sizeof(tasks)/sizeof(tasks[0]); ++i) {
        _delegate.SetParameter(tasks[i], HdTokens->collection,
            collection);
        GetRenderIndex()->GetChangeTracker().MarkTaskDirty(
            tasks[i], HdChangeTracker::DirtyCollection);
    }
}

void
HdxTaskController::SetRenderParams(HdxRenderTaskParams const& params)
{
    // Update the render params. If params.enableIdRender is set, we update
    // the id render task params; otherwise, we update the color render task
    // params.
    SdfPath& task = params.enableIdRender ? _idRenderTaskId : _renderTaskId;

    HdxRenderTaskParams oldParams = _delegate.GetParameter<HdxRenderTaskParams>(
        task, HdTokens->params);
    // We explicitly ignore params.viewport and params.camera
    HdxRenderTaskParams mergedParams = params;
    mergedParams.camera = oldParams.camera;
    mergedParams.viewport = oldParams.viewport;

    if (mergedParams != oldParams) {
        _delegate.SetParameter(task, HdTokens->params, mergedParams);
        GetRenderIndex()->GetChangeTracker().MarkTaskDirty(
                task, HdChangeTracker::DirtyParams);

        // Update shadow task in case materials have been enabled/disabled
        if (GetRenderIndex()->IsSprimTypeSupported(
            HdPrimTypeTokens->simpleLight)) {

            HdxShadowTaskParams oldShParams = 
                _delegate.GetParameter<HdxShadowTaskParams>(
                _shadowTaskId, HdTokens->params);

            if (oldShParams.enableSceneMaterials != 
                mergedParams.enableSceneMaterials) {

                oldShParams.enableSceneMaterials = 
                    mergedParams.enableSceneMaterials;
                _delegate.SetParameter(_shadowTaskId, 
                    HdTokens->params, oldShParams);

                GetRenderIndex()->GetChangeTracker().MarkTaskDirty(
                    _shadowTaskId, HdChangeTracker::DirtyParams);
            }
        }
    }
}

void
HdxTaskController::SetEnableShadows(bool enable)
{
    if (!GetRenderIndex()->IsSprimTypeSupported(HdPrimTypeTokens->simpleLight)){
        return;
    }

    HdxSimpleLightTaskParams params =
        _delegate.GetParameter<HdxSimpleLightTaskParams>(
            _simpleLightTaskId, HdTokens->params);

    if (params.enableShadows != enable) {
        params.enableShadows = enable;
        _delegate.SetParameter(_simpleLightTaskId, HdTokens->params, params);
        GetRenderIndex()->GetChangeTracker().MarkTaskDirty(
            _simpleLightTaskId, HdChangeTracker::DirtyParams);
    }
}

void
HdxTaskController::SetEnableSelection(bool enable)
{
    HdxSelectionTaskParams params =
        _delegate.GetParameter<HdxSelectionTaskParams>(
            _selectionTaskId, HdTokens->params);

    if (params.enableSelection != enable) {
        params.enableSelection = enable;
        _delegate.SetParameter(_selectionTaskId, HdTokens->params, params);
        GetRenderIndex()->GetChangeTracker().MarkTaskDirty(
            _selectionTaskId, HdChangeTracker::DirtyParams);
    }
}

void
HdxTaskController::SetSelectionColor(GfVec4f const& color)
{
    HdxSelectionTaskParams params =
        _delegate.GetParameter<HdxSelectionTaskParams>(
            _selectionTaskId, HdTokens->params);

    if (params.selectionColor != color) {
        params.selectionColor = color;
        _delegate.SetParameter(_selectionTaskId, HdTokens->params, params);
        GetRenderIndex()->GetChangeTracker().MarkTaskDirty(
            _selectionTaskId, HdChangeTracker::DirtyParams);
    }
}

void
HdxTaskController::SetPickResolution(unsigned int size)
{
    _intersector->SetResolution(GfVec2i(size, size));
}

bool
HdxTaskController::TestIntersection(
        HdEngine* engine,
        HdRprimCollection const& collection,
        HdxIntersector::Params const& qparams,
        TfToken const& intersectionMode,
        HdxIntersector::HitVector *allHits)
{
    if (allHits == nullptr) {
        TF_CODING_ERROR("Null hit vector passed to TestIntersection");
        return false;
    }

    HdxIntersector::Result result;
    if (!_intersector->Query(qparams, collection, engine, &result)) {
        return false;
    }

    if (intersectionMode == HdxIntersectionModeTokens->nearest) {
        HdxIntersector::Hit hit;
        if (!result.ResolveNearestToCenter(&hit)) {
            return false;
        }
        allHits->push_back(hit);
    } else if (intersectionMode == HdxIntersectionModeTokens->unique) {
        HdxIntersector::HitSet hits;
        if (!result.ResolveUnique(&hits)) {
            return false;
        }
        allHits->assign(hits.begin(), hits.end());
    } else if (intersectionMode == HdxIntersectionModeTokens->all) {
        if (!result.ResolveAll(allHits)) {
            return false;
        }
    }

    return true;
}

void
HdxTaskController::SetLightingState(GlfSimpleLightingContextPtr const& src)
{
    // If the backend doesn't support simpleLight, no need to set parameters
    // for simpleLightTask, or create simpleLight prims for lights in the
    // lighting context.
    if (!GetRenderIndex()->IsSprimTypeSupported(HdPrimTypeTokens->simpleLight)){
        return;
    }

    if (!src) {
        TF_CODING_ERROR("Null lighting context");
        return;
    }

    GlfSimpleLightVector const& lights = src->GetLights();

    // HdxTaskController inserts a set of light prims to represent the lights
    // passed in through the simple lighting context. These are managed by
    // the task controller, and not by the scene; they represent transient
    // application state such as camera lights.
    //
    // The light pool can be re-used as lights change, but we need to make sure
    // we have the right number of light prims. Add them as necessary until
    // there are enough light prims to represent the light context.
    while (_lightIds.size() < lights.size()) {
        SdfPath lightId = GetControllerId().AppendChild(TfToken(
            TfStringPrintf("light%d", (int)_lightIds.size())));
        _lightIds.push_back(lightId);

        GetRenderIndex()->InsertSprim(HdPrimTypeTokens->simpleLight, 
            &_delegate,
            lightId);

        // After inserting a light, initialize its parameters and mark the light
        // as dirty.
        _delegate.SetParameter(lightId, HdLightTokens->transform,
            VtValue());
        _delegate.SetParameter(lightId, HdLightTokens->shadowParams,
            HdxShadowParams());
        _delegate.SetParameter(lightId, HdLightTokens->shadowCollection,
            VtValue());
        _delegate.SetParameter(lightId, HdLightTokens->params,
            GlfSimpleLight());

        // Note: Marking the shadowCollection as dirty (included in AllDirty)
        // will mark the geometry collection dirty.
        GetRenderIndex()->GetChangeTracker().MarkSprimDirty(lightId,
            HdLight::AllDirty);
    }

    // If the light pool is too big for the light context, remove the extra
    // sprims.
    while (_lightIds.size() > lights.size()) {
        GetRenderIndex()->RemoveSprim(HdPrimTypeTokens->simpleLight,
            _lightIds.back());

        _lightIds.pop_back();
    }

    // Update light Sprims to match the lights passed in through the context;
    // hydra simpleLight prims store a GlfSimpleLight as their "params" field.
    for (size_t i = 0; i < lights.size(); ++i) {
        GlfSimpleLight lt = _delegate.GetParameter<GlfSimpleLight>(
            _lightIds[i], HdLightTokens->params);

        if (lt != lights[i]) {
            _delegate.SetParameter(_lightIds[i], HdLightTokens->params,
                lights[i]);

            GetRenderIndex()->GetChangeTracker().MarkSprimDirty(
                _lightIds[i], HdLight::DirtyParams);
        }
    }

    // In addition to lights, the lighting context contains material parameters.
    // These are passed in through the simple light task's "params" field, so
    // we need to update that field if the material parameters changed.
    //
    // It's unfortunate that the lighting context is split this way.
    HdxSimpleLightTaskParams lightParams =
        _delegate.GetParameter<HdxSimpleLightTaskParams>(_simpleLightTaskId,
            HdTokens->params);

    if (lightParams.sceneAmbient != src->GetSceneAmbient() ||
        lightParams.material != src->GetMaterial()) {

        lightParams.sceneAmbient = src->GetSceneAmbient();
        lightParams.material = src->GetMaterial();

        _delegate.SetParameter(
            _simpleLightTaskId, HdTokens->params, lightParams);
        GetRenderIndex()->GetChangeTracker().MarkTaskDirty(
            _simpleLightTaskId, HdChangeTracker::DirtyParams);
    }
}

void
HdxTaskController::SetCameraMatrices(GfMatrix4d const& viewMatrix,
                                     GfMatrix4d const& projMatrix)
{
    GfMatrix4d oldView = _delegate.GetParameter<GfMatrix4d>(_cameraId,
        HdCameraTokens->worldToViewMatrix);

    if (viewMatrix != oldView) {
        // Cache the new view matrix
        _delegate.SetParameter(_cameraId, HdCameraTokens->worldToViewMatrix,
            viewMatrix);
        // Invalidate the camera
        GetRenderIndex()->GetChangeTracker().MarkSprimDirty(_cameraId,
            HdCamera::DirtyViewMatrix);
    }

    GfMatrix4d oldProj = _delegate.GetParameter<GfMatrix4d>(_cameraId,
        HdCameraTokens->projectionMatrix);

    if (projMatrix != oldProj) {
        // Cache the new proj matrix
        _delegate.SetParameter(_cameraId, HdCameraTokens->projectionMatrix,
            projMatrix);
        // Invalidate the camera
        GetRenderIndex()->GetChangeTracker().MarkSprimDirty(_cameraId,
            HdCamera::DirtyProjMatrix);
    }
}

void
HdxTaskController::SetCameraViewport(GfVec4d const& viewport)
{
    SdfPath const tasks[] = {
        _renderTaskId,
        _idRenderTaskId,
    };
    // Update the viewport in the task params for each task.
    // Check only the first task to see if the value changed;
    // if so, update all tasks.
    GfVec4d oldViewport = _delegate.GetParameter<HdxRenderTaskParams>(
            tasks[0], HdTokens->params).viewport;
    if (oldViewport == viewport) {
        return;
    }

    for(size_t i = 0; i < sizeof(tasks)/sizeof(tasks[0]); ++i) {
        HdxRenderTaskParams params =
            _delegate.GetParameter<HdxRenderTaskParams>(
                tasks[i], HdTokens->params);
        params.viewport = viewport;
        _delegate.SetParameter(tasks[i], HdTokens->params, params);
        GetRenderIndex()->GetChangeTracker().MarkTaskDirty(
            tasks[i], HdChangeTracker::DirtyParams);
    }

    if (GetRenderIndex()->IsSprimTypeSupported(HdPrimTypeTokens->simpleLight)) {
        // The shadow and camera viewport should be the same
        // so we don't have to double check what the shadow task has.
        HdxShadowTaskParams params =
            _delegate.GetParameter<HdxShadowTaskParams>(
                _shadowTaskId, HdTokens->params);
        params.viewport = viewport;
        _delegate.SetParameter(_shadowTaskId, HdTokens->params, params);
        GetRenderIndex()->GetChangeTracker().MarkTaskDirty(
            _shadowTaskId, HdChangeTracker::DirtyParams);
    }
}

void
HdxTaskController::SetCameraClipPlanes(std::vector<GfVec4d> const& clipPlanes)
{
    // Cache the clip planes
    std::vector<GfVec4d> oldClipPlanes =
        _delegate.GetParameter<std::vector<GfVec4d>>(_cameraId,
            HdCameraTokens->clipPlanes);

    if (oldClipPlanes != clipPlanes) {
        _delegate.SetParameter(_cameraId, HdCameraTokens->clipPlanes,
            clipPlanes);
        GetRenderIndex()->GetChangeTracker().MarkSprimDirty(_cameraId,
            HdCamera::DirtyClipPlanes);
    }
}

void
HdxTaskController::SetCameraWindowPolicy(
    CameraUtilConformWindowPolicy windowPolicy)
{
    // Cache the window policy, if needed
    const CameraUtilConformWindowPolicy oldPolicy =
        _delegate.GetParameter<CameraUtilConformWindowPolicy>(
            _cameraId, HdCameraTokens->windowPolicy);

    if (oldPolicy != windowPolicy) {
        _delegate.SetParameter(_cameraId, HdCameraTokens->windowPolicy,
            windowPolicy);
        GetRenderIndex()->GetChangeTracker().MarkSprimDirty(_cameraId,
            HdCamera::DirtyWindowPolicy);
    }
}

bool
HdxTaskController::IsConverged() const
{
    // Pass this call through to HdxRenderTask's IsConverged().
    return static_cast<HdxRenderTask*>(
            GetRenderIndex()->GetTask(_renderTaskId).get())
        ->IsConverged();
}

PXR_NAMESPACE_CLOSE_SCOPE
