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

PXR_NAMESPACE_OPEN_SCOPE

class HdPrmanCamera;
class HdPrmanCameraContext;
class HdPrmanRenderDelegate;

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
    HdPrman_RenderParam();

    HDPRMAN_API
    ~HdPrman_RenderParam() override;

    virtual
    void Begin(HdPrmanRenderDelegate *renderDelegate) = 0; 

    // Convert any Hydra primvars that should be Riley instance attributes.
    HDPRMAN_API
    RtParamList
    ConvertAttributes(HdSceneDelegate *sceneDelegate, SdfPath const& id);

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
                        HdPrmanCamera *camera,
                        std::string const& integratorName,
                        RtParamList& params);

    // Callback to convert any camera settings that should become
    // parameters on the integrator.
    using IntegratorCameraCallback = void (*)
        (HdPrmanRenderDelegate *renderDelegate,
         HdPrmanCamera *camera,
         std::string const& integratorName,
         RtParamList &integratorParams);

    // Register a callback to process integrator settings
    HDPRMAN_API
    static void 
    RegisterIntegratorCallbackForCamera(
        IntegratorCameraCallback const& callback);

    HDPRMAN_API
    bool IsShutterInstantaneous() const;

    HDPRMAN_API
    void SetInstantaneousShutter(bool instantaneousShutter);

    // Get RIX vs XPU
    bool IsXpu() const { return _xpu; }

    // Adds VtValue contents to RtParamList
    bool SetParamFromVtValue(
        RtUString const& name,
        VtValue const& val,
        TfToken const& role,
        RtParamList& params);

    // Request edit access to the Riley scene and return it.
    virtual riley::Riley * AcquireRiley() = 0;

    // Provides external access to resources used to set parameters for
    // options and the active integrator.
    virtual riley::IntegratorId GetActiveIntegratorId() = 0;

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

    void UpdateIntegrator(HdRenderDelegate * renderDelegate);

    riley::IntegratorId GetIntegratorId() const { return _integratorId; }

    RtParamList &GetIntegratorParams() { return _integratorParams; }

    bool HasSceneLights() const { return _sceneLightCount > 0; }
    void IncreaseSceneLightCount() { ++_sceneLightCount; }
    void DecreaseSceneLightCount() { --_sceneLightCount; }
    
    // Provides external access to resources used to set parameters for
    // options and the active integrator.
    RtParamList &GetOptions() { return _options; }
    HdPrmanCameraContext &GetCameraContext() { return _cameraContext; }

    HdPrmanRenderViewContext &GetRenderViewContext() {
        return _renderViewContext;
    }

protected:
    void _CreateRiley();
    void _CreateFallbackMaterials();
    void _CreateFallbackLight();
    void _CreateIntegrator(HdRenderDelegate * renderDelegate);

    void _DestroyRiley();

    // Top-level entrypoint to PRMan.
    // Singleton used to access RixInterfaces.
    RixContext *_rix;

    // RixInterface for PRManBegin/End.
    RixRiCtl *_ri;

    // RixInterface for Riley.
    RixRileyManager *_mgr;

    // Xcpt Handler
    HdPrman_Xcpt _xcpt;

    // Riley instance.
    riley::Riley *_riley;

private:
    riley::ShadingNode _ComputeIntegratorNode(
        HdRenderDelegate * renderDelegate);

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

    // Coordinate system conversion cache.
    _GeomToHdCoordSysMap _geomToHdCoordSysMap;
    _HdToRileyCoordSysMap _hdToRileyCoordSysMap;
    std::mutex _coordSysMutex;

    RtParamList _options;
    HdPrmanCameraContext _cameraContext;
    HdPrmanRenderViewContext _renderViewContext;

    // A quick way to disable motion blur, making shutter close same as open
    bool _instantaneousShutter;

    // RIX or XPU
    bool _xpu;

    int _lastSettingsVersion;
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
