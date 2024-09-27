//
// Copyright 2019 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef EXT_RMANPKG_25_0_PLUGIN_RENDERMAN_PLUGIN_HD_PRMAN_RENDER_PARAM_H
#define EXT_RMANPKG_25_0_PLUGIN_RENDERMAN_PLUGIN_HD_PRMAN_RENDER_PARAM_H

#include "pxr/pxr.h"
#include "hdPrman/api.h"
#include "hdPrman/xcpt.h"
#include "hdPrman/cameraContext.h"
#include "hdPrman/renderViewContext.h"
#include "pxr/base/gf/vec2f.h"
#include "pxr/imaging/hd/sceneDelegate.h"
#include "pxr/imaging/hd/renderDelegate.h"
#if PXR_VERSION >= 2308
#include "pxr/imaging/hd/renderSettings.h"
#endif
#include "pxr/imaging/hd/material.h"
#if PXR_VERSION >= 2302
#include "pxr/imaging/hd/retainedSceneIndex.h"
#endif

#include "Riley.h"
#include "RixEventCallbacks.h"
#include <unordered_map>
#include <mutex>

class RixRiCtl;

namespace stats {
class Session;
};

PXR_NAMESPACE_OPEN_SCOPE

class HdPrmanFramebuffer;
class HdPrmanCamera;
class HdPrmanInstancer;
class HdPrmanRenderDelegate;
#if PXR_VERSION >= 2308
class HdPrman_RenderSettings;
#endif
class SdfAssetPath;

// Compile-time limit on max time samples.
// The idea is to avoid heap allocation of sample buffers in the Sync()
// calls by using fixed-size stack arrays with configured capacity.
// The capacity is indicated to the scene delegate when requesting
// time samples.

#if _PRMANAPI_VERSION_MAJOR_ >= 26
constexpr int HDPRMAN_MAX_TIME_SAMPLES = 16;
#else
constexpr int HDPRMAN_MAX_TIME_SAMPLES = 4;
#endif

#define HDPRMAN_SHUTTEROPEN_DEFAULT 0.f
#ifdef PIXAR_ANIM
#define HDPRMAN_SHUTTERCLOSE_DEFAULT 0.5f
#else
#define HDPRMAN_SHUTTERCLOSE_DEFAULT 0.f
#endif

// Render Param for HdPrman to communicate with an instance of PRMan.
class HdPrman_RenderParam : public HdRenderParam
{
public:
    HDPRMAN_API
    HdPrman_RenderParam(HdPrmanRenderDelegate *renderDelegate,
                        const std::string &rileyVariant,
                        const int& xpuCpuConfig,
                        const std::vector<int>& xpuGpuConfig,
                        const std::vector<std::string>& extraArgs);

    HDPRMAN_API
    ~HdPrman_RenderParam() override;

    HDPRMAN_API
    void Begin(HdPrmanRenderDelegate *renderDelegate); 

    // Convert any Hydra primvars that should be Riley instance attributes.
    // Stores visibility state in *visible (if provided) in addition to
    // representing it as attributes in the returned RtParamList.
    HDPRMAN_API
    RtParamList
    ConvertAttributes(HdSceneDelegate *sceneDelegate,
        SdfPath const& id, bool isGeometry);

    // A vector of Riley coordinate system id's.
    using RileyCoordSysIdVec = std::vector<riley::CoordinateSystemId>;
    // A ref-counting ptr to a vector of coordinate systems.
    using RileyCoordSysIdVecRefPtr = std::shared_ptr<RileyCoordSysIdVec>;

    /// Convert any coordinate system bindings for the given rprim id
    /// into a Riley equivalent form.  Retain the result internally
    /// in a cache, so that we may re-use the result with other
    /// rprims with the same set of bindings.
    HDPRMAN_API
    RileyCoordSysIdVecRefPtr ConvertAndRetainCoordSysBindings(
        HdSceneDelegate *sceneDelegate,
        SdfPath const& id);

    /// Convert a list of categories returned by Hydra to
    /// equivalent Prman grouping attributes.
    HDPRMAN_API
    void ConvertCategoriesToAttributes(
        SdfPath const& id,
        VtArray<TfToken> const& categories,
        RtParamList& attrs);
    
