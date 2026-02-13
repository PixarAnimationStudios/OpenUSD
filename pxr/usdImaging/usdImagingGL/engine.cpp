//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/usdImaging/usdImagingGL/engine.h"

#include "pxr/usdImaging/usdImaging/delegate.h"
#include "pxr/usdImaging/usdImaging/selectionSceneIndex.h"
#include "pxr/usdImaging/usdImaging/stageSceneIndex.h"
#include "pxr/usdImaging/usdImaging/rootOverridesSceneIndex.h"
#include "pxr/usdImaging/usdImaging/sceneIndices.h"
#include "pxr/usdImaging/usdImaging/usdSceneIndexInputArgsSchema.h"

#include "pxr/usd/usdGeom/tokens.h"
#include "pxr/usd/usdGeom/camera.h"
#include "pxr/usd/usdRender/tokens.h"
#include "pxr/usd/usdRender/settings.h"

#include "pxr/imaging/hd/cachingSceneIndex.h"
#include "pxr/imaging/hd/instancedBySchema.h"
#include "pxr/imaging/hd/materialBindingsSchema.h"
#include "pxr/imaging/hd/legacyRenderControlInterface.h"
#include "pxr/imaging/hd/light.h"
#include "pxr/imaging/hd/noticeBatchingSceneIndex.h"
#include "pxr/imaging/hd/overlayContainerDataSource.h"
#include "pxr/imaging/hd/prefixingSceneIndex.h"
#include "pxr/imaging/hd/renderBuffer.h"
#include "pxr/imaging/hd/renderDelegateInfo.h"
#include "pxr/imaging/hd/renderIndexAdapterSceneIndex.h"
#include "pxr/imaging/hd/renderer.h"
#include "pxr/imaging/hd/rendererCreateArgsSchema.h"
#include "pxr/imaging/hd/rendererPlugin.h"
#include "pxr/imaging/hd/rendererPluginRegistry.h"
#include "pxr/imaging/hd/retainedDataSource.h"
#include "pxr/imaging/hd/sceneIndexPluginRegistry.h"
#include "pxr/imaging/hd/systemMessages.h"
#include "pxr/imaging/hd/utils.h"
#include "pxr/imaging/hdsi/domeLightCameraVisibilitySceneIndex.h"
#include "pxr/imaging/hdsi/legacyDisplayStyleOverrideSceneIndex.h"
#include "pxr/imaging/hdsi/prefixPathPruningSceneIndex.h"
#include "pxr/imaging/hdsi/primTypeAndPathPruningSceneIndex.h"
#include "pxr/imaging/hdsi/sceneMaterialPruningSceneIndex.h"
#include "pxr/imaging/hdsi/sceneGlobalsSceneIndex.h"
#include "pxr/imaging/hdx/pickTask.h"
#include "pxr/imaging/hdx/task.h"
#include "pxr/imaging/hdx/taskController.h"
#include "pxr/imaging/hdx/taskControllerSceneIndex.h"
#include "pxr/imaging/hdx/tokens.h"

#include "pxr/imaging/hgi/hgi.h"
#include "pxr/imaging/hgi/tokens.h"

#include "pxr/base/tf/envSetting.h"
#include "pxr/base/tf/getenv.h"
#include "pxr/base/tf/staticData.h"
#include "pxr/base/tf/stl.h"

#include "pxr/base/gf/matrix4d.h"
#include "pxr/base/gf/vec3d.h"

#include <string>

PXR_NAMESPACE_OPEN_SCOPE

TF_DEFINE_ENV_SETTING(USDIMAGINGGL_ENGINE_DEBUG_SCENE_DELEGATE_ID, "/",
                      "Default usdImaging scene delegate id");

TF_DEFINE_ENV_SETTING(USDIMAGINGGL_ENGINE_ENABLE_SCENE_INDEX, true,
                      "Use Scene Index API for imaging scene input");

TF_DEFINE_ENV_SETTING(
    USDIMAGINGGL_ENGINE_ENABLE_SCENE_INDEX_DEPRECATION_WARNING, true,
    "Issue a deprecation warning when USDIMAGINGGL_ENGINE_ENABLE_SCENE_INDEX "
    "is overriden to false.");

/// \deprecated. Will always use task controller scene index in the future.
TF_DEFINE_ENV_SETTING(USDIMAGINGGL_ENGINE_ENABLE_TASK_SCENE_INDEX, true,
                      "Use Scene Index API for task controller");

/// \deprecated. Will always use HdRenderer in the future.
TF_DEFINE_ENV_SETTING(
    USDIMAGINGGL_ENGINE_ENABLE_SCENE_INDEX_OBSERVER_RENDERER, true,
    "Use API to instantiate the Hydra 2.0 HdRenderer instead of the "
    "Hydra 1.0 HdRenderDelegate.");

TF_DEFINE_ENV_SETTING(
    USDIMAGINGGL_ENGINE_ENABLE_CACHING_SCENE_INDEX, false,
    "Use caching scene index (also requires "
    "USDIMAGINGGL_ENGINE_ENABLE_SCENE_INDEX_OBSERVER_RENDERER)");

TF_DEFINE_ENV_SETTING(
    USDIMAGINGGL_ENGINE_ENABLE_SCENE_INDEX_INPUT_ARGS, false,
    "Use HdRendererPlugin::GetSceneIndexInputArgs to configure scene indices. "
    "Requires a renderer such as Storm to implement the new API.");

namespace UsdImagingGLEngine_Impl
{

// Struct that holds application scene indices created via the
// scene index plugin registration callback facility.
//
// It is also in charge of registering the callback with the scene index
// plugin registration and tracking itself so that it can populate itself
// during the callback.
//
struct _AppSceneIndices : std::enable_shared_from_this<_AppSceneIndices>
{
    HdsiSceneGlobalsSceneIndexRefPtr
        sceneGlobalsSceneIndex;
    HdsiDomeLightCameraVisibilitySceneIndexRefPtr
        domeLightCameraVisibilitySceneIndex;
    HdsiSceneMaterialPruningSceneIndexRefPtr
        sceneMaterialPruningSceneIndex;

    _AppSceneIndicesSharedPtr
    static New(const TfToken &renderInstanceId)
    {
        // Register application managed scene indices via the callback
        // facility which will be invoked during render index construction.
        static std::once_flag registerOnce;
        std::call_once(registerOnce, _Register);

        auto result = std::make_shared<_AppSceneIndices>(renderInstanceId);
        _GetTracker().RegisterInstance(renderInstanceId, result);
        return result;
    }

    _AppSceneIndices(const TfToken &renderInstanceId)
     : _renderInstanceId(renderInstanceId) { }

    ~_AppSceneIndices()
    {
        _GetTracker().UnregisterInstance(_renderInstanceId);
    }

private:
    const TfToken _renderInstanceId;

    using _Tracker = HdUtils::RenderInstanceTracker<_AppSceneIndices>;
    static _Tracker &_GetTracker()
    {
        static _Tracker tracker;
        return tracker;
    }

    HdSceneIndexBaseRefPtr _Append(
        const HdSceneIndexBaseRefPtr &inputScene)
    {
        HdSceneIndexBaseRefPtr sceneIndex = inputScene;

        sceneIndex =
            sceneGlobalsSceneIndex =
                HdsiSceneGlobalsSceneIndex::New(sceneIndex);

        sceneIndex =
            domeLightCameraVisibilitySceneIndex =
                HdsiDomeLightCameraVisibilitySceneIndex::New(sceneIndex);

        sceneIndex =
            sceneMaterialPruningSceneIndex =
                HdsiSceneMaterialPruningSceneIndex::New(sceneIndex);

        return sceneIndex;
    }

    static HdSceneIndexBaseRefPtr _AppendCallback(
        const std::string &renderInstanceId,
        const HdSceneIndexBaseRefPtr &inputScene,
        const HdContainerDataSourceHandle &inputArgs)
    {
        _AppSceneIndicesSharedPtr const instance =
            _GetTracker().GetInstance(renderInstanceId);
        if (!instance) {
            TF_CODING_ERROR("Did not find appSceneIndices instance for %s,",
                            renderInstanceId.c_str());
            return inputScene;
        }
        return instance->_Append(inputScene);
    }

    static void _Register()
    {
        // Insert earlier so downstream scene indices can query and be notified
        // of changes and also declare their dependencies (e.g., to support
        // rendering color spaces).
        const HdSceneIndexPluginRegistry::InsertionPhase insertionPhase = 0;

        // Note:
        // The pattern used below registers the static member fn as a callback,
        // which retreives the scene index instance using the
        // renderInstanceId argument of the callback.

        HdSceneIndexPluginRegistry::GetInstance().RegisterSceneIndexForRenderer(
            std::string(), // empty string implies all renderers
            &_AppendCallback,
            /* inputArgs = */ nullptr,
            insertionPhase,
            HdSceneIndexPluginRegistry::InsertionOrderAtStart
        );
    }


};

}

namespace
{

// RAII helper to enable and disable notice batching when using the stage scene
// index.
class _ScopedHydraNoticeBatch
{
public:
    _ScopedHydraNoticeBatch(
         const HdNoticeBatchingSceneIndexRefPtr &noticeBatchingSceneIndex)
        : _noticeBatchingSceneIndex(noticeBatchingSceneIndex)
    {
        if (_noticeBatchingSceneIndex) {
            _noticeBatchingSceneIndex->SetBatchingEnabled(true);
        }
    }

