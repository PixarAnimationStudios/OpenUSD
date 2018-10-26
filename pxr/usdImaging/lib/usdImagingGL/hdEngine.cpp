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
#include "pxr/imaging/glf/contextCaps.h"

#include "pxr/usdImaging/usdImagingGL/hdEngine.h"
#include "pxr/usdImaging/usdImaging/tokens.h"

#include "pxr/imaging/hd/debugCodes.h"
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

#include "pxr/base/tf/getenv.h"
#include "pxr/base/tf/stringUtils.h"

#include "pxr/base/arch/demangle.h"

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
    , _rendererPlugin(nullptr)
    , _taskController(nullptr)
    , _selectionColor(1.0f, 1.0f, 0.0f, 1.0f)
    , _viewport(0.0f, 0.0f, 512.0f, 512.0f)
    , _rootPath(rootPath)
    , _excludedPrimPaths(excludedPrimPaths)
    , _invisedPrimPaths(invisedPrimPaths)
    , _isPopulated(false)
    , _renderTags()
{
    // _renderIndex, _taskController, and _delegate are initialized
    // by the plugin system.
    if (!SetRendererPlugin(_GetDefaultRendererPluginId())) {
        TF_CODING_ERROR("No renderer plugins found! Check before creation.");
    }
}

UsdImagingGLHdEngine::~UsdImagingGLHdEngine() 
{
    _DeleteHydraResources();
}

HdRenderIndex *
UsdImagingGLHdEngine::_GetRenderIndex() const
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
                                       const UsdImagingGLRenderParams& params)
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
UsdImagingGLHdEngine::_PreSetTime(const UsdPrim& root, 
    const UsdImagingGLRenderParams& params)
{
    HD_TRACE_FUNCTION();

    // Set the fallback refine level, if this changes from the existing value,
    // all prim refine levels will be dirtied.
    int refineLevel = _GetRefineLevel(params.complexity);
    _delegate->SetRefineLevelFallback(refineLevel);

    // Apply any queued up scene edits.
    _delegate->ApplyPendingUpdates();
}

void
UsdImagingGLHdEngine::_PostSetTime(const UsdPrim& root, 
    const UsdImagingGLRenderParams& params)
{
    HD_TRACE_FUNCTION();
}

/*virtual*/
void
UsdImagingGLHdEngine::PrepareBatch(const UsdPrim& root, 
    const UsdImagingGLRenderParams &params)
{
    HD_TRACE_FUNCTION();

    if (_CanPrepareBatch(root, params)) {
        if (!_isPopulated) {
            _delegate->SetUsdDrawModesEnabled(params.enableUsdDrawModes);
            _delegate->Populate(root.GetStage()->GetPrimAtPath(_rootPath),
                               _excludedPrimPaths);
            _delegate->SetInvisedPrimPaths(_invisedPrimPaths);
            _isPopulated = true;
        }

        _PreSetTime(root, params);
        // SetTime will only react if time actually changes.
        _delegate->SetTime(params.frame);
        _PostSetTime(root, params);
    }
}

/* static */
bool
UsdImagingGLHdEngine::_UpdateHydraCollection(HdRprimCollection *collection,
                          SdfPathVector const& roots,
                          UsdImagingGLRenderParams const& params,
                          TfTokenVector *renderTags)
{
    if (collection == nullptr) {
        TF_CODING_ERROR("Null passed to _UpdateHydraCollection");
        return false;
    }

    // choose repr
    HdReprSelector reprSelector = HdReprSelector(HdReprTokens->smoothHull);
    bool refined = params.complexity > 1.0;

    if (params.drawMode == UsdImagingGLDrawMode::DRAW_GEOM_FLAT ||
        params.drawMode == UsdImagingGLDrawMode::DRAW_SHADED_FLAT) {
        // Flat shading
        reprSelector = HdReprSelector(HdReprTokens->hull);
    } else if (
        params.drawMode == UsdImagingGLDrawMode::DRAW_WIREFRAME_ON_SURFACE) {
        // Wireframe on surface
        reprSelector = HdReprSelector(refined ?
            HdReprTokens->refinedWireOnSurf : HdReprTokens->wireOnSurf);
    } else if (params.drawMode == UsdImagingGLDrawMode::DRAW_WIREFRAME) {
        // Wireframe
        reprSelector = HdReprSelector(refined ?
            HdReprTokens->refinedWire : HdReprTokens->wire);
    } else {
        // Smooth shading
        reprSelector = HdReprSelector(refined ?
            HdReprTokens->refined : HdReprTokens->smoothHull);
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
                 collection->GetReprSelector() == reprSelector &&
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
        if (match) return false;
    }

    // Recreate the collection.
    *collection = HdRprimCollection(colName, reprSelector);
    collection->SetRootPaths(roots);
    collection->SetRenderTags(*renderTags);

    return true;
}

