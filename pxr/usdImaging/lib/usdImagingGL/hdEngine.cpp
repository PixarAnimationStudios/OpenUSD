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

#include "pxr/usdImaging/usdImagingGL/defaultTaskDelegate.h"
#include "pxr/usdImaging/usdImagingGL/taskDelegate.h"

#include "pxr/imaging/hd/version.h"
#include "pxr/imaging/hd/tokens.h"
#include "pxr/imaging/hd/renderContextCaps.h"
#include "pxr/imaging/hd/resourceRegistry.h"
#include "pxr/imaging/hd/debugCodes.h"

#include "pxr/imaging/hdx/intersector.h"
#include "pxr/imaging/hdx/tokens.h"

#include "pxr/base/gf/matrix4d.h"
#include "pxr/base/gf/range3d.h"
#include "pxr/base/gf/rotation.h"
#include "pxr/base/gf/vec3d.h"

#include "pxr/base/plug/plugin.h"
#include "pxr/base/plug/registry.h"

#include "pxr/imaging/glf/diagnostic.h"
#include "pxr/imaging/glf/info.h"
#include "pxr/imaging/glf/simpleLightingContext.h"

PXR_NAMESPACE_OPEN_SCOPE


UsdImagingGLHdEngine::UsdImagingGLHdEngine(
        const SdfPath& rootPath,
        const SdfPathVector& excludedPrimPaths,
        const SdfPathVector& invisedPrimPaths,
        const SdfPath& delegateID)
    : UsdImagingGLEngine()
    , _renderIndex(nullptr)
    , _selTracker(new HdxSelectionTracker)
    , _intersector(nullptr)
    , _delegate(nullptr)
    , _defaultTaskDelegate()
    , _pluginDiscovered(false)
    , _rootPath(rootPath)
    , _excludedPrimPaths(excludedPrimPaths)
    , _invisedPrimPaths(invisedPrimPaths)
    , _isPopulated(false)
{
    _renderIndex = HdRenderIndex::New(&_renderDelegate);
    if (_renderIndex == nullptr) {
        TF_CODING_ERROR("Failed to create render index");
        return;
    }
    _delegate = new UsdImagingDelegate(_renderIndex, delegateID);
    _defaultTaskDelegate = UsdImagingGL_DefaultTaskDelegateSharedPtr(
                new UsdImagingGL_DefaultTaskDelegate(_renderIndex, delegateID));
    _intersector = HdxIntersectorSharedPtr(new HdxIntersector(_renderIndex));
}