    /// Prepends the given \p renderTag to grouping:membership in \p params.
    HDPRMAN_API
    static void AddRenderTagToGroupingMembership(
        const TfToken& renderTag,
        RtParamList& params);

    /// Release any coordinate system bindings cached for the given
    /// rprim id.
    HDPRMAN_API
    void ReleaseCoordSysBindings(SdfPath const& id);

    HDPRMAN_API
    void IncrementLightLinkCount(TfToken const& name);

    HDPRMAN_API
    void DecrementLightLinkCount(TfToken const& name);

    HDPRMAN_API
    bool IsLightLinkUsed(TfToken const& name);

    HDPRMAN_API
    void IncrementLightFilterCount(TfToken const& name);

    HDPRMAN_API
    void DecrementLightFilterCount(TfToken const& name);

    HDPRMAN_API
    bool IsLightFilterUsed(TfToken const& name);

    HDPRMAN_API
    void UpdateLegacyOptions();

    // Set integrator params from the HdRenderSettingsMap
    HDPRMAN_API
    void SetIntegratorParamsFromRenderSettingsMap(
        HdPrmanRenderDelegate *renderDelegate,
        const std::string& integratorName,
        RtParamList& params);

    // Set projection params from the HdRenderSettingsMap
    HDPRMAN_API
    void SetProjectionParamsFromRenderSettings(
                        HdPrmanRenderDelegate *renderDelegate,
                        std::string& projectionName,
                        RtParamList& params);

    // Set integrator params from the camera.
    // This invokes any callbacks registered with
    // RegisterIntegratorCallbackForCamera().
    HDPRMAN_API
    void SetIntegratorParamsFromCamera(
        HdPrmanRenderDelegate *renderDelegate,
        const HdPrmanCamera *camera,
        std::string const& integratorName,
        RtParamList& params);

    HDPRMAN_API
    void SetBatchCommandLineArgs(
        VtValue const &cmdLine,
        RtParamList * options);

    // Callback to convert any camera settings that should become
    // parameters on the integrator.
    using IntegratorCameraCallback = void (*)
        (HdPrmanRenderDelegate *renderDelegate,
         const HdPrmanCamera *camera,
         std::string const& integratorName,
         RtParamList &integratorParams);

    // Register a callback to process integrator settings
    HDPRMAN_API
    static void 
    RegisterIntegratorCallbackForCamera(
        IntegratorCameraCallback const& callback);

    HDPRMAN_API
    void SetFiltersFromRenderSettings(
                        HdPrmanRenderDelegate *renderDelegate);

    // Get RIX vs XPU
    bool IsXpu() const { return _xpu; }

    void UpdateRenderStats(VtDictionary& stats);

    HDPRMAN_API
    static void ProgressCallback(RixEventCallbacks::Event,
                                 RtConstPointer data, RtPointer);

    // Request edit access to the Riley scene and return it.
    HDPRMAN_API
    riley::Riley * AcquireRiley();

    // Get the current frame-relative shutter interval.
    // Note: This function should be called after SetRileyOptions.
    const GfVec2f& GetShutterInterval() {
        return _shutterInterval;
    }

    // Provides external access to resources used to set parameters for
    // options and the active integrator.
    riley::IntegratorId GetActiveIntegratorId();

    const riley::MaterialId GetFallbackMaterialId() const {
        return _fallbackMaterialId;
    }

    const riley::MaterialId GetFallbackVolumeMaterialId() const {
        return _fallbackVolumeMaterialId;
    }

    int GetLastLegacySettingsVersion() const {
        return _lastLegacySettingsVersion;
    }
    void SetLastLegacySettingsVersion(int version);

    // Legacy data flow to resolution from the render pass via render pass
    // state.
    GfVec2i const &GetResolution() { return _resolution; }
    void SetResolution(GfVec2i const & resolution);

    // Invalidate texture at path.
    void InvalidateTexture(const std::string &path);
    
    /// Call this from RenderPass to set which render tags should be enabled.
    /// Calling will always trigger a render index traversal, and may trigger an
    /// intergrator update, so only call it if you strongly suspect that either
    /// the set of active render tags or the set of rprim render tags in the
    /// scene has changed.
    void SetActiveRenderTags(
        const TfTokenVector& activeRenderTags,
        HdRenderIndex* renderIndex);

