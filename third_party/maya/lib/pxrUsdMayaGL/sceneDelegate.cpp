//
// Copyright 2018 Pixar
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
#include "pxrUsdMayaGL/sceneDelegate.h"

#include "pxr/pxr.h"
#include "pxrUsdMayaGL/api.h"
#include "pxrUsdMayaGL/renderParams.h"

#include "px_vp20/utils.h"

#include "pxr/base/gf/matrix4d.h"
#include "pxr/base/gf/vec4d.h"
#include "pxr/base/gf/vec4f.h"
#include "pxr/base/tf/diagnostic.h"
#include "pxr/base/tf/staticTokens.h"
#include "pxr/base/tf/stl.h"
#include "pxr/base/tf/stringUtils.h"
#include "pxr/base/tf/token.h"
#include "pxr/base/vt/value.h"
#include "pxr/imaging/glf/simpleLightingContext.h"
#include "pxr/imaging/hd/renderIndex.h"
#include "pxr/imaging/hd/rprimCollection.h"
#include "pxr/imaging/hd/sceneDelegate.h"
#include "pxr/imaging/hd/task.h"
#include "pxr/imaging/hd/tokens.h"
#include "pxr/imaging/hdSt/camera.h"
#include "pxr/imaging/hdSt/light.h"
#include "pxr/imaging/hdx/renderSetupTask.h"
#include "pxr/imaging/hdx/renderTask.h"
#include "pxr/imaging/hdx/selectionTask.h"
#include "pxr/imaging/hdx/simpleLightTask.h"
#include "pxr/imaging/hdx/tokens.h"
#include "pxr/usd/sdf/path.h"

#include <maya/MDrawContext.h>


PXR_NAMESPACE_OPEN_SCOPE


TF_DEFINE_PRIVATE_TOKENS(
    _tokens,

    (selectionTask)
);


PxrMayaHdSceneDelegate::PxrMayaHdSceneDelegate(
        HdRenderIndex* renderIndex,
        const SdfPath& delegateID) :
    HdSceneDelegate(renderIndex, delegateID)
{
    _lightingContext = GlfSimpleLightingContext::New();

    // populate tasks in renderindex

    // create an unique namespace
    _rootId = delegateID.AppendChild(
        TfToken(TfStringPrintf("_UsdImaging_%p", this)));

    _simpleLightTaskId = _rootId.AppendChild(HdxPrimitiveTokens->simpleLightTask);
    _cameraId = _rootId.AppendChild(HdPrimTypeTokens->camera);

    // camera
    {
        // Since the batch renderer is hardcoded to use HdStRenderDelegate, we
        // expect to have camera Sprims.
        TF_VERIFY(renderIndex->IsSprimTypeSupported(HdPrimTypeTokens->camera));

        renderIndex->InsertSprim(HdPrimTypeTokens->camera, this, _cameraId);
        _ValueCache& cache = _valueCacheMap[_cameraId];
        cache[HdStCameraTokens->worldToViewMatrix] = VtValue(GfMatrix4d(1.0));
        cache[HdStCameraTokens->projectionMatrix] = VtValue(GfMatrix4d(1.0));
        cache[HdStCameraTokens->windowPolicy] = VtValue();  // no window policy.
    }

    // simple lighting task (for Hydra native)
    {
        renderIndex->InsertTask<HdxSimpleLightTask>(this, _simpleLightTaskId);
        _ValueCache& cache = _valueCacheMap[_simpleLightTaskId];
        HdxSimpleLightTaskParams taskParams;
        taskParams.cameraPath = _cameraId;
        taskParams.viewport = GfVec4f(_viewport);
        cache[HdTokens->params] = VtValue(taskParams);
        cache[HdTokens->children] = VtValue(SdfPathVector());
    }
}

/*virtual*/
VtValue
PxrMayaHdSceneDelegate::Get(const SdfPath& id, const TfToken& key)
{
    _ValueCache* vcache = TfMapLookupPtr(_valueCacheMap, id);
    VtValue ret;
    if (vcache && TfMapLookup(*vcache, key, &ret)) {
        return ret;
    }

    TF_CODING_ERROR("%s:%s doesn't exist in the value cache\n",
                    id.GetText(),
                    key.GetText());

    return VtValue();
}