    ~_ScopedHydraNoticeBatch()
    {
        if (_noticeBatchingSceneIndex) {
            _noticeBatchingSceneIndex->SetBatchingEnabled(false);
        }
    }

private:
    HdNoticeBatchingSceneIndexRefPtr _noticeBatchingSceneIndex;
};

// ----------------------------------------------------------------------------

SdfPath const&
_GetUsdImagingDelegateId()
{
    static SdfPath const delegateId =
        SdfPath(TfGetEnvSetting(USDIMAGINGGL_ENGINE_DEBUG_SCENE_DELEGATE_ID));

    return delegateId;
}

bool
_GetSceneIndexObserverRenderer()
{
    static bool result =
        TfGetEnvSetting(
            USDIMAGINGGL_ENGINE_ENABLE_SCENE_INDEX_OBSERVER_RENDERER);

    return result;
}

bool
_GetUseTaskControllerSceneIndex()
{
    static bool result =
        HdRenderIndex::IsSceneIndexEmulationEnabled() &&
        TfGetEnvSetting(USDIMAGINGGL_ENGINE_ENABLE_TASK_SCENE_INDEX);

    return result;
}

bool
_IsEnabledTerminalCachingSceneIndex()
{
    static bool result =
        TfGetEnvSetting(USDIMAGINGGL_ENGINE_ENABLE_CACHING_SCENE_INDEX);

    return result;
}

bool
_IsEnabledSceneIndexInputArgs()
{
    static bool result =
        TfGetEnvSetting(USDIMAGINGGL_ENGINE_ENABLE_SCENE_INDEX_INPUT_ARGS);

    return result;
}

// Convert UsdImagingGLCullStyle to a HdCullStyleTokens value.
static TfToken
_CullStyleEnumToToken(UsdImagingGLCullStyle cullStyle)
{
    switch(cullStyle) {
    case UsdImagingGLCullStyle::CULL_STYLE_NO_OPINION:
        return TfToken();
    case UsdImagingGLCullStyle::CULL_STYLE_NOTHING:
        return HdCullStyleTokens->nothing;
    case UsdImagingGLCullStyle::CULL_STYLE_BACK:
        return HdCullStyleTokens->back;
    case UsdImagingGLCullStyle::CULL_STYLE_FRONT:
        return HdCullStyleTokens->front;
    case UsdImagingGLCullStyle::CULL_STYLE_BACK_UNLESS_DOUBLE_SIDED:
        return HdCullStyleTokens->backUnlessDoubleSided;
    default:
        // XXX There is currently no UsdImagingGLCullStyle enum value
        // equivalent to HdCullStyleTokens->frontUnlessDoubleSided,
        // but if we add it in the future we need to handle it here.
        TF_CODING_ERROR("UsdImagingGLEngine: Unrecognzied enum value %i",
                        int(cullStyle));
        return TfToken();
    }
}

struct _RootOverrides
{
    GfMatrix4d transform = GfMatrix4d(1.0);
    bool visibility = true;
};

template<typename ScenePointer>
_RootOverrides _GetRootOverrides(
    const ScenePointer &p)
{
    _RootOverrides overrides;
    if (!p) {
        return overrides;
    }
    overrides.transform = p->GetRootTransform();
    overrides.visibility = p->GetRootVisibility();
    return overrides;
}

template<typename ScenePointer>
void _SetRootOverrides(
    const _RootOverrides &overrides,
    const ScenePointer &p)
{
    p->SetRootTransform(overrides.transform);
    p->SetRootVisibility(overrides.visibility);
}

} // anonymous namespace

/// \note
/// We conservatively release/acquire the Python GIL in most of the
/// non-const public methods of UsdImagingGLEngine (where scene index's are
/// mutated) using TF_PY_ALLOW_THREADS_IN_SCOPE() to avoid a deadlock when
/// another thread attempts to acquite the GIL while the main thread is
/// holding it.
///
/// While Hydra code is not wrapped to Python (notable exception being
/// Usdviewq.HydraObserver), it is possible for Hydra processing on a thread
/// to call into Python code (for example, when loading an image plugin with
/// Python bindings) in which case the thread will need to acquire the GIL.
///

//----------------------------------------------------------------------------
// Construction
//----------------------------------------------------------------------------

UsdImagingGLEngine::UsdImagingGLEngine(
    const Parameters &params)
  : UsdImagingGLEngine(
      params.rootPath,
      params.excludedPaths,
      params.invisedPaths,
      params.sceneDelegateID,
      params.driver,
      params.rendererPluginId,
      params.gpuEnabled,
      params.displayUnloadedPrimsWithBounds,
      params.allowAsynchronousSceneProcessing,
      params.enableUsdDrawModes)
{
}

UsdImagingGLEngine::UsdImagingGLEngine(
    const HdDriver& driver,
    const TfToken& rendererPluginId,
    const bool gpuEnabled)
    : UsdImagingGLEngine(SdfPath::AbsoluteRootPath(),
            {},
            {},
            _GetUsdImagingDelegateId(),
            driver,
            rendererPluginId,
            gpuEnabled)
{
}

UsdImagingGLEngine::UsdImagingGLEngine(
    const SdfPath& rootPath,
    const SdfPathVector& excludedPaths,
    const SdfPathVector& invisedPaths,
    const SdfPath& sceneDelegateID,
    const HdDriver& driver,
    const TfToken& rendererPluginId,
    const bool gpuEnabled,
    const bool displayUnloadedPrimsWithBounds,
    const bool allowAsynchronousSceneProcessing,
    const bool enableUsdDrawModes)
    : _hgi()
    , _hgiDriver(driver)
    , _displayUnloadedPrimsWithBounds(displayUnloadedPrimsWithBounds)
    , _gpuEnabled(gpuEnabled)
    , _sceneDelegateId(sceneDelegateID)
    , _selTracker(std::make_shared<HdxSelectionTracker>())
    , _selectionColor(1.0f, 1.0f, 0.0f, 1.0f)
    , _domeLightCameraVisibility(true)
    , _rootPath(rootPath)
    , _excludedPrimPaths(excludedPaths)
    , _invisedPrimPaths(invisedPaths)
    , _isPopulated(false)
    , _allowAsynchronousSceneProcessing(allowAsynchronousSceneProcessing)
    , _enableUsdDrawModes(enableUsdDrawModes)
{
    if (!_gpuEnabled && _hgiDriver.name == HgiTokens->renderDriver &&
        _hgiDriver.driver.IsHolding<Hgi*>()) {
        TF_WARN("Trying to share GPU resources while disabling the GPU.");
        _gpuEnabled = true;
    }

    // _renderIndex, _taskController, and _sceneDelegate/_sceneIndex
    // are initialized by the plugin system.
    if (!SetRendererPlugin(!rendererPluginId.IsEmpty() ?
            rendererPluginId : _GetDefaultRendererPluginId())) {
        TF_CODING_ERROR("No renderer plugins found!");
    }
}

void
UsdImagingGLEngine::_DestroyHydraObjects()
{
    TRACE_FUNCTION();

    // Destroy objects in opposite order of construction.

    {
        TRACE_SCOPE("Destroying engine and task controller");

        _engine = nullptr;
        _taskController = nullptr;
    }

    {
        TRACE_SCOPE("Destroy UsdImaging delegate");

        _sceneDelegate = nullptr;
    }

    // We are not removing _usdImagingFinalSceneIndex from _renderIndex or
    // _mergingSceneIndex here.
    //
    // This is not necessary since we destroy _renderIndex and
    // ~HdRenderIndex calls ~HdSceneIndexAdapterSceneDelegate which
    // calls HdRenderIndex::_RemoveSubtree.

    {
        // Hydra 2.0
        TRACE_SCOPE("Destroy renderer");
        _renderer = nullptr;
    }
    
    {
        TRACE_SCOPE("Destroy plugin scene indices and render index.");

        _renderIndex = nullptr;
    }

    {
        TRACE_SCOPE("Destroy render delegate");

        _renderDelegate = nullptr;
    }

    {
        // Hydra 2.0
        TRACE_SCOPE("Destroying terminal scene index");

        _terminalSceneIndex = nullptr;
        _cachingSceneIndex = nullptr;
        _appSceneIndices = nullptr;
    }

    {
        TRACE_SCOPE("Destroy UsdImaging scene indices");

        _usdImagingFinalSceneIndex = nullptr;
        _displayStyleSceneIndex = nullptr;
        _selectionSceneIndex = nullptr;
        _postInstancingNoticeBatchingSceneIndex = nullptr;
        _rootOverridesSceneIndex = nullptr;
        _lightPruningSceneIndex = nullptr;
        _stageSceneIndex = nullptr;
    }

    {
        TRACE_SCOPE("Task controller scene index");

        _taskControllerSceneIndex = TfNullPtr;
    }
}

UsdImagingGLEngine::~UsdImagingGLEngine()
{
    TRACE_FUNCTION();

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
    if (ARCH_UNLIKELY(!_HasRenderer())) {
        return;
    }

    if (!_CanPrepare(root)) {
        return;
    }

    HD_TRACE_FUNCTION();
    TF_PY_ALLOW_THREADS_IN_SCOPE();

    // Scene time.
    {
        _PreSetTime(params);
        // SetTime will only react if time actually changes.
        if (UseUsdImagingSceneIndex()) {
            _stageSceneIndex->SetTime(params.frame);
        } else {
            _sceneDelegate->SetTime(params.frame);
        }
        _SetSceneGlobalsCurrentFrame(params.frame);
        _PostSetTime(params);
    }

    // Miscellaneous scene render configuration parameters.
    if (UseUsdImagingSceneIndex()) {
        if (_appSceneIndices) {
            if (HdsiSceneMaterialPruningSceneIndexRefPtr const &si =
                    _appSceneIndices->sceneMaterialPruningSceneIndex) {
                si->SetEnabled(!params.enableSceneMaterials);
            }
        }
        if (_lightPruningSceneIndex) {
            if (_lightPruningSceneIndexEnableSceneLights !=
                    params.enableSceneLights) {
                _lightPruningSceneIndex->SetPathPredicate(
                    params.enableSceneLights
                        // Empty predicate means we prune nothing.
                        ? HdsiPrimTypeAndPathPruningSceneIndex::PathPredicate()
                        // Predicate matches every path.
                        // Thus, scene index prunes every prim
                        // matching the prim types given earlier.
                        : [](const SdfPath &a) { return true; });
                _lightPruningSceneIndexEnableSceneLights =
                    params.enableSceneLights;
            }
        }
        if (_displayStyleSceneIndex) {
            _displayStyleSceneIndex->SetCullStyleFallback(
                _CullStyleEnumToToken(params.cullStyle));
        }
    } else {
        _sceneDelegate->SetSceneMaterialsEnabled(params.enableSceneMaterials);
        _sceneDelegate->SetSceneLightsEnabled(params.enableSceneLights);
    }

    // Populate after setting time & configuration parameters above,
    // to avoid extra unforced rounds of invalidation after population.
    if (!_isPopulated) {
        auto stage = root.GetStage();
        if (UseUsdImagingSceneIndex()) {
            _ScopedHydraNoticeBatch noticeBatch(
                _postInstancingNoticeBatchingSceneIndex);

            // Set timeCodesPerSecond in HdsiSceneGlobalsSceneIndex.
            if (_appSceneIndices) {
                if (auto &sgsi = _appSceneIndices->sceneGlobalsSceneIndex) {
                    sgsi->SetTimeCodesPerSecond(stage->GetTimeCodesPerSecond());
                }
            }

            // XXX(USD-7113): Add pruning based on _rootPath

            // XXX(USD-7115): Add invis overrides from _invisedPrimPaths.

            TF_VERIFY(_stageSceneIndex);
            _stageSceneIndex->SetStage(stage);

        } else {
            TF_VERIFY(_sceneDelegate);
            _sceneDelegate->SetUsdDrawModesEnabled(
                params.enableUsdDrawModes && _enableUsdDrawModes);
            _sceneDelegate->Populate(
                stage->GetPrimAtPath(_rootPath),
                _excludedPrimPaths);
            _sceneDelegate->SetInvisedPrimPaths(_invisedPrimPaths);

            // This is only necessary when using the legacy scene delegate.
            // The stage scene index provides this functionality.
            _SetActiveRenderSettingsPrimFromStageMetadata(stage);
        }

        _isPopulated = true;
    }
}

