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
#include "pxr/imaging/glf/glew.h"

#include "pxr/usdImaging/usdImagingGL/hdEngine.h"
#include "pxr/usdImaging/usdImaging/tokens.h"

#include "pxr/imaging/hd/debugCodes.h"
#include "pxr/imaging/hd/renderContextCaps.h"
#include "pxr/imaging/hd/renderDelegate.h"
#include "pxr/imaging/hd/resourceRegistry.h"
#include "pxr/imaging/hd/tokens.h"
#include "pxr/imaging/hd/version.h"

#include "pxr/imaging/hdx/intersector.h"
#include "pxr/imaging/hdx/rendererPluginRegistry.h"
#include "pxr/imaging/hdx/tokens.h"

#include "pxr/base/gf/matrix4d.h"
#include "pxr/base/gf/range3d.h"
#include "pxr/base/gf/rotation.h"
#include "pxr/base/gf/vec3d.h"

#include "pxr/base/tf/stringUtils.h"

#include "pxr/imaging/glf/diagnostic.h"
#include "pxr/imaging/glf/info.h"
#include "pxr/imaging/glf/simpleLightingContext.h"

PXR_NAMESPACE_OPEN_SCOPE

TF_DEFINE_PRIVATE_TOKENS(
    _tokens,
    (proxy)
    (render)
);

UsdImagingGLHdEngine::UsdImagingGLHdEngine(
        const SdfPath& rootPath,
        const SdfPathVector& excludedPrimPaths,
        const SdfPathVector& invisedPrimPaths,
        const SdfPath& delegateID)
    : UsdImagingGLEngine()
    , _renderIndex(nullptr)
    , _selTracker(new HdxSelectionTracker)
    , _delegateID(delegateID)
    , _delegate(nullptr)
    , _renderPlugin(nullptr)
    , _taskController(nullptr)
    , _selectionColor(1.0f, 1.0f, 0.0f, 1.0f)
    , _rootPath(rootPath)
    , _excludedPrimPaths(excludedPrimPaths)
    , _invisedPrimPaths(invisedPrimPaths)
    , _isPopulated(false)
    , _renderTags()
{
    // _renderIndex, _taskController, and _delegate are initialized
    // by the plugin system.
    if (!SetRendererPlugin(TfType())) {
        TF_CODING_ERROR("No renderer plugins found! Check before creation.");
    }
}

UsdImagingGLHdEngine::~UsdImagingGLHdEngine() 
{
    _DeleteHydraResources();
}

HdRenderIndex *
UsdImagingGLHdEngine::GetRenderIndex() const
{
    return _renderIndex;
}

void
UsdImagingGLHdEngine::InvalidateBuffers()
{
    //_delegate->GetRenderIndex().GetChangeTracker().MarkPrimDirty(path, flag);
}

static int
_GetRefineLevel(float c)
{
    // TODO: Change complexity to refineLevel when we refactor UsdImaging.
    //
    // Convert complexity float to refine level int.
    int refineLevel = 0;

    // to avoid floating point inaccuracy (e.g. 1.3 > 1.3f)
    c = std::min(c + 0.01f, 2.0f);

    if (1.0f <= c && c < 1.1f) { 
        refineLevel = 0;
    } else if (1.1f <= c && c < 1.2f) { 
        refineLevel = 1;
    } else if (1.2f <= c && c < 1.3f) { 
        refineLevel = 2;
    } else if (1.3f <= c && c < 1.4f) { 
        refineLevel = 3;
    } else if (1.4f <= c && c < 1.5f) { 
        refineLevel = 4;
    } else if (1.5f <= c && c < 1.6f) { 
        refineLevel = 5;
    } else if (1.6f <= c && c < 1.7f) { 
        refineLevel = 6;
    } else if (1.7f <= c && c < 1.8f) { 
        refineLevel = 7;
    } else if (1.8f <= c && c <= 2.0f) { 
        refineLevel = 8;
    } else {
        TF_CODING_ERROR("Invalid complexity %f, expected range is [1.0,2.0]\n", 
                c);
    }
    return refineLevel;
}

