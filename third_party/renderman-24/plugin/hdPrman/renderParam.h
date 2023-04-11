//
// Copyright 2019 Pixar
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
#ifndef EXT_RMANPKG_24_0_PLUGIN_RENDERMAN_PLUGIN_HD_PRMAN_RENDER_PARAM_H
#define EXT_RMANPKG_24_0_PLUGIN_RENDERMAN_PLUGIN_HD_PRMAN_RENDER_PARAM_H

#include "pxr/pxr.h"
#include "hdPrman/api.h"
#include "hdPrman/prmanArchDefs.h"
#include "hdPrman/xcpt.h"
#include "hdPrman/cameraContext.h"
#include "hdPrman/renderViewContext.h"
#include "pxr/imaging/hd/sceneDelegate.h"
#include "pxr/imaging/hd/renderDelegate.h"
#include "pxr/base/gf/matrix4d.h"

#include "Riley.h"
#include <unordered_map>
#include <mutex>

class RixRiCtl;

namespace stats {
class Session;
};

PXR_NAMESPACE_OPEN_SCOPE

class HdPrmanFramebuffer;
class HdPrmanCamera;
class HdPrmanRenderDelegate;
class HdPrman_RenderSettings;
class SdfAssetPath;

// Compile-time limit on max time samples.
// The idea is to avoid heap allocation of sample buffers in the Sync()
// calls by using fixed-size stack arrays with configured capacity.
// The capacity is indicated to the scene delegate when requesting
// time samples.
constexpr int HDPRMAN_MAX_TIME_SAMPLES = 4;

// Render Param for HdPrman to communicate with an instance of PRMan.
class HdPrman_RenderParam : public HdRenderParam
{
public:
    HDPRMAN_API
    HdPrman_RenderParam(const std::string &rileyVariant, 
        const std::string &xpuDevices,
        const std::vector<std::string>& extraArgs);

    HDPRMAN_API
    ~HdPrman_RenderParam() override;

    HDPRMAN_API
    void Begin(HdPrmanRenderDelegate *renderDelegate); 

    // Convert any Hydra primvars that should be Riley instance attributes.
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
    void SetOptionsFromRenderSettings(HdPrmanRenderDelegate *renderDelegate, 
                                      RtParamList& options);

    // Set integrator params from the HdRenderSettingsMap
    HDPRMAN_API
    void SetIntegratorParamsFromRenderSettings(
                        HdPrmanRenderDelegate *renderDelegate,
                        const std::string& integratorName,
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
                        HdPrmanRenderDelegate *renderDelegate,
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

    // Get RIX vs XPU
    bool IsXpu() const { return _xpu; }

    // Adds VtValue contents to RtParamList
    bool SetParamFromVtValue(
        RtUString const& name,
        VtValue const& val,
        TfToken const& role,
        RtParamList& params);

    // Request edit access to the Riley scene and return it.
    riley::Riley * AcquireRiley();

    // Provides external access to resources used to set parameters for
    // options and the active integrator.
    riley::IntegratorId GetActiveIntegratorId();

    const riley::MaterialId GetFallbackMaterialId() const {
        return _fallbackMaterialId;
    }

    const riley::MaterialId GetFallbackVolumeMaterialId() const {
        return _fallbackVolumeMaterialId;
    }

    int GetLastSettingsVersion() const { return _lastSettingsVersion; }
    void SetLastSettingsVersion(int version);

    // Invalidate texture at path.
    void InvalidateTexture(const std::string &path);

    void UpdateIntegrator(const HdRenderIndex * renderIndex);

    riley::IntegratorId GetIntegratorId() const { return _integratorId; }

    RtParamList &GetIntegratorParams() { return _integratorParams; }

    bool HasSceneLights() const { return _sceneLightCount > 0; }
    void IncreaseSceneLightCount() { ++_sceneLightCount; }
    void DecreaseSceneLightCount() { --_sceneLightCount; }
    
    // Provides external access to resources used to set parameters for
    // options and the active integrator.
    RtParamList &GetOptions() { return _options; }
    HdPrman_CameraContext &GetCameraContext() { return _cameraContext; }

    HdPrman_RenderViewContext &GetRenderViewContext() {
        return _renderViewContext;
    }

    void CreateRenderViewFromRenderSpec(const VtDictionary &renderSpec);

    void CreateRenderViewFromRenderSettingsPrim(
        HdPrman_RenderSettings const &renderSettingsPrim);

    // Starts the render thread (if needed), and tells the render thread to
    // call into riley and start a render.
    void StartRender();

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
        const HdRenderPassAovBindingVector& aovBindings);