    void UpdateIntegrator(const HdRenderIndex * renderIndex);

    riley::IntegratorId GetIntegratorId() const { return _integratorId; }

    RtParamList &GetIntegratorParams() { return _integratorParams; }

    // Indicate whether fallback lights should be enabled.
    void SetFallbackLightsEnabled(bool);

    bool HasSceneLights() const { return _sceneLightCount > 0; }
    void IncreaseSceneLightCount() { ++_sceneLightCount; }
    void DecreaseSceneLightCount() { --_sceneLightCount; }
    
    // Provides external access to resources used to set parameters for
    // scene options from the render settings map.
    RtParamList &GetLegacyOptions() { return _legacyOptions; }

    HdPrman_CameraContext &GetCameraContext() { return _cameraContext; }
    const HdPrman_CameraContext &GetCameraContext() const { return _cameraContext; }

    HdPrman_RenderViewContext &GetRenderViewContext() {
        return _renderViewContext;
    }

    void CreateRenderViewFromRenderSpec(const VtDictionary &renderSpec);

#if PXR_VERSION >= 2308
    void CreateRenderViewFromRenderSettingsProducts(
        HdRenderSettings::RenderProducts const &products,
        HdPrman_RenderViewContext *renderViewContext);
#endif

    // Starts the render thread (if needed), and tells the render thread to
    // call into riley and start a render.
    void StartRender();

    void End();

    // Requests riley stop rendering; if blocking is true, waits until riley
    // has exited and the render thread is idle before returning.  Note that
    // after the render stops, the render thread will be running but idle;
    // to stop the thread itself, call DeleteRenderThread. If the render thread
    // is not running, this call does nothing.
    void StopRender(bool blocking = true);

    // Returns whether the render thread is active and rendering currently.
    // Returns false if the render thread is active but idle (not in riley).
    bool IsRendering();

    // Returns whether the user has requested pausing the render.
    bool IsPauseRequested();

    // Deletes the render thread if there is one.
    void DeleteRenderThread();

    // Checks whether render param was successfully initialized.
    // ie. riley was created
    bool IsValid() const;

    // Creates displays in riley based on aovBindings vector together
    // with HdPrmanFramebuffer to transfer the result between the
    // render thread and the hydra render buffers.
    void CreateFramebufferAndRenderViewFromAovs(
#if PXR_VERSION >= 2308
        const HdRenderPassAovBindingVector& aovBindings,
        HdPrman_RenderSettings* renderSettings);
#else
        const HdRenderPassAovBindingVector& aovBindings);
#endif

    // Deletes HdPrmanFramebuffer (created with
    // CreateRenderViewFromAovs). Can be called if there is no frame
    // buffer (returning false).
    bool DeleteFramebuffer();

    // Returns HdPrmanFramebuffer
    HdPrmanFramebuffer * GetFramebuffer() const {
        return _framebuffer.get();
    }

    // Creates displays in riley based on rendersettings map
    void CreateRenderViewFromLegacyProducts(
        const VtArray<HdRenderSettingsMap>& renderProducts, int frame);

    // Scene version counter.
    std::atomic<int> sceneVersion;

    // For now, the renderPass needs the render target for each view, for
    // resolution edits, so we need to keep track of these too.
    void SetActiveIntegratorId(riley::IntegratorId integratorId);

    int frame;

    void UpdateQuickIntegrator(const HdRenderIndex * renderIndex);

    riley::IntegratorId GetQuickIntegratorId() const {
        return _quickIntegratorId;
    }

    // Compute shutter interval from the camera Sprim and legacy render settings
    // map and update the value on the legacy options param list.
    // Also invoke SetRileyOptions to commit it.
    void SetRileyShutterIntervalFromCameraContextCameraPath(
        const HdRenderIndex * renderIndex);

