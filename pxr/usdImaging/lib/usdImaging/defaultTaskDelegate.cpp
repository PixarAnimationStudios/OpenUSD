//
// Copyright 2016 Pixar
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
#include "pxr/usdImaging/usdImaging/defaultTaskDelegate.h"

#include "pxr/imaging/hd/version.h"
#include "pxr/imaging/hd/camera.h"
#include "pxr/imaging/hd/light.h"
#include "pxr/imaging/hd/tokens.h"

#include "pxr/imaging/hdx/renderTask.h"
#include "pxr/imaging/hdx/selectionTask.h"
#include "pxr/imaging/hdx/simpleLightTask.h"
#include "pxr/imaging/hdx/simpleLightBypassTask.h"
#include "pxr/imaging/hdx/tokens.h"

#include "pxr/imaging/glf/simpleLight.h"
#include "pxr/imaging/glf/simpleLightingContext.h"

#include "pxr/usdImaging/usdImaging/tokens.h"

#include "pxr/base/gf/matrix4d.h"
#include "pxr/base/tf/envSetting.h"
#include "pxr/base/tf/staticTokens.h"

TF_DEFINE_PRIVATE_TOKENS(
    _tokens,
    (idRenderTask)
    (renderTask)
    (selectionTask)
    (simpleLightTask)
    (simpleLightBypassTask)
    (camera)
);

static bool
_ShouldEnableLighting(UsdImagingEngine::RenderParams params)
{
    switch(params.drawMode) {
    case UsdImagingEngine::DRAW_GEOM_ONLY:
    case UsdImagingEngine::DRAW_POINTS:
        return false;
    default:
        return params.enableLighting and not params.enableIdRender;
    }
}

UsdImaging_DefaultTaskDelegate::UsdImaging_DefaultTaskDelegate(
    HdRenderIndexSharedPtr const& parentIndex,
    SdfPath const& delegateID)
    : UsdImagingTaskDelegate(parentIndex, delegateID)
    , _viewport(0,0,1,1)
    , _selectionColor(1,1,0,1)
{
    // create an unique namespace
    _rootId = delegateID.AppendChild(
        TfToken(TfStringPrintf("_UsdImaging_%p", this)));

    _renderTaskId               = _rootId.AppendChild(_tokens->renderTask);
    _idRenderTaskId             = _rootId.AppendChild(_tokens->idRenderTask);
    _selectionTaskId            = _rootId.AppendChild(_tokens->selectionTask);
    _simpleLightTaskId          = _rootId.AppendChild(_tokens->simpleLightTask);
    _simpleLightBypassTaskId    = _rootId.AppendChild(_tokens->simpleLightBypassTask);
    _cameraId                   = _rootId.AppendChild(_tokens->camera);
    _activeSimpleLightTaskId    = SdfPath();

    // TODO: tasks of shadow map generation, accumulation etc will be
    // prepared here.
    HdRenderIndex &renderIndex = GetRenderIndex();

    // camera
    {
        renderIndex.InsertCamera<HdCamera>(this, _cameraId);
        _ValueCache &cache = _valueCacheMap[_cameraId];
        cache[HdShaderTokens->worldToViewMatrix] = VtValue(GfMatrix4d(1));
        cache[HdShaderTokens->projectionMatrix]  = VtValue(GfMatrix4d(1));
        cache[HdTokens->cameraFrustum] = VtValue(); // we don't use GfFrustum.
        cache[HdTokens->windowPolicy] = VtValue();  // we don't use window policy.
    }

    // selection task
    {
        renderIndex.InsertTask<HdxSelectionTask>(this, _selectionTaskId);
        _ValueCache &cache = _valueCacheMap[_selectionTaskId];
        HdxSelectionTaskParams params;
        params.enableSelection = true;
        params.selectionColor = _selectionColor;
        params.locateColor = GfVec4f(0,0,1,1);
        cache[HdTokens->params] = VtValue(params);
        cache[HdTokens->children] = VtValue(SdfPathVector());
    }

    // simple lighting task (for Hydra native)
    {
        renderIndex.InsertTask<HdxSimpleLightTask>(this, _simpleLightTaskId);
        _ValueCache &cache = _valueCacheMap[_simpleLightTaskId];
        HdxSimpleLightTaskParams params;
        params.cameraPath = _cameraId;
        cache[HdTokens->params] = VtValue(params);
        cache[HdTokens->children] = VtValue(SdfPathVector());
    }

    // simple lighting task (for Presto UsdBaseIc compatible)
    {
        renderIndex.InsertTask<HdxSimpleLightBypassTask>(this,
                                                         _simpleLightBypassTaskId);
        _ValueCache &cache = _valueCacheMap[_simpleLightBypassTaskId];
        HdxSimpleLightBypassTaskParams params;
        params.cameraPath = _cameraId;
        cache[HdTokens->params] = VtValue(params);
        cache[HdTokens->children] = VtValue(SdfPathVector());
    }

    // render task
    _InsertRenderTask(_renderTaskId);
    _InsertRenderTask(_idRenderTaskId);

    // initialize HdxRenderTaskParams for render tasks
    _UpdateCollection(&_rprims, HdTokens->geometry, HdTokens->smoothHull,
                      SdfPathVector(1, SdfPath::AbsoluteRootPath()),
                      _renderTaskId,
                      _idRenderTaskId);
    _UpdateRenderParams(_renderParams,
                        _renderParams,
                        _renderTaskId);
    _UpdateRenderParams(_idRenderParams,
                        _idRenderParams,
                        _idRenderTaskId);
}