    // Deletes HdPrmanFramebuffer (created with
    // CreateRenderViewFromAovs). Can be called if there is no frame
    // buffer (returning false).
    bool DeleteFramebuffer();

    // Returns HdPrmanFramebuffer
    HdPrmanFramebuffer * GetFramebuffer() const {
        return _framebuffer.get();
    }

    // Creates displays in riley based on rendersettings map
    void CreateRenderViewFromProducts(
        const VtArray<HdRenderSettingsMap>& renderProducts, int frame);

    // Scene version counter.
    std::atomic<int> sceneVersion;

    // For now, the renderPass needs the render target for each view, for
    // resolution edits, so we need to keep track of these too.
    void SetActiveIntegratorId(riley::IntegratorId integratorId);

    GfVec2i resolution;

    // Some quantites previously given as options now need to be provided
    // through different Riley APIs. However, it is still convenient for these
    // values to be stored in _options (for now). This method returns a pruned
    // copy of the options, to be provided to SetOptions().
    RtParamList _GetDeprecatedOptionsPrunedList();

    void UpdateQuickIntegrator(const HdRenderIndex * renderIndex);

    riley::IntegratorId GetQuickIntegratorId() const {
        return _quickIntegratorId;
    }

    // Compute shutter interval from render settings and camera and
    // immediately set it as riley option.
    void UpdateRileyShutterInterval(const HdRenderIndex * renderIndex);

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

private:
    void _CreateStatsSession();
    void _CreateRiley(const std::string &rileyVariant, 
        const std::string &xpuVariant,
        const std::vector<std::string>& extraArgs);
    void _CreateFallbackMaterials();
    void _CreateFallbackLight();
    void _CreateIntegrator(HdRenderDelegate * renderDelegate);
    
    void _DestroyRiley();
    void _DestroyStatsSession();

    // Updates clear colors of AOV descriptors of framebuffer.
    // If this is not possible because the set of AOVs changed,
    // returns false.
    bool _UpdateFramebufferClearValues(
        const HdRenderPassAovBindingVector& aovBindings);

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

    // Riley instance.
    riley::Riley *_riley;

    riley::ShadingNode _ComputeIntegratorNode(
        HdRenderDelegate * renderDelegate,
        const HdPrmanCamera * cam);

    riley::ShadingNode _ComputeQuickIntegratorNode(
        HdRenderDelegate * renderDelegate,
        const HdPrmanCamera * cam);

    void _CreateQuickIntegrator(HdRenderDelegate * renderDelegate);

    void _RenderThreadCallback();

    void _CreateRileyDisplay(
        const RtUString& productName, const RtUString& productType,
        HdPrman_RenderViewDesc& renderViewDesc,
        const std::vector<size_t>& renderOutputIndices,
        RtParamList& displayParams, bool isXpu);

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

    riley::IntegratorId _integratorId;
    RtParamList _integratorParams;

    riley::IntegratorId _quickIntegratorId;
    RtParamList _quickIntegratorParams;

    // The integrator to use.
    // Updated from render pass state.
    riley::IntegratorId _activeIntegratorId;

    // Coordinate system conversion cache.
    _GeomToHdCoordSysMap _geomToHdCoordSysMap;
    _HdToRileyCoordSysMap _hdToRileyCoordSysMap;
    std::mutex _coordSysMutex;

    RtParamList _options;
    HdPrman_CameraContext _cameraContext;
    HdPrman_RenderViewContext _renderViewContext;

