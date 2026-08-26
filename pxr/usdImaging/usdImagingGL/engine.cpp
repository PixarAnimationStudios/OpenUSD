//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/usdImaging/usdImagingGL/engine.h"

#include "pxr/usd/sdf/path.h"
#include "pxr/usdImaging/usdImaging/delegate.h"
#include "pxr/usdImaging/usdImaging/legacyRenderSettingsSceneIndex.h"
#include "pxr/usdImaging/usdImaging/selectionSceneIndex.h"
#include "pxr/usdImaging/usdImaging/stageSceneIndex.h"
#include "pxr/usdImaging/usdImaging/rootOverridesSceneIndex.h"
#include "pxr/usdImaging/usdImaging/sceneIndex.h"
#include "pxr/usdImaging/usdImaging/sceneIndices.h"
#include "pxr/usdImaging/usdImaging/sceneIndexCreateArgsSchema.h"
#include "pxr/usdImaging/usdImaging/stageSceneIndexInterface.h"

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
#include "pxr/imaging/hd/renderSettingDescriptorSchema.h"
#include "pxr/imaging/hd/retainedDataSource.h"
#include "pxr/imaging/hd/sceneGlobalsSchema.h"
#include "pxr/imaging/hd/sceneIndexCreateArgsSchema.h"
#include "pxr/imaging/hd/sceneIndexPluginRegistry.h"
#include "pxr/imaging/hd/systemMessages.h"
#include "pxr/imaging/hd/utils.h"
#include "pxr/imaging/hdsi/applicationRenderSettingsSceneIndex.h"
#include "pxr/imaging/hdsi/domeLightCameraVisibilitySceneIndex.h"
#include "pxr/imaging/hdsi/legacyDisplayStyleOverrideSceneIndex.h"
#include "pxr/imaging/hdsi/prefixPathPruningSceneIndex.h"
#include "pxr/imaging/hdsi/primTypeAndPathPruningSceneIndex.h"
#include "pxr/imaging/hdsi/sceneMaterialPruningSceneIndex.h"
#include "pxr/imaging/hdsi/sceneGlobalsSceneIndex.h"
#include "pxr/imaging/hdx/pickTask.h"
#include "pxr/imaging/hdx/task.h"
#include "pxr/imaging/hdx/taskControllerSceneIndex.h"
#include "pxr/imaging/hdx/tokens.h"

#include "pxr/imaging/hgi/hgi.h"
#include "pxr/imaging/hgi/tokens.h"

#include "pxr/base/plug/registry.h"

#include "pxr/base/tf/diagnostic.h"
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

TF_DEFINE_ENV_SETTING(
    USDIMAGINGGL_ENGINE_ENABLE_CACHING_SCENE_INDEX, false,
    "Use caching scene index.");

TF_DEFINE_ENV_SETTING(
    USDIMAGINGGL_ENGINE_ENABLE_RENDER_SETTINGS_SCENE_INDEX, false,
    "Communicate render settings in-band through the application render "
    "settings scene index rather than through the "
    "HdLegacyRenderControlInterface.");

TF_DEFINE_ENV_SETTING(
    USDIMAGINGGL_ENGINE_ENABLE_EXEC_SCENE_INDEX, false,
    "Inserts an initial scene index which provides values computed by exec. "
    "The exec scene index is added by a merging scene index in the overrides "
    "scene index callback. Values provided by the exec scene index override "
    "values in the input scene. To use this feature, usdImaging must be "
    "built with PXR_BUILD_EXEC=ON.");

namespace UsdImagingGLEngine_Impl
{

// Struct that holds application scene indices created via the
// scene index plugin registration callback facility.
//
// It is also in charge of registering the callback with the scene index
// plugin registration and tracking itself so that it can populate itself
// during the callback.
//
struct _AppSceneIndices
{
    HdsiSceneGlobalsSceneIndexRefPtr
        sceneGlobalsSceneIndex;
    HdsiDomeLightCameraVisibilitySceneIndexRefPtr
        domeLightCameraVisibilitySceneIndex;
    HdsiSceneMaterialPruningSceneIndexRefPtr
        sceneMaterialPruningSceneIndex;
    HdsiApplicationRenderSettingsSceneIndexRefPtr
        applicationRenderSettingsSceneIndex;

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
        const HdSceneIndexBaseRefPtr &inputScene,
        const HdContainerDataSourceHandle &createArgs)
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

        static const bool enableRenderSettingsSceneIndex =
            TfGetEnvSetting(
                USDIMAGINGGL_ENGINE_ENABLE_RENDER_SETTINGS_SCENE_INDEX);
        if (enableRenderSettingsSceneIndex) {
            sceneIndex =
                applicationRenderSettingsSceneIndex =
                    HdsiApplicationRenderSettingsSceneIndex::New(
                        sceneIndex, createArgs);
        }