void
PxrMayaHdSceneDelegate::SetCameraState(
        const GfMatrix4d& worldToViewMatrix,
        const GfMatrix4d& projectionMatrix,
        const GfVec4d& viewport)
{
    // cache the camera matrices
    _ValueCache& cache = _valueCacheMap[_cameraId];
    cache[HdStCameraTokens->worldToViewMatrix] = VtValue(worldToViewMatrix);
    cache[HdStCameraTokens->projectionMatrix] = VtValue(projectionMatrix);
    cache[HdStCameraTokens->windowPolicy] = VtValue(); // no window policy.

    // invalidate the camera to be synced
    GetRenderIndex().GetChangeTracker().MarkSprimDirty(_cameraId,
                                                       HdStCamera::AllDirty);

    if (_viewport != viewport) {
        _viewport = viewport;

        // Update the simple light task.
        HdxSimpleLightTaskParams simpleLightTaskParams =
            _GetValue<HdxSimpleLightTaskParams>(_simpleLightTaskId,
                                                HdTokens->params);

        simpleLightTaskParams.viewport = GfVec4f(_viewport);
        _SetValue(_simpleLightTaskId, HdTokens->params, simpleLightTaskParams);

        GetRenderIndex().GetChangeTracker().MarkTaskDirty(
            _simpleLightTaskId,
            HdChangeTracker::DirtyParams);

        // Update all render setup tasks.
        for (const auto& it : _renderSetupTaskIdMap) {
            const SdfPath& renderSetupTaskId = it.second;

            HdxRenderTaskParams renderSetupTaskParams =
                _GetValue<HdxRenderTaskParams>(renderSetupTaskId,
                                               HdTokens->params);

            renderSetupTaskParams.viewport = _viewport;
            _SetValue(renderSetupTaskId,
                      HdTokens->params,
                      renderSetupTaskParams);

            GetRenderIndex().GetChangeTracker().MarkTaskDirty(
                renderSetupTaskId,
                HdChangeTracker::DirtyParams);
        }
    }
}

void
PxrMayaHdSceneDelegate::SetLightingStateFromVP1(
        const GfMatrix4d& worldToViewMatrix,
        const GfMatrix4d& projectionMatrix)
{
    // This function should only be called in a VP1.0 context. In VP2.0, we can
    // translate the lighting state from the MDrawContext directly into Glf,
    // but there is no draw context in VP1.0, so we have to transfer the state
    // through OpenGL.

    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadMatrixd(worldToViewMatrix.GetArray());
    _lightingContext->SetStateFromOpenGL();
    glPopMatrix();

    _lightingContext->SetCamera(worldToViewMatrix, projectionMatrix);

    _SetLightingStateFromLightingContext();
}

void
PxrMayaHdSceneDelegate::SetLightingStateFromMayaDrawContext(
        const MHWRender::MDrawContext& context)
{
    _lightingContext = px_vp20Utils::GetLightingContextFromDrawContext(context);

    _SetLightingStateFromLightingContext();
}