bool 
UsdImagingGLHdEngine::_CanPrepareBatch(const UsdPrim& root, 
                                     const RenderParams& params)
{
    HD_TRACE_FUNCTION();

    if (!TF_VERIFY(root, "Attempting to draw an invalid/null prim\n")) 
        return false;

    if (!root.GetPath().HasPrefix(_rootPath)) {
        TF_CODING_ERROR("Attempting to draw path <%s>, but HdEngine is rooted"
                    "at <%s>\n",
                    root.GetPath().GetText(),
                    _rootPath.GetText());
        return false;
    }

    return true;
}

void
UsdImagingGLHdEngine::_PreSetTime(const UsdPrim& root, const RenderParams& params)
{
    HD_TRACE_FUNCTION();

    // Set the fallback refine level, if this changes from the existing value,
    // all prim refine levels will be dirtied. 
    _delegate->SetRefineLevelFallback(_GetRefineLevel(params.complexity));
}

void
UsdImagingGLHdEngine::_PostSetTime(const UsdPrim& root, const RenderParams& params)
{
    HD_TRACE_FUNCTION();
    if (_isPopulated)
        return;

    // The delegate may have been populated from somewhere other than
    // where we are drawing. This applys a compensating transformation that
    // cancels out any accumulated transformation from the population root.
    _delegate->SetRootCompensation(root.GetPath());
}

/*virtual*/
void
UsdImagingGLHdEngine::PrepareBatch(const UsdPrim& root, RenderParams params)
{
    HD_TRACE_FUNCTION();

    if (_CanPrepareBatch(root, params)) {
        if (!_isPopulated) {
            _delegate->Populate(root.GetStage()->GetPrimAtPath(_rootPath),
                               _excludedPrimPaths);
            _delegate->SetInvisedPrimPaths(_invisedPrimPaths);
            _isPopulated = true;
        }

        _PreSetTime(root, params);
        // Reset progressive rendering if we're changing the timecode.
        if (_delegate->GetTime() != params.frame) {
            _taskController->ResetImage();
        }
        // SetTime will only react if time actually changes.
        _delegate->SetTime(params.frame);
        _PostSetTime(root, params);
    }
}

/* static */ 
void 
UsdImagingGLHdEngine::PrepareBatch(
    const UsdImagingGLHdEngineSharedPtrVector& engines,
    const UsdPrimVector& rootPrims,
    const std::vector<UsdTimeCode>& times,
    RenderParams params)
{
    HD_TRACE_FUNCTION();

    if (!(engines.size() == rootPrims.size() &&
             engines.size() == times.size())) {
        TF_CODING_ERROR("Mismatched parameters");
        return;
    }

    // Do some initial error checking.
    std::set<size_t> skipped;
    for (size_t i = 0; i < engines.size(); ++i) {
        if (!engines[i]->_CanPrepareBatch(rootPrims[i], params)) {
            skipped.insert(i);
        }
    }

    if (skipped.empty()) {
        _PrepareBatch(engines, rootPrims, times, params);
    }
    else {
        // Filter out all the engines that fail the error check.
        UsdImagingGLHdEngineSharedPtrVector tmpEngines = engines;
        UsdPrimVector tmpRootPrims = rootPrims;
        std::vector<UsdTimeCode> tmpTimes = times;

        TF_REVERSE_FOR_ALL(it, skipped) {
            tmpEngines.erase(tmpEngines.begin() + *it);
            tmpRootPrims.erase(tmpRootPrims.begin() + *it);
            tmpTimes.erase(tmpTimes.begin() + *it);
        }

        _PrepareBatch(tmpEngines, tmpRootPrims, tmpTimes, params);
    }
}

/* static */ 
void 
UsdImagingGLHdEngine::_PrepareBatch(
    const UsdImagingGLHdEngineSharedPtrVector& engines,
    const UsdPrimVector& rootPrims,
    const std::vector<UsdTimeCode>& times,
    const RenderParams& params)
{
    HD_TRACE_FUNCTION();

    _Populate(engines, rootPrims);
    _SetTimes(engines, rootPrims, times, params);
}