UsdImaging_DefaultTaskDelegate::~UsdImaging_DefaultTaskDelegate()
{
    // remove the render graph entities from the renderIndex
    HdRenderIndex &renderIndex = GetRenderIndex();
    renderIndex.RemoveCamera(_cameraId);
    renderIndex.RemoveTask(_selectionTaskId);
    renderIndex.RemoveTask(_simpleLightTaskId);
    renderIndex.RemoveTask(_simpleLightBypassTaskId);
    renderIndex.RemoveTask(_renderTaskId);
    renderIndex.RemoveTask(_idRenderTaskId);

    TF_FOR_ALL (id, _lightIds) {
        renderIndex.RemoveLight(*id);
    }
}

/*virtual*/
HdRprimCollection const&
UsdImaging_DefaultTaskDelegate::GetRprimCollection() const
{
    return _rprims;
}

void
UsdImaging_DefaultTaskDelegate::_InsertRenderTask(SdfPath const &id)
{
    GetRenderIndex().InsertTask<HdxRenderTask>(this, id);
    _ValueCache &cache = _valueCacheMap[id];
    HdxRenderTaskParams params;
    params.camera = _cameraId;
    // Initialize viewport to the latest value since render tasks can be lazily
    // instantiated, potentially even after SetCameraState.  All other
    // parameters will be updated by _UpdateRenderParams.
    params.viewport = _viewport;
    cache[HdTokens->params] = VtValue(params);
    cache[HdTokens->children] = VtValue(SdfPathVector());
    cache[HdTokens->collection] = VtValue();
}

HdTaskSharedPtrVector
UsdImaging_DefaultTaskDelegate::GetRenderTasks(
    UsdImagingEngine::RenderParams const &params)
{
    HdTaskSharedPtrVector tasks; // XXX: we can cache this vector
    tasks.reserve(3);

    // light
    if (not _activeSimpleLightTaskId.IsEmpty()) {
        tasks.push_back(GetRenderIndex().GetTask(_activeSimpleLightTaskId));
    }

    // render
    if (params.enableIdRender) {
        tasks.push_back(GetRenderIndex().GetTask(_idRenderTaskId));
    } else {
        tasks.push_back(GetRenderIndex().GetTask(_renderTaskId));
    }

    // selection highlighting (selectionTask comes after renderTask)
    if (not params.enableIdRender) {
        tasks.push_back(GetRenderIndex().GetTask(_selectionTaskId));
    }

    return tasks;
}

