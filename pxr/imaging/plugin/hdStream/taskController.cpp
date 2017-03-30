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
#include "pxr/imaging/hdStream/taskController.h"

#include "pxr/imaging/hdSt/camera.h"
#include "pxr/imaging/hdSt/light.h"
#include "pxr/imaging/hdx/intersector.h"
#include "pxr/imaging/hdx/renderTask.h"
#include "pxr/imaging/hdx/selectionTask.h"
#include "pxr/imaging/hdx/simpleLightTask.h"
#include "pxr/imaging/hdx/simpleLightBypassTask.h"
#include "pxr/imaging/hdx/tokens.h"

#include "pxr/imaging/glf/simpleLight.h"
#include "pxr/imaging/glf/simpleLightingContext.h"

PXR_NAMESPACE_OPEN_SCOPE

// ---------------------------------------------------------------------------
// Delegate implementation.

/* virtual */
VtValue
HdStreamTaskController::_Delegate::Get(SdfPath const& id, TfToken const& key)
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
HdStreamTaskController::_Delegate::IsEnabled(TfToken const& option) const
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
HdStreamTaskController::_Delegate::GetClipPlanes(SdfPath const& cameraId)
{
    return GetParameter<std::vector<GfVec4d>>(cameraId,
                HdStCameraTokens->clipPlanes);
}

// ---------------------------------------------------------------------------
// Task controller implementation.

TF_DEFINE_PRIVATE_TOKENS(
    _tokens,
    (idRenderTask)
    (renderTask)
    (selectionTask)
    (simpleLightTask)
    (simpleLightBypassTask)
    (camera)
);

HdStreamTaskController::HdStreamTaskController(HdRenderIndex *renderIndex,
                                               SdfPath const& controllerId)
    : HdxTaskController(renderIndex, controllerId)
    , _intersector(new HdxIntersector(renderIndex))
    , _delegate(renderIndex, controllerId)
{
    // We create camera and tasks here, but lights are created lazily by
    // SetLightingState. Camera needs to be created first, since it's a
    // parameter of most tasks.

    _CreateCamera();
    _CreateRenderTasks();
    _CreateSelectionTask();
    _CreateLightingTasks();
}

void
HdStreamTaskController::_CreateCamera()
{
    // Create a default camera, driven by SetCameraMatrices.
    _cameraId = GetControllerId().AppendChild(_tokens->camera);
    GetRenderIndex()->InsertSprim(HdPrimTypeTokens->camera,
        &_delegate, _cameraId);

    _delegate.SetParameter(_cameraId, HdStCameraTokens->windowPolicy,
        VtValue());
    _delegate.SetParameter(_cameraId, HdStCameraTokens->matrices,
        HdStCameraMatrices());
    _delegate.SetParameter(_cameraId, HdStCameraTokens->clipPlanes,
        VtValue(std::vector<GfVec4d>()));
}

void
HdStreamTaskController::_CreateRenderTasks()
{
    // Create two render tasks, one to create a color render, the other
    // to create an id render (so we don't need to thrash params).
    _renderTaskId = GetControllerId().AppendChild(_tokens->renderTask);
    _idRenderTaskId = GetControllerId().AppendChild(_tokens->idRenderTask);

    HdxRenderTaskParams renderParams;
    renderParams.camera = _cameraId;
    renderParams.viewport = GfVec4d(0,0,1,1);

    HdRprimCollection collection(HdTokens->geometry, HdTokens->smoothHull);
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
HdStreamTaskController::_CreateSelectionTask()
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
HdStreamTaskController::_CreateLightingTasks()
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

    _simpleLightBypassTaskId = GetControllerId().AppendChild(
        _tokens->simpleLightBypassTask);
    
    // Simple lighting bypass task uses lighting state from a lighting
    // context.
    HdxSimpleLightBypassTaskParams simpleLightBypassParams;
    simpleLightBypassParams.cameraPath = _cameraId;

    GetRenderIndex()->InsertTask<HdxSimpleLightBypassTask>(&_delegate,
        _simpleLightBypassTaskId);

    _delegate.SetParameter(_simpleLightBypassTaskId, HdTokens->params,
        simpleLightBypassParams);
    _delegate.SetParameter(_simpleLightBypassTaskId, HdTokens->children,
        SdfPathVector());
}

