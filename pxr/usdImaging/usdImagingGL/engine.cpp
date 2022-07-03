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
#include "pxr/imaging/garch/glApi.h"

#include "pxr/usdImaging/usdImagingGL/engine.h"

#include "pxr/usdImaging/usdImagingGL/legacyEngine.h"
#include "pxr/usdImaging/usdImagingGL/drawModeSceneIndex.h"

#include "pxr/usdImaging/usdImaging/delegate.h"
#include "pxr/usdImaging/usdImaging/stageSceneIndex.h"
#include "pxr/imaging/hd/flatteningSceneIndex.h"

#include "pxr/usd/usdGeom/tokens.h"
#include "pxr/usd/usdGeom/camera.h"

#include "pxr/imaging/hd/rendererPlugin.h"
#include "pxr/imaging/hd/rendererPluginRegistry.h"
#include "pxr/imaging/hdx/pickTask.h"
#include "pxr/imaging/hdx/taskController.h"
#include "pxr/imaging/hdx/tokens.h"

#include "pxr/imaging/hgi/hgi.h"
#include "pxr/imaging/hgi/tokens.h"

#include "pxr/base/tf/envSetting.h"
#include "pxr/base/tf/getenv.h"
#include "pxr/base/tf/stl.h"

#include "pxr/base/gf/matrix4d.h"
#include "pxr/base/gf/vec3d.h"

#include <string>

PXR_NAMESPACE_OPEN_SCOPE

TF_DEFINE_ENV_SETTING(USDIMAGINGGL_ENGINE_DEBUG_SCENE_DELEGATE_ID, "/",
                      "Default usdImaging scene delegate id");

TF_DEFINE_ENV_SETTING(USDIMAGINGGL_ENGINE_ENABLE_SCENE_INDEX, false,
                      "Use Scene Index API for imaging scene input");

namespace {

bool
_GetHydraEnabledEnvVar()
{
    // XXX: Note that we don't cache the result here.  This is primarily because
    // of the way usdview currently interacts with this setting.  This should
    // be cleaned up, and the new class hierarchy around UsdImagingGLEngine
    // makes it much easier to do so.
    return TfGetenv("HD_ENABLED", "1") == "1";
}

SdfPath const&
_GetUsdImagingDelegateId()
{
    static SdfPath const delegateId =
        SdfPath(TfGetEnvSetting(USDIMAGINGGL_ENGINE_DEBUG_SCENE_DELEGATE_ID));

    return delegateId;
}

bool
_GetUseSceneIndices()
{
    // Use UsdImagingStageSceneIndex for input if:
    // - USDIMAGINGGL_ENGINE_ENABLE_SCENE_INDEX is true (feature flag)
    // - HdRenderIndex has scene index emulation enabled (otherwise,
    //     AddInputScene won't work).
    static bool useSceneIndices =
        HdRenderIndex::IsSceneIndexEmulationEnabled() &&
        (TfGetEnvSetting(USDIMAGINGGL_ENGINE_ENABLE_SCENE_INDEX) == true);

    return useSceneIndices;
}


bool
_IsHydraEnabled()
{
    if (!_GetHydraEnabledEnvVar()) {
        return false;
    }
    
    // Check to see if we have a default plugin for the renderer
    TfToken defaultPlugin = 
        HdRendererPluginRegistry::GetInstance().GetDefaultPluginId();

    return !defaultPlugin.IsEmpty();
}

std::string
_GetPlatformDependentRendererDisplayName(HfPluginDesc const &pluginDescriptor)
{
#if defined(__APPLE__)
    // Rendering for Storm is delegated to Hgi. We override the
    // display name for macOS since the Hgi implementation for
    // macOS uses Metal instead of GL. Eventually, this should
    // properly delegate to using Hgi to determine the display
    // name for Storm.
    static const TfToken _stormRendererPluginName("HdStormRendererPlugin");
    if (pluginDescriptor.id == _stormRendererPluginName) {
        return "Metal";
    }
#endif
    return pluginDescriptor.displayName;
}

} // anonymous namespace

//----------------------------------------------------------------------------
// Global State
//----------------------------------------------------------------------------

/*static*/
bool
UsdImagingGLEngine::IsHydraEnabled()
{
    static bool isHydraEnabled = _IsHydraEnabled();
    return isHydraEnabled;
}

//----------------------------------------------------------------------------
// Construction
//----------------------------------------------------------------------------

UsdImagingGLEngine::UsdImagingGLEngine(const HdDriver& driver)
    : UsdImagingGLEngine(SdfPath::AbsoluteRootPath(),
            {},
            {},
            _GetUsdImagingDelegateId(),
            driver
        )
{
}

UsdImagingGLEngine::UsdImagingGLEngine(
    const SdfPath& rootPath,
    const SdfPathVector& excludedPaths,
    const SdfPathVector& invisedPaths,
    const SdfPath& sceneDelegateID,
    const HdDriver& driver)
    : _hgi()
    , _hgiDriver(driver)
    , _sceneDelegateId(sceneDelegateID)
    , _selTracker(std::make_shared<HdxSelectionTracker>())
    , _selectionColor(1.0f, 1.0f, 0.0f, 1.0f)
    , _rootPath(rootPath)
    , _excludedPrimPaths(excludedPaths)
    , _invisedPrimPaths(invisedPaths)
    , _isPopulated(false)
    , _sceneIndex(nullptr)
    , _sceneDelegate(nullptr)
{

    if (IsHydraEnabled()) {

        // _renderIndex, _taskController, and _sceneDelegate/_sceneIndex
        // are initialized by the plugin system.
        if (!SetRendererPlugin(_GetDefaultRendererPluginId())) {
            TF_CODING_ERROR("No renderer plugins found! "
                            "Check before creation.");
        }

    } else {

        // In the legacy implementation, both excluded paths and invised paths 
        // are treated the same way.
        SdfPathVector pathsToExclude = excludedPaths;
        pathsToExclude.insert(pathsToExclude.end(), 
            invisedPaths.begin(), invisedPaths.end());
        _legacyImpl =std::make_unique<UsdImagingGLLegacyEngine>(pathsToExclude);
    }
}