#if PXR_VERSION >= 2308
    // Path to the Integrator from the Render Settings Prim
    void SetRenderSettingsIntegratorPath(HdSceneDelegate *sceneDelegate,
        SdfPath const &renderSettingsIntegratorPath);
    SdfPath GetRenderSettingsIntegratorPath() {
        return _renderSettingsIntegratorPath;
    }
    void SetRenderSettingsIntegratorNode(
        HdRenderIndex *renderIndex, HdMaterialNode2 const &integratorNode);
    HdMaterialNode2 GetRenderSettingsIntegratorNode() {
        return _renderSettingsIntegratorNode;
    };
#endif

    // Path to the connected Sample Filter from the Render Settings Prim
    void SetConnectedSampleFilterPaths(HdSceneDelegate *sceneDelegate,
        SdfPathVector const& connectedSampleFilterPaths);
    SdfPathVector GetConnectedSampleFilterPaths() {
        return _connectedSampleFilterPaths;
    }

    // Riley Data from the Sample Filter Prim
    void AddSampleFilter(
        HdSceneDelegate *sceneDelegate, 
        SdfPath const& path, 
        riley::ShadingNode const& node);
    void CreateSampleFilterNetwork(HdSceneDelegate *sceneDelegate);
    riley::SampleFilterList GetSampleFilterList();

    // Path to the connected Display Filter from the Render Settings Prim
    void SetConnectedDisplayFilterPaths(HdSceneDelegate *sceneDelegate,
        SdfPathVector const& connectedDisplayFilterPaths);
    SdfPathVector GetConnectedDisplayFilterPaths() {
        return _connectedDisplayFilterPaths;
    }

    // Riley Data from the Display Filter Prim
    void AddDisplayFilter(
        HdSceneDelegate *sceneDelegate, 
        SdfPath const& path, 
        riley::ShadingNode const& node);
    void CreateDisplayFilterNetwork(HdSceneDelegate *sceneDelegate);
    riley::DisplayFilterList GetDisplayFilterList();

    void FatalError(const char* msg);

    // Instancer by id
    HdPrmanInstancer* GetInstancer(const SdfPath& id);

    // Cache riley options coming from the scene index observer observing
    // the riley:globals prim.
    void SetRileySceneIndexObserverOptions(RtParamList const &params);

    // Cache scene options from the render settings prim.
    void SetRenderSettingsPrimOptions(RtParamList const &params);

    // Set path of the driving render settings prim.
    void SetDrivingRenderSettingsPrimPath(SdfPath const &path);

    // Get path of the driving render settings prim.
    SdfPath const& GetDrivingRenderSettingsPrimPath() const;

    // Set Riley scene options by composing opinion sources.
    void SetRileyOptions();

    // Returns true if the render delegate in interactive mode (as opposed to
    // batched/offline mode).
    bool IsInteractive() const;

private:
    void _CreateStatsSession();
    void _CreateRiley(const std::string &rileyVariant,
                      const int& xpuCpuConfig,
                      const std::vector<int>& xpuGpuConfig,
                      const std::vector<std::string>& extraArgs);

    // Creation of riley prims that are either not backed by the scene 
    // (e.g., fallback materials) OR those that are
    // currently managed by render param (such as the camera, render view and
    // render terminals).
    void _CreateInternalPrims();
    void _DeleteInternalPrims();
    void _CreateFallbackMaterials();
    void _CreateFallbackLight();
    void _CreateIntegrator(HdRenderDelegate * renderDelegate);
    void _CreateQuickIntegrator(HdRenderDelegate * renderDelegate);
    
    void _DestroyRiley();
    void _DestroyStatsSession();

    // Initialize internals of PRMan system
    void _PRManSystemBegin(const std::vector<std::string>& extraArgs);
    // Initialize internals of PRMan renderer
    int _PRManRenderBegin(const std::vector<std::string>& extraArgs);

    bool _UpdatePixelFilter();
    bool _UpdateQNSettings();

    // Updates clear colors of AOV descriptors of framebuffer.
    // If this is not possible because the set of AOVs changed,
    // returns false.
    bool _UpdateFramebufferClearValues(
        const HdRenderPassAovBindingVector& aovBindings);

    riley::ShadingNode _ComputeIntegratorNode(
        HdRenderDelegate * renderDelegate,
        const HdPrmanCamera * cam);

    riley::ShadingNode _ComputeQuickIntegratorNode(
        HdRenderDelegate * renderDelegate,
        const HdPrmanCamera * cam);

    void _RenderThreadCallback();
    static void _ProgressCallback(RixEventCallbacks::Event,
                                  RtConstPointer data, RtPointer clientData);

    void _CreateRileyDisplay(
        const RtUString& productName, const RtUString& productType,
        HdPrman_RenderViewDesc& renderViewDesc,
        const std::vector<size_t>& renderOutputIndices,
        RtParamList& displayParams, bool isXpu);
    
    void _UpdateShutterInterval(const RtParamList &composedParams);
    
    static
    void _SetExcludeSubset(
        const TfToken::Set& excludedTags,
        RtParamList& params);