/* virtual */
HdStreamTaskController::~HdStreamTaskController()
{
    GetRenderIndex()->RemoveSprim(HdPrimTypeTokens->camera, _cameraId);
    SdfPath const tasks[] = {
        _renderTaskId,
        _idRenderTaskId,
        _selectionTaskId,
        _simpleLightTaskId,
        _simpleLightBypassTaskId,
    };
    for (size_t i = 0; i < sizeof(tasks)/sizeof(tasks[0]); ++i) {
        GetRenderIndex()->RemoveTask(tasks[i]);
    }
    TF_FOR_ALL (id, _lightIds) {
        GetRenderIndex()->RemoveSprim(HdPrimTypeTokens->light, *id);
    }
}

/* virtual */
HdTaskSharedPtrVector const& 
HdStreamTaskController::GetTasks(TfToken const& taskSet)
{
    _tasks.clear();

    // Light
    if (!_activeLightTaskId.IsEmpty()) {
        _tasks.push_back(GetRenderIndex()->GetTask(_activeLightTaskId));
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

/* virtual */
void
HdStreamTaskController::SetCollection(HdRprimCollection const& collection)
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

/* virtual */
void
HdStreamTaskController::SetRenderParams(HdxRenderTaskParams const& params)
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
    }
}

/* virtual */
void
HdStreamTaskController::SetEnableSelection(bool enable)
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

/* virtual */
void
HdStreamTaskController::SetSelectionColor(GfVec4f const& color)
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

/* virtual */
void
HdStreamTaskController::SetPickResolution(unsigned int size)
{
    _intersector->SetResolution(GfVec2i(size, size));
}