void
UsdImagingGLEngine::_DestroyHydraObjects()
{
    // Destroy objects in opposite order of construction.
    _engine = nullptr;
    _taskController = nullptr;
    if (_GetUseSceneIndices()) {
        if (_sceneIndex) {
            _renderIndex->RemoveSceneIndex(_sceneIndex);
            _sceneIndex = nullptr;
        }
    } else {
        _sceneDelegate = nullptr;
    }
    _renderIndex = nullptr;
    _renderDelegate = nullptr;
}

UsdImagingGLEngine::~UsdImagingGLEngine()
{
    TF_PY_ALLOW_THREADS_IN_SCOPE();

    _DestroyHydraObjects();
}

//----------------------------------------------------------------------------
// Rendering
//----------------------------------------------------------------------------

void
UsdImagingGLEngine::PrepareBatch(
    const UsdPrim& root, 
    const UsdImagingGLRenderParams& params)
{
    if (ARCH_UNLIKELY(_legacyImpl)) {
        return;
    }

    HD_TRACE_FUNCTION();

    if (_CanPrepare(root)) {
        if (!_isPopulated) {
            if (_GetUseSceneIndices()) {
                TF_VERIFY(_sceneIndex);
                _sceneIndex->SetStage(root.GetStage());
                _sceneIndex->Populate();

                // XXX(USD-7113): Add pruning based on _rootPath,
                // _excludedPrimPaths

                // XXX(USD-7114): Add draw mode support based on
                // params.enableUsdDrawModes.

                // XXX(USD-7115): Add invis overrides from _invisedPrimPaths.
            } else {
                TF_VERIFY(_sceneDelegate);
                _sceneDelegate->SetUsdDrawModesEnabled(
                        params.enableUsdDrawModes);
                _sceneDelegate->Populate(
                        root.GetStage()->GetPrimAtPath(_rootPath),
                        _excludedPrimPaths);
                _sceneDelegate->SetInvisedPrimPaths(_invisedPrimPaths);
            }
            _isPopulated = true;
        }

        _PreSetTime(params);

        // SetTime will only react if time actually changes.
        if (_GetUseSceneIndices()) {
            _sceneIndex->SetTime(params.frame);
        } else {
            _sceneDelegate->SetTime(params.frame);
        }

        _PostSetTime(params);
    }
}

void
UsdImagingGLEngine::_PrepareRender(const UsdImagingGLRenderParams& params)
{
    TF_VERIFY(_taskController);

    _taskController->SetFreeCameraClipPlanes(params.clipPlanes);

    TfTokenVector renderTags;
    _ComputeRenderTags(params, &renderTags);
    _taskController->SetRenderTags(renderTags);

    _taskController->SetRenderParams(
        _MakeHydraUsdImagingGLRenderParams(params));

    // Forward scene materials enable option.
    if (_GetUseSceneIndices()) {
        // XXX(USD-7116): params.enableSceneMaterials, params.enableSceneLights
    } else {
        _sceneDelegate->SetSceneMaterialsEnabled(params.enableSceneMaterials);
        _sceneDelegate->SetSceneLightsEnabled(params.enableSceneLights);
    }
}

void
UsdImagingGLEngine::RenderBatch(
    const SdfPathVector& paths, 
    const UsdImagingGLRenderParams& params)
{
    if (ARCH_UNLIKELY(_legacyImpl)) {
        return;
    }

    TF_VERIFY(_taskController);

    _UpdateHydraCollection(&_renderCollection, paths, params);
    _taskController->SetCollection(_renderCollection);

    _PrepareRender(params);

    SetColorCorrectionSettings(params.colorCorrectionMode, params.ocioDisplay,
        params.ocioView, params.ocioColorSpace, params.ocioLook);

    // XXX App sets the clear color via 'params' instead of setting up Aovs 
    // that has clearColor in their descriptor. So for now we must pass this
    // clear color to the color AOV.
    HdAovDescriptor colorAovDesc = 
        _taskController->GetRenderOutputSettings(HdAovTokens->color);
    if (colorAovDesc.format != HdFormatInvalid) {
        colorAovDesc.clearValue = VtValue(params.clearColor);
        _taskController->SetRenderOutputSettings(
            HdAovTokens->color, colorAovDesc);
    }

    _taskController->SetEnableSelection(params.highlight);
    VtValue selectionValue(_selTracker);
    _engine->SetTaskContextData(HdxTokens->selectionState, selectionValue);
    _Execute(params, _taskController->GetRenderingTasks());
}