void
UsdImagingGLEngine::_PrepareRender(const UsdImagingGLRenderParams &params)
{
    TfTokenVector renderTags;
    _ComputeRenderTags(params, &renderTags);

    if (_taskControllerSceneIndex) {
        _taskControllerSceneIndex->SetFreeCameraClipPlanes(params.clipPlanes);
        _taskControllerSceneIndex->SetRenderTags(renderTags);
        _taskControllerSceneIndex->SetRenderParams(
            _MakeHydraUsdImagingGLRenderParams(params));
    } else if (_taskController) {
        _taskController->SetFreeCameraClipPlanes(params.clipPlanes);
        _taskController->SetRenderTags(renderTags);
        _taskController->SetRenderParams(
            _MakeHydraUsdImagingGLRenderParams(params));
    } else {
        TF_CODING_ERROR("No task controller or task controller scene index.");
    }
}

void
UsdImagingGLEngine::_SetActiveRenderSettingsPrimFromStageMetadata(
    UsdStageWeakPtr stage)
{
    if (!TF_VERIFY(stage)) {
        return;
    }

    HdSceneIndexBaseRefPtr const terminalSceneIndex =
        _GetTerminalSceneIndex();
    if (!TF_VERIFY(terminalSceneIndex)) {
        return;
    }

    // If we already have an opinion, skip the stage metadata.
    if (!HdUtils::HasActiveRenderSettingsPrim(terminalSceneIndex)) {
        std::string pathStr;
        if (stage->HasAuthoredMetadata(
                UsdRenderTokens->renderSettingsPrimPath)) {
            stage->GetMetadata(
                UsdRenderTokens->renderSettingsPrimPath, &pathStr);
        }
        // Add the delegateId prefix since the scene globals scene index is
        // inserted into the merging scene index.
        if (!pathStr.empty()) {
            SetActiveRenderSettingsPrimPath(
                SdfPath(pathStr).ReplacePrefix(
                    SdfPath::AbsoluteRootPath(), _sceneDelegateId));
        }
    }
}

void
UsdImagingGLEngine::_UpdateDomeLightCameraVisibility()
{
    // The application communicates the dome light camera visibility
    // (that is whether to see the dome light texture behind the geometry)
    // through a render setting.
    //
    // Render settings set on a render delegate are not (yet) seen by
    // a scene index. So we pick it up here and set it on a scene index
    // populating the respective data for each dome light.
    //
    // Note that hdPrman and hdStorm implement dome light camera visibility
    // differently.
    //
    // hdPrman (at least when compiled against HDSI_API_VERSION >= 16) is
    // reading the dome light camera visibility from the corresponding data
    // source for the corresponding dome light in the scene index.
    //
    // Storm (or more precisely, the HdxSkydomeTask in Storm's render graph)
    // is actually reading the render setting.
    //
    // We might revisit the implementation of _UpdateDomeLightCameraVisibility
    // as we move towards Hydra 2.0 render delegates and render settings are
    // communicated in-band through scene indices.

    // The absence of a setting in the map is the same as camera visibility
    // being on.
    bool domeLightCamVisSetting;
    if (_renderer) {
        HdLegacyRenderControlInterface * const renderControl =
            _renderer->GetLegacyRenderControl();
        if (!renderControl) {
            return;
        }
        const VtValue v = renderControl->GetRenderSetting(
            HdRenderSettingsTokens->domeLightCameraVisibility);
        domeLightCamVisSetting = v.GetWithDefault<bool>(true);
    } else if (_renderDelegate) {
        domeLightCamVisSetting =    
            _renderDelegate->
            GetRenderSetting<bool>(
                HdRenderSettingsTokens->domeLightCameraVisibility,
                true);
    } else {
        return;
    }
        
    if (_domeLightCameraVisibility != domeLightCamVisSetting) {
        // Camera visibility state changed, so we need to mark any dome lights
        // as dirty to ensure they have the proper state on all backends.
        _domeLightCameraVisibility = domeLightCamVisSetting;

        if (_renderIndex &&
            _renderIndex->IsSprimTypeSupported(HdPrimTypeTokens->domeLight)) {
            // For old implementation where hdPrman would read the dome light
            // camera visibility render setting in HdPrman_Light::Sync and thus
            // required invalidation for each dome light.
            //
            // Note that MarkSprimDirty only works for prims originating from a
            // delegate, not a scene index.
            //
            // This code block can probably be deleted.

            for (const SdfPath &path :
                     _renderIndex->GetSprimSubtree(
                         HdPrimTypeTokens->domeLight,
                         SdfPath::AbsoluteRootPath())) {
                _renderIndex->GetChangeTracker().MarkSprimDirty(
                    path, HdLight::DirtyParams);
            }
        }

        if (_appSceneIndices) {
            if (HdsiDomeLightCameraVisibilitySceneIndexRefPtr const &si =
                    _appSceneIndices->domeLightCameraVisibilitySceneIndex) {
                si->SetDomeLightCameraVisibility(domeLightCamVisSetting);
            }
        }
    }
}

void
UsdImagingGLEngine::_SetBBoxParams(
    const BBoxVector& bboxes,
    const GfVec4f& bboxLineColor,
    float bboxLineDashSize)
{
    if (ARCH_UNLIKELY(!_HasRenderer())) {
        return;
    }

    HdxBoundingBoxTaskParams params;
    params.bboxes = bboxes;
    params.color = bboxLineColor;
    params.dashSize = bboxLineDashSize;

    if (_taskControllerSceneIndex) {
        _taskControllerSceneIndex->SetBBoxParams(params);
    } else if (_taskController) {
        _taskController->SetBBoxParams(params);
    } else {
        TF_CODING_ERROR("No task controller or task controller scene index.");
    }
}

void
UsdImagingGLEngine::RenderBatch(
    const SdfPathVector& paths,
    const UsdImagingGLRenderParams& params)
{
    if (ARCH_UNLIKELY(!_HasRenderer())) {
        return;
    }

    TF_PY_ALLOW_THREADS_IN_SCOPE();

    _UpdateHydraCollection(&_renderCollection, paths, params);
    if (_taskControllerSceneIndex) {
        _taskControllerSceneIndex->SetCollection(_renderCollection);
    } else if (_taskController) {
        _taskController->SetCollection(_renderCollection);
    } else {
        TF_CODING_ERROR("No task controller or task controller scene index.");
    }


    _PrepareRender(params);

    SetColorCorrectionSettings(params.colorCorrectionMode, params.ocioDisplay,
        params.ocioView, params.ocioColorSpace, params.ocioLook);

    _SetBBoxParams(params.bboxes, params.bboxLineColor, params.bboxLineDashSize);

    // XXX App sets the clear color via 'params' instead of setting up Aovs
    // that has clearColor in their descriptor. So for now we must pass this
    // clear color to the color AOV.
    if (_taskControllerSceneIndex) {
        _taskControllerSceneIndex->SetEnableSelection(params.highlight);

        HdAovDescriptor colorAovDesc =
            _taskControllerSceneIndex->GetRenderOutputSettings(HdAovTokens->color);
        if (colorAovDesc.format != HdFormatInvalid) {
            colorAovDesc.clearValue = VtValue(params.clearColor);
            _taskControllerSceneIndex->SetRenderOutputSettings(
                HdAovTokens->color, colorAovDesc);
        }
    } else if (_taskController) {
        _taskController->SetEnableSelection(params.highlight);

        HdAovDescriptor colorAovDesc =
            _taskController->GetRenderOutputSettings(HdAovTokens->color);
        if (colorAovDesc.format != HdFormatInvalid) {
            colorAovDesc.clearValue = VtValue(params.clearColor);
            _taskController->SetRenderOutputSettings(
                HdAovTokens->color, colorAovDesc);
        }
    }

    _UpdateDomeLightCameraVisibility();

    const VtValue selectionValue(_selTracker);
    if (_renderer) {
        if (HdLegacyRenderControlInterface * const renderControl =
                _renderer->GetLegacyRenderControl()) {
            renderControl->SetTaskContextData(HdxTokens->selectionState, selectionValue);
        }
    } else if (_engine) {
        _engine->SetTaskContextData(HdxTokens->selectionState, selectionValue);
    }
    if (_taskControllerSceneIndex) {
        _Execute(params, _taskControllerSceneIndex->GetRenderingTaskPaths());
    } else if (_taskController) {
        _Execute(params, _taskController->GetRenderingTaskPaths());
    }
}

void
UsdImagingGLEngine::Render(
    const UsdPrim& root,
    const UsdImagingGLRenderParams &params)
{
    if (ARCH_UNLIKELY(!_HasRenderer())) {
        return;
    }

    // We release/acquire the GIL in PrepareBatch and RenderBatch.
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
    if (_renderer) {
        if (!_taskControllerSceneIndex) {
            TF_CODING_ERROR("No task controller scene index.");
            return true;
        }
        if (HdLegacyRenderControlInterface * const renderControl =
                _renderer->GetLegacyRenderControl()) {
            return renderControl->AreTasksConverged(
                _taskControllerSceneIndex->GetRenderingTaskPaths());
        }
        return true;
    } else if (_renderIndex) {
        if (_taskControllerSceneIndex) {
            return _engine->AreTasksConverged(
                _renderIndex.get(), 
                _taskControllerSceneIndex->GetRenderingTaskPaths());
        } else if (_taskController) {
            return _engine->AreTasksConverged(
                _renderIndex.get(), 
                _taskController->GetRenderingTaskPaths());
        } else {
            TF_CODING_ERROR("No task controller or task controller scene index.");
            return true;
        }
    }
         
    return true;
}

//----------------------------------------------------------------------------
// Root and Transform Visibility
//----------------------------------------------------------------------------