        return sceneIndex;
    }

    static HdSceneIndexBaseRefPtr _AppendCallback(
        const std::string &renderInstanceId,
        const HdSceneIndexBaseRefPtr &inputScene,
        const HdContainerDataSourceHandle &createArgs)
    {
        _AppSceneIndicesSharedPtr const instance =
            _GetTracker().GetInstance(renderInstanceId);
        if (!instance) {
            TF_CODING_ERROR("Did not find appSceneIndices instance for %s,",
                            renderInstanceId.c_str());
            return inputScene;
        }
        return instance->_Append(inputScene, createArgs);
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
_IsEnabledTerminalCachingSceneIndex()
{
    static bool result =
        TfGetEnvSetting(USDIMAGINGGL_ENGINE_ENABLE_CACHING_SCENE_INDEX);

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
    : _hgi(nullptr)
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
    if (driver.name == HgiTokens->renderDriver) {
        _hgi = driver.driver.GetWithDefault<Hgi*>(nullptr);
        if (_hgi && !_gpuEnabled) {
            TF_WARN("Trying to share GPU resources while disabling the GPU.");
            _gpuEnabled = true;
        }
    }

    // The renderer, task controller scene index, and
    // _sceneDelegate/_sceneIndex are initialized by the plugin system.
    if (!SetRendererPlugin(
            !rendererPluginId.IsEmpty()
                ? rendererPluginId
                : _GetDefaultRendererPluginId())) {
        TF_CODING_ERROR("No renderer plugins found!");
    }
}

void
UsdImagingGLEngine::_DestroyHydraObjects()
{
    TRACE_FUNCTION();

    // Destroy objects in opposite order of construction.
    {
        TRACE_SCOPE("Destroy UsdImaging delegate");

        _sceneDelegate = nullptr;

        // Reset flag.
        _setActiveRenderSettingsPrimPathCalled = false;
    }

    // We are not removing _usdImagingFinalSceneIndex from _mergingSceneIndex
    // here. Destroying the renderer (and the render index it owns) tears down
    // the downstream observers, so explicit removal is unnecessary.

    {
        TRACE_SCOPE("Destroy renderer");
        _renderer = nullptr;
    }

    {
        TRACE_SCOPE("Destroying terminal scene index");

        _terminalSceneIndex = nullptr;
        _cachingSceneIndex = nullptr;
        _appSceneIndices = nullptr;
        _mergingSceneIndex = nullptr;
    }

    {
        TRACE_SCOPE("Destroy UsdImaging scene indices");

        _usdImagingSceneIndex = nullptr;
        _displayStyleSceneIndex = nullptr;
        _legacyRenderSettingsSceneIndex = nullptr;
        _rootOverridesSceneIndex = nullptr;
        _lightPruningSceneIndex = nullptr;
        _noticeBatchingStageSceneIndex = nullptr;
        _execStageSceneIndex = nullptr;
    }

    {
        TRACE_SCOPE("Task controller scene index");

        _taskControllerSceneIndex = TfNullPtr;
    }

    _sceneIndexCreateArgs = nullptr;

    _isPopulated = false;
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
    if (!_renderer) {
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
            _usdImagingSceneIndex->SetTime(params.frame);
            if (_execStageSceneIndex) {
                _execStageSceneIndex->SetTime(params.frame);
                _noticeBatchingStageSceneIndex->Flush();
            }
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
                _usdImagingSceneIndex->
                    GetPostInstancingNoticeBatchingSceneIndex());
            _usdImagingSceneIndex->SetStage(stage);
            if (_execStageSceneIndex) {
                _execStageSceneIndex->SetStage(stage);
            }
        } else {
            TF_VERIFY(_sceneDelegate);
            _sceneDelegate->SetUsdDrawModesEnabled(
                params.enableUsdDrawModes && _enableUsdDrawModes);
            _sceneDelegate->Populate(
                stage->GetPrimAtPath(_rootPath),
                _excludedPrimPaths);
            _sceneDelegate->SetInvisedPrimPaths(_invisedPrimPaths);
            if (!_setActiveRenderSettingsPrimPathCalled) {
                _SetActiveRenderSettingsPrimFromStageMetadata(stage);
            }
        }
        _isPopulated = true;
    }
}

void
UsdImagingGLEngine::_PrepareRender(const UsdImagingGLRenderParams &params)
{
    if (!_taskControllerSceneIndex) {
        TF_CODING_ERROR("No task controller scene index.");
        return;
    }

    TfTokenVector renderTags;
    _ComputeRenderTags(params, &renderTags);

    _taskControllerSceneIndex->SetFreeCameraClipPlanes(params.clipPlanes);
    _taskControllerSceneIndex->SetRenderTags(renderTags);
    _taskControllerSceneIndex->SetRenderParams(
        _MakeHydraUsdImagingGLRenderParams(params));
}

void
UsdImagingGLEngine::_SetActiveRenderSettingsPrimFromStageMetadata(
    UsdStageWeakPtr stage)
{
    if (UseUsdImagingSceneIndex()) {
        TF_CODING_ERROR(
            "The canonical source of the active render settings prim path of "
            "the UsdStage is the UsdImagingSceneIndex.");
    }
    
    if (!TF_VERIFY(stage)) {
        return;
    }

    if (!stage->HasAuthoredMetadata(UsdRenderTokens->renderSettingsPrimPath)) {
        return;
    }

    std::string pathStr;
    stage->GetMetadata(UsdRenderTokens->renderSettingsPrimPath, &pathStr);

    if (pathStr.empty()) {
        return;
    }

    // Add the delegateId prefix since the scene globals scene index is
    // inserted into the merging scene index.
    SetActiveRenderSettingsPrimPath(
        SdfPath(pathStr).ReplacePrefix(
            SdfPath::AbsoluteRootPath(), _sceneDelegateId));
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

    const VtValue v = 
        GetRendererSetting(HdRenderSettingsTokens->domeLightCameraVisibility);

    // The absence of a setting in the map is the same as camera visibility
    // being on.
    const bool domeLightCamVisSetting = v.GetWithDefault<bool>(true);
    if (_domeLightCameraVisibility == domeLightCamVisSetting) {
        return;
    }

    // Camera visibility state changed, so we need to mark any dome lights
    // as dirty to ensure they have the proper state on all backends.
    _domeLightCameraVisibility = domeLightCamVisSetting;

    if (!_appSceneIndices) {
        TF_CODING_ERROR("Expected _appSceneIndices.");
        return;
    }

    HdsiDomeLightCameraVisibilitySceneIndexRefPtr const &si =
        _appSceneIndices->domeLightCameraVisibilitySceneIndex;
    if (!si) {
        TF_CODING_ERROR("Expected Dome light camera visibility scene index.");
        return;
    }

    si->SetDomeLightCameraVisibility(domeLightCamVisSetting);
}

void
UsdImagingGLEngine::_SetBBoxParams(
    const BBoxVector& bboxes,
    const GfVec4f& bboxLineColor,
    float bboxLineDashSize)
{
    if (ARCH_UNLIKELY(!_renderer)) {
        return;
    }

    if (!_taskControllerSceneIndex) {
        TF_CODING_ERROR("No task controller scene index.");
        return;
    }

    HdxBoundingBoxTaskParams params;
    params.bboxes = bboxes;
    params.color = bboxLineColor;
    params.dashSize = bboxLineDashSize;

    _taskControllerSceneIndex->SetBBoxParams(params);
}