/*static */
void
UsdImagingGLHdEngine::_SetTimes(const UsdImagingGLHdEngineSharedPtrVector& engines,
                              const UsdPrimVector& rootPrims,
                              const std::vector<UsdTimeCode>& times,
                              const RenderParams& params)
{
    HD_TRACE_FUNCTION();

    std::vector<UsdImagingDelegate*> delegates;
    delegates.reserve(engines.size());

    for (size_t i = 0; i < engines.size(); ++i) {
        engines[i]->_PreSetTime(rootPrims[i], params);
        delegates.push_back(engines[i]->_delegate);
        // Reset progressive rendering in each engine before setting timecode.
        engines[i]->_taskController->ResetImage();
    }

    UsdImagingDelegate::SetTimes(delegates, times);

    for (size_t i = 0; i < engines.size(); ++i) {
        engines[i]->_PostSetTime(rootPrims[i], params);
    }
}

/* static */
void 
UsdImagingGLHdEngine::_Populate(const UsdImagingGLHdEngineSharedPtrVector& engines,
                              const UsdPrimVector& rootPrims)
{
    HD_TRACE_FUNCTION();

    std::vector<UsdImagingDelegate*> delegatesToPopulate;
    delegatesToPopulate.reserve(engines.size());

    UsdPrimVector primsToPopulate;
    primsToPopulate.reserve(engines.size());

    std::vector<SdfPathVector> pathsToExclude, pathsToInvis;
    pathsToExclude.reserve(engines.size());
    pathsToInvis.reserve(engines.size());

    for (size_t i = 0; i < engines.size(); ++i) {
        if (!engines[i]->_isPopulated) {
            delegatesToPopulate.push_back(engines[i]->_delegate);
            primsToPopulate.push_back(
                rootPrims[i].GetStage()->GetPrimAtPath(engines[i]->_rootPath));
            pathsToExclude.push_back(engines[i]->_excludedPrimPaths);
            pathsToInvis.push_back(engines[i]->_invisedPrimPaths);

            // Set _isPopulated to true immediately to weed out any duplicate
            // engines. This is equivalent to what would happen if the 
            // consumer called the non-vectorized PrepareBatch on each
            // engine individually.
            engines[i]->_isPopulated = true;
        }
    }

    UsdImagingDelegate::Populate(delegatesToPopulate, primsToPopulate, 
                                 pathsToExclude, pathsToInvis);
}

/* static */
void
UsdImagingGLHdEngine::_UpdateHydraCollection(HdRprimCollection *collection,
                          SdfPathVector const& roots,
                          UsdImagingGLEngine::RenderParams const& params,
                          TfTokenVector *renderTags)
{
    if (collection == nullptr) {
        TF_CODING_ERROR("Null passed to _UpdateHydraCollection");
        return;
    }

    // choose repr
    TfToken reprName = HdTokens->smoothHull;
    bool refined = params.complexity > 1.0;

    if (params.drawMode == UsdImagingGLEngine::DRAW_GEOM_FLAT ||
        params.drawMode == UsdImagingGLEngine::DRAW_SHADED_FLAT) {
        // Flat shading
        reprName = HdTokens->hull;
    } else if (
        params.drawMode == UsdImagingGLEngine::DRAW_WIREFRAME_ON_SURFACE) {
        // Wireframe on surface
        reprName = refined ? HdTokens->refinedWireOnSurf : HdTokens->wireOnSurf;
    } else if (params.drawMode == UsdImagingGLEngine::DRAW_WIREFRAME) {
        // Wireframe
        reprName = refined ? HdTokens->refinedWire : HdTokens->wire;
    } else {
        // Smooth shading
        reprName = refined ? HdTokens->refined : HdTokens->smoothHull;
    }

    // Calculate the rendertags needed based on the parameters passed by
    // the application
    renderTags->clear();
    renderTags->push_back(HdTokens->geometry);
    if (params.showGuides) {
        renderTags->push_back(HdxRenderTagsTokens->guide);
    }
    if (params.showProxy) {
        renderTags->push_back(_tokens->proxy);
    }
    if (params.showRender) {
        renderTags->push_back(_tokens->render);
    } 

    // By default our main collection will be called geometry
    TfToken colName = HdTokens->geometry;

    // Check if the collection needs to be updated (so we can avoid the sort).
    SdfPathVector const& oldRoots = collection->GetRootPaths();

    // inexpensive comparison first
    bool match = collection->GetName() == colName &&
                 oldRoots.size() == roots.size() &&
                 collection->GetReprName() == reprName &&
                 collection->GetRenderTags().size() == renderTags->size();

    // Only take the time to compare root paths if everything else matches.
    if (match) {
        // Note that oldRoots is guaranteed to be sorted.
        for(size_t i = 0; i < roots.size(); i++) {
            // Avoid binary search when both vectors are sorted.
            if (oldRoots[i] == roots[i])
                continue;
            // Binary search to find the current root.
            if (!std::binary_search(oldRoots.begin(), oldRoots.end(), roots[i])) 
            {
                match = false;
                break;
            }
        }

        // Compare if rendertags match
        if (*renderTags != collection->GetRenderTags()) {
            match = false;
        }

        // if everything matches, do nothing.
        if (match) return;
    }

    // Recreate the collection.
    *collection = HdRprimCollection(colName, reprName);
    collection->SetRootPaths(roots);
    collection->SetRenderTags(*renderTags);
}