void 
UsdImagingGLEngine::Render(
    const UsdPrim& root, 
    const UsdImagingGLRenderParams &params)
{
    if (ARCH_UNLIKELY(_legacyImpl)) {
        return _legacyImpl->Render(root, params);
    }

    TF_VERIFY(_taskController);

    PrepareBatch(root, params);

    // XXX(UsdImagingPaths): This bit is weird: we get the stage from "root",
    // gate population by _rootPath (which may be different), and then pass
    // root.GetPath() to hydra as the root to draw from. Note that this
    // produces incorrect results in UsdImagingDelegate for native instancing.
    const SdfPathVector paths = {
        root.GetPath().ReplacePrefix(
            SdfPath::AbsoluteRootPath(), _sceneDelegateId)
    };

    RenderBatch(paths, params);
}

bool
UsdImagingGLEngine::IsConverged() const
{
    if (ARCH_UNLIKELY(_legacyImpl)) {
        return true;
    }

    TF_VERIFY(_taskController);
    return _taskController->IsConverged();
}

//----------------------------------------------------------------------------
// Root and Transform Visibility
//----------------------------------------------------------------------------

void
UsdImagingGLEngine::SetRootTransform(GfMatrix4d const& xf)
{
    if (ARCH_UNLIKELY(_legacyImpl)) {
        return;
    }

    if (_GetUseSceneIndices()) {
        // XXX(USD-7115): root transform
    } else {
        TF_VERIFY(_sceneDelegate);
        _sceneDelegate->SetRootTransform(xf);
    }
}

void
UsdImagingGLEngine::SetRootVisibility(bool isVisible)
{
    if (ARCH_UNLIKELY(_legacyImpl)) {
        return;
    }

    if (_GetUseSceneIndices()) {
        // XXX(USD-7115): root visibility
    } else {
        TF_VERIFY(_sceneDelegate);
        _sceneDelegate->SetRootVisibility(isVisible);
    }
}

//----------------------------------------------------------------------------
// Camera and Light State
//----------------------------------------------------------------------------

void
UsdImagingGLEngine::SetRenderViewport(GfVec4d const& viewport)
{
    if (ARCH_UNLIKELY(_legacyImpl)) {
        _legacyImpl->SetRenderViewport(viewport);
        return;
    }

    TF_VERIFY(_taskController);
    _taskController->SetRenderViewport(viewport);
}

void
UsdImagingGLEngine::SetFraming(CameraUtilFraming const& framing)
{
    if (ARCH_UNLIKELY(_legacyImpl)) {
        // legacy implementation does not support camera framing.
        return;
    }

    if (TF_VERIFY(_taskController)) {
        _taskController->SetFraming(framing);
    }
}

void
UsdImagingGLEngine::SetOverrideWindowPolicy(
    const std::pair<bool, CameraUtilConformWindowPolicy> &policy)
{
    if (ARCH_UNLIKELY(_legacyImpl)) {
        // legacy implementation does not support camera framing.
        return;
    }

    if (TF_VERIFY(_taskController)) {
        _taskController->SetOverrideWindowPolicy(policy);
    }
}

void
UsdImagingGLEngine::SetRenderBufferSize(GfVec2i const& size)
{
    if (ARCH_UNLIKELY(_legacyImpl)) {
        // legacy implementation does not support camera framing.
        return;
    }

    if (TF_VERIFY(_taskController)) {
        _taskController->SetRenderBufferSize(size);
    }
}

void
UsdImagingGLEngine::SetWindowPolicy(CameraUtilConformWindowPolicy policy)
{
    if (ARCH_UNLIKELY(_legacyImpl)) {
        _legacyImpl->SetWindowPolicy(policy);
        return;
    }

    TF_VERIFY(_taskController);
    // Note: Free cam uses SetCameraState, which expects the frustum to be
    // pre-adjusted for the viewport size.

    if (_GetUseSceneIndices()) {
        // XXX(USD-7115): window policy
    } else {
        // The usdImagingDelegate manages the window policy for scene cameras.
        _sceneDelegate->SetWindowPolicy(policy);
    }
}

void
UsdImagingGLEngine::SetCameraPath(SdfPath const& id)
{
    if (ARCH_UNLIKELY(_legacyImpl)) {
        _legacyImpl->SetCameraPath(id);
        return;
    }

    TF_VERIFY(_taskController);
    _taskController->SetCameraPath(id);

    // The camera that is set for viewing will also be used for
    // time sampling.
    // XXX(HYD-2304): motion blur shutter window.
    if (!_GetUseSceneIndices()) {
        _sceneDelegate->SetCameraForSampling(id);
    }
}

void 
UsdImagingGLEngine::SetCameraState(const GfMatrix4d& viewMatrix,
                                   const GfMatrix4d& projectionMatrix)
{
    if (ARCH_UNLIKELY(_legacyImpl)) {
        _legacyImpl->SetFreeCameraMatrices(viewMatrix, projectionMatrix);
        return;
    }

    TF_VERIFY(_taskController);
    _taskController->SetFreeCameraMatrices(viewMatrix, projectionMatrix);
}

void
UsdImagingGLEngine::SetLightingState(GlfSimpleLightingContextPtr const &src)
{
    if (ARCH_UNLIKELY(_legacyImpl)) {
        return;
    }

    TF_VERIFY(_taskController);
    _taskController->SetLightingState(src);
}