/* static */
HdxRenderTaskParams
UsdImagingGLHdEngine::_MakeHydraUsdImagingGLRenderParams(
    UsdImagingGLRenderParams const& renderParams)
{
    // Note this table is dangerous and making changes to the order of the 
    // enums in UsdImagingGLCullStyle, will affect this with no compiler help.
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
              == (size_t)UsdImagingGLCullStyle::CULL_STYLE_COUNT),
        "enum size mismatch");

    HdxRenderTaskParams params;

    params.overrideColor       = renderParams.overrideColor;
    params.wireframeColor      = renderParams.wireframeColor;

    if (renderParams.drawMode == UsdImagingGLDrawMode::DRAW_GEOM_ONLY ||
        renderParams.drawMode == UsdImagingGLDrawMode::DRAW_POINTS) {
        params.enableLighting = false;
    } else {
        params.enableLighting =  renderParams.enableLighting &&
                                !renderParams.enableIdRender;
    }

    params.enableIdRender      = renderParams.enableIdRender;
    params.depthBiasUseDefault = true;
    params.depthFunc           = HdCmpFuncLess;
    params.cullStyle           = USD_2_HD_CULL_STYLE[
        (size_t)renderParams.cullStyle];
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

    params.enableSceneMaterials = renderParams.enableSceneMaterials;

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
UsdImagingGLHdEngine::RenderBatch(const SdfPathVector& paths, 
    const UsdImagingGLRenderParams& params)
{
    _taskController->SetCameraClipPlanes(params.clipPlanes);
    _UpdateHydraCollection(&_renderCollection, paths, params, &_renderTags);
    _taskController->SetCollection(_renderCollection);

    HdxRenderTaskParams hdParams = _MakeHydraUsdImagingGLRenderParams(params);
    _taskController->SetRenderParams(hdParams);
    _taskController->SetEnableSelection(params.highlight);

    _Render(params);
}

/*virtual*/
void
UsdImagingGLHdEngine::Render(const UsdPrim& root, 
    const UsdImagingGLRenderParams& params)
{
    PrepareBatch(root, params);

    SdfPath rootPath = _delegate->GetPathForIndex(root.GetPath());
    SdfPathVector roots(1, rootPath);

    _taskController->SetCameraClipPlanes(params.clipPlanes);
    _UpdateHydraCollection(&_renderCollection, roots, params, &_renderTags);
    _taskController->SetCollection(_renderCollection);

    HdxRenderTaskParams hdParams = _MakeHydraUsdImagingGLRenderParams(params);
    _taskController->SetRenderParams(hdParams);
    _taskController->SetEnableSelection(params.highlight);

    _Render(params);
}