/* static */
HdxRenderTaskParams
UsdImagingGLHdEngine::_MakeHydraRenderParams(
                  UsdImagingGLHdEngine::RenderParams const& renderParams)
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

    HdxRenderTaskParams params;

    params.overrideColor       = renderParams.overrideColor;
    params.wireframeColor      = renderParams.wireframeColor;

    if (renderParams.drawMode == UsdImagingGLEngine::DRAW_GEOM_ONLY ||
        renderParams.drawMode == UsdImagingGLEngine::DRAW_POINTS) {
        params.enableLighting = false;
    } else {
        params.enableLighting =  renderParams.enableLighting &&
                                !renderParams.enableIdRender;
    }

    params.enableIdRender      = renderParams.enableIdRender;
    params.depthBiasUseDefault = true;
    params.depthFunc           = HdCmpFuncLess;
    params.cullStyle           = USD_2_HD_CULL_STYLE[renderParams.cullStyle];
    // 32.0 is the default tessLevel of HdRasterState. we can change if we like.
    params.tessLevel           = 32.0;

    const float tinyThreshold = 0.9f;
    params.drawingRange = GfVec2f(tinyThreshold, -1.0f);

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

    // Leave default values for:
    // - params.geomStyle
    // - params.complexity
    // - params.hullVisibility
    // - params.surfaceVisibility

    // We don't provide the following because task controller ignores them:
    // - params.camera
    // - params.viewport

    return params;
}

/*virtual*/
void
UsdImagingGLHdEngine::RenderBatch(const SdfPathVector& paths, RenderParams params)
{
    _taskController->SetCameraClipPlanes(params.clipPlanes);
    _UpdateHydraCollection(&_renderCollection, paths, params, &_renderTags);
    _taskController->SetCollection(_renderCollection);

    HdxRenderTaskParams hdParams = _MakeHydraRenderParams(params);
    _taskController->SetRenderParams(hdParams);
    _taskController->SetEnableSelection(params.highlight);

    Render(params);
}

/*virtual*/
void
UsdImagingGLHdEngine::Render(const UsdPrim& root, RenderParams params)
{
    PrepareBatch(root, params);

    SdfPath rootPath = _delegate->GetPathForIndex(root.GetPath());
    SdfPathVector roots(1, rootPath);

    _taskController->SetCameraClipPlanes(params.clipPlanes);
    _UpdateHydraCollection(&_renderCollection, roots, params, &_renderTags);
    _taskController->SetCollection(_renderCollection);

    HdxRenderTaskParams hdParams = _MakeHydraRenderParams(params);
    _taskController->SetRenderParams(hdParams);
    _taskController->SetEnableSelection(params.highlight);

    Render(params);
}