void
UsdImagingGLEngine::SetRootTransform(GfMatrix4d const& xf)
{
    if (ARCH_UNLIKELY(!_HasRenderer())) {
        return;
    }

    TF_PY_ALLOW_THREADS_IN_SCOPE();

    if (UseUsdImagingSceneIndex()) {
        _rootOverridesSceneIndex->SetRootTransform(xf);
    } else {
        _sceneDelegate->SetRootTransform(xf);
    }
}

void
UsdImagingGLEngine::SetRootVisibility(const bool isVisible)
{
    if (ARCH_UNLIKELY(!_HasRenderer())) {
        return;
    }

    TF_PY_ALLOW_THREADS_IN_SCOPE();

    if (UseUsdImagingSceneIndex()) {
        _rootOverridesSceneIndex->SetRootVisibility(isVisible);
    } else {
        _sceneDelegate->SetRootVisibility(isVisible);
    }
}

//----------------------------------------------------------------------------
// Camera and Light State
//----------------------------------------------------------------------------

void
UsdImagingGLEngine::SetRenderViewport(GfVec4d const& viewport)
{
    if (ARCH_UNLIKELY(!_HasRenderer())) {
        return;
    }

    TF_PY_ALLOW_THREADS_IN_SCOPE();

    if (_taskControllerSceneIndex) {
        _taskControllerSceneIndex->SetRenderViewport(viewport);
    } else if (_taskController) {
        _taskController->SetRenderViewport(viewport);
    } else {
        TF_CODING_ERROR("No task controller or task controller scene index.");
    }
}

void
UsdImagingGLEngine::SetFraming(CameraUtilFraming const& framing)
{
    if (ARCH_UNLIKELY(!_HasRenderer())) {
        return;
    }

    TF_PY_ALLOW_THREADS_IN_SCOPE();

    if (_taskControllerSceneIndex) {
        _taskControllerSceneIndex->SetFraming(framing);
    } else if (_taskController) {
        _taskController->SetFraming(framing);
    } else {
        TF_CODING_ERROR("No task controller or task controller scene index.");
    }
}

void
UsdImagingGLEngine::SetOverrideWindowPolicy(
    const std::optional<CameraUtilConformWindowPolicy> &policy)
{
    if (ARCH_UNLIKELY(!_HasRenderer())) {
        return;
    }

    TF_PY_ALLOW_THREADS_IN_SCOPE();

    if (_taskControllerSceneIndex) {
        _taskControllerSceneIndex->SetOverrideWindowPolicy(policy);
    } else if (_taskController) {
        _taskController->SetOverrideWindowPolicy(policy);
    } else {
        TF_CODING_ERROR("No task controller or task controller scene index.");
    }
}

void
UsdImagingGLEngine::SetRenderBufferSize(GfVec2i const& size)
{
    if (ARCH_UNLIKELY(!_HasRenderer())) {
        return;
    }

    TF_PY_ALLOW_THREADS_IN_SCOPE();

    if (_taskControllerSceneIndex) {
        _taskControllerSceneIndex->SetRenderBufferSize(size);
    } else if (_taskController) {
        _taskController->SetRenderBufferSize(size);
    } else {
        TF_CODING_ERROR("No task controller or task controller scene index.");
    }
}

void
UsdImagingGLEngine::SetWindowPolicy(CameraUtilConformWindowPolicy policy)
{
    if (ARCH_UNLIKELY(!_HasRenderer())) {
        return;
    }

    TF_PY_ALLOW_THREADS_IN_SCOPE();

    // Note: Free cam uses SetCameraState, which expects the frustum to be
    // pre-adjusted for the viewport size.

    if (UseUsdImagingSceneIndex()) {
        // XXX(USD-7115): window policy
    } else {
        // The usdImagingDelegate manages the window policy for scene cameras.
        _sceneDelegate->SetWindowPolicy(policy);
    }
}

void
UsdImagingGLEngine::SetCameraPath(SdfPath const& id)
{
    if (ARCH_UNLIKELY(!_HasRenderer())) {
        return;
    }

    TF_PY_ALLOW_THREADS_IN_SCOPE();

    if (_taskControllerSceneIndex) {
        _taskControllerSceneIndex->SetCameraPath(id);
    } else if (_taskController) {
        _taskController->SetCameraPath(id);
    } else {
        TF_CODING_ERROR("No task controller or task controller scene index.");
    }

    // The camera that is set for viewing will also be used for
    // time sampling.
    // XXX(HYD-2304): motion blur shutter window.
    if (UseUsdImagingSceneIndex()) {
        // Set camera path on HdsiSceneGlobalsSceneIndex.
        if (_appSceneIndices) {
            if (auto &sgsi = _appSceneIndices->sceneGlobalsSceneIndex) {
                sgsi->SetPrimaryCameraPrimPath(id);
            }
        }
    } else {
        _sceneDelegate->SetCameraForSampling(id);
    }
}

void
UsdImagingGLEngine::SetCameraState(const GfMatrix4d& viewMatrix,
                                   const GfMatrix4d& projectionMatrix)
{
    if (ARCH_UNLIKELY(!_HasRenderer())) {
        return;
    }

    TF_PY_ALLOW_THREADS_IN_SCOPE();

    if (_taskControllerSceneIndex) {
        _taskControllerSceneIndex->SetFreeCameraMatrices(viewMatrix, projectionMatrix);
    } else if (_taskController) {
        _taskController->SetFreeCameraMatrices(viewMatrix, projectionMatrix);
    } else {
        TF_CODING_ERROR("No task controller or task controller scene index.");
    }
}

void
UsdImagingGLEngine::SetLightingState(GlfSimpleLightingContextPtr const &src)
{
    if (ARCH_UNLIKELY(!_HasRenderer())) {
        return;
    }

    TF_PY_ALLOW_THREADS_IN_SCOPE();

    if (_taskControllerSceneIndex) {
        _taskControllerSceneIndex->SetLightingState(src);
    } else if (_taskController) {
        _taskController->SetLightingState(src);
    } else {
        TF_CODING_ERROR("No task controller or task controller scene index.");
    }
}

void
UsdImagingGLEngine::SetLightingState(
    GlfSimpleLightVector const &lights,
    GlfSimpleMaterial const &material,
    GfVec4f const &sceneAmbient)
{
    if (ARCH_UNLIKELY(!_HasRenderer())) {
        return;
    }

    TF_PY_ALLOW_THREADS_IN_SCOPE();

    // we still use _lightingContextForOpenGLState for convenience, but
    // set the values directly.
    if (!_lightingContextForOpenGLState) {
        _lightingContextForOpenGLState = GlfSimpleLightingContext::New();
    }
    _lightingContextForOpenGLState->SetLights(lights);
    _lightingContextForOpenGLState->SetMaterial(material);
    _lightingContextForOpenGLState->SetSceneAmbient(sceneAmbient);
    _lightingContextForOpenGLState->SetUseLighting(lights.size() > 0);

    if (_taskControllerSceneIndex) {
        _taskControllerSceneIndex->SetLightingState(
            _lightingContextForOpenGLState);
    } else if (_taskController) {
        _taskController->SetLightingState(
            _lightingContextForOpenGLState);
    } else {
        TF_CODING_ERROR("No task controller or task controller scene index.");
    }
}

//----------------------------------------------------------------------------
// Selection Highlighting
//----------------------------------------------------------------------------