void
PxrMayaHdSceneDelegate::_SetLightingStateFromLightingContext()
{
    const GlfSimpleLightVector& lights = _lightingContext->GetLights();

    bool hasNumLightsChanged = false;

    // Insert light Ids into the render index for those that do not yet exist.
    while (_lightIds.size() < lights.size()) {
        const SdfPath lightId(
            TfStringPrintf("%s/light%zu", _rootId.GetText(), _lightIds.size()));
        _lightIds.push_back(lightId);

        // Since we're hardcoded to use HdStRenderDelegate, we expect to have
        // light Sprims.
        TF_VERIFY(GetRenderIndex().
            IsSprimTypeSupported(HdPrimTypeTokens->simpleLight));

        GetRenderIndex().
            InsertSprim(HdPrimTypeTokens->simpleLight, this, lightId);
        hasNumLightsChanged = true;
    }

    // Remove unused light Ids from HdRenderIndex
    while (_lightIds.size() > lights.size()) {
        GetRenderIndex().
            RemoveSprim(HdPrimTypeTokens->simpleLight, _lightIds.back());
        _lightIds.pop_back();
        hasNumLightsChanged = true;
    }

    // invalidate HdLights
    for (size_t i = 0; i < lights.size(); ++i) {
        _ValueCache& cache = _valueCacheMap[_lightIds[i]];
        // store GlfSimpleLight directly.
        cache[HdLightTokens->params] = VtValue(lights[i]);
        cache[HdLightTokens->transform] = VtValue();
        cache[HdLightTokens->shadowParams] = VtValue(HdxShadowParams());
        cache[HdLightTokens->shadowCollection] = VtValue();

        // Only mark as dirty the parameters to avoid unnecessary invalidation
        // specially marking as dirty lightShadowCollection will trigger
        // a collection dirty on geometry and we don't want that to happen
        // always
        GetRenderIndex().GetChangeTracker().MarkSprimDirty(
            _lightIds[i],
            HdStLight::AllDirty);
    }

    // sadly the material also comes from lighting context right now...
    HdxSimpleLightTaskParams taskParams =
        _GetValue<HdxSimpleLightTaskParams>(_simpleLightTaskId,
                                            HdTokens->params);
    taskParams.sceneAmbient = _lightingContext->GetSceneAmbient();
    taskParams.material = _lightingContext->GetMaterial();

    // invalidate HdxSimpleLightTask too
    if (hasNumLightsChanged) {
        _SetValue(_simpleLightTaskId, HdTokens->params, taskParams);

        GetRenderIndex().GetChangeTracker().MarkTaskDirty(
            _simpleLightTaskId,
            HdChangeTracker::DirtyParams);
    }
}

HdTaskSharedPtrVector
PxrMayaHdSceneDelegate::GetSetupTasks()
{
    HdTaskSharedPtrVector tasks;

    tasks.push_back(GetRenderIndex().GetTask(_simpleLightTaskId));

    return tasks;
}