bool
UsdImagingGLHdEngine::TestIntersection(
    const GfMatrix4d &viewMatrix,
    const GfMatrix4d &projectionMatrix,
    const GfMatrix4d &worldToLocalSpace,
    const UsdPrim& root, 
    RenderParams params,
    GfVec3d *outHitPoint,
    SdfPath *outHitPrimPath,
    SdfPath *outHitInstancerPath,
    int *outHitInstanceIndex,
    int *outHitElementIndex)
{
    if (!HdRenderContextCaps::GetInstance().SupportsHydra()) {
        TF_CODING_ERROR("Current GL context doesn't support Hydra");

       return false;
    }

    SdfPath rootPath = _delegate->GetPathForIndex(root.GetPath());
    SdfPathVector roots(1, rootPath);
    _UpdateHydraCollection(&_intersectCollection, roots, params, &_renderTags);

    HdxIntersector::HitVector allHits;
    HdxIntersector::Params qparams;
    qparams.viewMatrix = worldToLocalSpace * viewMatrix;
    qparams.projectionMatrix = projectionMatrix;
    qparams.alphaThreshold = params.alphaThreshold;
    qparams.renderTags = _renderTags;
    qparams.cullStyle = HdCullStyleNothing;

    if (!_taskController->TestIntersection(
            &_engine,
            _intersectCollection,
            qparams,
            HdxIntersectionModeTokens->nearest,
            &allHits)) {
        return false;
    }

    // Since we are in nearest-hit mode, and TestIntersection
    // returned true, we know allHits has a single point in it.
    TF_VERIFY(allHits.size() == 1);

    HdxIntersector::Hit &hit = allHits[0];

    if (outHitPoint) {
        *outHitPoint = GfVec3d(hit.worldSpaceHitPoint[0],
                               hit.worldSpaceHitPoint[1],
                               hit.worldSpaceHitPoint[2]);
    }
    if (outHitPrimPath) {
        *outHitPrimPath = hit.objectId;
    }
    if (outHitInstancerPath) {
        *outHitInstancerPath = hit.instancerId;
    }
    if (outHitInstanceIndex) {
        *outHitInstanceIndex = hit.instanceIndex;
    }
    if (outHitElementIndex) {
        *outHitElementIndex = hit.elementIndex;
    }

    return true;
}

bool
UsdImagingGLHdEngine::TestIntersectionBatch(
    const GfMatrix4d &viewMatrix,
    const GfMatrix4d &projectionMatrix,
    const GfMatrix4d &worldToLocalSpace,
    const SdfPathVector& paths, 
    RenderParams params,
    unsigned int pickResolution,
    PathTranslatorCallback pathTranslator,
    HitBatch *outHit)
{
    if (!HdRenderContextCaps::GetInstance().SupportsHydra()) {
        TF_CODING_ERROR("Current GL context doesn't support Hydra");
       return false;
    }

    _UpdateHydraCollection(&_intersectCollection, paths, params, &_renderTags);

    static const HdCullStyle USD_2_HD_CULL_STYLE[] =
    {
        HdCullStyleDontCare,              // No opinion, unused
        HdCullStyleNothing,               // CULL_STYLE_NOTHING,
        HdCullStyleBack,                  // CULL_STYLE_BACK,
        HdCullStyleFront,                 // CULL_STYLE_FRONT,
        HdCullStyleBackUnlessDoubleSided, // CULL_STYLE_BACK_UNLESS_DOUBLE_SIDED
    };
    static_assert(((sizeof(USD_2_HD_CULL_STYLE) / 
                    sizeof(USD_2_HD_CULL_STYLE[0])) 
                == UsdImagingGLEngine::CULL_STYLE_COUNT),"enum size mismatch");

    HdxIntersector::HitVector allHits;
    HdxIntersector::Params qparams;
    qparams.viewMatrix = worldToLocalSpace * viewMatrix;
    qparams.projectionMatrix = projectionMatrix;
    qparams.alphaThreshold = params.alphaThreshold;
    qparams.cullStyle = USD_2_HD_CULL_STYLE[params.cullStyle];
    qparams.renderTags = _renderTags;

    _taskController->SetPickResolution(pickResolution);
    if (!_taskController->TestIntersection(
            &_engine,
            _intersectCollection,
            qparams,
            HdxIntersectionModeTokens->unique,
            &allHits)) {
        return false;
    }

    if (!outHit) {
        return true;
    }

    for (const HdxIntersector::Hit& hit : allHits) {
        const SdfPath primPath = hit.objectId;
        const SdfPath instancerPath = hit.instancerId;
        const int instanceIndex = hit.instanceIndex;

        HitInfo& info = (*outHit)[pathTranslator(primPath, instancerPath,
            instanceIndex)];
        info.worldSpaceHitPoint = GfVec3d(hit.worldSpaceHitPoint[0],
                                          hit.worldSpaceHitPoint[1],
                                          hit.worldSpaceHitPoint[2]);
        info.hitInstanceIndex = instanceIndex;
    }

    return true;
}