void
UsdImagingGLEngine::RenderBatch(
    const SdfPathVector& paths,
    const UsdImagingGLRenderParams& params)
{
    if (ARCH_UNLIKELY(!_renderer)) {
        return;
    }

    if (!_taskControllerSceneIndex) {
        TF_CODING_ERROR("No task controller scene index.");
        return;
    }

    TF_PY_ALLOW_THREADS_IN_SCOPE();

    _UpdateHydraCollection(&_renderCollection, paths, params);
    _taskControllerSceneIndex->SetCollection(_renderCollection);

    _PrepareRender(params);

    SetColorCorrectionSettings(params.colorCorrectionMode, params.ocioDisplay,
        params.ocioView, params.ocioColorSpace, params.ocioLook);

    _SetBBoxParams(params.bboxes, params.bboxLineColor, params.bboxLineDashSize);

    // XXX App sets the clear color via 'params' instead of setting up Aovs
    // that has clearColor in their descriptor. So for now we must pass this
    // clear color to the color AOV.
    _taskControllerSceneIndex->SetEnableSelection(params.highlight);

    HdAovDescriptor colorAovDesc =
        _taskControllerSceneIndex->GetRenderOutputSettings(HdAovTokens->color);
    if (colorAovDesc.format != HdFormatInvalid) {
        colorAovDesc.clearValue = VtValue(params.clearColor);
        _taskControllerSceneIndex->SetRenderOutputSettings(
            HdAovTokens->color, colorAovDesc);
    }

    _UpdateDomeLightCameraVisibility();

    const VtValue selectionValue(_selTracker);
    if (HdLegacyRenderControlInterface * const renderControl =
            _GetLegacyRenderControl()) {
        renderControl->SetTaskContextData(HdxTokens->selectionState, selectionValue);
    }
    _Execute(params, _taskControllerSceneIndex->GetRenderingTaskPaths());
}