HdTaskSharedPtrVector
PxrMayaHdSceneDelegate::GetRenderTasks(
        const size_t hash,
        const PxrMayaHdRenderParams& renderParams,
        const HdRprimCollectionVector& rprimCollections)
{
    SdfPath renderSetupTaskId;
    if (!TfMapLookup(_renderSetupTaskIdMap, hash, &renderSetupTaskId)) {
        // Create a new render setup task if one does not exist for this hash.
        renderSetupTaskId = _rootId.AppendChild(
            TfToken(TfStringPrintf("%s_%zx",
                                   HdxPrimitiveTokens->renderSetupTask.GetText(),
                                   hash)));

        GetRenderIndex().InsertTask<HdxRenderSetupTask>(this,
                                                        renderSetupTaskId);

        HdxRenderTaskParams renderSetupTaskParams;
        renderSetupTaskParams.camera = _cameraId;
        // Initialize viewport to the latest value since render setup tasks can
        // be lazily instantiated, potentially even after SetCameraState().
        renderSetupTaskParams.viewport = _viewport;

        _ValueCache& cache = _valueCacheMap[renderSetupTaskId];
        cache[HdTokens->params] = VtValue(renderSetupTaskParams);
        cache[HdTokens->children] = VtValue(SdfPathVector());
        cache[HdTokens->collection] = VtValue();

        _renderSetupTaskIdMap[hash] = renderSetupTaskId;
    }

    SdfPath renderTaskId;
    if (!TfMapLookup(_renderTaskIdMap, hash, &renderTaskId)) {
        // Create a new render task if one does not exist for this hash.
        renderTaskId = _rootId.AppendChild(
            TfToken(TfStringPrintf("%s_%zx",
                                   HdxPrimitiveTokens->renderTask.GetText(),
                                   hash)));

        GetRenderIndex().InsertTask<HdxRenderTask>(this, renderTaskId);

        // Note that the render task has no params of its own. All of the
        // render params are on the render setup task instead.
        _ValueCache& cache = _valueCacheMap[renderTaskId];
        cache[HdTokens->params] = VtValue();
        cache[HdTokens->children] = VtValue(SdfPathVector());
        cache[HdTokens->collection] = VtValue();

        _renderTaskIdMap[hash] = renderTaskId;
    }

    SdfPath selectionTaskId;
    if (!TfMapLookup(_selectionTaskIdMap, hash, &selectionTaskId)) {
        // Create a new selection task if one does not exist for this hash.
        selectionTaskId = _rootId.AppendChild(
            TfToken(TfStringPrintf("%s_%zx",
                                   _tokens->selectionTask.GetText(),
                                   hash)));

        GetRenderIndex().InsertTask<HdxSelectionTask>(this, selectionTaskId);
        HdxSelectionTaskParams selectionTaskParams;
        selectionTaskParams.enableSelection = true;

        // Note that the selection color is a constant zero value. This is to
        // mimic selection behavior in Maya where the wireframe color is what
        // changes to indicate selection and not the object color. As a result,
        // we don't need to dirty the selectionTaskParams below.
        selectionTaskParams.selectionColor = GfVec4f(0.0f);

        _ValueCache& cache = _valueCacheMap[selectionTaskId];
        cache[HdTokens->params] = VtValue(selectionTaskParams);
        cache[HdTokens->children] = VtValue(SdfPathVector());
        cache[HdTokens->collection] = VtValue();

        _selectionTaskIdMap[hash] = selectionTaskId;
    }

    //
    // (meta-XXX): The notes below are actively being addressed with an
    // HdRprimCollection now being created and managed by the shape adapter of
    // each shape being drawn. I'm leaving the full notes intact while work
    // continues, as they've been immensely helpful in optimizing our usage of
    // the Hydra API.
    //
    // ------------------------------------------------------------------------
    //
    // XXX: The Maya-Hydra plugin needs refactoring such that the plugin is
    // creating a different collection name for each collection it is trying to
    // manage. (i.e. Each collection within a frame that has different content
    // should have a different collection name)
    //
    // With Hydra, changing the contents of a collection can be
    // an expensive operation as it causes draw batches to be rebuilt.
    //
    // The Maya-Hydra Plugin is currently reusing the same collection
    // name for all collections within a frame.
    // (This stems from a time when collection name had a significant meaning
    // rather than id'ing a collection).
    //
    // The plugin should also track deltas to the contents of a collection
    // and set Hydra's dirty state when prims get added and removed from
    // the collection.
    //
    // Another possible change that can be made to this code is HdxRenderTask
    // now takes an array of collections, so it is possible to support different
    // reprs using the same task.  Therefore, this code should be modified to
    // only add one task that is provided with the active set of collections.
    //
    // However, a further improvement to the code could be made using
    // UsdDelegate's fallback repr feature instead of using multiple
    // collections as it would avoid modifying the collection as a Maya shape
    // object display state changes.  This would result in a much cheaper state
    // transition within Hydra itself.
    //

    // Get the render setup task params from the value cache.
    HdxRenderTaskParams renderSetupTaskParams =
        _GetValue<HdxRenderTaskParams>(renderSetupTaskId, HdTokens->params);

    // Update the render setup task params.
    renderSetupTaskParams.overrideColor = renderParams.overrideColor;
    renderSetupTaskParams.wireframeColor = renderParams.wireframeColor;
    renderSetupTaskParams.enableLighting = renderParams.enableLighting;
    renderSetupTaskParams.enableIdRender = false;
    renderSetupTaskParams.alphaThreshold = 0.1f;
    renderSetupTaskParams.tessLevel = 32.0f;
    const float tinyThreshold = 0.9f;
    renderSetupTaskParams.drawingRange = GfVec2f(tinyThreshold, -1.0f);
    renderSetupTaskParams.enableHardwareShading = true;
    renderSetupTaskParams.depthBiasUseDefault = true;
    renderSetupTaskParams.depthFunc = HdCmpFuncLess;
    renderSetupTaskParams.geomStyle = HdGeomStylePolygons;

    // Store the updated render setup task params back in the cache and mark
    // them dirty.
    _SetValue(renderSetupTaskId, HdTokens->params, renderSetupTaskParams);
    GetRenderIndex().GetChangeTracker().MarkTaskDirty(
        renderSetupTaskId,
        HdChangeTracker::DirtyParams);


    // Update the collections on the render task and mark them dirty.
    _SetValue(renderTaskId, HdTokens->collection, rprimCollections);
    GetRenderIndex().GetChangeTracker().MarkTaskDirty(
        renderTaskId,
        HdChangeTracker::DirtyCollection);


    return { GetRenderIndex().GetTask(renderSetupTaskId),
             GetRenderIndex().GetTask(renderTaskId),
             GetRenderIndex().GetTask(selectionTaskId) };
}


PXR_NAMESPACE_CLOSE_SCOPE