bool
UsdImagingGLHdEngine::TestIntersection(
    const GfMatrix4d &viewMatrix,
    const GfMatrix4d &projectionMatrix,
    const GfMatrix4d &worldToLocalSpace,
    const UsdPrim& root, 
    const UsdImagingGLRenderParams& params,
    GfVec3d *outHitPoint,
    SdfPath *outHitPrimPath,
    SdfPath *outHitInstancerPath,
    int *outHitInstanceIndex,
    int *outHitElementIndex)
{
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
    qparams.enableSceneMaterials = params.enableSceneMaterials;

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

class _DebugGroupTaskWrapper : public HdTask {
    const HdTaskSharedPtr _task;
    public:
    _DebugGroupTaskWrapper(const HdTaskSharedPtr task)
        : _task(task)
    {

    }

    void
    _Execute(HdTaskContext* ctx) override
    {
        GlfDebugGroup dbgGroup((ArchGetDemangled(typeid(*_task.get())) +
                "::Execute").c_str());
        _task->Execute(ctx);
    }

    void
    _Sync(HdTaskContext* ctx) override
    {
        GlfDebugGroup dbgGroup((ArchGetDemangled(typeid(*_task.get())) +
                "::Sync").c_str());
        _task->Sync(ctx);
    }
};

/*virtual*/
void
UsdImagingGLHdEngine::_Render(const UsdImagingGLRenderParams& params)
{
    // Forward scene materials enable option to delegate
    _delegate->SetSceneMaterialsEnabled(params.enableSceneMaterials);


    // User is responsible for initializing GL context and glew
    bool isCoreProfileContext = GlfContextCaps::GetInstance().coreProfile;

    GLF_GROUP_FUNCTION();

    GLuint vao;
    if (isCoreProfileContext) {
        // We must bind a VAO (Vertex Array Object) because core profile 
        // contexts do not have a default vertex array object. VAO objects are 
        // container objects which are not shared between contexts, so we create
        // and bind a VAO here so that core rendering code does not have to 
        // explicitly manage per-GL context state.
        glGenVertexArrays(1, &vao);
        glBindVertexArray(vao);
    } else {
        glPushAttrib(GL_ENABLE_BIT | GL_POLYGON_BIT | GL_DEPTH_BUFFER_BIT);
    }

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
        case UsdImagingGLDrawMode::DRAW_POINTS:
            glPolygonMode(GL_FRONT_AND_BACK, GL_POINT);
            break;
        default:
            glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
            break;
        }
    }

    VtValue selectionValue(_selTracker);
    _engine.SetTaskContextData(HdxTokens->selectionState, selectionValue);
    VtValue renderTags(_renderTags);
    _engine.SetTaskContextData(HdxTokens->renderTags, renderTags);

    HdTaskSharedPtrVector tasks;
    
    if (false) {
        tasks = _taskController->GetTasks();
    } else {
        TF_FOR_ALL(it, _taskController->GetTasks()) {
            tasks.push_back(boost::make_shared<_DebugGroupTaskWrapper>(*it));
        }
    }
    _engine.Execute(*_renderIndex, tasks);

    if (isCoreProfileContext) {

        glBindVertexArray(0);
        // XXX: We should not delete the VAO on every draw call, but we 
        // currently must because it is GL Context state and we do not control 
        // the context.
        glDeleteVertexArrays(1, &vao);

    } else {
        glPopAttrib(); // GL_ENABLE_BIT | GL_POLYGON_BIT | GL_DEPTH_BUFFER_BIT
    }
}

/*virtual*/
void 
UsdImagingGLHdEngine::SetCameraState(const GfMatrix4d& viewMatrix,
                            const GfMatrix4d& projectionMatrix,
                            const GfVec4d& viewport)
{
    // usdview passes these matrices from OpenGL state.
    // update the camera in the task controller accordingly.
    _taskController->SetCameraMatrices(viewMatrix, projectionMatrix);
    _taskController->SetCameraViewport(viewport);
    _viewport = viewport;
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

    _taskController->SetLightingState(_lightingContextForOpenGLState);
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

    _taskController->SetLightingState(_lightingContextForOpenGLState);
}