void
UsdImagingGLEngine::Render(
    const UsdPrim& root,
    const UsdImagingGLRenderParams &params)
{
    if (ARCH_UNLIKELY(!_renderer)) {
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
    HdLegacyRenderControlInterface * const renderControl =
        _GetLegacyRenderControl();
    if (!renderControl) {
        return true;
    }

    if (!_taskControllerSceneIndex) {
        TF_CODING_ERROR("No task controller scene index.");
        return true;
    }

    return renderControl->AreTasksConverged(
        _taskControllerSceneIndex->GetRenderingTaskPaths());
}

//----------------------------------------------------------------------------
// Root and Transform Visibility
//----------------------------------------------------------------------------

void
UsdImagingGLEngine::SetRootTransform(GfMatrix4d const& xf)
{
    if (ARCH_UNLIKELY(!_renderer)) {
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
    if (ARCH_UNLIKELY(!_renderer)) {
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
    if (ARCH_UNLIKELY(!_renderer)) {
        return;
    }

    if (!_taskControllerSceneIndex) {
        TF_CODING_ERROR("No task controller scene index.");
        return;
    }

    TF_PY_ALLOW_THREADS_IN_SCOPE();

    _taskControllerSceneIndex->SetRenderViewport(viewport);
}

void
UsdImagingGLEngine::SetFraming(CameraUtilFraming const& framing)
{
    if (ARCH_UNLIKELY(!_renderer)) {
        return;
    }

    if (!_taskControllerSceneIndex) {
        TF_CODING_ERROR("No task controller scene index.");
        return;
    }

    TF_PY_ALLOW_THREADS_IN_SCOPE();

    _taskControllerSceneIndex->SetFraming(framing);
}

void
UsdImagingGLEngine::SetOverrideWindowPolicy(
    const std::optional<CameraUtilConformWindowPolicy> &policy)
{
    if (ARCH_UNLIKELY(!_renderer)) {
        return;
    }

    if (!_taskControllerSceneIndex) {
        TF_CODING_ERROR("No task controller scene index.");
        return;
    }

    TF_PY_ALLOW_THREADS_IN_SCOPE();

    _taskControllerSceneIndex->SetOverrideWindowPolicy(policy);
}

void
UsdImagingGLEngine::SetRenderBufferSize(GfVec2i const& size)
{
    if (ARCH_UNLIKELY(!_renderer)) {
        return;
    }

    if (!_taskControllerSceneIndex) {
        TF_CODING_ERROR("No task controller scene index.");
        return;
    }

    TF_PY_ALLOW_THREADS_IN_SCOPE();

    _taskControllerSceneIndex->SetRenderBufferSize(size);
}

void
UsdImagingGLEngine::SetWindowPolicy(CameraUtilConformWindowPolicy policy)
{
    if (ARCH_UNLIKELY(!_renderer)) {
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
    if (ARCH_UNLIKELY(!_renderer)) {
        return;
    }

    if (!_taskControllerSceneIndex) {
        TF_CODING_ERROR("No task controller scene index.");
        return;
    }

    TF_PY_ALLOW_THREADS_IN_SCOPE();

    _taskControllerSceneIndex->SetCameraPath(id);

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
    if (ARCH_UNLIKELY(!_renderer)) {
        return;
    }

    if (!_taskControllerSceneIndex) {
        TF_CODING_ERROR("No task controller scene index.");
        return;
    }

    TF_PY_ALLOW_THREADS_IN_SCOPE();

    SdfPath freeCameraPath;

    _taskControllerSceneIndex->SetFreeCameraMatrices(viewMatrix, projectionMatrix);
    freeCameraPath = _taskControllerSceneIndex->GetFreeCameraPath();

    if (UseUsdImagingSceneIndex()) {
        if (_appSceneIndices) {
            if (auto& sgsi = _appSceneIndices->sceneGlobalsSceneIndex) {
                sgsi->SetPrimaryCameraPrimPath(freeCameraPath);
            }
        }
    }
    // XXX: Do not set camera for sampling on legacy delegate; this camera does
    // not actually exist in the usdImaging scene delegate!
}

void
UsdImagingGLEngine::SetLightingState(GlfSimpleLightingContextPtr const &src)
{
    if (ARCH_UNLIKELY(!_renderer)) {
        return;
    }

    if (!_taskControllerSceneIndex) {
        TF_CODING_ERROR("No task controller scene index.");
        return;
    }

    TF_PY_ALLOW_THREADS_IN_SCOPE();

    _taskControllerSceneIndex->SetLightingState(src);
}

void
UsdImagingGLEngine::SetLightingState(
    GlfSimpleLightVector const &lights,
    GlfSimpleMaterial const &material,
    GfVec4f const &sceneAmbient)
{
    if (ARCH_UNLIKELY(!_renderer)) {
        return;
    }

    if (!_taskControllerSceneIndex) {
        TF_CODING_ERROR("No task controller scene index.");
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

    _taskControllerSceneIndex->SetLightingState(
        _lightingContextForOpenGLState);
}

//----------------------------------------------------------------------------
// Selection Highlighting
//----------------------------------------------------------------------------

void
UsdImagingGLEngine::SetSelected(SdfPathVector const& paths)
{
    if (ARCH_UNLIKELY(!_renderer)) {
        return;
    }

    TF_PY_ALLOW_THREADS_IN_SCOPE();

    if (UseUsdImagingSceneIndex()) {
        _usdImagingSceneIndex->ClearSelection();

        for (const SdfPath &path : paths) {
            _usdImagingSceneIndex->AddSelection(path);
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
    if (ARCH_UNLIKELY(!_renderer)) {
        return;
    }

    TF_PY_ALLOW_THREADS_IN_SCOPE();

    if (UseUsdImagingSceneIndex()) {
        _usdImagingSceneIndex->ClearSelection();
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
    if (ARCH_UNLIKELY(!_renderer)) {
        return;
    }

    TF_PY_ALLOW_THREADS_IN_SCOPE();

    if (UseUsdImagingSceneIndex()) {
        _usdImagingSceneIndex->AddSelection(path);
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
    if (ARCH_UNLIKELY(!_renderer)) {
        return;
    }

    if (!_taskControllerSceneIndex) {
        TF_CODING_ERROR("No task controller scene index.");
        return;
    }

    TF_PY_ALLOW_THREADS_IN_SCOPE();

    _selectionColor = color;

    _taskControllerSceneIndex->SetSelectionColor(_selectionColor);
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
    HdSceneIndexBaseRefPtr terminalSceneIndex = _GetTerminalSceneIndex();
    if (!terminalSceneIndex) {
        return SdfPath();
    }
    const auto schema = HdInstancedBySchema::GetFromParent(
        terminalSceneIndex->GetPrim(sceneIndexPath).dataSource);
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

bool
UsdImagingGLEngine::TestIntersection(
    const PickParams& pickParams,
    const GfMatrix4d& viewMatrix,
    const GfMatrix4d& projectionMatrix,
    const UsdPrim& root,
    const UsdImagingGLRenderParams& params,
    IntersectionResultVector* outResults)
{
    if (ARCH_UNLIKELY(!_renderer)) {
        return false;
    }

    if (!_taskControllerSceneIndex) {
        TF_CODING_ERROR("No task controller scene index.");
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

    if (HdLegacyRenderControlInterface * const renderControl =
            _GetLegacyRenderControl()) {
        renderControl->SetTaskContextData(
            HdxPickTokens->pickParams, vtPickCtxParams);
    }
    _Execute(params, _taskControllerSceneIndex->GetPickingTaskPaths());

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
    const SdfPath sceneIndexPath = _GetSceneIndexPrimPathFromPrimId(primIdx);
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

UsdImagingGLEngine::DecodeResultVector
UsdImagingGLEngine::DecodeIntersections(const PickIdVector& pickIds)
{
    DecodeResultVector results;
    results.resize(pickIds.size());

    if (_sceneDelegate) {
        // Legacy scene delegate path: resolve each rprimPath individually
        for (size_t i = 0; i < pickIds.size(); ++i) {
            const auto [primIdx, instanceIdx] = pickIds[i];

            const SdfPath sceneIndexPath =
                _GetSceneIndexPrimPathFromPrimId(primIdx);
            if (sceneIndexPath.IsEmpty()) {
                continue;
            }

            DecodeResult& res = results[i];
            res.primPath = _sceneDelegate->GetScenePrimPath(
                sceneIndexPath, instanceIdx, &res.instancerContext);
        }
    } else {
        // Scene index path: use HdxPrimOriginInfo::FromPickHits which amortizes
        // instancer topology computation.
        std::vector<HdxPickHit> hits;
        hits.resize(pickIds.size());

        for (size_t i = 0; i < pickIds.size(); ++i) {
            const PickId& pickId = pickIds[i];
            HdxPickHit& hit = hits[i];
            hit.objectId = _GetSceneIndexPrimPathFromPrimId(pickId.primId);
            hit.instanceIndex = pickId.instanceId;
        }

        const std::vector<HdxPrimOriginInfo> infos =
            HdxPrimOriginInfo::FromPickHits(
                _GetTerminalSceneIndex(), hits);

        for (size_t i = 0; i < infos.size(); ++i) {
            DecodeResult& res = results[i];
            res.primPath = infos[i].GetFullPath();
            res.instancerContext = infos[i].ComputeInstancerContext();
        }
    }

    return results;
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
    if (ARCH_UNLIKELY(!_renderer)) {
        return {};
    }

    return _renderer.GetPluginId();
}

bool
UsdImagingGLEngine::SetRendererPlugin(TfToken const &id)
{
    const HdRendererCreateArgsSchema rendererCreateArgs(
        HdRendererCreateArgsSchema::Builder()
            .SetGpuEnabled(
                HdRetainedTypedSampledDataSource<bool>::New(_gpuEnabled))
            .SetDrivers(
                HdRetainedContainerDataSource::New(
                    HdRendererCreateArgsSchemaTokens->hgi,
                    HdRetainedTypedSampledDataSource<Hgi*>::New(
                        _GetOrCreateHgi())))
            .Build());

    HdRendererPluginRegistry &registry =
        HdRendererPluginRegistry::GetInstance();

    const TfToken resolvedId =
        id.IsEmpty()
            ? registry.GetDefaultPluginId(rendererCreateArgs)
            : id;

    if (_renderer && _renderer.GetPluginId() == resolvedId) {
        return true;
    }

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

    TF_PY_ALLOW_THREADS_IN_SCOPE();

    // Create all scene indices (usd imaging scene indices, task controller
    // scene index, filtering scene indices) that eventually feed into the
    // renderer which we create.
    if (!_CreateSceneIndicesAndRenderer(plugin, rendererCreateArgs)) {
        return false;
    }

    if (_allowAsynchronousSceneProcessing) {
        if (HdSceneIndexBaseRefPtr const si = _GetTerminalSceneIndex()) {
            si->SystemMessage(HdSystemMessageTokens->asyncAllow, nullptr);
        }
    }

    return true;
}

bool
UsdImagingGLEngine::_CreateSceneIndicesAndRenderer(
    HdRendererPluginHandle const &plugin,
    const HdRendererCreateArgsSchema &rendererCreateArgs)
{
    TRACE_FUNCTION();

    // Remember root overrides.
    const _RootOverrides rootOverrides =
        UseUsdImagingSceneIndex()
            ? _GetRootOverrides(_rootOverridesSceneIndex)
            : _GetRootOverrides(_sceneDelegate);

    // Destroy old structures.
    _DestroyHydraObjects();

    // Arguments passed to the constructors of various scene indices.
    HdContainerDataSourceHandle const sceneIndexCreateArgs =
        _GetSceneIndexCreateArgs(plugin);

    // Retain the create-args so that GetRendererSettingsList() can source the
    // renderer's advertised render settings from them.
    _sceneIndexCreateArgs = sceneIndexCreateArgs;

    // Create the merging scene index, all subsequent scene indices
    // and the renderer.
    _CreateSceneIndexChainAndRenderer(
        sceneIndexCreateArgs, plugin, rendererCreateArgs);
    
    if (!_renderer) {
        return false;
    }

    // Create the usd imaging scene indices.
    if (UseUsdImagingSceneIndex()) {
        TRACE_SCOPE("UsdImaging scene indices");

        // Setup Usd imaging scene indices.
        HdSceneIndexBaseRefPtr sceneIndex =
            _CreateUsdImagingSceneIndices(sceneIndexCreateArgs);
        _SetRootOverrides(rootOverrides, _rootOverridesSceneIndex);

        _mergingSceneIndex->InsertInputScenes({{sceneIndex , _sceneDelegateId }});
    } else {
        TRACE_SCOPE("UsdImaging scene delegate");

        HdRenderIndexAdapterSceneIndexRefPtr const adapter =
            HdRenderIndexAdapterSceneIndex::New(
                HdCreateOverlayContainerDataSource(
                    sceneIndexCreateArgs,
                    _GetSceneIndexCreateArgsFromLegacyRenderControl()));

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
                _GetLegacyRenderControl()) {
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

HdSceneIndexBaseRefPtr
UsdImagingGLEngine::_AppendOverridesSceneIndices(
    HdSceneIndexBaseRefPtr const &inputScene,
    HdContainerDataSourceHandle const &)
{
    HdSceneIndexBaseRefPtr sceneIndex = inputScene;

    if (TfGetEnvSetting(USDIMAGINGGL_ENGINE_ENABLE_EXEC_SCENE_INDEX)) {
        sceneIndex = _CreateExecSceneIndices(sceneIndex);
    }

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

    sceneIndex = _legacyRenderSettingsSceneIndex =
        UsdImagingLegacyRenderSettingsSceneIndex::New(sceneIndex);

    return sceneIndex;
}

HdSceneIndexBaseRefPtr
UsdImagingGLEngine::_CreateExecSceneIndices(
    const HdSceneIndexBaseRefPtr &inputScene)
{
    // Try to manufacture an instance of UsdExecImaging_StageSceneIndex from a
    // registered factory object. If this cannot be done, emit an error and do
    // not modify the input scene.

    const TfType execStageSceneIndexType =
        PlugRegistry::FindDerivedTypeByName<UsdImagingStageSceneIndexInterface>(
            "UsdExecImaging_StageSceneIndex");
    if (!TF_VERIFY(!execStageSceneIndexType.IsUnknown())) {
        return inputScene;
    }

    const auto *const execSceneIndexFactory =
        execStageSceneIndexType.GetFactory<
            UsdImagingStageSceneIndexInterfaceFactoryBase>();
    if (!TF_VERIFY(execSceneIndexFactory)) {
        return inputScene;
    }

    _execStageSceneIndex = execSceneIndexFactory->New();
    if (!TF_VERIFY(_execStageSceneIndex)) {
        return inputScene;
    }

    // Insert a merging scene index that merges the input scene with the
    // _execStageSceneIndex. The _execStageSceneIndex is added first so that its
    // data sources overshadow the corresponding data sources from the input
    // scene.
    const HdMergingSceneIndexRefPtr mergingSceneIndex =
        HdMergingSceneIndex::New();
    mergingSceneIndex->AddInputScene(
        _execStageSceneIndex,
        SdfPath::AbsoluteRootPath());
    mergingSceneIndex->AddInputScene(
        inputScene,
        SdfPath::AbsoluteRootPath());

    // Insert a notice batching scene index that batches all notices
    // originating from the UsdImagingStageSceneIndex and the
    // UsdExecImagingStageSceneIndex. This ensures the exec scene index
    // is able to refresh its exec request before notices are flushed
    // to downstream scene indices.
    _noticeBatchingStageSceneIndex = HdNoticeBatchingSceneIndex::New(
        mergingSceneIndex);
    _noticeBatchingStageSceneIndex->SetBatchingEnabled(true);

    return _noticeBatchingStageSceneIndex;
}

HdSceneIndexBaseRefPtr
UsdImagingGLEngine::_CreateUsdImagingSceneIndices(
    HdContainerDataSourceHandle const &sceneIndexCreateArgs)
{
    HdSceneIndexBaseRefPtr sceneIndex;

    UsdImagingSceneIndex::SceneIndexAppendCallbacks callbacks;
    callbacks.overridesSceneIndexCallback =
        std::bind(
            &UsdImagingGLEngine::_AppendOverridesSceneIndices,
            this, std::placeholders::_1, std::placeholders::_2);

    sceneIndex =
        _usdImagingSceneIndex =
            UsdImagingSceneIndex::New(sceneIndexCreateArgs, callbacks);

    sceneIndex =
        _displayStyleSceneIndex =
             HdsiLegacyDisplayStyleOverrideSceneIndex::New(sceneIndex);

    if (!_sceneDelegateId.IsAbsoluteRootPath()) {
        sceneIndex =
            HdPrefixingSceneIndex::New(sceneIndex, _sceneDelegateId);
    }
    
    return sceneIndex;
}

HdContainerDataSourceHandle
UsdImagingGLEngine::_GetSceneIndexCreateArgs(
    HdRendererPluginHandle const &plugin)
{
    HdContainerDataSourceHandle const rendererPluginArgs =
        plugin->GetSceneIndexCreateArgs();

    HdContainerDataSourceHandle const usdImagingArgs =
        HdRetainedContainerDataSource::New(
            UsdImagingSceneIndexCreateArgsSchema::GetSchemaToken(),
            UsdImagingSceneIndexCreateArgsSchema::Builder()
                .SetAddDrawModeSceneIndex(
                    HdRetainedTypedSampledDataSource<bool>::New(
                        _enableUsdDrawModes))
                .SetDisplayUnloadedPrimsWithBounds(
                    HdRetainedTypedSampledDataSource<bool>::New(
                        _displayUnloadedPrimsWithBounds))
                .Build());

    return HdCreateOverlayContainerDataSource(
        rendererPluginArgs,
        usdImagingArgs);
}

HdContainerDataSourceHandle
UsdImagingGLEngine::_GetSceneIndexCreateArgsFromLegacyRenderControl()
{
    if (!_renderer) {
        TF_CODING_ERROR("Need renderer to access render control.");
        return nullptr;
    }
    
    HdLegacyRenderControlInterface * const renderControl =
        _GetLegacyRenderControl();
    if (!renderControl) {
        return nullptr;
    }
    return
        HdSceneIndexCreateArgsSchema::Builder()
            .SetLegacyRenderDelegateInfo(
                HdRetainedTypedSampledDataSource<HdRenderDelegateInfo>::New(
                    renderControl->GetRenderDelegateInfo()))
            .Build();
}

void
UsdImagingGLEngine::_CreateSceneIndexChainAndRenderer(
    HdContainerDataSourceHandle const &sceneIndexCreateArgs,
    HdRendererPluginHandle const &plugin,
    const HdRendererCreateArgsSchema &rendererCreateArgs)
{
    TRACE_FUNCTION();
    
    HdSceneIndexBaseRefPtr sceneIndex =
        _mergingSceneIndex =
             HdMergingSceneIndex::New();

    // Setup scene indices following HdMergingSceneIndex.

    const std::string rendererDisplayName = plugin->GetDisplayName();

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
                    rendererDisplayName, sceneIndex, renderInstanceId,
                    /* appName = */ std::string(), sceneIndexCreateArgs);
    }

    if (_IsEnabledTerminalCachingSceneIndex()) {
        sceneIndex =
            _cachingSceneIndex =
            HdCachingSceneIndex::New(sceneIndex);
    }

    _terminalSceneIndex =
        sceneIndex;

    _renderer =
        plugin->CreateRenderer(
            _terminalSceneIndex, rendererCreateArgs);
}

//----------------------------------------------------------------------------
// AOVs and Renderer Settings
//----------------------------------------------------------------------------

TfTokenVector
UsdImagingGLEngine::GetRendererAovs() const
{
    HdLegacyRenderControlInterface * const renderControl =
        _GetLegacyRenderControl();
    if (!renderControl) {
        return {};
    }

    static const TfToken candidateAovs[] =
        { HdAovTokens->primId,
          HdAovTokens->depth,
          HdAovTokens->normal,
          HdAovTokens->Neye,
          HdAovTokensMakePrimvar(TfToken("st")) };

    TfTokenVector aovs = { HdAovTokens->color };
    for (auto const& aov : candidateAovs) {
        const HdFormat format =
            renderControl->GetDefaultAovDescriptor(aov).format;
        if (format != HdFormatInvalid) {
            aovs.push_back(aov);
        }
    }
    return aovs;
}

bool
UsdImagingGLEngine::SetRendererAov(TfToken const &id)
{
    if (ARCH_UNLIKELY(!_renderer)) {
        return false;
    }

    if (!_taskControllerSceneIndex) {
        TF_CODING_ERROR("No task controller scene index.");
        return true;
    }

    TF_PY_ALLOW_THREADS_IN_SCOPE();

    _taskControllerSceneIndex->SetRenderOutputs({id});
    return true;
}

bool
UsdImagingGLEngine::SetRendererAovs(TfTokenVector const &ids)
{
    if (ARCH_UNLIKELY(!_renderer)) {
        return false;
    }

    if (!_taskControllerSceneIndex) {
        TF_CODING_ERROR("No task controller scene index.");
        return true;
    }

    TF_PY_ALLOW_THREADS_IN_SCOPE();

    _taskControllerSceneIndex->SetRenderOutputs(ids);
    return true;
}

HgiTextureHandle
UsdImagingGLEngine::GetAovTexture(
    TfToken const& name) const
{
    HdLegacyRenderControlInterface * const renderControl =
        _GetLegacyRenderControl();
    if (!renderControl) {
        return {};
    }

    VtValue aov;
    if (!renderControl->GetTaskContextData(name, &aov)) {
        return {};
    }
    if (!aov.IsHolding<HgiTextureHandle>()) {
        return {};
    }
    return aov.Get<HgiTextureHandle>();
}

HdRenderBuffer*
UsdImagingGLEngine::GetAovRenderBuffer(TfToken const& name) const
{
    HdLegacyRenderControlInterface * const renderControl =
        _GetLegacyRenderControl();
    if (!renderControl) {
        return nullptr;
    }

    if (!_taskControllerSceneIndex) {
        TF_CODING_ERROR("No task controller scene index.");
        return nullptr;
    }

    const SdfPath path =
        _taskControllerSceneIndex->GetRenderBufferPath(name);
    return renderControl->GetRenderBuffer(path);
}

// Determine the kind of UI widget to create based on the type of the
// setting's default value. Returns false (and warns) if the type is not
// supported, in which case the setting should be skipped.
static bool
_ComputeRendererSettingType(UsdImagingGLRendererSetting * const r)
{
    if (r->defValue.IsHolding<bool>()) {
        r->type = UsdImagingGLRendererSetting::TYPE_FLAG;
    } else if (r->defValue.IsHolding<int>() ||
               r->defValue.IsHolding<unsigned int>()) {
        r->type = UsdImagingGLRendererSetting::TYPE_INT;
    } else if (r->defValue.IsHolding<float>()) {
        r->type = UsdImagingGLRendererSetting::TYPE_FLOAT;
    } else if (r->defValue.IsHolding<std::string>()) {
        r->type = UsdImagingGLRendererSetting::TYPE_STRING;
    } else {
        TF_WARN("Setting '%s' with type '%s' doesn't have a UI"
                " implementation...",
                r->name.c_str(),
                r->defValue.GetTypeName().c_str());
        return false;
    }
    return true;
}
    
UsdImagingGLRendererSettingsList
UsdImagingGLEngine::GetRendererSettingsList() const
{
    UsdImagingGLRendererSettingsList ret;

    // Use the application render setting scene index to get the
    // render settings advertised by the renderer plugin.
    const HdsiApplicationRenderSettingsSceneIndexRefPtr si =
        _GetApplicationRenderSettingsSceneIndex();
    if (si) {
        const HdRenderSettingDescriptorContainerSchema descriptors =
            si->GetRenderSettingDescriptorsFromRenderer();
        if (descriptors.IsDefined()) {
            // The container maps each setting's key (the entry name) to a
            // descriptor.
            for (const TfToken &name : descriptors.GetNames()) {
                const HdRenderSettingDescriptorSchema descriptor =
                    descriptors.Get(name);

                UsdImagingGLRendererSetting r;
                r.key = name;
                if (const HdStringDataSourceHandle nameDs =
                    descriptor.GetName()) {
                    r.name = nameDs->GetTypedValue(0.0f);
                }
                if (const HdSampledDataSourceHandle defValueDs =
                    descriptor.GetDefaultValue()) {
                    r.defValue = defValueDs->GetValue(0.0f);
                }

                if (_ComputeRendererSettingType(&r)) {
                    ret.push_back(r);
                }
            }
            return ret;
        }
    }

    // Fallbck to getting the render setting through the
    // HdLegacyRenderControlInterface (really a Hydra 1.0 backdoor).
    if (HdLegacyRenderControlInterface * const renderControl =
            _GetLegacyRenderControl()) {
        for (const HdRenderSettingDescriptor& desc :
                 renderControl->GetRenderSettingDescriptors()) {
            UsdImagingGLRendererSetting r;
            r.key = desc.key;
            r.name = desc.name;
            r.defValue = desc.defaultValue;

            if (_ComputeRendererSettingType(&r)) {
                ret.push_back(r);
            }
        }

        return ret;
    }

    return ret;
    
}

VtValue
UsdImagingGLEngine::GetRendererSetting(TfToken const& id) const
{
    // Use the application render setting scene index to see the current
    // value the application sets it to.
    if (const HdsiApplicationRenderSettingsSceneIndexRefPtr si =
            _GetApplicationRenderSettingsSceneIndex()) {
        const VtValue setting =
            si->GetRenderSetting(
                HdsiApplicationRenderSettingsSceneIndex::
                RenderSettingsSource::Resolved,
                id);
        if (setting.IsEmpty()) {
            if (HdLegacyRenderControlInterface * const renderControl =
                _GetLegacyRenderControl()) {
                for (const HdRenderSettingDescriptor& desc :
                         renderControl->GetRenderSettingDescriptors()) {
                    if (desc.key == id) {
                        return desc.defaultValue;
                    }
                }
            }
        }
        return setting;
    }

    // Fallbck to getting the render setting through the
    // HdLegacyRenderControlInterface (really a Hydra 1.0 backdoor).
    if (HdLegacyRenderControlInterface * const renderControl =
            _GetLegacyRenderControl()) {
        return renderControl->GetRenderSetting(id);
    }

    return {};
}

void
UsdImagingGLEngine::SetRendererSetting(TfToken const& id, VtValue const& value)
{
    if (_legacyRenderSettingsSceneIndex) {
        _legacyRenderSettingsSceneIndex->SetRenderSetting(id, value);
    }

    TF_PY_ALLOW_THREADS_IN_SCOPE();

    // Prefer the application render settings scene index (Hydra 2.0 path);
    // fall back to the legacy render control interface if the renderer did not
    // advertise render setting descriptors.
    if (const HdsiApplicationRenderSettingsSceneIndexRefPtr si =
            _GetApplicationRenderSettingsSceneIndex()) {
        si->SetApplicationOverrideRenderSetting(id, value);
        return;
    }

    if (HdLegacyRenderControlInterface * const renderControl =
            _GetLegacyRenderControl()) {
        renderControl->SetRenderSetting(id, value);
        return;
    }
}

SdfPath
UsdImagingGLEngine::GetActiveRenderPassPrimPath() const
{
    HdSceneIndexBaseRefPtr const terminalSceneIndex =
        _GetTerminalSceneIndex();
    if (ARCH_UNLIKELY(!terminalSceneIndex)) {
        return {};
    }

    SdfPath activeRenderPassPath;
    if (!HdUtils::HasActiveRenderPassPrim(
            terminalSceneIndex, &activeRenderPassPath)) {
        return {};
    }

    return activeRenderPassPath;
}

SdfPath
UsdImagingGLEngine::GetActiveRenderSettingsPrimPath() const
{
    if (!_appSceneIndices) {
        return {};
    }

    // After the HdsiApplicationRenderSettingsSceneIndex,
    // the active render settings prim path is always the same.
    //
    // Thus, pick it up just after the scene globals scene index.
    // That way it corresponds to either the path coming from the
    // UsdImagingSceneIndex or the one explicitly set by
    // UsdImagingGLEngine::SetActiveRenderSettingsPrimPath.

    HdSceneIndexBaseRefPtr const sceneIndex =
        _appSceneIndices->sceneGlobalsSceneIndex;
    if (!TF_VERIFY(sceneIndex)) {
        return {};
    }

    SdfPath activeRenderSettingsPath;
    if (!HdUtils::HasActiveRenderSettingsPrim(
            sceneIndex, &activeRenderSettingsPath)) {
        return {};
    }
    return activeRenderSettingsPath;
}

/* static */
SdfPathVector
UsdImagingGLEngine::GetAvailableRenderSettingsPrimPaths(UsdPrim const& root)
{
    // UsdRender OM uses the convention that all render settings prims must
    // live under /Render.
    static const SdfPath renderRoot("/Render");

    UsdPrim render = root.GetStage()->GetPrimAtPath(renderRoot);
    if (!render) {
        return {};
    }

    SdfPathVector paths;
    for (const UsdPrim child : render.GetChildren()) {
        if (child.IsA<UsdRenderSettings>()) {
            paths.push_back(child.GetPrimPath());
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

    if (!UseUsdImagingSceneIndex()) {
        _setActiveRenderSettingsPrimPathCalled = true;
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
    if (ARCH_UNLIKELY(!_renderer)) {
        return;
    }

    if (!_taskControllerSceneIndex) {
        TF_CODING_ERROR("No task controller scene index.");
        return;
    }

    TF_PY_ALLOW_THREADS_IN_SCOPE();

    _taskControllerSceneIndex->SetEnablePresentation(enabled);
}

void
UsdImagingGLEngine::SetPresentationOutput(
    TfToken const &api,
    VtValue const &framebuffer)
{
    if (ARCH_UNLIKELY(!_renderer)) {
        return;
    }

    if (!_taskControllerSceneIndex) {
        TF_CODING_ERROR("No task controller scene index.");
        return;
    }

    TF_PY_ALLOW_THREADS_IN_SCOPE();

    _userFramebuffer = framebuffer;
    _taskControllerSceneIndex->SetPresentationOutput(api, framebuffer);
}

// ---------------------------------------------------------------------
// Command API
// ---------------------------------------------------------------------

HdCommandDescriptors
UsdImagingGLEngine::GetRendererCommandDescriptors() const
{
    HdLegacyRenderControlInterface * const renderControl =
        _GetLegacyRenderControl();
    if (!renderControl) {
        return {};
    }
    return renderControl->GetCommandDescriptors();
}

bool
UsdImagingGLEngine::InvokeRendererCommand(
    const TfToken &command, const HdCommandArgs &args) const
{
    HdLegacyRenderControlInterface * const renderControl =
        _GetLegacyRenderControl();
    if (!renderControl) {
        return false;
    }
    return renderControl->InvokeCommand(command, args);
}

// ---------------------------------------------------------------------
// Control of background rendering threads.
// ---------------------------------------------------------------------
bool
UsdImagingGLEngine::IsPauseRendererSupported() const
{
    HdLegacyRenderControlInterface * const renderControl =
        _GetLegacyRenderControl();
    if (!renderControl) {
        return false;
    }
    return renderControl->IsPauseSupported();
}

bool
UsdImagingGLEngine::PauseRenderer()
{
    HdLegacyRenderControlInterface * const renderControl =
        _GetLegacyRenderControl();
    if (!renderControl) {
        return false;
    }
    return renderControl->Pause();
}

bool
UsdImagingGLEngine::ResumeRenderer()
{
    HdLegacyRenderControlInterface * const renderControl =
        _GetLegacyRenderControl();
    if (!renderControl) {
        return false;
    }
    return renderControl->Resume();
}

bool
UsdImagingGLEngine::IsStopRendererSupported() const
{
    HdLegacyRenderControlInterface * const renderControl =
        _GetLegacyRenderControl();
    if (!renderControl) {
        return false;
    }
    return renderControl->IsStopSupported();
}

bool
UsdImagingGLEngine::StopRenderer()
{
    HdLegacyRenderControlInterface * const renderControl =
        _GetLegacyRenderControl();
    if (!renderControl) {
        return false;
    }
    return renderControl->Stop();
}

bool
UsdImagingGLEngine::RestartRenderer()
{
    HdLegacyRenderControlInterface * const renderControl =
        _GetLegacyRenderControl();
    if (!renderControl) {
        return false;
    }
    return renderControl->Restart();
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
    if (ARCH_UNLIKELY(!_renderer) ||
        !IsColorCorrectionCapable()) {
        return;
    }

    if (!_taskControllerSceneIndex) {
        TF_CODING_ERROR("No task controller scene index.");
        return;
    }

    TF_PY_ALLOW_THREADS_IN_SCOPE();

    HdxColorCorrectionTaskParams hdParams;
    hdParams.colorCorrectionMode = colorCorrectionMode;
    hdParams.displayOCIO = ocioDisplay.GetString();
    hdParams.viewOCIO = ocioView.GetString();
    hdParams.colorspaceOCIO = ocioColorSpace.GetString();
    hdParams.looksOCIO = ocioLook.GetString();

    _taskControllerSceneIndex->SetColorCorrectionParams(hdParams);
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
    HdLegacyRenderControlInterface * const renderControl =
        _GetLegacyRenderControl();
    if (!renderControl) {
        return {};
    }
    return renderControl->GetRenderStats();
}

Hgi *
UsdImagingGLEngine::_GetOrCreateHgi()
{
    if (_gpuEnabled && !_hgi) {
        _defaultHgi = Hgi::CreatePlatformDefaultHgi();
        _hgi = _defaultHgi.get();
    }

    return _hgi;
}

Hgi*
UsdImagingGLEngine::GetHgi()
{
    return _GetOrCreateHgi();
}

//----------------------------------------------------------------------------
// Private/Protected
//----------------------------------------------------------------------------

void
UsdImagingGLEngine::_Execute(const UsdImagingGLRenderParams &params,
                             const SdfPathVector &taskPaths)
{
    {
        // Release the GIL before calling into hydra, in case any hydra plugins
        // call into python.
        TF_PY_ALLOW_THREADS_IN_SCOPE();
        if (HdLegacyRenderControlInterface * const renderControl =
                _GetLegacyRenderControl()) {
            renderControl->Execute(taskPaths);
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
        // We provide a fallback value here for all prims.
        _displayStyleSceneIndex->SetRefineLevelFallback(refineLevel);

        {
            // The post instancing notice batching scene index will be flushed
            // after the stage notice batching scene index (if one exists).
            _ScopedHydraNoticeBatch noticeBatch(
                _usdImagingSceneIndex->
                    GetPostInstancingNoticeBatchingSceneIndex());

            _usdImagingSceneIndex->ApplyPendingUpdates();
            if (_execStageSceneIndex) {
                _execStageSceneIndex->ApplyPendingUpdates();
                _noticeBatchingStageSceneIndex->Flush();
            }
        }
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

HdSceneIndexBaseRefPtr
UsdImagingGLEngine::_GetTerminalSceneIndex() const
{
    return _terminalSceneIndex;
}

HdLegacyRenderControlInterface *
UsdImagingGLEngine::_GetLegacyRenderControl() const
{
    if (ARCH_UNLIKELY(!_renderer)) {
        return nullptr;
    }

    HdLegacyRenderControlInterface * const renderControl =
        _renderer->GetLegacyRenderControl();

    if (!renderControl) {
        TF_CODING_ERROR("Expected render control interface.");
    }

    return renderControl;
}

HdsiApplicationRenderSettingsSceneIndexRefPtr
UsdImagingGLEngine::_GetApplicationRenderSettingsSceneIndex() const
{
    if (!_appSceneIndices) {
        return nullptr;
    }
    return _appSceneIndices->applicationRenderSettingsSceneIndex;
}

SdfPath
UsdImagingGLEngine::_GetSceneIndexPrimPathFromPrimId(const int primId) const
{
    HdSceneIndexBaseRefPtr const si = _GetTerminalSceneIndex();
    if (!si) {
        return {};
    }

    // Prefer prim id from terminal scene index.
    if (HdPathDataSourceHandle const ds =
            HdSceneGlobalsSchema::GetFromSceneIndex(si)
                .GetPrimIdToPath()
                .GetElement(primId)) {
        return ds->GetTypedValue(0.0f);
    }

    // Fall back to the render index's own prim id table.
    if (HdLegacyRenderControlInterface * const renderControl =
            _GetLegacyRenderControl()) {
        return renderControl->GetRprimPathFromPrimId(primId);
    }

    return {};
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