private:
    void _AddCryptomatteFixes(const RtUString& riName, VtValue& val);

    // Top-level entrypoint to PRMan.
    // Singleton used to access RixInterfaces.
    RixContext *_rix;

    // RixInterface for PRManBegin/End.
    RixRiCtl *_ri;

    // RixInterface for Riley.
    RixRileyManager *_mgr;

    // Xcpt Handler
    HdPrman_Xcpt _xcpt;

    // Roz stats session
    stats::Session *_statsSession;
    int _progressPercent;
    int _progressMode;

    // Riley instance.
    riley::Riley *_riley;
    // Mutex around riley->Stop(), so as to be well-behaved callers.
    std::mutex _stopMutex;

#if PXR_VERSION >= 2302
    // Stats scene index currently used for querying live stats server ID
    HdRetainedSceneIndexRefPtr _statsSceneIndex;
#endif

    std::unique_ptr<class HdRenderThread> _renderThread;
    std::unique_ptr<HdPrmanFramebuffer> _framebuffer;

    int _sceneLightCount;

    // Refcounts for each category mentioned by a light link.
    // This is used to convey information from lights back to the
    // geometry -- in Renderman, geometry must subscribe
    // to the linked lights.
    std::unordered_map<TfToken, size_t, TfToken::HashFunctor> _lightLinkRefs;

    // Mutex protecting lightLinkRefs.
    std::mutex _lightLinkMutex;

    std::unordered_map<TfToken, size_t, TfToken::HashFunctor> _lightFilterRefs;

    // Mutex protecting lightFilterRefs.
    std::mutex _lightFilterMutex;

    // Map from Hydra coordinate system vector pointer to Riley equivalent.
    using _HdToRileyCoordSysMap =
        std::unordered_map<HdIdVectorSharedPtr, RileyCoordSysIdVecRefPtr>;
    // Map from Hydra id to cached, converted coordinate systems.
    using _GeomToHdCoordSysMap =
        std::unordered_map<SdfPath, HdIdVectorSharedPtr, SdfPath::Hash>;

    // A fallback material to use for any geometry that
    // does not have a bound material.
    riley::MaterialId _fallbackMaterialId;

    // Fallback material for volumes that don't have materials.
    riley::MaterialId _fallbackVolumeMaterialId;
    
    TfToken::Set _lastExcludedRenderTags;

    riley::IntegratorId _quickIntegratorId;
    RtParamList _quickIntegratorParams;

    // The integrator to use.
    // Updated from render pass state OR render settings prim.
    riley::IntegratorId _activeIntegratorId;

    // Coordinate system conversion cache.
    _GeomToHdCoordSysMap _geomToHdCoordSysMap;
    _HdToRileyCoordSysMap _hdToRileyCoordSysMap;
    std::mutex _coordSysMutex;

    // The fallback light.  HdPrman_RenderPass calls
    // SetFallbackLightsEnabled() to maintain visibility
    // of the fallback light XOR other lights in the scene.
    riley::LightInstanceId _fallbackLight;
    riley::LightShaderId _fallbackLightShader;
    RtParamList _fallbackLightAttrs;
    bool _fallbackLightEnabled;

    HdPrman_CameraContext _cameraContext;
    HdPrman_RenderViewContext _renderViewContext;

    // Frame-relative shutter window used to determine if motion blur is
    // enabled.
    GfVec2f _shutterInterval;

    // Flag to indicate whether Riley scene options were set.
    bool _initRileyOptions;

    // Environment and fallback scene options.
    RtParamList _envOptions;
    RtParamList _fallbackOptions;

    /// ------------------------------------------------------------------------
    // Render settings prim driven state
    //

    SdfPath _drivingRenderSettingsPrimPath;

    RtParamList _rileySceneIndexObserverOptions;
    RtParamList _renderSettingsPrimOptions;

    // Render terminals
    SdfPath _renderSettingsIntegratorPath;
    HdMaterialNode2 _renderSettingsIntegratorNode;
    riley::IntegratorId _integratorId;

    SdfPathVector _connectedSampleFilterPaths;
    std::map<SdfPath, riley::ShadingNode> _sampleFilterNodes;
    riley::SampleFilterId _sampleFiltersId;

    SdfPathVector _connectedDisplayFilterPaths;
    std::map<SdfPath, riley::ShadingNode> _displayFilterNodes;
    riley::DisplayFilterId _displayFiltersId;
    /// ------------------------------------------------------------------------

    /// ------------------------------------------------------------------------
    // Legacy render settings and render pass driven state
    //
    // Params from the render settings map.
    RtParamList _legacyOptions;
    int _lastLegacySettingsVersion;

    // Resolution for the render pass via render pass state.
    GfVec2i _resolution;

    RtParamList _integratorParams;
    /// ------------------------------------------------------------------------

    // RIX or XPU
    bool _xpu;

    HdRenderPassAovBindingVector _lastBindings;

    // Solaris Legacy Sample & Display Filter Support
    std::unordered_map<RtUString, riley::ShadingNode> _displayFilters;
    std::unordered_map<RtUString, riley::ShadingNode> _sampleFilters;
    bool _displayFiltersDirty;
    bool _sampleFiltersDirty;
    riley::SampleFilterId _sampleFilterId;
    riley::DisplayFilterId _displayFilterId;    

    RtUString _pixelFilter;
    GfVec2f _pixelFilterWidth;

    HdPrmanRenderDelegate* _renderDelegate;

    // Husk command line arguments
    std::vector<std::string> _outputNames;
    int _huskFrameStart;
    int _huskFrameIncrement;
    std::string _huskTileSuffix;
    bool _usingHusk;

    // QuicklyNoiseless settings
    bool _useQN;
    bool _qnCheapPass;
    int _qnMinSamples;
    int _qnInterval;
};