/* virtual */
void
UsdImagingGLHdEngine::SetLightingState(GlfSimpleLightingContextPtr const &src)
{
    _taskController->SetLightingState(src);
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
    HdSelectionSharedPtr selection(new HdSelection);
    // XXX: Usdview currently supports selection on click. If we extend to
    // rollover (locate) selection, we need to pass that mode here.
    HdSelection::HighlightMode mode = HdSelection::HighlightModeSelect;
    for (SdfPath const& path : paths) {
        _delegate->PopulateSelection(mode,
                                     path,
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
    HdSelectionSharedPtr selection(new HdSelection);
    _selTracker->SetSelection(selection);
}

/* virtual */
void
UsdImagingGLHdEngine::AddSelected(SdfPath const &path, int instanceIndex)
{
    HdSelectionSharedPtr selection = _selTracker->GetSelectionMap();
    if (!selection) {
        selection.reset(new HdSelection);
    }
    // XXX: Usdview currently supports selection on click. If we extend to
    // rollover (locate) selection, we need to pass that mode here.
    HdSelection::HighlightMode mode = HdSelection::HighlightModeSelect;
    _delegate->PopulateSelection(mode, path, instanceIndex, selection);

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
TfTokenVector
UsdImagingGLHdEngine::GetRendererPlugins() const
{
    HfPluginDescVector pluginDescriptors;
    HdxRendererPluginRegistry::GetInstance().GetPluginDescs(&pluginDescriptors);

    TfTokenVector plugins;
    for(size_t i = 0; i < pluginDescriptors.size(); ++i) {
        plugins.push_back(pluginDescriptors[i].id);
    }
    return plugins;
}

/* virtual */
std::string
UsdImagingGLHdEngine::GetRendererDisplayName(TfToken const &id) const
{
    HfPluginDesc pluginDescriptor;
    if (!TF_VERIFY(HdxRendererPluginRegistry::GetInstance().
                   GetPluginDesc(id, &pluginDescriptor))) {
        return std::string();
    }

    return pluginDescriptor.displayName;
}

/* virtual */
TfToken
UsdImagingGLHdEngine::GetCurrentRendererId() const
{
    return _rendererId;
}

/* static */
TfToken
UsdImagingGLHdEngine::_GetDefaultRendererPluginId()
{
    std::string defaultRendererDisplayName = 
        TfGetenv("HD_DEFAULT_RENDERER", "");

    if (defaultRendererDisplayName.empty()) {
        return TfToken();
    }

    HfPluginDescVector pluginDescs;
    HdxRendererPluginRegistry::GetInstance().GetPluginDescs(&pluginDescs);

    // Look for the one with the matching display name
    for (size_t i = 0; i < pluginDescs.size(); ++i) {
        if (pluginDescs[i].displayName == defaultRendererDisplayName) {
            return pluginDescs[i].id;
        }
    }

    TF_WARN("Failed to find default renderer with display name '%s'.",
            defaultRendererDisplayName.c_str());

    return TfToken();
}

/* virtual */
bool
UsdImagingGLHdEngine::SetRendererPlugin(TfToken const &id)
{
    HdxRendererPlugin *plugin = nullptr;
    TfToken actualId = id;

    // Special case: TfToken() selects the first plugin in the list.
    if (actualId.IsEmpty()) {
        actualId = HdxRendererPluginRegistry::GetInstance().
            GetDefaultPluginId();
    }
    plugin = HdxRendererPluginRegistry::GetInstance().
        GetRendererPlugin(actualId);

    if (plugin == nullptr) {
        TF_CODING_ERROR("Couldn't find plugin for id %s", actualId.GetText());
        return false;
    } else if (plugin == _rendererPlugin) {
        // It's a no-op to load the same plugin twice.
        HdxRendererPluginRegistry::GetInstance().ReleasePlugin(plugin);
        return true;
    } else if (!plugin->IsSupported()) {
        // Don't do anything if the plugin isn't supported on the running
        // system, just return that we're not able to set it.
        HdxRendererPluginRegistry::GetInstance().ReleasePlugin(plugin);
        return false;
    }

    // Pull old delegate/task controller state.
    GfMatrix4d rootTransform = GfMatrix4d(1.0);
    bool isVisible = true;
    if (_delegate != nullptr) {
        rootTransform = _delegate->GetRootTransform();
        isVisible = _delegate->GetRootVisibility();
    }
    HdSelectionSharedPtr selection = _selTracker->GetSelectionMap();
    if (!selection) {
        selection.reset(new HdSelection);
    }

    // Delete hydra state.
    _DeleteHydraResources();

    // Recreate the render index.
    _rendererPlugin = plugin;
    _rendererId = actualId;

    // Pass the viewport dimensions into CreateRenderDelegate, for backends that
    // need to allocate the viewport early.
    HdRenderSettingsMap renderSettings;
    renderSettings[HdRenderSettingsTokens->renderBufferWidth] =
        int(_viewport[2]);
    renderSettings[HdRenderSettingsTokens->renderBufferHeight] =
        int(_viewport[3]);

    HdRenderDelegate *renderDelegate =
        _rendererPlugin->CreateRenderDelegate(renderSettings);
    _renderIndex = HdRenderIndex::New(renderDelegate);

    // Create the new delegate & task controller.
    _delegate = new UsdImagingDelegate(_renderIndex, _delegateID);
    _isPopulated = false;

    _taskController = new HdxTaskController(_renderIndex,
        _delegateID.AppendChild(TfToken(TfStringPrintf(
            "_UsdImaging_%s_%p",
            TfMakeValidIdentifier(actualId.GetText()).c_str(),
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
    // Unwinding order: remove data sources first (task controller, scene
    // delegate); then render index; then render delegate; finally the
    // renderer plugin used to manage the render delegate.
    
    if (_taskController != nullptr) {
        delete _taskController;
        _taskController = nullptr;
    }
    if (_delegate != nullptr) {
        delete _delegate;
        _delegate = nullptr;
    }
    HdRenderDelegate *renderDelegate = nullptr;
    if (_renderIndex != nullptr) {
        renderDelegate = _renderIndex->GetRenderDelegate();
        delete _renderIndex;
        _renderIndex = nullptr;
    }
    if (_rendererPlugin != nullptr) {
        if (renderDelegate != nullptr) {
            _rendererPlugin->DeleteRenderDelegate(renderDelegate);
        }
        HdxRendererPluginRegistry::GetInstance().ReleasePlugin(_rendererPlugin);
        _rendererPlugin = nullptr;
        _rendererId = TfToken();
    }
}

/* virtual */
TfTokenVector
UsdImagingGLHdEngine::GetRendererAovs() const
{
    if (_renderIndex->IsBprimTypeSupported(HdPrimTypeTokens->renderBuffer)) {
        return TfTokenVector(
            { HdAovTokens->color,
              HdAovTokens->primId,
              HdAovTokens->depth,
              HdAovTokens->normal,
              HdAovTokensMakePrimvar(TfToken("st")) }
        );
    }
    return TfTokenVector();
}

/* virtual */
bool
UsdImagingGLHdEngine::SetRendererAov(TfToken const& id)
{
    if (_renderIndex->IsBprimTypeSupported(HdPrimTypeTokens->renderBuffer)) {
        // For color, render straight to the viewport instead of rendering
        // to an AOV and colorizing (which is the same, but more work).
        if (id == HdAovTokens->color) {
            _taskController->SetRenderOutputs(TfTokenVector());
        } else {
            _taskController->SetRenderOutputs({id});
        }
        return true;
    }
    return false;
}

/* virtual */
VtDictionary
UsdImagingGLHdEngine::GetResourceAllocation() const
{
    return _renderIndex->GetResourceRegistry()->GetResourceAllocation();
}

/* virtual */
UsdImagingGLRendererSettingsList
UsdImagingGLHdEngine::GetRendererSettingsList() const
{
    HdRenderSettingDescriptorList descriptors =
        _renderIndex->GetRenderDelegate()->GetRenderSettingDescriptors();
    UsdImagingGLRendererSettingsList ret;

    for (auto const& desc : descriptors) {
        UsdImagingGLRendererSetting r;
        r.key = desc.key;
        r.name = desc.name;
        r.defValue = desc.defaultValue;

        // Use the type of the default value to tell us what kind of
        // widget to create...
        if (r.defValue.IsHolding<bool>()) {
            r.type = UsdImagingGLRendererSetting::TYPE_FLAG;
        } else if (r.defValue.IsHolding<int>() ||
                   r.defValue.IsHolding<unsigned int>()) {
            r.type = UsdImagingGLRendererSetting::TYPE_INT;
        } else if (r.defValue.IsHolding<float>()) {
            r.type = UsdImagingGLRendererSetting::TYPE_FLOAT;
        } else if (r.defValue.IsHolding<std::string>()) {
            r.type = UsdImagingGLRendererSetting::TYPE_STRING;
        } else {
            TF_WARN("Setting '%s' with type '%s' doesn't have a UI"
                    " implementation...",
                    r.name.c_str(),
                    r.defValue.GetTypeName().c_str());
            continue;
        }
        ret.push_back(r);
    }

    return ret;
}

/* virtual */
VtValue
UsdImagingGLHdEngine::GetRendererSetting(TfToken const& id) const
{
    return _renderIndex->GetRenderDelegate()->GetRenderSetting(id);
}

/* virtual */
void
UsdImagingGLHdEngine::SetRendererSetting(TfToken const& id,
                                         VtValue const& value)
{
    _renderIndex->GetRenderDelegate()->SetRenderSetting(id, value);
}

PXR_NAMESPACE_CLOSE_SCOPE