/* virtual */
bool
HdStreamTaskController::TestIntersection(
        HdEngine* engine,
        GfMatrix4d const& viewMatrix,
        GfMatrix4d const& projMatrix,
        HdRprimCollection const& collection,
        float alphaThreshold,
        HdCullStyle cullStyle,
        TfToken const& intersectionMode,
        HdxIntersector::HitVector *allHits)
{
    if (allHits == nullptr) {
        TF_CODING_ERROR("Null hit vector passed to TestIntersection");
        return false;
    }

    HdxIntersector::Params qparams;
    qparams.viewMatrix = viewMatrix;
    qparams.projectionMatrix = projMatrix;
    qparams.alphaThreshold = alphaThreshold;
    qparams.cullStyle = cullStyle;

    HdxIntersector::Result result;
    if (!_intersector->Query(qparams, collection, engine, &result)) {
        return false;
    }

    if (intersectionMode == HdxIntersectionModeTokens->nearest) {
        HdxIntersector::Hit hit;
        if (!result.ResolveNearest(&hit)) {
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

/* virtual */
void
HdStreamTaskController::SetLightingState(GlfSimpleLightingContextPtr const& src,
                                         bool bypass)
{
    if (bypass) {
        // If we're using HdxSimpleLightBypassTask, we just pass the context
        // through to the task.
        HdxSimpleLightBypassTaskParams params =
            _delegate.GetParameter<HdxSimpleLightBypassTaskParams>(
                _simpleLightBypassTaskId, HdTokens->params);

        params.simpleLightingContext = src;
        _delegate.SetParameter(_simpleLightBypassTaskId, HdTokens->params,
            params);

        GetRenderIndex()->GetChangeTracker().MarkTaskDirty(
            _simpleLightBypassTaskId, HdChangeTracker::DirtyParams);

        _activeLightTaskId = _simpleLightBypassTaskId;
        return;
    }

    // Otherwise, we need to map the context into light Sprims, and update
    // the lighting task params.
    if (!src) {
        TF_CODING_ERROR("Null lighting context");
        return;
    }

    GlfSimpleLightVector const& lights = src->GetLights();
    bool hasNumLightsChanged = false;

    // Create or remove Sprims so that the render index has the correct
    // number of lights.
    while (_lightIds.size() < lights.size()) {
        SdfPath lightId = GetControllerId().AppendChild(TfToken(
            TfStringPrintf("light%d", (int)_lightIds.size())));
        _lightIds.push_back(lightId);

        GetRenderIndex()->InsertSprim(HdPrimTypeTokens->light, &_delegate,
            lightId);
        hasNumLightsChanged = true;
    }
    while (_lightIds.size() > lights.size()) {
        GetRenderIndex()->RemoveSprim(HdPrimTypeTokens->light,
            _lightIds.back());

        _lightIds.pop_back();
        hasNumLightsChanged = true;
    }

    // Update light Sprims
    for (size_t i = 0; i < lights.size(); ++i) {
        _delegate.SetParameter(_lightIds[i], HdStLightTokens->params,
            lights[i]);
        _delegate.SetParameter(_lightIds[i], HdStLightTokens->transform,
            VtValue());
        _delegate.SetParameter(_lightIds[i], HdStLightTokens->shadowParams,
            HdxShadowParams());
        _delegate.SetParameter(_lightIds[i], HdStLightTokens->shadowCollection,
            VtValue());

        // Only mark the parameters dirty to avoid unnecessary invalidation.
        // Marking the shadowCollection as dirty will mark the geometry
        // collection dirty and we don't want that to happen every time.
        GetRenderIndex()->GetChangeTracker().MarkSprimDirty(
            _lightIds[i], HdStLight::DirtyParams);
    }

    // Update the material: sadly, this comes from the lighting context
    // and lives in HdxSimpleLightTaskParams right now.
    //
    // HdxSimpleLightTask::Sync() pulls the list of lights on dirty params,
    // so if we've changed the number of lights we should mark params dirty,
    // even if params are the same...
    HdxSimpleLightTaskParams params =
        _delegate.GetParameter<HdxSimpleLightTaskParams>(_simpleLightTaskId,
            HdTokens->params);

    if (params.sceneAmbient != src->GetSceneAmbient() ||
        params.material != src->GetMaterial() ||
        hasNumLightsChanged) {

        params.sceneAmbient = src->GetSceneAmbient();
        params.material = src->GetMaterial();

        _delegate.SetParameter(_simpleLightTaskId, HdTokens->params, params);
        GetRenderIndex()->GetChangeTracker().MarkTaskDirty(
            _simpleLightTaskId, HdChangeTracker::DirtyParams);
    }
    _activeLightTaskId = _simpleLightTaskId;
}

/* virtual */
void
HdStreamTaskController::SetCameraMatrices(GfMatrix4d const& viewMatrix,
                                          GfMatrix4d const& projMatrix)
{
    HdStCameraMatrices oldMatrices =
        _delegate.GetParameter<HdStCameraMatrices>(
            _cameraId, HdStCameraTokens->matrices);

    HdStCameraMatrices newMatrices = HdStCameraMatrices(viewMatrix, projMatrix);
    if (oldMatrices != newMatrices) {
        // Cache the camera matrices
        _delegate.SetParameter(_cameraId, HdStCameraTokens->matrices,
            newMatrices);
        // Invalidate the camera.
        GetRenderIndex()->GetChangeTracker().MarkSprimDirty(_cameraId,
            HdStCamera::DirtyMatrices);
    }
}

/* virtual */
void
HdStreamTaskController::SetCameraViewport(GfVec4d const& viewport)
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
}

/* virtual */
void
HdStreamTaskController::SetCameraClipPlanes(
    std::vector<GfVec4d> const& clipPlanes)
{
    // Cache the clip planes
    std::vector<GfVec4d> oldClipPlanes =
        _delegate.GetParameter<std::vector<GfVec4d>>(_cameraId,
            HdStCameraTokens->clipPlanes);

    if (oldClipPlanes != clipPlanes) {
        _delegate.SetParameter(_cameraId, HdStCameraTokens->clipPlanes,
            clipPlanes);
        GetRenderIndex()->GetChangeTracker().MarkSprimDirty(_cameraId,
            HdStCamera::DirtyClipPlanes);
    }
}

PXR_NAMESPACE_CLOSE_SCOPE