/// Convert Hydra points to Riley point primvar.
///
void
HdPrman_ConvertPointsPrimvar(
    HdSceneDelegate* sceneDelegate,
    const SdfPath& id,
    const GfVec2f& shutterInterval,
    RtPrimVarList& primvars,
    size_t npoints);

/// Count hydra points to set element count on primvars and then
/// convert them to Riley point primvar.
/// 
size_t
HdPrman_ConvertPointsPrimvarForPoints(
    HdSceneDelegate* sceneDelegate,
    const SdfPath& id,
    const GfVec2f& shutterInterval,
    RtPrimVarList& primvars);

/// Convert any Hydra primvars that should be Riley primvars.
void
HdPrman_ConvertPrimvars(
    HdSceneDelegate *sceneDelegate,
    SdfPath const& id,
    RtPrimVarList& primvars,
    int numUniform,
    int numVertex,
    int numVarying,
    int numFaceVarying,
    const GfVec2d &shutterInterval,
    float time = 0.f);

/// Check for any primvar opinions on the material that should be Riley primvars.
void
HdPrman_TransferMaterialPrimvarOpinions(
    HdSceneDelegate *sceneDelegate,
    SdfPath const& hdMaterialId,
    RtPrimVarList& primvars);

/// Resolve Hd material ID to the corresponding Riley material & displacement
bool
HdPrman_ResolveMaterial(
    HdSceneDelegate *sceneDelegate,
    SdfPath const& hdMaterialId,
    riley::Riley *riley,
    riley::MaterialId *materialId,
    riley::DisplacementId *dispId);

PXR_NAMESPACE_CLOSE_SCOPE

#endif // EXT_RMANPKG_25_0_PLUGIN_RENDERMAN_PLUGIN_HD_PRMAN_RENDER_PARAM_H