void
UsdImaging_DefaultTaskDelegate::SetCollectionAndRenderParams(
    const SdfPathVector &roots,
    const UsdImagingEngine::RenderParams &params)
{
    // choose repr
    TfToken repr = HdTokens->smoothHull;
    bool refined = params.complexity > 1.0;

    if (params.drawMode == UsdImagingEngine::DRAW_GEOM_FLAT or
        params.drawMode == UsdImagingEngine::DRAW_SHADED_FLAT) {
        repr = HdTokens->hull;
    } else if (params.drawMode == UsdImagingEngine::DRAW_WIREFRAME_ON_SURFACE) {
        if (refined) {
            repr = HdTokens->refinedWireOnSurf;
        } else {
            repr = HdTokens->wireOnSurf;
        }
    } else if (params.drawMode == UsdImagingEngine::DRAW_WIREFRAME) {
        if (refined) {
            repr = HdTokens->refinedWire;
        } else {
            repr = HdTokens->wire;
        }
    } else {
        if (refined) {
            repr = HdTokens->refined;
        } else {
            repr = HdTokens->smoothHull;
        }
    }

    // By default, don't show any guides.
    TfToken colName = HdTokens->geometry;

    // Pickup guide geometry if requested:
    if (params.showGuides and not params.showRenderGuides) {
        colName = UsdImagingCollectionTokens->geometryAndInteractiveGuides;
    } else if (not params.showGuides and params.showRenderGuides) {
        colName = UsdImagingCollectionTokens->geometryAndRenderGuides;
    } else if (params.showGuides and params.showRenderGuides) {
        colName = UsdImagingCollectionTokens->geometryAndGuides;
    }

    _UpdateCollection(&_rprims, colName, repr, roots,
                      _renderTaskId,
                      _idRenderTaskId);

    UsdImagingEngine::RenderParams &oldParams = params.enableIdRender
                    ? _idRenderParams
                    : _renderParams;
    SdfPath const &taskId = params.enableIdRender
                    ? _idRenderTaskId
                    : _renderTaskId;

    if (oldParams != params) {
        _UpdateRenderParams(params, oldParams, taskId);
        oldParams = params;
    }
}

void
UsdImaging_DefaultTaskDelegate::_UpdateCollection(
    HdRprimCollection *rprims,
    TfToken const &colName, TfToken const &reprName,
    SdfPathVector const &roots,
    SdfPath const &renderTaskId,
    SdfPath const &idRenderTaskId)
{
    SdfPathVector const& oldRoots = rprims->GetRootPaths();

    // inexpensive comparison first
    bool match
          = rprims->GetName() == colName
        and oldRoots.size() == roots.size()
        and rprims->GetReprName() == reprName;

    // Only take the time to compare root paths if everything else matches.
    if (match) {
        // Note that oldRoots is guaranteed to be sorted.
        for(size_t i = 0; i < roots.size(); i++) {
            // Avoid binary search when both vectors are sorted.
            if (oldRoots[i] == roots[i])
                continue;
            // Binary search to find the current root.
            if (not std::binary_search(oldRoots.begin(), oldRoots.end(),
                                       roots[i])) 
            {
                match = false;
                break;
            }
        }
        // if everything matches, do nothing.
        if (match) return;
    }

    // Update collection in the value cache
    *rprims = HdRprimCollection(colName, reprName);
    rprims->SetRootPaths(roots);

    // update value cache
    _SetValue(renderTaskId, HdTokens->collection, *rprims);
    _SetValue(idRenderTaskId, HdTokens->collection, *rprims);

    // invalidate
    GetRenderIndex().GetChangeTracker().MarkTaskDirty(
        renderTaskId, HdChangeTracker::DirtyCollection);
    GetRenderIndex().GetChangeTracker().MarkTaskDirty(
        idRenderTaskId, HdChangeTracker::DirtyCollection);
}