void
UsdImagingGLEngine::SetLightingState(
    GlfSimpleLightVector const &lights,
    GlfSimpleMaterial const &material,
    GfVec4f const &sceneAmbient)
{
    if (ARCH_UNLIKELY(_legacyImpl)) {
        _legacyImpl->SetLightingState(lights, material, sceneAmbient);
        return;
    }

    TF_VERIFY(_taskController);

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

//----------------------------------------------------------------------------
// Selection Highlighting
//----------------------------------------------------------------------------

void
UsdImagingGLEngine::SetSelected(SdfPathVector const& paths)
{
    if (ARCH_UNLIKELY(_legacyImpl)) {
        return;
    }

    if (_GetUseSceneIndices()) {
        // XXX(HYD-2299): selection support
        return;
    }

    TF_VERIFY(_sceneDelegate);

    // populate new selection
    HdSelectionSharedPtr const selection = std::make_shared<HdSelection>();
    // XXX: Usdview currently supports selection on click. If we extend to
    // rollover (locate) selection, we need to pass that mode here.
    static const HdSelection::HighlightMode mode =
        HdSelection::HighlightModeSelect;
    for (SdfPath const& path : paths) {
        _sceneDelegate->PopulateSelection(mode,
                                          path,
                                          UsdImagingDelegate::ALL_INSTANCES,
                                          selection);
    }

    // set the result back to selection tracker
    _selTracker->SetSelection(selection);
}

void
UsdImagingGLEngine::ClearSelected()
{
    if (ARCH_UNLIKELY(_legacyImpl)) {
        return;
    }

    TF_VERIFY(_selTracker);

    _selTracker->SetSelection(std::make_shared<HdSelection>());
}

HdSelectionSharedPtr
UsdImagingGLEngine::_GetSelection() const
{
    if (HdSelectionSharedPtr const selection = _selTracker->GetSelectionMap()) {
        return selection;
    }

    return std::make_shared<HdSelection>();
}

void
UsdImagingGLEngine::AddSelected(SdfPath const &path, int instanceIndex)
{
    if (ARCH_UNLIKELY(_legacyImpl)) {
        return;
    }

    if (_GetUseSceneIndices()) {
        // XXX(HYD-2299): selection support
        return;
    }

    TF_VERIFY(_sceneDelegate);

    HdSelectionSharedPtr const selection = _GetSelection();

    // XXX: Usdview currently supports selection on click. If we extend to
    // rollover (locate) selection, we need to pass that mode here.
    static const HdSelection::HighlightMode mode =
        HdSelection::HighlightModeSelect;
    _sceneDelegate->PopulateSelection(mode, path, instanceIndex, selection);

    // set the result back to selection tracker
    _selTracker->SetSelection(selection);
}

void
UsdImagingGLEngine::SetSelectionColor(GfVec4f const& color)
{
    if (ARCH_UNLIKELY(_legacyImpl)) {
        return;
    }

    TF_VERIFY(_taskController);

    _selectionColor = color;
    _taskController->SetSelectionColor(_selectionColor);
}

//----------------------------------------------------------------------------
// Picking
//----------------------------------------------------------------------------

bool 
UsdImagingGLEngine::TestIntersection(
    const GfMatrix4d &viewMatrix,
    const GfMatrix4d &projectionMatrix,
    const UsdPrim& root,
    const UsdImagingGLRenderParams& params,
    GfVec3d *outHitPoint,
    GfVec3d *outHitNormal,
    SdfPath *outHitPrimPath,
    SdfPath *outHitInstancerPath,
    int *outHitInstanceIndex,
    HdInstancerContext *outInstancerContext)
{
    if (ARCH_UNLIKELY(_legacyImpl)) {
        return _legacyImpl->TestIntersection(
            viewMatrix,
            projectionMatrix,
            root,
            params,
            outHitPoint,
            outHitPrimPath,
            outHitInstancerPath,
            outHitInstanceIndex);
    }

    if (_GetUseSceneIndices()) {
        // XXX(HYD-2299): picking support
        return false;
    }

    TF_VERIFY(_sceneDelegate);
    TF_VERIFY(_taskController);

    PrepareBatch(root, params);

    // XXX(UsdImagingPaths): This is incorrect...  "Root" points to a USD
    // subtree, but the subtree in the hydra namespace might be very different
    // (e.g. for native instancing).  We need a translation step.
    const SdfPathVector paths = {
        root.GetPath().ReplacePrefix(
            SdfPath::AbsoluteRootPath(), _sceneDelegateId)
    };
    _UpdateHydraCollection(&_intersectCollection, paths, params);

    _PrepareRender(params);

    HdxPickHitVector allHits;
    HdxPickTaskContextParams pickParams;
    pickParams.resolveMode = HdxPickTokens->resolveNearestToCenter;
    pickParams.viewMatrix = viewMatrix;
    pickParams.projectionMatrix = projectionMatrix;
    pickParams.clipPlanes = params.clipPlanes;
    pickParams.collection = _intersectCollection;
    pickParams.outHits = &allHits;
    const VtValue vtPickParams(pickParams);

    _engine->SetTaskContextData(HdxPickTokens->pickParams, vtPickParams);
    _Execute(params, _taskController->GetPickingTasks());

    // Since we are in nearest-hit mode, we expect allHits to have
    // a single point in it.
    if (allHits.size() != 1) {
        return false;
    }

    HdxPickHit &hit = allHits[0];

    if (outHitPoint) {
        *outHitPoint = hit.worldSpaceHitPoint;
    }

    if (outHitNormal) {
        *outHitNormal = hit.worldSpaceHitNormal;
    }

    hit.objectId = _sceneDelegate->GetScenePrimPath(
        hit.objectId, hit.instanceIndex, outInstancerContext);
    hit.instancerId = _sceneDelegate->ConvertIndexPathToCachePath(
        hit.instancerId).GetAbsoluteRootOrPrimPath();

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
UsdImagingGLEngine::DecodeIntersection(
    unsigned char const primIdColor[4],
    unsigned char const instanceIdColor[4],
    SdfPath *outHitPrimPath,
    SdfPath *outHitInstancerPath,
    int *outHitInstanceIndex,
    HdInstancerContext *outInstancerContext)
{
    if (ARCH_UNLIKELY(_legacyImpl)) {
        return false;
    }

    if (_GetUseSceneIndices()) {
        // XXX(HYD-2299): picking
        return false;
    }

    TF_VERIFY(_sceneDelegate);

    const int primId = HdxPickTask::DecodeIDRenderColor(primIdColor);
    const int instanceIdx = HdxPickTask::DecodeIDRenderColor(instanceIdColor);
    SdfPath primPath =
        _sceneDelegate->GetRenderIndex().GetRprimPathFromPrimId(primId);

    if (primPath.IsEmpty()) {
        return false;
    }

    SdfPath delegateId, instancerId;
    _sceneDelegate->GetRenderIndex().GetSceneDelegateAndInstancerIds(
        primPath, &delegateId, &instancerId);

    primPath = _sceneDelegate->GetScenePrimPath(
        primPath, instanceIdx, outInstancerContext);
    instancerId = _sceneDelegate->ConvertIndexPathToCachePath(instancerId)
                  .GetAbsoluteRootOrPrimPath();

    if (outHitPrimPath) {
        *outHitPrimPath = primPath;
    }
    if (outHitInstancerPath) {
        *outHitInstancerPath = instancerId;
    }
    if (outHitInstanceIndex) {
        *outHitInstanceIndex = instanceIdx;
    }

    return true;
}

//----------------------------------------------------------------------------
// Renderer Plugin Management
//----------------------------------------------------------------------------

/* static */
TfTokenVector
UsdImagingGLEngine::GetRendererPlugins()
{
    if (ARCH_UNLIKELY(!_GetHydraEnabledEnvVar())) {
        // No plugins if the legacy implementation is active.
        return std::vector<TfToken>();
    }

    HfPluginDescVector pluginDescriptors;
    HdRendererPluginRegistry::GetInstance().GetPluginDescs(&pluginDescriptors);

    TfTokenVector plugins;
    for(size_t i = 0; i < pluginDescriptors.size(); ++i) {
        plugins.push_back(pluginDescriptors[i].id);
    }
    return plugins;
}

/* static */
std::string
UsdImagingGLEngine::GetRendererDisplayName(TfToken const &id)
{
    if (ARCH_UNLIKELY(!_GetHydraEnabledEnvVar() || id.IsEmpty())) {
        // No renderer name is returned if the user requested to disable Hydra, 
        // or if the machine does not support any of the available renderers 
        // and it automatically switches to our legacy engine.
        return std::string();
    }

    HfPluginDesc pluginDescriptor;
    if (!TF_VERIFY(HdRendererPluginRegistry::GetInstance().
                   GetPluginDesc(id, &pluginDescriptor))) {
        return std::string();
    }

    return _GetPlatformDependentRendererDisplayName(pluginDescriptor);
}

TfToken
UsdImagingGLEngine::GetCurrentRendererId() const
{
    if (ARCH_UNLIKELY(_legacyImpl)) {
        // No renderer support if the legacy implementation is active.
        return TfToken();
    }

    return _renderDelegate.GetPluginId();
}

void
UsdImagingGLEngine::_InitializeHgiIfNecessary()
{
    // If the client of UsdImagingGLEngine does not provide a HdDriver, we
    // construct a default one that is owned by UsdImagingGLEngine.
    // The cleanest pattern is for the client app to provide this since you
    // may have multiple UsdImagingGLEngines in one app that ideally all use
    // the same HdDriver and Hgi to share GPU resources.
    if (_hgiDriver.driver.IsEmpty()) {
        _hgi = Hgi::CreatePlatformDefaultHgi();
        _hgiDriver.name = HgiTokens->renderDriver;
        _hgiDriver.driver = VtValue(_hgi.get());
    }
}

bool
UsdImagingGLEngine::SetRendererPlugin(TfToken const &id)
{
    if (ARCH_UNLIKELY(_legacyImpl)) {
        return false;
    }

    _InitializeHgiIfNecessary();

    HdRendererPluginRegistry &registry =
        HdRendererPluginRegistry::GetInstance();

    // Special case: id = TfToken() selects the first plugin in the list.
    const TfToken resolvedId =
        id.IsEmpty() ? registry.GetDefaultPluginId() : id;

    if ( _renderDelegate && _renderDelegate.GetPluginId() == resolvedId) {
        return true;
    }

    TF_PY_ALLOW_THREADS_IN_SCOPE();

    HdPluginRenderDelegateUniqueHandle renderDelegate =
        registry.CreateRenderDelegate(resolvedId);
    if(!renderDelegate) {
        return false;
    }

    _SetRenderDelegateAndRestoreState(std::move(renderDelegate));

    return true;
}

void
UsdImagingGLEngine::_SetRenderDelegateAndRestoreState(
    HdPluginRenderDelegateUniqueHandle &&renderDelegate)
{
    // Pull old scene/task controller state. Note that the scene index/delegate
    // may not have been created, if this is the first time through this
    // function, so we guard for null and use default values for xform/vis.
    GfMatrix4d rootTransform = GfMatrix4d(1.0);
    bool rootVisibility = true;

    if (_GetUseSceneIndices()) {
        // XXX(USD-7115): root transform, visibility...
    } else {
        if (_sceneDelegate) {
            rootTransform = _sceneDelegate->GetRootTransform();
            rootVisibility = _sceneDelegate->GetRootVisibility();
        }
    }

    HdSelectionSharedPtr const selection = _GetSelection();

    // Rebuild the imaging stack
    _SetRenderDelegate(std::move(renderDelegate));

    // Reload saved state.
    if (_GetUseSceneIndices()) {
        // XXX(USD-7115): root transform, visibility...
    } else {
        _sceneDelegate->SetRootVisibility(rootVisibility);
        _sceneDelegate->SetRootTransform(rootTransform);
    }
    _selTracker->SetSelection(selection);
    _taskController->SetSelectionColor(_selectionColor);
}

SdfPath
UsdImagingGLEngine::_ComputeControllerPath(
    const HdPluginRenderDelegateUniqueHandle &renderDelegate)
{
    const std::string pluginId =
        TfMakeValidIdentifier(renderDelegate.GetPluginId().GetText());
    const TfToken rendererName(
        TfStringPrintf("_UsdImaging_%s_%p", pluginId.c_str(), this));

    return _sceneDelegateId.AppendChild(rendererName);
}

void
UsdImagingGLEngine::_SetRenderDelegate(
    HdPluginRenderDelegateUniqueHandle &&renderDelegate)
{
    // This relies on SetRendererPlugin to release the GIL...

    // Destruction
    _DestroyHydraObjects();

    _isPopulated = false;

    // Creation

    // Use the new render delegate.
    _renderDelegate = std::move(renderDelegate);

    // Recreate the render index
    _renderIndex.reset(
        HdRenderIndex::New(
            _renderDelegate.Get(), {&_hgiDriver}));

    // Create the new scene API
    if (_GetUseSceneIndices()) {
        _sceneIndex = UsdImagingStageSceneIndex::New();
        _renderIndex->InsertSceneIndex(
            UsdImagingGLDrawModeSceneIndex::New(
                HdFlatteningSceneIndex::New(_sceneIndex),
                /* inputArgs = */ nullptr),
            _sceneDelegateId);
    } else {
        _sceneDelegate = std::make_unique<UsdImagingDelegate>(
                _renderIndex.get(), _sceneDelegateId);
    }

    // Create the new task controller
    _taskController = std::make_unique<HdxTaskController>(
        _renderIndex.get(),
        _ComputeControllerPath(_renderDelegate));

    // The task context holds on to resources in the render
    // deletegate, so we want to destroy it first and thus
    // create it last.
    _engine = std::make_unique<HdEngine>();
}

//----------------------------------------------------------------------------
// AOVs and Renderer Settings
//----------------------------------------------------------------------------

TfTokenVector
UsdImagingGLEngine::GetRendererAovs() const
{
    if (ARCH_UNLIKELY(_legacyImpl)) {
        return std::vector<TfToken>();
    }

    TF_VERIFY(_renderIndex);

    if (_renderIndex->IsBprimTypeSupported(HdPrimTypeTokens->renderBuffer)) {

        static const TfToken candidates[] =
            { HdAovTokens->primId,
              HdAovTokens->depth,
              HdAovTokens->normal,
              HdAovTokensMakePrimvar(TfToken("st")) };

        TfTokenVector aovs = { HdAovTokens->color };
        for (auto const& aov : candidates) {
            if (_renderDelegate->GetDefaultAovDescriptor(aov).format 
                    != HdFormatInvalid) {
                aovs.push_back(aov);
            }
        }
        return aovs;
    }
    return TfTokenVector();
}

bool
UsdImagingGLEngine::SetRendererAov(TfToken const &id)
{
    if (ARCH_UNLIKELY(_legacyImpl)) {
        return false;
    }

    TF_VERIFY(_renderIndex);
    if (_renderIndex->IsBprimTypeSupported(HdPrimTypeTokens->renderBuffer)) {
        _taskController->SetRenderOutputs({id});
        return true;
    }
    return false;
}

HgiTextureHandle
UsdImagingGLEngine::GetAovTexture(
    TfToken const& name) const
{
    VtValue aov;
    HgiTextureHandle aovTexture;

    if (_engine->GetTaskContextData(name, &aov)) {
        if (aov.IsHolding<HgiTextureHandle>()) {
            aovTexture = aov.Get<HgiTextureHandle>();
        }
    }

    return aovTexture;
}

HdRenderBuffer*
UsdImagingGLEngine::GetAovRenderBuffer(TfToken const& name) const
{
    return _taskController->GetRenderOutput(name);
}

UsdImagingGLRendererSettingsList
UsdImagingGLEngine::GetRendererSettingsList() const
{
    if (ARCH_UNLIKELY(_legacyImpl)) {
        return UsdImagingGLRendererSettingsList();
    }

    TF_VERIFY(_renderDelegate);

    const HdRenderSettingDescriptorList descriptors =
        _renderDelegate->GetRenderSettingDescriptors();
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

VtValue
UsdImagingGLEngine::GetRendererSetting(TfToken const& id) const
{
    if (ARCH_UNLIKELY(_legacyImpl)) {
        return VtValue();
    }

    TF_VERIFY(_renderDelegate);
    return _renderDelegate->GetRenderSetting(id);
}

void
UsdImagingGLEngine::SetRendererSetting(TfToken const& id, VtValue const& value)
{
    if (ARCH_UNLIKELY(_legacyImpl)) {
        return;
    }

    TF_VERIFY(_renderDelegate);
    _renderDelegate->SetRenderSetting(id, value);
}

void
UsdImagingGLEngine::SetEnablePresentation(bool enabled)
{
    if (ARCH_UNLIKELY(_legacyImpl)) {
        return;
    }

    if (TF_VERIFY(_taskController)) {
        _taskController->SetEnablePresentation(enabled);
    }
}

void
UsdImagingGLEngine::SetPresentationOutput(
    TfToken const &api,
    VtValue const &framebuffer)
{
    if (ARCH_UNLIKELY(_legacyImpl)) {
        return;
    }

    if (TF_VERIFY(_taskController)) {
        _userFramebuffer = framebuffer;
        _taskController->SetPresentationOutput(api, framebuffer);
    }
}

// ---------------------------------------------------------------------
// Command API
// ---------------------------------------------------------------------

HdCommandDescriptors 
UsdImagingGLEngine::GetRendererCommandDescriptors() const
{
    if (ARCH_UNLIKELY(_legacyImpl)) {
        return HdCommandDescriptors();
    }

    if (ARCH_UNLIKELY(!_renderDelegate)) {
        return HdCommandDescriptors();
    }

    return _renderDelegate->GetCommandDescriptors();
}

bool
UsdImagingGLEngine::InvokeRendererCommand(
    const TfToken &command, const HdCommandArgs &args) const
{
    if (ARCH_UNLIKELY(_legacyImpl)) {
        return false;
    }

    if (ARCH_UNLIKELY(!_renderDelegate)) {
        return false;
    }

    return _renderDelegate->InvokeCommand(command, args);
}

// ---------------------------------------------------------------------
// Control of background rendering threads.
// ---------------------------------------------------------------------
bool
UsdImagingGLEngine::IsPauseRendererSupported() const
{
    if (ARCH_UNLIKELY(_legacyImpl)) {
        return false;
    }

    TF_VERIFY(_renderDelegate);
    return _renderDelegate->IsPauseSupported();
}

bool
UsdImagingGLEngine::PauseRenderer()
{
    if (ARCH_UNLIKELY(_legacyImpl)) {
        return false;
    }

    TF_PY_ALLOW_THREADS_IN_SCOPE();

    TF_VERIFY(_renderDelegate);
    return _renderDelegate->Pause();
}

bool
UsdImagingGLEngine::ResumeRenderer()
{
    if (ARCH_UNLIKELY(_legacyImpl)) {
        return false;
    }

    TF_PY_ALLOW_THREADS_IN_SCOPE();

    TF_VERIFY(_renderDelegate);
    return _renderDelegate->Resume();
}

bool
UsdImagingGLEngine::IsStopRendererSupported() const
{
    if (ARCH_UNLIKELY(_legacyImpl)) {
        return false;
    }

    TF_VERIFY(_renderDelegate);
    return _renderDelegate->IsStopSupported();
}

bool
UsdImagingGLEngine::StopRenderer()
{
    if (ARCH_UNLIKELY(_legacyImpl)) {
        return false;
    }

    TF_PY_ALLOW_THREADS_IN_SCOPE();

    TF_VERIFY(_renderDelegate);
    return _renderDelegate->Stop();
}

bool
UsdImagingGLEngine::RestartRenderer()
{
    if (ARCH_UNLIKELY(_legacyImpl)) {
        return false;
    }

    TF_PY_ALLOW_THREADS_IN_SCOPE();

    TF_VERIFY(_renderDelegate);
    return _renderDelegate->Restart();
}

//----------------------------------------------------------------------------
// Color Correction
//----------------------------------------------------------------------------
void 
UsdImagingGLEngine::SetColorCorrectionSettings(
    TfToken const& colorCorrectionMode,
    TfToken const& ocioDisplay,
    TfToken const& ocioView,
    TfToken const& ocioColorSpace,
    TfToken const& ocioLook)
{
    if (ARCH_UNLIKELY(_legacyImpl)) {
        return;
    }

    if (!IsColorCorrectionCapable()) {
        return;
    }

    TF_VERIFY(_taskController);

    HdxColorCorrectionTaskParams hdParams;
    hdParams.colorCorrectionMode = colorCorrectionMode;
    hdParams.displayOCIO = ocioDisplay.GetString();
    hdParams.viewOCIO = ocioView.GetString();
    hdParams.colorspaceOCIO = ocioColorSpace.GetString();
    hdParams.looksOCIO = ocioLook.GetString();
    _taskController->SetColorCorrectionParams(hdParams);
}

bool
UsdImagingGLEngine::IsColorCorrectionCapable()
{
    return true;
}

//----------------------------------------------------------------------------
// Resource Information
//----------------------------------------------------------------------------

VtDictionary
UsdImagingGLEngine::GetRenderStats() const
{
    if (ARCH_UNLIKELY(_legacyImpl)) {
        return VtDictionary();
    }

    TF_VERIFY(_renderDelegate);
    return _renderDelegate->GetRenderStats();
}

Hgi*
UsdImagingGLEngine::GetHgi()
{
    return _hgi.get();
}

//----------------------------------------------------------------------------
// Private/Protected
//----------------------------------------------------------------------------

HdRenderIndex *
UsdImagingGLEngine::_GetRenderIndex() const
{
    if (ARCH_UNLIKELY(_legacyImpl)) {
        return nullptr;
    }

    return _renderIndex.get();
}

void 
UsdImagingGLEngine::_Execute(const UsdImagingGLRenderParams &params,
                             HdTaskSharedPtrVector tasks)
{
    if (ARCH_UNLIKELY(_legacyImpl)) {
        return;
    }

    {
        // Release the GIL before calling into hydra, in case any hydra plugins
        // call into python.
        TF_PY_ALLOW_THREADS_IN_SCOPE();
        _engine->Execute(_renderIndex.get(), &tasks);
    }
}

bool 
UsdImagingGLEngine::_CanPrepare(const UsdPrim& root)
{
    HD_TRACE_FUNCTION();

    if (!TF_VERIFY(root, "Attempting to draw an invalid/null prim\n")) 
        return false;

    if (!root.GetPath().HasPrefix(_rootPath)) {
        TF_CODING_ERROR("Attempting to draw path <%s>, but engine is rooted"
                    "at <%s>\n",
                    root.GetPath().GetText(),
                    _rootPath.GetText());
        return false;
    }

    return true;
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

void
UsdImagingGLEngine::_PreSetTime(const UsdImagingGLRenderParams& params)
{
    HD_TRACE_FUNCTION();

    const int refineLevel = _GetRefineLevel(params.complexity);

    if (_GetUseSceneIndices()) {
        // XXX(USD-7115): fallback refine level
        _sceneIndex->ApplyPendingUpdates();
    } else {
        // Set the fallback refine level; if this changes from the
        // existing value, all prim refine levels will be dirtied.
        _sceneDelegate->SetRefineLevelFallback(refineLevel);

        // Apply any queued up scene edits.
        _sceneDelegate->ApplyPendingUpdates();
    }
}

void
UsdImagingGLEngine::_PostSetTime(const UsdImagingGLRenderParams& params)
{
    HD_TRACE_FUNCTION();
}


/* static */
bool
UsdImagingGLEngine::_UpdateHydraCollection(
    HdRprimCollection *collection,
    SdfPathVector const& roots,
    UsdImagingGLRenderParams const& params)
{
    if (collection == nullptr) {
        TF_CODING_ERROR("Null passed to _UpdateHydraCollection");
        return false;
    }

    // choose repr
    HdReprSelector reprSelector = HdReprSelector(HdReprTokens->smoothHull);
    const bool refined = params.complexity > 1.0;
    
    if (params.drawMode == UsdImagingGLDrawMode::DRAW_POINTS) {
        reprSelector = HdReprSelector(HdReprTokens->points);
    } else if (params.drawMode == UsdImagingGLDrawMode::DRAW_GEOM_FLAT ||
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

    // By default our main collection will be called geometry
    const TfToken colName = HdTokens->geometry;

    // Check if the collection needs to be updated (so we can avoid the sort).
    SdfPathVector const& oldRoots = collection->GetRootPaths();

    // inexpensive comparison first
    bool match = collection->GetName() == colName &&
                 oldRoots.size() == roots.size() &&
                 collection->GetReprSelector() == reprSelector;

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

        // if everything matches, do nothing.
        if (match) return false;
    }

    // Recreate the collection.
    *collection = HdRprimCollection(colName, reprSelector);
    collection->SetRootPaths(roots);

    return true;
}

/* static */
HdxRenderTaskParams
UsdImagingGLEngine::_MakeHydraUsdImagingGLRenderParams(
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
    params.enableSceneLights = renderParams.enableSceneLights;

    // We don't provide the following because task controller ignores them:
    // - params.camera
    // - params.viewport

    return params;
}

//static
void
UsdImagingGLEngine::_ComputeRenderTags(UsdImagingGLRenderParams const& params,
                                       TfTokenVector *renderTags)
{
    // Calculate the rendertags needed based on the parameters passed by
    // the application
    renderTags->clear();
    renderTags->reserve(4);
    renderTags->push_back(HdRenderTagTokens->geometry);
    if (params.showGuides) {
        renderTags->push_back(HdRenderTagTokens->guide);
    }
    if (params.showProxy) {
        renderTags->push_back(HdRenderTagTokens->proxy);
    }
    if (params.showRender) {
        renderTags->push_back(HdRenderTagTokens->render);
    }
}

/* static */
TfToken
UsdImagingGLEngine::_GetDefaultRendererPluginId()
{
    static const std::string defaultRendererDisplayName = 
        TfGetenv("HD_DEFAULT_RENDERER", "");

    if (defaultRendererDisplayName.empty()) {
        return TfToken();
    }

    HfPluginDescVector pluginDescs;
    HdRendererPluginRegistry::GetInstance().GetPluginDescs(&pluginDescs);

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

UsdImagingDelegate *
UsdImagingGLEngine::_GetSceneDelegate() const
{
    if (_GetUseSceneIndices()) {
        // XXX(USD-7118): this API needs to be removed for full
        // scene index support.
        TF_CODING_ERROR("_GetSceneDelegate API is unsupported");
        return nullptr;
    } else {
        return _sceneDelegate.get();
    }
}

HdEngine *
UsdImagingGLEngine::_GetHdEngine()
{
    return _engine.get();
}

HdxTaskController *
UsdImagingGLEngine::_GetTaskController() const
{
    return _taskController.get();
}

bool
UsdImagingGLEngine::_IsUsingLegacyImpl() const
{
    return bool(_legacyImpl);
}

PXR_NAMESPACE_CLOSE_SCOPE