UsdImagingGLHdEngine::~UsdImagingGLHdEngine() 
{
    _defaultTaskDelegate.reset();
    _pluginTaskDelegates.clear();
    delete _delegate;
    delete _renderIndex;
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

UsdImagingGLTaskDelegateSharedPtr
UsdImagingGLHdEngine::_GetTaskDelegate(const RenderParams &params) const
{
    // if \p params can be handled by the plugin task, return it
    if (_currentPluginTaskDelegate) {
        if (_currentPluginTaskDelegate->CanRender(params)) {
            return _currentPluginTaskDelegate;
        }
    }

    // fallback to the defualt task delegate
    return _defaultTaskDelegate;
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
        // Will only react if time actually changes.
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


/*virtual*/
void
UsdImagingGLHdEngine::RenderBatch(const SdfPathVector& paths, RenderParams params)
{
    _GetTaskDelegate(params)->SetCollectionAndRenderParams(
        paths, params);

    Render(_delegate->GetRenderIndex(), params);
}

/*virtual*/
void
UsdImagingGLHdEngine::Render(const UsdPrim& root, RenderParams params)
{
    PrepareBatch(root, params);

    SdfPath rootPath = _delegate->GetPathForIndex(root.GetPath());
    SdfPathVector roots(1, rootPath);

    _GetTaskDelegate(params)->SetCollectionAndRenderParams(
        roots, params);

    Render(_delegate->GetRenderIndex(), params);
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
    int *outHitInstanceIndex)
{
    if (!HdRenderContextCaps::GetInstance().SupportsHydra()) {
        TF_CODING_ERROR("Current GL context doesn't support Hydra");

       return false;
    }
    if (!TF_VERIFY(_intersector)) {
        return false;
    }

    SdfPath rootPath = _delegate->GetPathForIndex(root.GetPath());
    SdfPathVector roots(1, rootPath);
    _GetTaskDelegate(params)->SetCollectionAndRenderParams(roots, params);
    HdRprimCollection const& col = 
                                _GetTaskDelegate(params)->GetRprimCollection();

    HdxIntersector::Params qparams;
    qparams.viewMatrix = worldToLocalSpace * viewMatrix;
    qparams.projectionMatrix = projectionMatrix;
    qparams.alphaThreshold = params.alphaThreshold;
    
    HdxIntersector::Result result;
    HdxIntersector::Hit hit;

    if (!_intersector->Query(qparams, col, &_engine, &result)) {
        return false;
    }
    if (!result.ResolveNearest(&hit)) {
        return false;
    }
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
    if (!TF_VERIFY(_intersector)) {
        return false;
    }

    _GetTaskDelegate(params)->SetCollectionAndRenderParams(paths, params);
    HdRprimCollection const& col = 
                                _GetTaskDelegate(params)->GetRprimCollection();

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

    HdxIntersector::Params qparams;
    qparams.viewMatrix = worldToLocalSpace * viewMatrix;
    qparams.projectionMatrix = projectionMatrix;
    qparams.alphaThreshold = params.alphaThreshold;
    qparams.cullStyle = USD_2_HD_CULL_STYLE[params.cullStyle];

    _intersector->SetResolution(GfVec2i(pickResolution, pickResolution));
    
    HdxIntersector::Result result;
    HdxIntersector::HitSet hits;

    if (!_intersector->Query(qparams, col, &_engine, &result)) {
        return false;
    }
    if (!result.ResolveUnique(&hits)) {
        return false;
    }
    if (!outHit) {
        return true;
    }

    for (const HdxIntersector::Hit& hit : hits) {
        const SdfPath primPath = hit.objectId;
        const SdfPath instancerPath = hit.instancerId;
        const int instanceIndex = hit.instanceIndex;

        HitInfo& info = (*outHit)[pathTranslator(primPath, instancerPath, instanceIndex)];
        info.worldSpaceHitPoint = GfVec3d(hit.worldSpaceHitPoint[0],
                                          hit.worldSpaceHitPoint[1],
                                          hit.worldSpaceHitPoint[2]);
        info.hitInstanceIndex = instanceIndex;
    }

    return true;
}

void
UsdImagingGLHdEngine::Render(HdRenderIndex& index, RenderParams params)
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
    _engine.Execute(index, _GetTaskDelegate(params)->GetRenderTasks(params));

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
    // usdview passes these matrices from OpenGL state.
    // update the HdCamera in TaskDelegate accordingly.
    _defaultTaskDelegate->SetCameraState(
        viewMatrix, projectionMatrix, viewport);

    if (_currentPluginTaskDelegate)
        _currentPluginTaskDelegate->SetCameraState(
            viewMatrix, projectionMatrix, viewport);
}

/*virtual*/
SdfPath 
UsdImagingGLHdEngine::GetPrimPathFromPrimIdColor(GfVec4i const & primIdColor,
                                               GfVec4i const & instanceIdColor,
                                               int * instanceIndexOut)
{
    return _delegate->GetRenderIndex().GetPrimPathFromPrimIdColor(
                        primIdColor, instanceIdColor, instanceIndexOut);
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

    _defaultTaskDelegate->SetLightingState(_lightingContextForOpenGLState);

    if (_currentPluginTaskDelegate)
        _currentPluginTaskDelegate->SetLightingState(
            _lightingContextForOpenGLState);
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

    _defaultTaskDelegate->SetLightingState(_lightingContextForOpenGLState);

    if (_currentPluginTaskDelegate)
        _currentPluginTaskDelegate->SetLightingState(
            _lightingContextForOpenGLState);
}

/* virtual */
void
UsdImagingGLHdEngine::SetLightingState(GlfSimpleLightingContextPtr const &src)
{
    // leave all lighting plumbing work to the incoming lighting context.
    // XXX: this call will be replaced with SetLightingState() when Phd takes
    // over all imaging in Presto.
    _defaultTaskDelegate->SetBypassedLightingState(src);

    // no need to set bypassed light to plugin task delegates.
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
    HdxSelectionSharedPtr selection(new HdxSelection(&*_renderIndex));
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
    HdxSelectionSharedPtr selection(new HdxSelection(&*_renderIndex));
    _selTracker->SetSelection(selection);
}

/* virtual */
void
UsdImagingGLHdEngine::AddSelected(SdfPath const &path, int instanceIndex)
{
    HdxSelectionSharedPtr selection = _selTracker->GetSelectionMap();
    if (!selection) {
        selection.reset(new HdxSelection(&*_renderIndex));
    }

    _delegate->PopulateSelection(path, instanceIndex, selection);

    // set the result back to selection tracker
    _selTracker->SetSelection(selection);
}

/*virtual*/
void
UsdImagingGLHdEngine::SetSelectionColor(GfVec4f const& color)
{
    _defaultTaskDelegate->SetSelectionColor(color);
}

/* virtual */
bool
UsdImagingGLHdEngine::IsConverged() const
{
    if (_currentPluginTaskDelegate)
        return _currentPluginTaskDelegate->IsConverged();

    return _defaultTaskDelegate->IsConverged();
}

/* virtual */
std::vector<TfType>
UsdImagingGLHdEngine::GetRenderGraphPlugins()
{
    // discover plugins
    if (!_pluginDiscovered) {
        _pluginDiscovered = true;

        std::set<TfType> pluginTaskTypes;
        PlugRegistry::GetAllDerivedTypes(
            TfType::Find<UsdImagingGLTaskDelegate>(), &pluginTaskTypes);

        // create entries (!load plugins yet)
        TF_FOR_ALL (it, pluginTaskTypes) {
            _pluginTaskDelegates[*it] = UsdImagingGLTaskDelegateSharedPtr();
        }
    }

    // return the plugin type vector
    std::vector<TfType> types;
    types.reserve(_pluginTaskDelegates.size());
    TF_FOR_ALL (it, _pluginTaskDelegates) {
        types.push_back(it->first);
    }
    return types;
}

/* virtual */
bool
UsdImagingGLHdEngine::SetRenderGraphPlugin(TfType const &type)
{
    _currentPluginTaskDelegate.reset();
    if (!type) {
        // revert to default task delegate.
        return true;
    }

    // lookup plugin task delegate
    _PluginTaskDelegateMap::iterator it = _pluginTaskDelegates.find(type);
    if (it == _pluginTaskDelegates.end()) {
        TF_WARN("RenderGraph plugin not found : %s\n", type.GetTypeName().c_str());
        return false;
    }

    if (it->second) {
        _currentPluginTaskDelegate = it->second;
        return true;
    }

    // create if not constructed
    PlugPluginPtr plugin
        = PlugRegistry::GetInstance().GetPluginForType(type);
    if (plugin) {
        if (!plugin->Load()) {
            TF_WARN("Fail to load plugin: %s\n", plugin->GetName().c_str());
            return false;
        }
    } else {
        TF_WARN("Plugin not found for type: %s\n", type.GetTypeName().c_str());
        return false;
    }

    UsdImagingGLTaskDelegateFactoryBase* factory =
        type.GetFactory<UsdImagingGLTaskDelegateFactoryBase>();
    if (!factory) {
        TF_WARN("Plugin type not manufacturable: %s\n", type.GetTypeName().c_str());
        return false;
    }

    UsdImagingGLTaskDelegateSharedPtr taskDelegate =
        factory->New(_renderIndex, _delegate->GetDelegateID()/*=shareId*/);
    if (!taskDelegate) {
        TF_WARN("Fail to manufacture plugin %s\n", type.GetTypeName().c_str());
        return false;
    }
    _currentPluginTaskDelegate = taskDelegate;

    _pluginTaskDelegates[type] = _currentPluginTaskDelegate;

    return true;
}

/* virtual */
VtDictionary
UsdImagingGLHdEngine::GetResourceAllocation() const
{
    return HdResourceRegistry::GetInstance().GetResourceAllocation();
}

PXR_NAMESPACE_CLOSE_SCOPE