void
UsdImaging_DefaultTaskDelegate::_UpdateRenderParams(
    UsdImagingEngine::RenderParams const &renderParams,
    UsdImagingEngine::RenderParams const &oldRenderParams,
    SdfPath const &renderTaskId)
{
    static const HdCullStyle USD_2_HD_CULL_STYLE[] =
    {
        HdCullStyleNothing,               // CULL_STYLE_NOTHING,
        HdCullStyleBack,                  // CULL_STYLE_BACK,
        HdCullStyleFront,                 // CULL_STYLE_FRONT,
        HdCullStyleBackUnlessDoubleSided, // CULL_STYLE_BACK_UNLESS_DOUBLE_SIDED
    };
    static_assert(((sizeof(USD_2_HD_CULL_STYLE) / 
                    sizeof(USD_2_HD_CULL_STYLE[0])) 
                == UsdImagingEngine::CULL_STYLE_COUNT),"enum size mismatch");

    HdxRenderTaskParams params =
                _GetValue<HdxRenderTaskParams>(renderTaskId, HdTokens->params);

    // update params
    params.overrideColor       = renderParams.overrideColor;
    params.wireframeColor      = renderParams.wireframeColor;
    params.enableLighting      = _ShouldEnableLighting(renderParams);
    params.enableIdRender      = renderParams.enableIdRender;
    params.depthBiasUseDefault = true;
    params.depthFunc           = HdCmpFuncLess;
    params.cullStyle           = USD_2_HD_CULL_STYLE[renderParams.cullStyle];
    params.alphaThreshold      = renderParams.alphaThreshold;
    // 32.0 is the default tessLevel of HdRasterState. we can change if we like.
    params.tessLevel           = 32.0;

    const float tinyThreshold = 0.9f;
    params.drawingRange = GfVec2f(tinyThreshold, -1.0f);

    // note that params.rprims and params.viewport are not updated
    // in this function, and needed to be preserved.

    // Decrease the alpha threshold if we are using sample alpha to
    // coverage.
    if (renderParams.alphaThreshold < 0.0) {
        params.alphaThreshold =
            renderParams.enableSampleAlphaToCoverage ? 0.1f : 0.5f;
    } else {
        params.alphaThreshold =
            renderParams.alphaThreshold;
    }

    params.enableHardwareShading = renderParams.enableHardwareShading;

    if (renderParams.highlight != oldRenderParams.highlight) {
        GetRenderIndex().GetChangeTracker().MarkTaskDirty(
                            _selectionTaskId, HdChangeTracker::DirtyParams);
        _ValueCache &cache = _valueCacheMap[_selectionTaskId];
        HdxSelectionTaskParams params = 
                        cache[HdTokens->params].Get<HdxSelectionTaskParams>();
        params.enableSelection = renderParams.highlight;
        cache[HdTokens->params] = VtValue(params);
    }

    if (renderParams.clipPlanes != oldRenderParams.clipPlanes) {
        GetRenderIndex().GetChangeTracker().MarkCameraDirty(
                            _cameraId, HdChangeTracker::DirtyClipPlanes);
    }


    // store into cache
    _SetValue(renderTaskId, HdTokens->params, params);

    // invalidate
    GetRenderIndex().GetChangeTracker().MarkTaskDirty(
        renderTaskId,  HdChangeTracker::DirtyParams);
}

void
UsdImaging_DefaultTaskDelegate::SetLightingState(
    const GlfSimpleLightingContextPtr &src)
{
    if (not TF_VERIFY(src)) return;

    // cache the GlfSimpleLight vector
    GlfSimpleLightVector const &lights = src->GetLights();

    bool hasNumLightsChanged = false;

    // Insert the light Ids into HdRenderIndex for those not yet exist.
    while (_lightIds.size() < lights.size()) {
        SdfPath lightId(
            TfStringPrintf("%s/light%d", _rootId.GetText(),
                           (int)_lightIds.size()));
        _lightIds.push_back(lightId);

        GetRenderIndex().InsertLight<HdLight>(this, lightId);
        hasNumLightsChanged = true;
    }
    // Remove unused light Ids from HdRenderIndex
    while (_lightIds.size() > lights.size()) {
        GetRenderIndex().RemoveLight(_lightIds.back());
        _lightIds.pop_back();
        hasNumLightsChanged = true;
    }

    // invalidate HdLights
    for (size_t i = 0; i < lights.size(); ++i) {
        _ValueCache &cache = _valueCacheMap[_lightIds[i]];
        // store GlfSimpleLight directly.
        cache[HdTokens->lightParams] = VtValue(lights[i]);
        cache[HdTokens->lightTransform] = VtValue();
        cache[HdTokens->lightShadowParams] = VtValue(HdxShadowParams());
        cache[HdTokens->lightShadowCollection] = VtValue();

        // Only mark as dirty the parameters to avoid unnecessary invalidation
        // specially marking as dirty lightShadowCollection will trigger
        // a collection dirty on geometry and we don't want that to happen
        // always
        GetRenderIndex().GetChangeTracker().MarkLightDirty(
            _lightIds[i], HdChangeTracker::DirtyParams);
    }

    // sadly the material also comes from lighting context right now...
    HdxSimpleLightTaskParams params
        = _GetValue<HdxSimpleLightTaskParams>(_simpleLightTaskId,
                                              HdTokens->params);
    params.sceneAmbient = src->GetSceneAmbient();
    params.material = src->GetMaterial();

    // invalidate HdxSimpleLightTask too
    if (hasNumLightsChanged) {
        _SetValue(_simpleLightTaskId, HdTokens->params, params);

        GetRenderIndex().GetChangeTracker().MarkTaskDirty(
            _simpleLightTaskId, HdChangeTracker::DirtyParams);
    }

    // set HdxSimpleLightTask as the lighting task
    _activeSimpleLightTaskId = _simpleLightTaskId;
}

