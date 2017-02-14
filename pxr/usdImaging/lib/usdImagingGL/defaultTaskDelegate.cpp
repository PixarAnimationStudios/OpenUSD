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
#include "pxr/usdImaging/usdImagingGL/defaultTaskDelegate.h"

#include "pxr/imaging/hd/version.h"
#include "pxr/imaging/hd/tokens.h"

#include "pxr/imaging/hdx/camera.h"
#include "pxr/imaging/hdx/light.h"
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

PXR_NAMESPACE_OPEN_SCOPE


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
_ShouldEnableLighting(UsdImagingGLEngine::RenderParams params)
{
    switch(params.drawMode) {
    case UsdImagingGLEngine::DRAW_GEOM_ONLY:
    case UsdImagingGLEngine::DRAW_POINTS:
        return false;
    default:
        return params.enableLighting && !params.enableIdRender;
    }
}

UsdImagingGL_DefaultTaskDelegate::UsdImagingGL_DefaultTaskDelegate(
    HdRenderIndexSharedPtr const& parentIndex,
    SdfPath const& delegateID)
    : UsdImagingGLTaskDelegate(parentIndex, delegateID)
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
        renderIndex.InsertSprim<HdxCamera>(this, _cameraId);
        _ValueCache &cache = _valueCacheMap[_cameraId];
        cache[HdxCameraTokens->windowPolicy] = VtValue(); // no window policy.
        cache[HdxCameraTokens->matrices] = VtValue(HdxCameraMatrices());
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

UsdImagingGL_DefaultTaskDelegate::~UsdImagingGL_DefaultTaskDelegate()
{
    // remove the render graph entities from the renderIndex
    HdRenderIndex &renderIndex = GetRenderIndex();
    renderIndex.RemoveSprim(HdPrimTypeTokens->camera, _cameraId);
    renderIndex.RemoveTask(_selectionTaskId);
    renderIndex.RemoveTask(_simpleLightTaskId);
    renderIndex.RemoveTask(_simpleLightBypassTaskId);
    renderIndex.RemoveTask(_renderTaskId);
    renderIndex.RemoveTask(_idRenderTaskId);

    TF_FOR_ALL (id, _lightIds) {
        renderIndex.RemoveSprim(HdPrimTypeTokens->light, *id);
    }
}

/*virtual*/
HdRprimCollection const&
UsdImagingGL_DefaultTaskDelegate::GetRprimCollection() const
{
    return _rprims;
}

void
UsdImagingGL_DefaultTaskDelegate::_InsertRenderTask(SdfPath const &id)
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
UsdImagingGL_DefaultTaskDelegate::GetRenderTasks(
    UsdImagingGLEngine::RenderParams const &params)
{
    HdTaskSharedPtrVector tasks; // XXX: we can cache this vector
    tasks.reserve(3);

    // light
    if (!_activeSimpleLightTaskId.IsEmpty()) {
        tasks.push_back(GetRenderIndex().GetTask(_activeSimpleLightTaskId));
    }

    // render
    if (params.enableIdRender) {
        tasks.push_back(GetRenderIndex().GetTask(_idRenderTaskId));
    } else {
        tasks.push_back(GetRenderIndex().GetTask(_renderTaskId));
    }

    // selection highlighting (selectionTask comes after renderTask)
    if (!params.enableIdRender) {
        tasks.push_back(GetRenderIndex().GetTask(_selectionTaskId));
    }

    return tasks;
}