void
UsdImagingGLHdEngine::Render(RenderParams params)
{
    // User is responsible for initalizing GL contenxt and glew
    if (!HdRenderContextCaps::GetInstance().SupportsHydra()) {
        TF_CODING_ERROR("Current GL context doesn't support Hydra");
        return;
    }

    // XXX: HdEngine should do this.
    GLuint vao;
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);
    glBindVertexArray(0);

    glPushAttrib(GL_ENABLE_BIT | GL_POLYGON_BIT);

    // hydra orients all geometry during topological processing so that
    // front faces have ccw winding. We disable culling because culling
    // is handled by fragment shader discard.
    if (params.flipFrontFacing) {
        glFrontFace(GL_CW); // < State is pushed via GL_POLYGON_BIT
    } else {
        glFrontFace(GL_CCW); // < State is pushed via GL_POLYGON_BIT
    }
    glDisable(GL_CULL_FACE);

    if (params.applyRenderState) {
        glDisable(GL_BLEND);
    }

    // note: to get benefit of alpha-to-coverage, the target framebuffer
    // has to be a MSAA buffer.
    if (params.enableIdRender) {
        glDisable(GL_SAMPLE_ALPHA_TO_COVERAGE);
    } else if (params.enableSampleAlphaToCoverage) {
        glEnable(GL_SAMPLE_ALPHA_TO_COVERAGE);
    }

    // for points width
    glEnable(GL_PROGRAM_POINT_SIZE);

    // TODO:
    //  * forceRefresh
    //  * showGuides, showRender, showProxy
    //  * gammaCorrectColors

    if (params.applyRenderState) {
        // drawmode.
        // XXX: Temporary solution until shader-based styling implemented.
        switch (params.drawMode) {
        case DRAW_POINTS:
            glPolygonMode(GL_FRONT_AND_BACK, GL_POINT);
            break;
        default:
            glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
            break;
        }
    }

    glBindVertexArray(vao);

    VtValue selectionValue(_selTracker);
    _engine.SetTaskContextData(HdxTokens->selectionState, selectionValue);
    VtValue renderTags(_renderTags);
    _engine.SetTaskContextData(HdxTokens->renderTags, renderTags);

    TfToken const& renderMode = params.enableIdRender ?
        HdxTaskSetTokens->idRender : HdxTaskSetTokens->colorRender;
    _engine.Execute(*_renderIndex, _taskController->GetTasks(renderMode));

    glBindVertexArray(0);

    glPopAttrib(); // GL_ENABLE_BIT | GL_POLYGON_BIT

    // XXX: We should not delete the VAO on every draw call, but we currently
    // must because it is GL Context state and we do not control the context.
    glDeleteVertexArrays(1, &vao);
}

/*virtual*/
void 
UsdImagingGLHdEngine::SetCameraState(const GfMatrix4d& viewMatrix,
                            const GfMatrix4d& projectionMatrix,
                            const GfVec4d& viewport)
{
    // If the view matrix changes at all, we need to reset the progressive
    // render.
    if (viewMatrix != _lastViewMatrix || viewport != _lastViewport) {
        _lastViewMatrix = viewMatrix;
        _lastViewport = viewport;
        _taskController->ResetImage();
    }

    // usdview passes these matrices from OpenGL state.
    // update the camera in the task controller accordingly.
    _taskController->SetCameraMatrices(viewMatrix, projectionMatrix);
    _taskController->SetCameraViewport(viewport);
}

/*virtual*/
SdfPath
UsdImagingGLHdEngine::GetRprimPathFromPrimId(int primId) const 
{
    return _delegate->GetRenderIndex().GetRprimPathFromPrimId(primId);
}