void
UsdImaging_DefaultTaskDelegate::SetBypassedLightingState(
    const GlfSimpleLightingContextPtr &src)
{
    HdxSimpleLightBypassTaskParams params;
    params.cameraPath = _cameraId;
    params.simpleLightingContext = src;
    _SetValue(_simpleLightBypassTaskId, HdTokens->params, params);

    // invalidate HdxSimpleLightBypassTask
    GetRenderIndex().GetChangeTracker().MarkTaskDirty(
        _simpleLightBypassTaskId, HdChangeTracker::DirtyParams);

    // set HdxSimpleLightBypassTask as the lighting task
    _activeSimpleLightTaskId = _simpleLightBypassTaskId;
}

void
UsdImaging_DefaultTaskDelegate::SetCameraState(
    const GfMatrix4d& viewMatrix,
    const GfMatrix4d& projectionMatrix,
    const GfVec4d& viewport)
{
    // cache the camera matrices
    _ValueCache &cache = _valueCacheMap[_cameraId];
    cache[HdShaderTokens->worldToViewMatrix] = VtValue(viewMatrix);
    cache[HdShaderTokens->projectionMatrix]  = VtValue(projectionMatrix);
    cache[HdTokens->cameraFrustum] = VtValue(); // we don't use GfFrustum.
    cache[HdTokens->windowPolicy]  = VtValue(); // we don't use window policy.

    // invalidate the camera to be synced
    GetRenderIndex().GetChangeTracker().MarkCameraDirty(_cameraId);

    if (_viewport != viewport) {
        // viewport is also read by HdxRenderTaskParam. invalidate it.
        _viewport = viewport;

        SdfPath tasks[4];
        int nTasks = 0;
        tasks[nTasks++] = _renderTaskId;
        tasks[nTasks++] = _idRenderTaskId;
        for (int i = 0; i < nTasks; ++i) {
            HdxRenderTaskParams params
                = _GetValue<HdxRenderTaskParams>(tasks[i], HdTokens->params);
            // update viewport in HdxRenderTaskParams
            params.viewport = viewport;
            _SetValue(tasks[i], HdTokens->params, params);

            // invalidate
            GetRenderIndex().GetChangeTracker().MarkTaskDirty(
                tasks[i], HdChangeTracker::DirtyParams);
        }
    }
}

void
UsdImaging_DefaultTaskDelegate::SetSelectionColor(GfVec4f const& color)
{
    if (_selectionColor != color) {
        _selectionColor = color;
        GetRenderIndex().GetChangeTracker().MarkTaskDirty(
            _selectionTaskId, HdChangeTracker::DirtyParams);
        _ValueCache &cache = _valueCacheMap[_selectionTaskId];
        HdxSelectionTaskParams params = 
                        cache[HdTokens->params].Get<HdxSelectionTaskParams>();
        params.enableSelection = true;
        params.selectionColor = _selectionColor;
        cache[HdTokens->params] = VtValue(params);
    }
}

/* virtual */
VtValue
UsdImaging_DefaultTaskDelegate::Get(SdfPath const& id, TfToken const& key)
{
    _ValueCache *vcache = TfMapLookupPtr(_valueCacheMap, id);
    VtValue ret;
    if (vcache and TfMapLookup(*vcache, key, &ret)) {
        return ret;
    }
    TF_CODING_ERROR("%s:%s doesn't exist in the value cache\n",
                    id.GetText(), key.GetText());
    return VtValue();
}

/* virtual */
bool
UsdImaging_DefaultTaskDelegate::CanRender(
    const UsdImagingEngine::RenderParams &params)
{
    return true;
}

/* virtual */
bool
UsdImaging_DefaultTaskDelegate::IsConverged() const
{
    // default task always converges.
    return true;
}



/* virtual */
std::vector<GfVec4d>
UsdImaging_DefaultTaskDelegate::GetClipPlanes(SdfPath const& cameraId)
{
    return _renderParams.clipPlanes;
}