    // SampleFilter
    SdfPathVector _connectedSampleFilterPaths;
    std::map<SdfPath, riley::ShadingNode> _sampleFilterNodes;
    riley::SampleFilterId _sampleFiltersId;

    // DisplayFilter
    SdfPathVector _connectedDisplayFilterPaths;
    std::map<SdfPath, riley::ShadingNode> _displayFilterNodes;
    riley::DisplayFilterId _displayFiltersId;

    // RIX or XPU
    bool _xpu;
    std::vector<int> _xpuGpuConfig;

    int _lastSettingsVersion;

    std::vector<std::string> _outputNames;
};

// Helper to convert matrix types, handling double->float conversion.
inline RtMatrix4x4
HdPrman_GfMatrixToRtMatrix(const GfMatrix4d &m)
{
    const double *d = m.GetArray();
    return RtMatrix4x4(
        d[0], d[1], d[2], d[3],
        d[4], d[5], d[6], d[7],
        d[8], d[9], d[10], d[11],
        d[12], d[13], d[14], d[15]);
}

// Helper to convert matrix types, handling float->double conversion.
inline GfMatrix4d
HdPrman_RtMatrixToGfMatrix(const RtMatrix4x4 &m)
{
    return GfMatrix4d(
        m.m[0][0], m.m[0][1], m.m[0][2], m.m[0][3],
        m.m[1][0], m.m[1][1], m.m[1][2], m.m[1][3],
        m.m[2][0], m.m[2][1], m.m[2][2], m.m[2][3],
        m.m[3][0], m.m[3][1], m.m[3][2], m.m[3][3]);
}

// Convert Hydra points to Riley point primvar.
void
HdPrman_ConvertPointsPrimvar(HdSceneDelegate *sceneDelegate, SdfPath const &id,
                             RtPrimVarList& primvars, size_t npoints);

// Count hydra points to set element count on primvars and then
// convert them to Riley point primvar.
size_t
HdPrman_ConvertPointsPrimvarForPoints(HdSceneDelegate *sceneDelegate, SdfPath const &id,
                                      RtPrimVarList& primvars);

// Convert any Hydra primvars that should be Riley primvars.
void
HdPrman_ConvertPrimvars(HdSceneDelegate *sceneDelegate, SdfPath const& id,
                        RtPrimVarList& primvars, int numUniform, int numVertex,
                        int numVarying, int numFaceVarying);

// Check for any primvar opinions on the material that should be Riley primvars.
void
HdPrman_TransferMaterialPrimvarOpinions(HdSceneDelegate *sceneDelegate,
                                        SdfPath const& hdMaterialId,
                                        RtPrimVarList& primvars);

// Resolve Hd material ID to the corresponding Riley material & displacement
bool
HdPrman_ResolveMaterial(HdSceneDelegate *sceneDelegate,
                        SdfPath const& hdMaterialId,
                        riley::MaterialId *materialId,
                        riley::DisplacementId *dispId);

// Attempt to extract a useful texture identifier from the given \p asset.
// If \p asset is determined to not be a .tex file, attempt to use the Hio
// based Rtx plugin to load the texture.  If \p asset is non-empty, we will
// always return _something_
RtUString
HdPrman_ResolveAssetToRtUString(SdfAssetPath const &asset,
                                bool flipTexture = true,
                                char const *debugNodeType=nullptr);

/// Update the supplied list of options using searchpaths
/// pulled from envrionment variables:
///
/// - RMAN_SHADERPATH
/// - RMAN_TEXTUREPATH
/// - RMAN_RIXPLUGINPATH
/// - RMAN_PROCEDURALPATH
///
HDPRMAN_API
void 
HdPrman_UpdateSearchPathsFromEnvironment(RtParamList& options);

PXR_NAMESPACE_CLOSE_SCOPE

#endif // EXT_RMANPKG_24_0_PLUGIN_RENDERMAN_PLUGIN_HD_PRMAN_RENDER_PARAM_H