/* virtual */
SdfPath
UsdImagingGLHdEngine::GetPrimPathFromInstanceIndex(
    SdfPath const& protoPrimPath,
    int instanceIndex,
    int *absoluteInstanceIndex,
    SdfPath * rprimPath,
    SdfPathVector *instanceContext)
{
    return _delegate->GetPathForInstanceIndex(protoPrimPath, instanceIndex,
                                             absoluteInstanceIndex, rprimPath,
                                             instanceContext);
}

/* virtual */
void
UsdImagingGLHdEngine::SetLightingStateFromOpenGL()
{
    if (!_lightingContextForOpenGLState) {
        _lightingContextForOpenGLState = GlfSimpleLightingContext::New();
    }
    _lightingContextForOpenGLState->SetStateFromOpenGL();

    // Don't use the bypass lighting task.
    _taskController->SetLightingState(_lightingContextForOpenGLState, false);
}

/* virtual */
void
UsdImagingGLHdEngine::SetLightingState(GlfSimpleLightVector const &lights,
                                       GlfSimpleMaterial const &material,
                                       GfVec4f const &sceneAmbient)
{
    // we still use _lightingContextForOpenGLState for convenience, but
    // set the values directly.
    if (!_lightingContextForOpenGLState) {
        _lightingContextForOpenGLState = GlfSimpleLightingContext::New();
    }
    _lightingContextForOpenGLState->SetLights(lights);
    _lightingContextForOpenGLState->SetMaterial(material);
    _lightingContextForOpenGLState->SetSceneAmbient(sceneAmbient);
    _lightingContextForOpenGLState->SetUseLighting(lights.size() > 0);

    // Don't use the bypass lighting task.
    _taskController->SetLightingState(_lightingContextForOpenGLState, false);
}

/* virtual */
void
UsdImagingGLHdEngine::SetLightingState(GlfSimpleLightingContextPtr const &src)
{
    // Use the bypass lighting task; leave all lighting plumbing work to
    // the incoming lighting context.
    // XXX: the bypass lighting task will be removed when Phd takes over
    // all imaging in Presto.
    _taskController->SetLightingState(src, true);
}

/* virtual */
void
UsdImagingGLHdEngine::SetRootTransform(GfMatrix4d const& xf)
{
    _delegate->SetRootTransform(xf);
}

/* virtual */
void
UsdImagingGLHdEngine::SetRootVisibility(bool isVisible)
{
    _delegate->SetRootVisibility(isVisible);
}


/*virtual*/
void
UsdImagingGLHdEngine::SetSelected(SdfPathVector const& paths)
{
    // populate new selection
    HdxSelectionSharedPtr selection(new HdxSelection);
    for (SdfPath const& path : paths) {
        _delegate->PopulateSelection(path,
                                     UsdImagingDelegate::ALL_INSTANCES,
                                     selection);
    }

    // set the result back to selection tracker
    _selTracker->SetSelection(selection);
}

/*virtual*/
void
UsdImagingGLHdEngine::ClearSelected()
{
    HdxSelectionSharedPtr selection(new HdxSelection);
    _selTracker->SetSelection(selection);
}

/* virtual */
void
UsdImagingGLHdEngine::AddSelected(SdfPath const &path, int instanceIndex)
{
    HdxSelectionSharedPtr selection = _selTracker->GetSelectionMap();
    if (!selection) {
        selection.reset(new HdxSelection);
    }

    _delegate->PopulateSelection(path, instanceIndex, selection);

    // set the result back to selection tracker
    _selTracker->SetSelection(selection);
}

/*virtual*/
void
UsdImagingGLHdEngine::SetSelectionColor(GfVec4f const& color)
{
    _selectionColor = color;
    _taskController->SetSelectionColor(_selectionColor);
}

/* virtual */
bool
UsdImagingGLHdEngine::IsConverged() const
{
    return _taskController->IsConverged();
}

/* virtual */
std::vector<TfType>
UsdImagingGLHdEngine::GetRendererPlugins()
{
    HfPluginDescVector pluginDescriptors;
    HdxRendererPluginRegistry::GetInstance().GetPluginDescs(&pluginDescriptors);

    std::vector<TfType> plugins;
    for(size_t i = 0; i < pluginDescriptors.size(); ++i) {
        plugins.push_back(
                TfType::FindByName(pluginDescriptors[i].id.GetString()));
    }
    return plugins;
}