void
UsdImagingGL_DefaultTaskDelegate::SetCollectionAndRenderParams(
    const SdfPathVector &roots,
    const UsdImagingGLEngine::RenderParams &params)
{
    // choose repr
    TfToken repr = HdTokens->smoothHull;
    bool refined = params.complexity > 1.0;

    if (params.drawMode == UsdImagingGLEngine::DRAW_GEOM_FLAT ||
        params.drawMode == UsdImagingGLEngine::DRAW_SHADED_FLAT) {
        repr = HdTokens->hull;
    } else if (params.drawMode == UsdImagingGLEngine::DRAW_WIREFRAME_ON_SURFACE) {
        if (refined) {
            repr = HdTokens->refinedWireOnSurf;
        } else {
            repr = HdTokens->wireOnSurf;
        }
    } else if (params.drawMode == UsdImagingGLEngine::DRAW_WIREFRAME) {
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

    // By default, show only default geometry, and default is *always* included
    TfToken colName = HdTokens->geometry;

    // Pickup proxy, guide, and render geometry if requested:
    if (params.showGuides && !params.showRender) {
        colName = params.showProxy ?
            UsdImagingCollectionTokens->geometryAndProxyAndGuides :
            UsdImagingCollectionTokens->geometryAndGuides;
    } else if (!params.showGuides && params.showRender) {
        colName = params.showProxy ?
            UsdImagingCollectionTokens->geometryAndProxyAndRender :
            UsdImagingCollectionTokens->geometryAndRender;
    } else if (params.showGuides && params.showRender) {
        colName = params.showProxy ?
            UsdImagingCollectionTokens->geometryAllPurposes :
            UsdImagingCollectionTokens->geometryAndRenderAndGuides;
    } else if (params.showProxy){
        colName = UsdImagingCollectionTokens->geometryAndProxy;
    }

    _UpdateCollection(&_rprims, colName, repr, roots,
                      _renderTaskId,
                      _idRenderTaskId);

    UsdImagingGLEngine::RenderParams &oldParams = params.enableIdRender
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
UsdImagingGL_DefaultTaskDelegate::_UpdateCollection(
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
        && oldRoots.size() == roots.size()
        && rprims->GetReprName() == reprName;

    // Only take the time to compare root paths if everything else matches.
    if (match) {
        // Note that oldRoots is guaranteed to be sorted.
        for(size_t i = 0; i < roots.size(); i++) {
            // Avoid binary search when both vectors are sorted.
            if (oldRoots[i] == roots[i])
                continue;
            // Binary search to find the current root.
            if (!std::binary_search(oldRoots.begin(), oldRoots.end(),
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
UsdImagingGL_DefaultTaskDelegate::_UpdateRenderParams(
    UsdImagingGLEngine::RenderParams const &renderParams,
    UsdImagingGLEngine::RenderParams const &oldRenderParams,
    SdfPath const &renderTaskId)
{
    static const HdCullStyle USD_2_HD_CULL_STYLE[] =
    {
        HdCullStyleDontCare,              // Cull No Opinion (unused)
        HdCullStyleNothing,               // CULL_STYLE_NOTHING,
        HdCullStyleBack,                  // CULL_STYLE_BACK,
        HdCullStyleFront,                 // CULL_STYLE_FRONT,
        HdCullStyleBackUnlessDoubleSided, // CULL_STYLE_BACK_UNLESS_DOUBLE_SIDED
    };
    static_assert(((sizeof(USD_2_HD_CULL_STYLE) / 
                    sizeof(USD_2_HD_CULL_STYLE[0])) 
                == UsdImagingGLEngine::CULL_STYLE_COUNT),"enum size mismatch");

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

    // Cache the clip planes.
    _clipPlanes = renderParams.clipPlanes;

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
        GetRenderIndex().GetChangeTracker().MarkSprimDirty(
                            _cameraId, HdxCamera::DirtyClipPlanes);
    }


    // store into cache
    _SetValue(renderTaskId, HdTokens->params, params);

    // invalidate
    GetRenderIndex().GetChangeTracker().MarkTaskDirty(
        renderTaskId,  HdChangeTracker::DirtyParams);
}

void
UsdImagingGL_DefaultTaskDelegate::SetLightingState(
    const GlfSimpleLightingContextPtr &src)
{
    if (!TF_VERIFY(src)) return;

    // cache the GlfSimpleLight vector
    GlfSimpleLightVector const &lights = src->GetLights();

    bool hasNumLightsChanged = false;

    // Insert the light Ids into HdRenderIndex for those not yet exist.
    while (_lightIds.size() < lights.size()) {
        SdfPath lightId(
            TfStringPrintf("%s/light%d", _rootId.GetText(),
                           (int)_lightIds.size()));
        _lightIds.push_back(lightId);

        GetRenderIndex().InsertSprim<HdxLight>(this, lightId);
        hasNumLightsChanged = true;
    }
    // Remove unused light Ids from HdRenderIndex
    while (_lightIds.size() > lights.size()) {
        GetRenderIndex().RemoveSprim(HdPrimTypeTokens->light, _lightIds.back());
        _lightIds.pop_back();
        hasNumLightsChanged = true;
    }

    // invalidate HdLights
    for (size_t i = 0; i < lights.size(); ++i) {
        _ValueCache &cache = _valueCacheMap[_lightIds[i]];
        // store GlfSimpleLight directly.
        cache[HdxLightTokens->params] = VtValue(lights[i]);
        cache[HdxLightTokens->transform] = VtValue();
        cache[HdxLightTokens->shadowParams] = VtValue(HdxShadowParams());
        cache[HdxLightTokens->shadowCollection] = VtValue();

        // Only mark as dirty the parameters to avoid unnecessary invalidation
        // specially marking as dirty lightShadowCollection will trigger
        // a collection dirty on geometry and we don't want that to happen
        // always
        GetRenderIndex().GetChangeTracker().MarkSprimDirty(
            _lightIds[i], HdxLight::DirtyParams);
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
UsdImagingGL_DefaultTaskDelegate::SetBypassedLightingState(
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
UsdImagingGL_DefaultTaskDelegate::SetCameraState(
    const GfMatrix4d& viewMatrix,
    const GfMatrix4d& projectionMatrix,
    const GfVec4d& viewport)
{
    // cache the camera matrices
    _ValueCache &cache = _valueCacheMap[_cameraId];
    cache[HdxCameraTokens->windowPolicy]  = VtValue(); // no window policy.
    cache[HdxCameraTokens->matrices] = 
        VtValue(HdxCameraMatrices(viewMatrix, projectionMatrix));

    // invalidate the camera to be synced
    GetRenderIndex().GetChangeTracker().MarkSprimDirty(_cameraId,
                                                       HdxCamera::AllDirty);

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
UsdImagingGL_DefaultTaskDelegate::SetSelectionColor(GfVec4f const& color)
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
UsdImagingGL_DefaultTaskDelegate::Get(SdfPath const& id, TfToken const& key)
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
UsdImagingGL_DefaultTaskDelegate::CanRender(
    const UsdImagingGLEngine::RenderParams &params)
{
    return true;
}

/* virtual */
bool
UsdImagingGL_DefaultTaskDelegate::IsConverged() const
{
    // default task always converges.
    return true;
}

/* virtual */
bool
UsdImagingGL_DefaultTaskDelegate::IsEnabled(TfToken const& option) const
{
    if (option == HdxOptionTokens->taskSetAlphaToCoverage) {
        // UsdImagingGLHdEngine enables ALPHA_TO_COVERAGE as needed.
        return true;
    }
    return HdSceneDelegate::IsEnabled(option);
}

/* virtual */
std::vector<GfVec4d>
UsdImagingGL_DefaultTaskDelegate::GetClipPlanes(SdfPath const& cameraId)
{
    return _clipPlanes;
}

PXR_NAMESPACE_CLOSE_SCOPE