void
UsdImagingGLEngine::SetSelected(SdfPathVector const& paths)
{
    if (ARCH_UNLIKELY(!_HasRenderer())) {
        return;
    }

    TF_PY_ALLOW_THREADS_IN_SCOPE();

    if (UseUsdImagingSceneIndex()) {
        _selectionSceneIndex->ClearSelection();

        for (const SdfPath &path : paths) {
            _selectionSceneIndex->AddSelection(path);
        }
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
    if (ARCH_UNLIKELY(!_HasRenderer())) {
        return;
    }

    TF_PY_ALLOW_THREADS_IN_SCOPE();

    if (UseUsdImagingSceneIndex()) {
        _selectionSceneIndex->ClearSelection();
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
    if (ARCH_UNLIKELY(!_HasRenderer())) {
        return;
    }

    TF_PY_ALLOW_THREADS_IN_SCOPE();

    if (UseUsdImagingSceneIndex()) {
        _selectionSceneIndex->AddSelection(path);
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
    if (ARCH_UNLIKELY(!_HasRenderer())) {
        return;
    }

    TF_PY_ALLOW_THREADS_IN_SCOPE();

    _selectionColor = color;

    if (_taskControllerSceneIndex) {
        _taskControllerSceneIndex->SetSelectionColor(_selectionColor);
    } else if (_taskController) {
        _taskController->SetSelectionColor(_selectionColor);
    } else {
        TF_CODING_ERROR("No task controller or task controller scene index.");
    }
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
    PickParams pickParams = { HdxPickTokens->resolveNearestToCenter };
    IntersectionResultVector results;

    if (TestIntersection(pickParams, viewMatrix, projectionMatrix,
            root, params, &results)) {
        if (results.size() != 1) {
            // Since we are in nearest-hit mode, we expect allHits to have a
            // single point in it.
            return false;
        }
        IntersectionResult &result = results.front();
        if (outHitPoint) {
            *outHitPoint = result.hitPoint;
        }
        if (outHitNormal) {
            *outHitNormal = result.hitNormal;
        }
        if (outHitPrimPath) {
            *outHitPrimPath = result.hitPrimPath;
        }
        if (outHitInstancerPath) {
            *outHitInstancerPath = result.hitInstancerPath;
        }
        if (outHitInstanceIndex) {
            *outHitInstanceIndex = result.hitInstanceIndex;
        }
        if (outInstancerContext) {
            *outInstancerContext = result.instancerContext;
        }
        return true;
    }
    return false;
}

SdfPath
UsdImagingGLEngine::_GetInstancerForPrim(const SdfPath &sceneIndexPath) const
{
    if (_GetTerminalSceneIndex()) {
        const auto schema = HdInstancedBySchema::GetFromParent(
            _GetTerminalSceneIndex()->GetPrim(sceneIndexPath).dataSource);
        HdPathArrayDataSourceHandle const ds = schema.GetPaths();
        if (!ds) {
            return SdfPath();
        }
        const VtArray<SdfPath> &paths = ds->GetTypedValue(0.0f);
        if (paths.empty()) {
            return SdfPath();
        }
        return paths[0];
    }
    if (_renderIndex) {
        SdfPath delegateId, instancerId;
        _renderIndex->GetSceneDelegateAndInstancerIds(
            sceneIndexPath, &delegateId, &instancerId);
        return instancerId;
    }
    return SdfPath();
}

bool
UsdImagingGLEngine::TestIntersection(
    const PickParams& pickParams,
    const GfMatrix4d& viewMatrix,
    const GfMatrix4d& projectionMatrix,
    const UsdPrim& root,
    const UsdImagingGLRenderParams& params,
    IntersectionResultVector* outResults)
{
    if (ARCH_UNLIKELY(!_HasRenderer())) {
        return false;
    }

    TF_PY_ALLOW_THREADS_IN_SCOPE();

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
    HdxPickTaskContextParams pickCtxParams;
    pickCtxParams.resolveMode = pickParams.resolveMode;
    pickCtxParams.viewMatrix = viewMatrix;
    pickCtxParams.projectionMatrix = projectionMatrix;
    pickCtxParams.clipPlanes = params.clipPlanes;
    pickCtxParams.collection = _intersectCollection;
    pickCtxParams.outHits = &allHits;
    const VtValue vtPickCtxParams(pickCtxParams);

    if (_renderer) {
        if (HdLegacyRenderControlInterface * const renderControl =
                _renderer->GetLegacyRenderControl()) {
            renderControl->SetTaskContextData(
                HdxPickTokens->pickParams, vtPickCtxParams);
        }
    } else if (_engine) {
        _engine->SetTaskContextData(
                HdxPickTokens->pickParams, vtPickCtxParams);
    }
    if (_taskControllerSceneIndex) {
        _Execute(params, _taskControllerSceneIndex->GetPickingTaskPaths());
    } else if (_taskController) {
        _Execute(params, _taskController->GetPickingTaskPaths());
    } else {
        TF_CODING_ERROR("No task controller or task controller scene index.");
    }


    // return false if there were no hits
    if (allHits.size() == 0) {
        return false;
    }

    for(HdxPickHit& hit : allHits)
    {
        IntersectionResult res;

        if (_sceneDelegate) {
            res.hitPrimPath = _sceneDelegate->GetScenePrimPath(
                    hit.objectId, hit.instanceIndex, &(res.instancerContext));
            res.hitInstancerPath =
                _sceneDelegate
                    ->ConvertIndexPathToCachePath(
                        _GetInstancerForPrim(hit.objectId))
                    .GetAbsoluteRootOrPrimPath();
        } else {
            const HdxPrimOriginInfo info = HdxPrimOriginInfo::FromPickHit(
                _GetTerminalSceneIndex(), hit);
            res.hitPrimPath = info.GetFullPath();
            res.instancerContext = info.ComputeInstancerContext();
            if (!res.instancerContext.empty()) {
                res.hitInstancerPath = res.instancerContext.back().first;
            }
        }

        res.hitPoint = hit.worldSpaceHitPoint;
        res.hitNormal = hit.worldSpaceHitNormal;
        res.hitInstanceIndex = hit.instanceIndex;

        if (outResults) {
            outResults->push_back(res);
        }
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
    const int primIdx = HdxPickTask::DecodeIDRenderColor(primIdColor);
    const int instanceIdx = HdxPickTask::DecodeIDRenderColor(instanceIdColor);

    return DecodeIntersection(primIdx, instanceIdx, outHitPrimPath,
        outHitInstancerPath, outHitInstanceIndex, outInstancerContext);
}

bool
UsdImagingGLEngine::DecodeIntersection(
    const int primIdx,
    const int instanceIdx,
    SdfPath * const outHitPrimPath,
    SdfPath * const outHitInstancerPath,
    int * const outHitInstanceIndex,
    HdInstancerContext *const outInstancerContext)
{
    SdfPath sceneIndexPath;
    if (_renderer) {
        if (HdLegacyRenderControlInterface * const renderControl =
                _renderer->GetLegacyRenderControl()) {
            sceneIndexPath =
                renderControl->GetRprimPathFromPrimId(primIdx);
        }
    } else if (_renderIndex) {
        sceneIndexPath = _renderIndex->GetRprimPathFromPrimId(primIdx);
    }

    if (sceneIndexPath.IsEmpty()) {
        return false;
    }

    if (_sceneDelegate) {
        if (outHitPrimPath) {
            *outHitPrimPath =
                _sceneDelegate->GetScenePrimPath(
                    sceneIndexPath, instanceIdx, outInstancerContext);
        }
        if (outHitInstancerPath) {
            *outHitInstancerPath =
                _sceneDelegate
                    ->ConvertIndexPathToCachePath(
                        _GetInstancerForPrim(sceneIndexPath))
                    .GetAbsoluteRootOrPrimPath();
        }

    } else {
        HdxPickHit hit;
        hit.objectId = sceneIndexPath;
        hit.instanceIndex = instanceIdx;

        const HdxPrimOriginInfo info =
            HdxPrimOriginInfo::FromPickHit(
                _GetTerminalSceneIndex(), hit);
        if (outHitPrimPath) {
            *outHitPrimPath = info.GetFullPath();
        }
        
        if (outInstancerContext) {
            *outInstancerContext = info.ComputeInstancerContext();
        }
        if (outHitInstancerPath) {
            const HdInstancerContext &ctx =
                outInstancerContext
                    ? *outInstancerContext
                    : info.ComputeInstancerContext();
            if (!ctx.empty()) {
                *outHitInstancerPath = ctx.back().first;
            }
        }
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
    HfPluginDesc pluginDescriptor;
    bool foundPlugin = HdRendererPluginRegistry::GetInstance().
        GetPluginDesc(id, &pluginDescriptor);

    if (!foundPlugin) {
        return std::string();
    }

    // Storm's display name is GL, but that's just confusing since it
    // also has Metal and Vulkan implementations. Change it here for now,
    // eventually it will have to be properly renamed.
    static const TfToken _stormRendererPluginName("HdStormRendererPlugin");
    if (pluginDescriptor.id == _stormRendererPluginName) {
        return "Storm";
    }

    return pluginDescriptor.displayName;
}

std::string
UsdImagingGLEngine::GetRendererHgiDisplayName() const
{
    if (!_hgi) {
        return "";
    }

    return _hgi->GetAPIName();
}

bool
UsdImagingGLEngine::GetGPUEnabled() const
{
    return _gpuEnabled;
}

TfToken
UsdImagingGLEngine::GetCurrentRendererId() const
{
    if (_renderer) {
        return _renderer.GetPluginId();
    } else if (_renderDelegate) {
        return _renderDelegate.GetPluginId();
    }
    return TfToken();
}

void
UsdImagingGLEngine::_InitializeHgiIfNecessary()
{
    // If the client of UsdImagingGLEngine does not provide a HdDriver, we
    // construct a default one that is owned by UsdImagingGLEngine.
    // The cleanest pattern is for the client app to provide this since you
    // may have multiple UsdImagingGLEngines in one app that ideally all use
    // the same HdDriver and Hgi to share GPU resources.
    if (_gpuEnabled && _hgiDriver.driver.IsEmpty()) {
        _hgi = Hgi::CreatePlatformDefaultHgi();
        _hgiDriver.name = HgiTokens->renderDriver;
        _hgiDriver.driver = VtValue(_hgi.get());
    }
}

bool
UsdImagingGLEngine::SetRendererPlugin(TfToken const &id)
{
    _InitializeHgiIfNecessary();

    HdRendererPluginRegistry &registry =
        HdRendererPluginRegistry::GetInstance();

    HdRendererCreateArgs rendererCreateArgs;
    rendererCreateArgs.gpuEnabled = _gpuEnabled;
    rendererCreateArgs.hgi = _hgi.get();

    const TfToken resolvedId =
        id.IsEmpty()
            ? registry.GetDefaultPluginId(rendererCreateArgs)
            : id;

    HdRendererPluginHandle const plugin = registry.GetOrCreateRendererPlugin(resolvedId);
    if (!plugin) {
        TF_CODING_ERROR("Invalid plugin id %s", resolvedId.GetText());
        return false;
    }

    std::string errorStr;
    if (!plugin->IsSupported(rendererCreateArgs, &errorStr)) {
        TF_CODING_ERROR(
            "Plugin %s is unsupported: %s",
            resolvedId.GetText(), errorStr.c_str());
        return false;
    }

    HdContainerDataSourceHandle const sceneIndexInputArgs =
        HdOverlayContainerDataSource::OverlayedContainerDataSources(
            plugin->GetSceneIndexInputArgs(),
            HdRetainedContainerDataSource::New(
                UsdImagingUsdSceneIndexInputArgsSchema::GetSchemaToken(),
                UsdImagingUsdSceneIndexInputArgsSchema::Builder()
                    .SetAddDrawModeSceneIndex(
                        HdRetainedTypedSampledDataSource<bool>::New(
                            _enableUsdDrawModes))
                    .SetDisplayUnloadedPrimsWithBounds(
                        HdRetainedTypedSampledDataSource<bool>::New(
                            _displayUnloadedPrimsWithBounds))
                    .Build()));
    
    if (_GetSceneIndexObserverRenderer()) {
        if (_renderer && _renderer.GetPluginId() == resolvedId) {
            return true;
        }

        TF_PY_ALLOW_THREADS_IN_SCOPE();

        return _CreateSceneIndicesAndRenderer(plugin, sceneIndexInputArgs);
    } else {
        if (_renderDelegate && _renderDelegate.GetPluginId() == resolvedId) {
            return true;
        }

        TF_PY_ALLOW_THREADS_IN_SCOPE();

        HdPluginRenderDelegateUniqueHandle renderDelegate =
            plugin->CreateDelegate();
        if (!renderDelegate) {
            return false;
        }
        _SetRenderDelegateAndRestoreState(
            std::move(renderDelegate), sceneIndexInputArgs);
    }

    return true;
}

bool
UsdImagingGLEngine::_CreateSceneIndicesAndRenderer(
    HdRendererPluginHandle const &plugin,
    HdContainerDataSourceHandle const &sceneIndexInputArgs)
{
    TRACE_FUNCTION();

    const _RootOverrides rootOverrides =
        UseUsdImagingSceneIndex()
            ? _GetRootOverrides(_rootOverridesSceneIndex)
            : _GetRootOverrides(_sceneDelegate);

    _DestroyHydraObjects();

    _isPopulated = false;

    _mergingSceneIndex = HdMergingSceneIndex::New();

    {
        TRACE_SCOPE("Post-merging scene indices");
        
        // Setup scene indices following HdMergingSceneIndex.

        const std::string rendererDisplayName = plugin->GetDisplayName();

        HdSceneIndexBaseRefPtr sceneIndex = _mergingSceneIndex;
        if (!rendererDisplayName.empty()) {
            // Use the render delegate ptr (rather than 'this' ptr) for generating
            // the unique id.
            const std::string renderInstanceId =
                TfStringPrintf(
                    "UsdImagingGLEngine_%s_%p",
                    rendererDisplayName.c_str(), (void *) this);

            _appSceneIndices =
                UsdImagingGLEngine_Impl::_AppSceneIndices::New(
                    TfToken(renderInstanceId));

            sceneIndex =
                HdSceneIndexPluginRegistry::GetInstance()
                    .AppendSceneIndicesForRenderer(
                        rendererDisplayName, sceneIndex, renderInstanceId);
        }

        if (_IsEnabledTerminalCachingSceneIndex()) {
            sceneIndex =
                _cachingSceneIndex =
                    HdCachingSceneIndex::New(sceneIndex);
        }
        _terminalSceneIndex = sceneIndex;
    }

    _renderer =
        plugin->CreateRenderer(
            _terminalSceneIndex,
            HdRendererCreateArgsSchema::Builder()
                .SetGpuEnabled(
                    HdRetainedTypedSampledDataSource<bool>::New(
                        _gpuEnabled))
                .SetDrivers(
                    HdRetainedContainerDataSource::New(
                        HdRendererCreateArgsSchemaTokens->hgi,
                        HdRetainedTypedSampledDataSource<Hgi*>::New(
                            _hgiDriver.driver.GetWithDefault<Hgi*>(nullptr))))
                .Build());

    if (!_renderer) {
        return false;
    }

    if (UseUsdImagingSceneIndex()) {
        TRACE_SCOPE("UsdImaging scene indices");
        
        // Setup Usd imaging scene indices.

        _CreateUsdImagingSceneIndices(sceneIndexInputArgs);
        _SetRootOverrides(rootOverrides, _rootOverridesSceneIndex);

        if (!_sceneDelegateId.IsAbsoluteRootPath()) {
            _usdImagingFinalSceneIndex = HdPrefixingSceneIndex::New(
                _usdImagingFinalSceneIndex, _sceneDelegateId);
        }
        _mergingSceneIndex->InsertInputScenes(
            {{ _usdImagingFinalSceneIndex, _sceneDelegateId }});
    } else {
        TRACE_SCOPE("UsdImaging scene delegate");

        HdRenderIndexAdapterSceneIndexRefPtr adapter;

        if (_IsEnabledSceneIndexInputArgs()) {
            adapter = HdRenderIndexAdapterSceneIndex::New(
                sceneIndexInputArgs);
        } else {
            HdRenderDelegateInfo info;
            if (HdLegacyRenderControlInterface * const renderControl =
                    _renderer->GetLegacyRenderControl()) {
                info = renderControl->GetRenderDelegateInfo();
            }
            adapter = HdRenderIndexAdapterSceneIndex::New(info);
        }

        _mergingSceneIndex->InsertInputScenes(
            {{adapter, SdfPath::AbsoluteRootPath()}});

        _sceneDelegate = std::make_unique<UsdImagingDelegate>(
            adapter->GetRenderIndex(), _sceneDelegateId);

        _sceneDelegate->SetDisplayUnloadedPrimsWithBounds(
            _displayUnloadedPrimsWithBounds);

        _SetRootOverrides(rootOverrides, _sceneDelegate);
    }

    {
        TRACE_SCOPE("Task controller scene index");
        
        // Setup task controller scene index.

        bool requiresStormTasks = false;
        if (HdLegacyRenderControlInterface * const renderControl =
                _renderer->GetLegacyRenderControl()) {
            requiresStormTasks = renderControl->RequiresStormTasks();
        }

        const SdfPath taskControllerPath =
            _ComputeControllerPath(TfToken(plugin->GetDisplayName()));
        HdxTaskControllerSceneIndex::Parameters params {
            taskControllerPath,
            [renderer = _renderer.Get()](const TfToken &name) {
                HdLegacyRenderControlInterface * const renderControl =
                    renderer->GetLegacyRenderControl();
                if (!renderControl) { return HdAovDescriptor(); }
                return renderControl->GetDefaultAovDescriptor(name); },
            requiresStormTasks,
            _gpuEnabled
        };

        _taskControllerSceneIndex = HdxTaskControllerSceneIndex::New(params);
        _taskControllerSceneIndex->SetSelectionColor(_selectionColor);
        _mergingSceneIndex->InsertInputScenes(
            {{ _taskControllerSceneIndex, taskControllerPath }});
    }

    return true;
}

void
UsdImagingGLEngine::_SetRenderDelegateAndRestoreState(
    HdPluginRenderDelegateUniqueHandle &&renderDelegate,
    HdContainerDataSourceHandle const &sceneIndexInputArgs)
{
    // Pull old scene/task controller state. Note that the scene index/delegate
    // may not have been created, if this is the first time through this
    // function, so we guard for null and use default values for xform/vis.
    const _RootOverrides rootOverrides =
        UseUsdImagingSceneIndex()
            ? _GetRootOverrides(_rootOverridesSceneIndex)
            : _GetRootOverrides(_sceneDelegate);

    HdSelectionSharedPtr const selection = _GetSelection();

    // Rebuild the imaging stack
    _SetRenderDelegate(std::move(renderDelegate), sceneIndexInputArgs);

    // Reload saved state.
    if (UseUsdImagingSceneIndex()) {
        _SetRootOverrides(rootOverrides, _rootOverridesSceneIndex);
    } else {
        _SetRootOverrides(rootOverrides, _sceneDelegate);
    }
    _selTracker->SetSelection(selection);

    if (_taskControllerSceneIndex) {
        _taskControllerSceneIndex->SetSelectionColor(_selectionColor);
    } else if (_taskController) {
        _taskController->SetSelectionColor(_selectionColor);
    } else {
        TF_CODING_ERROR("No task controller or task controller scene index.");
    }
}

SdfPath
UsdImagingGLEngine::_ComputeControllerPath(
    const TfToken &pluginId)
{
    const std::string pluginIdStr =
        TfMakeValidIdentifier(pluginId.GetText());
    const TfToken rendererName(
        TfStringPrintf("_UsdImaging_%s_%p", pluginIdStr.c_str(), this));

    return _sceneDelegateId.AppendChild(rendererName);
}

SdfPath
UsdImagingGLEngine::_ComputeControllerPath(
    const HdPluginRenderDelegateUniqueHandle &renderDelegate)
{
    return _ComputeControllerPath(renderDelegate.GetPluginId());
}

HdSceneIndexBaseRefPtr
UsdImagingGLEngine::_AppendOverridesSceneIndices(
    HdSceneIndexBaseRefPtr const &inputScene)
{
    HdSceneIndexBaseRefPtr sceneIndex = inputScene;

    const HdContainerDataSourceHandle prefixPathPruningInputArgs =
        HdRetainedContainerDataSource::New(
            HdsiPrefixPathPruningSceneIndexTokens->excludePathPrefixes,
            HdRetainedTypedSampledDataSource<SdfPathVector>::New(
                _excludedPrimPaths));

    sceneIndex = HdsiPrefixPathPruningSceneIndex::New(
        sceneIndex, prefixPathPruningInputArgs);

    static HdContainerDataSourceHandle const lightPruningInputArgs =
        HdRetainedContainerDataSource::New(
            HdsiPrimTypeAndPathPruningSceneIndexTokens->primTypes,
            HdRetainedTypedSampledDataSource<TfTokenVector>::New(
                HdLightPrimTypeTokens()));

    sceneIndex = _lightPruningSceneIndex =
        HdsiPrimTypeAndPathPruningSceneIndex::New(
            sceneIndex, lightPruningInputArgs);
    // _lightPruningSceneIndex comes with empty predicate which corresponds to
    // enableSceneLights = true.
    _lightPruningSceneIndexEnableSceneLights = true;

    sceneIndex = _rootOverridesSceneIndex =
        UsdImagingRootOverridesSceneIndex::New(sceneIndex);

    return sceneIndex;
}

void
UsdImagingGLEngine::_CreateUsdImagingSceneIndices(
    HdContainerDataSourceHandle const &sceneIndexInputArgs)
{
    const UsdImagingSceneIndices sceneIndices =
        UsdImagingCreateSceneIndices(
            sceneIndexInputArgs,
            std::bind(
                &UsdImagingGLEngine::_AppendOverridesSceneIndices,
                this, std::placeholders::_1));

    _stageSceneIndex =
        sceneIndices.stageSceneIndex;
    _postInstancingNoticeBatchingSceneIndex =
        sceneIndices.postInstancingNoticeBatchingSceneIndex;
    _selectionSceneIndex =
        sceneIndices.selectionSceneIndex;

    HdSceneIndexBaseRefPtr sceneIndex =
        sceneIndices.finalSceneIndex;
    sceneIndex = _displayStyleSceneIndex =
        HdsiLegacyDisplayStyleOverrideSceneIndex::New(sceneIndex);

    _usdImagingFinalSceneIndex = sceneIndex;
}

void
UsdImagingGLEngine::_SetRenderDelegate(
    HdPluginRenderDelegateUniqueHandle &&renderDelegate,
    HdContainerDataSourceHandle const &sceneIndexInputArgs)
{
    // This relies on SetRendererPlugin to release the GIL...

    // Destruction
    _DestroyHydraObjects();

    _isPopulated = false;

    // Creation
    // Use the new render delegate.
    _renderDelegate = std::move(renderDelegate);

    {
        // Use the render delegate ptr (rather than 'this' ptr) for generating
        // the unique id.
        const std::string renderInstanceId =
            TfStringPrintf("UsdImagingGLEngine_%s_%p",
                           _renderDelegate.GetPluginId().GetText(),
                           (void *) _renderDelegate.Get());

        _appSceneIndices =
            UsdImagingGLEngine_Impl::_AppSceneIndices::New(
                TfToken(renderInstanceId));

        // Recreate the render index
        _renderIndex.reset(
            HdRenderIndex::New(
                _renderDelegate.Get(), {&_hgiDriver}, renderInstanceId));
    }

    if (UseUsdImagingSceneIndex()) {
        _CreateUsdImagingSceneIndices(sceneIndexInputArgs);
        _renderIndex->InsertSceneIndex(
            _usdImagingFinalSceneIndex, _sceneDelegateId);
    } else {
        _sceneDelegate = std::make_unique<UsdImagingDelegate>(
                _renderIndex.get(), _sceneDelegateId);

        _sceneDelegate->SetDisplayUnloadedPrimsWithBounds(
            _displayUnloadedPrimsWithBounds);
    }

    if (_allowAsynchronousSceneProcessing) {
        if (HdSceneIndexBaseRefPtr const si = _GetTerminalSceneIndex()) {
            si->SystemMessage(HdSystemMessageTokens->asyncAllow, nullptr);
        }
    }

    if (_GetUseTaskControllerSceneIndex()) {
        const SdfPath taskControllerPath =
            _ComputeControllerPath(_renderDelegate);
        HdxTaskControllerSceneIndex::Parameters params {
            taskControllerPath,
            [renderDelegate = _renderDelegate.Get()](const TfToken &name) {
                return renderDelegate->GetDefaultAovDescriptor(name); },
            _renderIndex.get()->GetRenderDelegate()->RequiresStormTasks(),
            _gpuEnabled
        };
        _taskControllerSceneIndex = HdxTaskControllerSceneIndex::New(params);
        _taskControllerSceneIndex->SetSelectionColor(_selectionColor);
        _renderIndex->InsertSceneIndex(
            _taskControllerSceneIndex,
            taskControllerPath,
            /* needsPrefixing = */ false);
    } else {
        _taskController = std::make_unique<HdxTaskController>(
            _renderIndex.get(),
            _ComputeControllerPath(_renderDelegate),
            _gpuEnabled);
    }

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
    static const TfToken candidateAovs[] =
        { HdAovTokens->primId,
          HdAovTokens->depth,
          HdAovTokens->normal,
          HdAovTokens->Neye,
          HdAovTokensMakePrimvar(TfToken("st")) };

    if (_renderer) {
        HdLegacyRenderControlInterface * const renderControl =
            _renderer->GetLegacyRenderControl();
        if (!renderControl) {
            return {};
        }
        TfTokenVector aovs = { HdAovTokens->color };
        for (auto const& aov : candidateAovs) {
            const HdFormat format =
                renderControl->GetDefaultAovDescriptor(aov).format;
            if (format != HdFormatInvalid) {
                aovs.push_back(aov);
            }
        }
        return aovs;
    } else if (_renderDelegate) {
        TfTokenVector aovs = { HdAovTokens->color };
        for (auto const& aov : candidateAovs) {
            const HdFormat format =
                _renderDelegate->GetDefaultAovDescriptor(aov).format;
            if (format != HdFormatInvalid) {
                aovs.push_back(aov);
            }
        }
        return aovs;
    }
    return {};
}

bool
UsdImagingGLEngine::SetRendererAov(TfToken const &id)
{
    if (ARCH_UNLIKELY(!_HasRenderer())) {
        return false;
    }

    if (_renderIndex) {
        if (!_renderIndex->IsBprimTypeSupported(
                HdPrimTypeTokens->renderBuffer)) {
            return false;
        }
    }

    TF_PY_ALLOW_THREADS_IN_SCOPE();

    if (_taskControllerSceneIndex) {
        _taskControllerSceneIndex->SetRenderOutputs({id});
    } else if (_taskController) {
        _taskController->SetRenderOutputs({id});
    } else {
        TF_CODING_ERROR("No task controller or task controller scene index.");
    }
    return true;
}

bool
UsdImagingGLEngine::SetRendererAovs(TfTokenVector const &ids)
{
    if (ARCH_UNLIKELY(!_HasRenderer())) {
        return false;
    }

    if (_renderIndex) {
        if (!_renderIndex->IsBprimTypeSupported(
                HdPrimTypeTokens->renderBuffer)) {
            return false;
        }
    }

    TF_PY_ALLOW_THREADS_IN_SCOPE();

    if (_taskControllerSceneIndex) {
        _taskControllerSceneIndex->SetRenderOutputs(ids);
    } else if (_taskController) {
        _taskController->SetRenderOutputs(ids);
    } else {
        TF_CODING_ERROR("No task controller or task controller scene index.");
    }
    return true;
}

HgiTextureHandle
UsdImagingGLEngine::GetAovTexture(
    TfToken const& name) const
{
    if (_renderer) {
        if (HdLegacyRenderControlInterface * const renderControl =
                _renderer->GetLegacyRenderControl()) {
            VtValue aov;
            if (renderControl->GetTaskContextData(name, &aov)) {
                if (aov.IsHolding<HgiTextureHandle>()) {
                    return aov.Get<HgiTextureHandle>();
                }
            }
        }
    } else if (_engine) {
        VtValue aov;
        if (_engine->GetTaskContextData(name, &aov)) {
            if (aov.IsHolding<HgiTextureHandle>()) {
                return aov.Get<HgiTextureHandle>();
            }
        }
    }

    return {};
}

HdRenderBuffer*
UsdImagingGLEngine::GetAovRenderBuffer(TfToken const& name) const
{
    if (_renderer) {
        if (!_taskControllerSceneIndex) {
            TF_CODING_ERROR("No task controller scene index.");
            return nullptr;
        }

        if (HdLegacyRenderControlInterface * const renderControl =
                _renderer->GetLegacyRenderControl()) {
            const SdfPath path =
                _taskControllerSceneIndex->GetRenderBufferPath(name);
            return renderControl->GetRenderBuffer(path);
        }
        return nullptr;
    } else if (_renderIndex) {
        if (_taskControllerSceneIndex) {
            const SdfPath path =
                _taskControllerSceneIndex->GetRenderBufferPath(name);
            return dynamic_cast<HdRenderBuffer*>(
                _renderIndex->GetBprim(
                    HdPrimTypeTokens->renderBuffer, path));
        } else if (_taskController) {
            return _taskController->GetRenderOutput(name);
        } else {
            TF_CODING_ERROR(
                "No task controller or task controller scene index.");
            return nullptr;
        }
    }

    return nullptr;
}

UsdImagingGLRendererSettingsList
UsdImagingGLEngine::GetRendererSettingsList() const
{
    HdRenderSettingDescriptorList descriptors;
    
    if (_renderer) {
        if (HdLegacyRenderControlInterface * const renderControl =
                _renderer->GetLegacyRenderControl()) {
            descriptors = renderControl->GetRenderSettingDescriptors();
        }
    } else if (_renderDelegate) {
        descriptors = _renderDelegate->GetRenderSettingDescriptors();
    }

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
    if (_renderer) {
        if (HdLegacyRenderControlInterface * const renderControl =
                _renderer->GetLegacyRenderControl()) {
            return renderControl->GetRenderSetting(id);
        }
    } else if (_renderDelegate) {
        return _renderDelegate->GetRenderSetting(id);
    }

    return {};
}

void
UsdImagingGLEngine::SetRendererSetting(TfToken const& id, VtValue const& value)
{
    if (_renderer) {
        TF_PY_ALLOW_THREADS_IN_SCOPE();
        if (HdLegacyRenderControlInterface * const renderControl =
                _renderer->GetLegacyRenderControl()) {
            renderControl->SetRenderSetting(id, value);
        }
    } else if (_renderDelegate) {
        TF_PY_ALLOW_THREADS_IN_SCOPE();
        _renderDelegate->SetRenderSetting(id, value);
    }
}

SdfPath
UsdImagingGLEngine::GetActiveRenderPassPrimPath() const
{
    HdSceneIndexBaseRefPtr const terminalSceneIndex =
        _GetTerminalSceneIndex();
    if (ARCH_UNLIKELY(!terminalSceneIndex)) {
        return SdfPath::EmptyPath();
    }

    SdfPath activeRenderPassPath;
    if (HdUtils::HasActiveRenderPassPrim(
            terminalSceneIndex, &activeRenderPassPath)) {
        return activeRenderPassPath;
    }

    return SdfPath::EmptyPath();
}

SdfPath
UsdImagingGLEngine::GetActiveRenderSettingsPrimPath() const
{
    HdSceneIndexBaseRefPtr const terminalSceneIndex =
        _GetTerminalSceneIndex();
    if (ARCH_UNLIKELY(!terminalSceneIndex)) {
        return SdfPath::EmptyPath();
    }

    SdfPath activeRenderSettingsPath;
    if (HdUtils::HasActiveRenderSettingsPrim(
            terminalSceneIndex, &activeRenderSettingsPath)) {
        return activeRenderSettingsPath;
    }

    return SdfPath::EmptyPath();
}

/* static */
SdfPathVector
UsdImagingGLEngine::GetAvailableRenderSettingsPrimPaths(UsdPrim const& root)
{
    // UsdRender OM uses the convention that all render settings prims must
    // live under /Render.
    static const SdfPath renderRoot("/Render");

    const auto stage = root.GetStage();

    SdfPathVector paths;
    if (UsdPrim render = stage->GetPrimAtPath(renderRoot)) {
        for (const UsdPrim child : render.GetChildren()) {
            if (child.IsA<UsdRenderSettings>()) {
                paths.push_back(child.GetPrimPath());
            }
        }
    }
    return paths;
}

void
UsdImagingGLEngine::SetActiveRenderPassPrimPath(SdfPath const &path)
{
    if (ARCH_UNLIKELY(!_appSceneIndices)) {
        return;
    }

    TF_PY_ALLOW_THREADS_IN_SCOPE();

    auto &sgsi = _appSceneIndices->sceneGlobalsSceneIndex;
    if (ARCH_UNLIKELY(!sgsi)) {
        return;
    }

    sgsi->SetActiveRenderPassPrimPath(path);
}

void
UsdImagingGLEngine::SetActiveRenderSettingsPrimPath(SdfPath const &path)
{
    if (ARCH_UNLIKELY(!_appSceneIndices)) {
        return;
    }

    TF_PY_ALLOW_THREADS_IN_SCOPE();

    auto &sgsi = _appSceneIndices->sceneGlobalsSceneIndex;
    if (ARCH_UNLIKELY(!sgsi)) {
        return;
    }

    sgsi->SetActiveRenderSettingsPrimPath(path);
}

void UsdImagingGLEngine::_SetSceneGlobalsCurrentFrame(UsdTimeCode const &time)
{
    if (ARCH_UNLIKELY(!_appSceneIndices)) {
        return;
    }
    auto &sgsi = _appSceneIndices->sceneGlobalsSceneIndex;
    if (ARCH_UNLIKELY(!sgsi)) {
        return;
    }

    sgsi->SetCurrentFrame(time.GetValue());
}

void
UsdImagingGLEngine::SetEnablePresentation(bool enabled)
{
    if (ARCH_UNLIKELY(!_HasRenderer())) {
        return;
    }

    TF_PY_ALLOW_THREADS_IN_SCOPE();

    if (_taskControllerSceneIndex) {
        _taskControllerSceneIndex->SetEnablePresentation(enabled);
    } else if (_taskController) {
        _taskController->SetEnablePresentation(enabled);
    } else {
        TF_CODING_ERROR("No task controller or task controller scene index.");
    }

}

void
UsdImagingGLEngine::SetPresentationOutput(
    TfToken const &api,
    VtValue const &framebuffer)
{
    if (ARCH_UNLIKELY(!_HasRenderer())) {
        return;
    }

    TF_PY_ALLOW_THREADS_IN_SCOPE();

    _userFramebuffer = framebuffer;
    if (_taskControllerSceneIndex) {
        _taskControllerSceneIndex->SetPresentationOutput(api, framebuffer);
    } else if (_taskController) {
        _taskController->SetPresentationOutput(api, framebuffer);
    } else {
        TF_CODING_ERROR("No task controller or task controller scene index.");
    }
}

// ---------------------------------------------------------------------
// Command API
// ---------------------------------------------------------------------

HdCommandDescriptors
UsdImagingGLEngine::GetRendererCommandDescriptors() const
{
    if (_renderer) {
        if (HdLegacyRenderControlInterface * const renderControl =
                _renderer->GetLegacyRenderControl()) {
            return renderControl->GetCommandDescriptors();
        }
        return {};
    } if (_renderDelegate) {
        return _renderDelegate->GetCommandDescriptors();
    }

    return {};
}

bool
UsdImagingGLEngine::InvokeRendererCommand(
    const TfToken &command, const HdCommandArgs &args) const
{
    if (_renderer) {
        if (HdLegacyRenderControlInterface * const renderControl =
                _renderer->GetLegacyRenderControl()) {
            return renderControl->InvokeCommand(command, args);
        }
        return false;
    } if (_renderDelegate) {
        TF_PY_ALLOW_THREADS_IN_SCOPE();
        return _renderDelegate->InvokeCommand(command, args);
    }

    return false;
}

// ---------------------------------------------------------------------
// Control of background rendering threads.
// ---------------------------------------------------------------------
bool
UsdImagingGLEngine::IsPauseRendererSupported() const
{
    if (_renderer) {
        if (HdLegacyRenderControlInterface * const renderControl =
                _renderer->GetLegacyRenderControl()) {
            return renderControl->IsPauseSupported();
        }
        return false;
    } else if (_renderDelegate) {
        return _renderDelegate->IsPauseSupported();
    }

    return false;
}

bool
UsdImagingGLEngine::PauseRenderer()
{
    if (_renderer) {
        if (HdLegacyRenderControlInterface * const renderControl =
                _renderer->GetLegacyRenderControl()) {
            return renderControl->Pause();
        }
        return false;
    } else if (_renderDelegate) {
        TF_PY_ALLOW_THREADS_IN_SCOPE();
        return _renderDelegate->Pause();
    }

    return false;
}

bool
UsdImagingGLEngine::ResumeRenderer()
{
    if (_renderer) {
        if (HdLegacyRenderControlInterface * const renderControl =
                _renderer->GetLegacyRenderControl()) {
            return renderControl->Resume();
        }
        return false;
    } else if (_renderDelegate) {
        TF_PY_ALLOW_THREADS_IN_SCOPE();
        return _renderDelegate->Resume();
    }

    return false;
}

bool
UsdImagingGLEngine::IsStopRendererSupported() const
{
    if (_renderer) {
        if (HdLegacyRenderControlInterface * const renderControl =
                _renderer->GetLegacyRenderControl()) {
            return renderControl->IsStopSupported();
        }
        return false;
    } else if (_renderDelegate) {
        return _renderDelegate->IsStopSupported();
    }

    return false;
}

bool
UsdImagingGLEngine::StopRenderer()
{
    if (_renderer) {
        if (HdLegacyRenderControlInterface * const renderControl =
                _renderer->GetLegacyRenderControl()) {
            return renderControl->Stop();
        }
        TF_WARN("Stop renderer not supported by HdRenderer");
        return false;
    } else if (_renderDelegate) {
        TF_PY_ALLOW_THREADS_IN_SCOPE();
        return _renderDelegate->Stop();
    }

    return false;
}

bool
UsdImagingGLEngine::RestartRenderer()
{
    if (_renderer) {
        if (HdLegacyRenderControlInterface * const renderControl =
                _renderer->GetLegacyRenderControl()) {
            return renderControl->Restart();
        }
        return false;
    } else if (_renderDelegate) {
        TF_PY_ALLOW_THREADS_IN_SCOPE();
        return _renderDelegate->Restart();
    }

    return false;
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
    if (ARCH_UNLIKELY(!_HasRenderer()) ||
        !IsColorCorrectionCapable()) {
        return;
    }

    TF_PY_ALLOW_THREADS_IN_SCOPE();

    HdxColorCorrectionTaskParams hdParams;
    hdParams.colorCorrectionMode = colorCorrectionMode;
    hdParams.displayOCIO = ocioDisplay.GetString();
    hdParams.viewOCIO = ocioView.GetString();
    hdParams.colorspaceOCIO = ocioColorSpace.GetString();
    hdParams.looksOCIO = ocioLook.GetString();

    if (_taskControllerSceneIndex) {
        _taskControllerSceneIndex->SetColorCorrectionParams(hdParams);
    } else if (_taskController) {
        _taskController->SetColorCorrectionParams(hdParams);
    } else {
        TF_CODING_ERROR("No task controller or task controller scene index.");
    }
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
    if (_renderer) {
        if (HdLegacyRenderControlInterface * const renderControl =
                _renderer->GetLegacyRenderControl()) {
            return renderControl->GetRenderStats();
        }
    } else if (_renderDelegate) {
        return _renderDelegate->GetRenderStats();
    }

    return {};
}

Hgi*
UsdImagingGLEngine::GetHgi()
{
    if (ARCH_UNLIKELY(!_HasRenderer())) {
        return nullptr;
    }

    return _hgi.get();
}

//----------------------------------------------------------------------------
// Private/Protected
//----------------------------------------------------------------------------

HdRenderIndex *
UsdImagingGLEngine::_GetRenderIndex() const
{
    return _renderIndex.get();
}

void
UsdImagingGLEngine::_Execute(const UsdImagingGLRenderParams &params,
                             const SdfPathVector &taskPaths)
{
    {
        // Release the GIL before calling into hydra, in case any hydra plugins
        // call into python.
        TF_PY_ALLOW_THREADS_IN_SCOPE();
        if (_renderer) {
            if (HdLegacyRenderControlInterface * const renderControl =
                    _renderer->GetLegacyRenderControl()) {
                renderControl->Execute(taskPaths);
            }
        } else if (_engine) {
            _engine->Execute(_renderIndex.get(), taskPaths);
        }
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

    if (UseUsdImagingSceneIndex()) {
        // The UsdImagingStageSceneIndex has no complexity opinion.
        // We force the value here upon all prims.
        _displayStyleSceneIndex->SetRefineLevel({true, refineLevel});

        _ScopedHydraNoticeBatch noticeBatch(
                _postInstancingNoticeBatchingSceneIndex);

        _stageSceneIndex->ApplyPendingUpdates();
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
        params.enableLighting =  renderParams.enableLighting;
    }

    params.depthBiasUseDefault = true;
    params.depthFunc           = HdCmpFuncLess;
    params.cullStyle           = USD_2_HD_CULL_STYLE[
        (size_t)renderParams.cullStyle];

    if (renderParams.alphaThreshold < 0.0) {
        // If no alpha threshold is set, use default of 0.1.
        params.alphaThreshold = 0.1f;
    } else {
        params.alphaThreshold = renderParams.alphaThreshold;
    }

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
        if (pluginDescs[i].displayName == defaultRendererDisplayName
        ||  pluginDescs[i].id == defaultRendererDisplayName) {
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
    if (UseUsdImagingSceneIndex()) {
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

HdSceneIndexBaseRefPtr
UsdImagingGLEngine::_GetTerminalSceneIndex() const
{
    if (_terminalSceneIndex) {
        return _terminalSceneIndex;
    }

    if (_renderIndex) {
        return _renderIndex->GetTerminalSceneIndex();
    }

    return nullptr;
}

bool
UsdImagingGLEngine::_HasRenderer() const
{
    return bool(_renderDelegate) || bool(_renderer);
}

bool
UsdImagingGLEngine::PollForAsynchronousUpdates() const
{
    class _Observer : public HdSceneIndexObserver
    {
    public:

        void PrimsAdded(
                const HdSceneIndexBase &sender,
                const AddedPrimEntries &entries) override
        {

            _changed = true;
        }

        void PrimsRemoved(
            const HdSceneIndexBase &sender,
            const RemovedPrimEntries &entries) override
        {
            _changed = true;
        }

        void PrimsDirtied(
            const HdSceneIndexBase &sender,
            const DirtiedPrimEntries &entries) override
        {
            _changed = true;
        }

        void PrimsRenamed(
            const HdSceneIndexBase &sender,
            const RenamedPrimEntries &entries) override
        {
            _changed = true;
        }

        bool IsChanged() { return _changed; }
    private:
        bool _changed = false;
    };

    if (_allowAsynchronousSceneProcessing) {
        if (HdSceneIndexBaseRefPtr si = _GetTerminalSceneIndex()) {
            _Observer ob;
            si->AddObserver(HdSceneIndexObserverPtr(&ob));
            si->SystemMessage(HdSystemMessageTokens->asyncPoll, nullptr);
            si->RemoveObserver(HdSceneIndexObserverPtr(&ob));
            return ob.IsChanged();
        }
    }

    return false;
}

bool
UsdImagingGLEngine::UseUsdImagingSceneIndex()
{
    static bool result =
        HdRenderIndex::IsSceneIndexEmulationEnabled() &&
        TfGetEnvSetting(USDIMAGINGGL_ENGINE_ENABLE_SCENE_INDEX);

    // Provide a single-shot deprecation notice.
    if (!result) {
        static std::once_flag once;
        std::call_once(once, []() {
            if (TfGetEnvSetting(
                USDIMAGINGGL_ENGINE_ENABLE_SCENE_INDEX_DEPRECATION_WARNING)) {
                TF_WARN("*** Warning: USDIMAGINGGL_ENGINE_ENABLE_SCENE_INDEX "
                    "is overridden to 0.  This code path is deprecated, "
                    "and will be removed in a future release of USD.  This "
                    "deprecation notice can be suppressed by setting "
                   "USDIMAGINGGL_ENGINE_ENABLE_SCENE_INDEX_DEPRECATION_WARNING"
                    "to '0'. ***");
            }
        });
    }

    return result;
}

PXR_NAMESPACE_CLOSE_SCOPE