/* static */
bool
UsdImagingGLHdEngine::IsDefaultPluginAvailable()
{
    HfPluginDescVector descs;
    HdxRendererPluginRegistry::GetInstance().GetPluginDescs(&descs);
    return descs.size() > 0;
}

/* virtual */
bool
UsdImagingGLHdEngine::SetRendererPlugin(TfType const &type)
{
    HfPluginDescVector pluginDescriptors;
    HdxRendererPluginRegistry::GetInstance().GetPluginDescs(&pluginDescriptors);

    HdxRendererPlugin *plugin = nullptr;
    TfType actualType = type;
    // Special case: TfType() selects the first plugin in the list.
    if (type.IsUnknown() && pluginDescriptors.size() > 0) {
        plugin = HdxRendererPluginRegistry::GetInstance().
            GetRendererPlugin(pluginDescriptors[0].id);
        actualType = TfType::FindByName(pluginDescriptors[0].id.GetString());
    }
    // General case: find the matching type id.
    else {
        for(size_t i = 0; i < pluginDescriptors.size(); ++i) {
            if(pluginDescriptors[i].id == type.GetTypeName()) {
                plugin = HdxRendererPluginRegistry::GetInstance().
                    GetRendererPlugin(pluginDescriptors[i].id);
                break;
            }
        }
    }

    if (plugin == nullptr) {
        TF_CODING_ERROR("Couldn't find plugin named %s",
            type.GetTypeName().c_str());
        return false;
    }

    // Pull old delegate/task controller state.
    GfMatrix4d rootTransform = GfMatrix4d(1.0);
    bool isVisible = true;
    if (_delegate != nullptr) {
        rootTransform = _delegate->GetRootTransform();
        isVisible = _delegate->GetRootVisibility();
    }
    HdxSelectionSharedPtr selection = _selTracker->GetSelectionMap();
    if (!selection) {
        selection.reset(new HdxSelection);
    }

    // Delete hydra state.
    _DeleteHydraResources();

    // Recreate the render index.
    _renderPlugin = plugin;
    HdRenderDelegate *renderDelegate = _renderPlugin->CreateRenderDelegate();
    _renderIndex = HdRenderIndex::New(renderDelegate);

    // Create the new delegate & task controller.
    _delegate = new UsdImagingDelegate(_renderIndex, _delegateID);

    _isPopulated = false;
    _taskController = _renderPlugin->CreateTaskController(_renderIndex,
        _delegateID.AppendChild(TfToken(TfStringPrintf(
            "_UsdImaging_%s_%p",
            TfMakeValidIdentifier(actualType.GetTypeName()).c_str(),
            this))));

    // Rebuild state in the new delegate/task controller.
    _delegate->SetRootVisibility(isVisible);
    _delegate->SetRootTransform(rootTransform);
    _selTracker->SetSelection(selection);
    _taskController->SetSelectionColor(_selectionColor);

    return true;
}

void
UsdImagingGLHdEngine::_DeleteHydraResources()
{
    // The unwinding order is a little complicated; it's the same as
    // initialization order, but we need to be null-safe and track all
    // the pointers down.
    //
    // 1. Task Controller
    // 2. USD delegate
    // 3. Render Index
    // 4. Render Delegate (from the RI)
    // 5. Render plugin
    
    if (_renderPlugin != nullptr && _taskController != nullptr) {
        _renderPlugin->DeleteTaskController(_taskController);
    }
    if (_delegate != nullptr) {
        delete _delegate;
    }
    HdRenderDelegate *renderDelegate = nullptr;
    if (_renderIndex != nullptr) {
        renderDelegate = _renderIndex->GetRenderDelegate();
        delete _renderIndex;
    }
    if (_renderPlugin != nullptr) {
        if (renderDelegate != nullptr) {
            _renderPlugin->DeleteRenderDelegate(renderDelegate);
        }
        HdxRendererPluginRegistry::GetInstance().ReleasePlugin(_renderPlugin);
    }
}

/* virtual */
VtDictionary
UsdImagingGLHdEngine::GetResourceAllocation() const
{
    return HdResourceRegistry::GetInstance().GetResourceAllocation();
}

PXR_NAMESPACE_CLOSE_SCOPE

