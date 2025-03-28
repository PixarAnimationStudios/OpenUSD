//
// Copyright 2019 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "hdPrman/renderDelegate.h"
#include "hdPrman/basisCurves.h"
#include "hdPrman/camera.h"
#if PXR_VERSION >= 2208
#include "hdPrman/cone.h"
#include "hdPrman/cylinder.h"
#include "hdPrman/sphere.h"
#endif
#include "hdPrman/renderParam.h"
#include "hdPrman/renderBuffer.h"
#if PXR_VERSION >= 2308
#include "hdPrman/renderSettings.h"
#include "hdPrman/integrator.h"
#include "hdPrman/sampleFilter.h"
#include "hdPrman/displayFilter.h"
#endif
#include "hdPrman/coordSys.h"
#include "hdPrman/instancer.h"
#include "hdPrman/renderParam.h"
#include "hdPrman/renderPass.h"
#include "hdPrman/light.h"
#include "hdPrman/lightFilter.h"
#include "hdPrman/material.h"
#include "hdPrman/mesh.h"
#include "hdPrman/points.h"
#include "hdPrman/resourceRegistry.h"
#include "hdPrman/tokens.h"
#include "hdPrman/volume.h"
#include "hdPrman/sceneIndexObserverApi.h"

#ifdef HDPRMAN_USE_SCENE_INDEX_OBSERVER
#include "hdPrman/rileyPrimFactory.h"
#include "pxr/imaging/hdsi/primManagingSceneIndexObserver.h"
#include "pxr/imaging/hdsi/primTypeNoticeBatchingSceneIndex.h"
#endif

#include "pxr/imaging/hd/bprim.h"
#include "pxr/imaging/hd/camera.h"
#include "pxr/imaging/hd/extComputation.h"
#include "pxr/imaging/hd/rprim.h"
#include "pxr/imaging/hd/sceneIndex.h"
#include "pxr/imaging/hd/sprim.h"
#include "pxr/imaging/hd/tokens.h"
#include "pxr/imaging/hd/version.h"

#if HD_API_VERSION >= 60
#include "pxr/imaging/hd/renderCapabilitiesSchema.h"
#include "pxr/imaging/hd/retainedDataSource.h"
#endif

#include "pxr/base/tf/envSetting.h"
#include "pxr/base/tf/getenv.h"

PXR_NAMESPACE_OPEN_SCOPE

#if HD_API_VERSION >= 71
TF_DEFINE_ENV_SETTING(
    HD_PRMAN_ENABLE_PARALLEL_PRIM_SYNC, true,
    "Enables parallel prim Sync for supported prim types");
static bool _enableParallelPrimSync =
    TfGetEnvSetting(HD_PRMAN_ENABLE_PARALLEL_PRIM_SYNC);
#endif

// \class HdPrmanRenderDelegate::_RileySceneIndices.
//
// Holds the scene indices and scene index observers past the terminal scene
// index coming from the render index. The Hydra 2.0 implementation also
// relies on several plugin scene indices inserted by the render index (only if
// HD_PRMAN_EXPERIMENTAL_RILEY_SCENE_INDEX_OBSERVER is true).
//
// Overall, the scene indices are as follows:
//
// 1. HdPrman_RileyFallbackMaterialSceneIndexPlugin
//    Adds a hard-coded riley material at GetFallbackMaterialPath().
//
// 2. HdPrman_RileyConversionSceneIndexPlugin
//
//    Converts some hydra prims to riley prims (following, e.g.,
//    HdPrmanRileyGeometryPrototypeSchema).
//
//    Note that we can have some hydra prims be handled by converting them to
//    riley prims in this scene index and others go through emulation and
//    the Hydra 1.0 path.
//
//    For example, the scene index converts a sphere to a
//    riley:geometryPrototype and riley:geometryInstance. These prims will
//    be observed (see later) by
//    HdPrmanRenderDelegate::_RileySceneIndices::_primManagingSceneIndexObserver
//    which will issue the corresponding riley Create/Modify/Delete calls.
//    Because the original sphere has been converted to different prim types,
//    there is no instantiation of HdPrman_Sphere.
//    Also, note that we do not report riley:geometryPrototype or
//    riley:geometryInstance by any
//    HdPrmanRenderDelegate::GetSupported[RSB]primTypes().
//
//    Another example is mesh. The scene index does not convert a mesh.
//    mesh is reported by HdPrmanRenderDelegate::GetSupportedRprimTypes().
//    Thus, HdSceneIndexAdapterSceneDelegate will call _InsertRprim for
//    a mesh and thus we instantiate HdPrman_mesh.
//
// The conversion scene index is also the terminal scene index in the render
// index. However, _RileySceneIndices continues the chain of filtering scene
// indices and observers as follows:
//
// 3. HdsiPrimTypeNoticeBatchingSceneIndex _noticeBatchingSceneIndex
//
//    This scene index postpones any prim messages until we sync.
//    During sync (more precisely, in HdPrmanRenderDelegate::Update()), it
//    sorts and batches the messages to fulfill dependencies between prims.
//    E.g. the Riley::CreateGeometryInstance call needs the result of
//    Riley::CreateGeometryPrototype, so this scene index sends out the
//    messages for riley:geometryInstance first.
//
// 4. HdsiPrimManagingSceneIndexObserver _primManagingSceneIndexObserver
//
//    This observer calls, e.g., Riley::Create/Modify/DeleteGeometryInstance
//    in response to add/modify/delete prim messages.
//
struct HdPrmanRenderDelegate::_RileySceneIndices
{
#ifdef HDPRMAN_USE_SCENE_INDEX_OBSERVER
    _RileySceneIndices(
        HdSceneIndexBaseRefPtr const &terminalSceneIndex,
        HdPrman_RenderParam * const renderParam)
      : _noticeBatchingSceneIndex(
            HdsiPrimTypeNoticeBatchingSceneIndex::New(
                terminalSceneIndex,
                HdPrman_RileyPrimFactory::
                    GetPrimTypeNoticeBatchingSceneIndexInputArgs()))
      , _primManagingSceneIndexObserver(
            HdsiPrimManagingSceneIndexObserver::New(
                _noticeBatchingSceneIndex,
                _Args(renderParam)))
    {
    }

    static
    HdContainerDataSourceHandle
    _Args(HdPrman_RenderParam * const renderParam)
    {
        using DataSource =
            HdRetainedTypedSampledDataSource<
                HdsiPrimManagingSceneIndexObserver::PrimFactoryBaseHandle>;

        return
            HdRetainedContainerDataSource::New(
                HdsiPrimManagingSceneIndexObserverTokens->primFactory,
                DataSource::New(
                    std::make_shared<HdPrman_RileyPrimFactory>(renderParam)));
    }

    void Update()
    {
        _noticeBatchingSceneIndex->Flush();
    }

    HdsiPrimTypeNoticeBatchingSceneIndexRefPtr _noticeBatchingSceneIndex;
    HdsiPrimManagingSceneIndexObserverRefPtr _primManagingSceneIndexObserver;
#endif
};

extern TfEnvSetting<bool> HD_PRMAN_ENABLE_QUICKINTEGRATE;

TF_DEFINE_PRIVATE_TOKENS(
    _tokens,
    (openvdbAsset)
    (field3dAsset)
    (ri)
    ((outputsRi, "outputs:ri"))
    ((mtlxRenderContext, "mtlx"))
    (renderCameraPath)
    (DefaultMayaLight)
    (__FnKat_bbox)
);

TF_DEFINE_PUBLIC_TOKENS(HdPrmanRenderSettingsTokens,
    HDPRMAN_RENDER_SETTINGS_TOKENS);

TF_DEFINE_PUBLIC_TOKENS(HdPrmanExperimentalRenderSpecTokens,
    HDPRMAN_EXPERIMENTAL_RENDER_SPEC_TOKENS);

TF_DEFINE_PUBLIC_TOKENS(HdPrmanIntegratorTokens,
    HDPRMAN_INTEGRATOR_TOKENS);

TF_DEFINE_PUBLIC_TOKENS(HdPrmanProjectionTokens,
    HDPRMAN_PROJECTION_TOKENS);

TF_DEFINE_PUBLIC_TOKENS(HdPrmanRenderProductTokens,
    HDPRMAN_RENDER_PRODUCT_TOKENS);

TF_DEFINE_PUBLIC_TOKENS(HdPrmanAovSettingsTokens,
    HDPRMAN_AOV_SETTINGS_TOKENS);

#if PXR_VERSION <= 2308
TF_DEFINE_PUBLIC_TOKENS(HdAspectRatioConformPolicyTokens,
                        HD_ASPECT_RATIO_CONFORM_POLICY);
#endif

const TfTokenVector HdPrmanRenderDelegate::SUPPORTED_RPRIM_TYPES =
{
#if PXR_VERSION >= 2208
    HdPrimTypeTokens->cone,
    HdPrimTypeTokens->cylinder,
    HdPrimTypeTokens->sphere,
#endif
    HdPrimTypeTokens->mesh,
    HdPrimTypeTokens->basisCurves,
    HdPrimTypeTokens->points,
    HdPrimTypeTokens->volume,

    // New type, specific to mesh light source geom.
    HdPrmanTokens->meshLightSourceMesh,
    HdPrmanTokens->meshLightSourceVolume
};

const TfTokenVector HdPrmanRenderDelegate::SUPPORTED_SPRIM_TYPES =
{
    HdPrimTypeTokens->camera,
    HdPrimTypeTokens->material,
    HdPrimTypeTokens->distantLight,
    HdPrimTypeTokens->domeLight,
    HdPrimTypeTokens->light,
    HdPrimTypeTokens->lightFilter,
    HdPrimTypeTokens->rectLight,
    HdPrimTypeTokens->diskLight,
    HdPrimTypeTokens->cylinderLight,
    HdPrimTypeTokens->sphereLight,
#if PXR_VERSION <= 2211
    HdPrmanTokens->meshLight,
#else
    HdPrimTypeTokens->meshLight,
#endif
    HdPrimTypeTokens->pluginLight,
    HdPrimTypeTokens->extComputation,
    HdPrimTypeTokens->coordSys,
#if PXR_VERSION >= 2308
    HdPrimTypeTokens->integrator,
    HdPrimTypeTokens->sampleFilter,
    HdPrimTypeTokens->displayFilter,
#endif
};

const TfTokenVector HdPrmanRenderDelegate::SUPPORTED_BPRIM_TYPES =
{
    HdPrimTypeTokens->renderBuffer,
#if PXR_VERSION >= 2308
    HdPrimTypeTokens->renderSettings,
#endif
    _tokens->openvdbAsset,
#ifndef HDPRMAN_DISABLE_FIELD3D
    _tokens->field3dAsset,
#endif
};

static
std::string
_ToLower(const std::string &s)
{
    std::string result = s;
    for(auto &c : result) {
        c = tolower(c);
    }
    return result;
}

static
std::vector<std::string>
_GetExtraArgs(const HdRenderSettingsMap &settingsMap)
{
    std::string extraArgs;

    auto it = settingsMap.find(HdPrmanRenderSettingsTokens->batchCommandLine);
    if (it != settingsMap.end()) {
        // husk's --delegate-options arg allows users to pass an arbitrary
        // string of args which we pass along to PRManBegin.
        if (it->second.IsHolding<VtArray<std::string>>()) {
            const VtArray<std::string>& v =
                it->second.UncheckedGet<VtArray<std::string>>();
            for (VtArray<std::string>::const_iterator i = v.cbegin();
                 i != v.cend(); ++i) {
                if (*i == "--delegate-options") {
                    ++i;
                    if(i != v.cend()) {
                        extraArgs = *i;
                    }
                }
            }
        }
    }
    return TfStringTokenize(extraArgs, " ");
}

std::string
HdPrmanRenderDelegate::_GetRenderVariant(const HdRenderSettingsMap &settingsMap)
{
    std::string renderVariant;
    auto it = settingsMap.find(HdPrmanRenderSettingsTokens->renderVariant);
    if(it != settingsMap.end()) {
        assert(it->second.IsHolding<TfToken>());
        renderVariant = it->second.UncheckedGet<TfToken>().GetText();
    } else {
        renderVariant =
                _ToLower(
                    GetRenderSetting<std::string>(
                        HdPrmanRenderSettingsTokens->rileyVariant,
                        TfGetenv("RILEY_VARIANT")));
    }
    return renderVariant;
}

int
HdPrmanRenderDelegate::_GetCpuConfig(const HdRenderSettingsMap &settingsMap)
{
    int xpuCpuConfig = 1;

    auto it = settingsMap.find(HdPrmanRenderSettingsTokens->xpuDevices);
    if( it != settingsMap.end()) {
        std::string xpuDevices = it->second.UncheckedGet<std::string>();
        xpuCpuConfig = xpuDevices.find("cpu") != std::string::npos;
    } else {
        auto it = settingsMap.find(HdPrmanRenderSettingsTokens->xpuCpuConfig);
        if (it != settingsMap.end()) {
            xpuCpuConfig = it->second.UncheckedGet<int>();
        }
    }
    return xpuCpuConfig;
}

std::vector<int>
HdPrmanRenderDelegate::_GetGpuConfig(const HdRenderSettingsMap &settingsMap)
{
    std::vector<int> xpuGpuConfig;

    auto it = settingsMap.find(HdPrmanRenderSettingsTokens->xpuDevices);
    if( it != settingsMap.end()) {
        std::string xpuDevices = it->second.UncheckedGet<std::string>();
        if (xpuDevices.find("gpu") != std::string::npos) {
            xpuGpuConfig.push_back(0);
        }
    } else {
        auto it = settingsMap.find(HdPrmanRenderSettingsTokens->xpuGpuConfig);
        if (it != settingsMap.end()) {
            xpuGpuConfig = it->second.UncheckedGet< std::vector<int> >();
        }
    }
    return xpuGpuConfig;
}

HdPrmanRenderDelegate::HdPrmanRenderDelegate(
    HdRenderSettingsMap const& settingsMap)
  : HdRenderDelegate(settingsMap)
  , _renderParam(
      std::make_unique<HdPrman_RenderParam>(
                     this,
                     _GetRenderVariant(settingsMap),
                     _GetCpuConfig(settingsMap),
                     _GetGpuConfig(settingsMap),
                     _GetExtraArgs(settingsMap)))
{
    if(_renderParam->IsValid()) {
        _Initialize();
    }
}

bool
HdPrmanRenderDelegate::IsInteractive() const
{
    return GetRenderSetting<bool>(
        HdRenderSettingsTokens->enableInteractive, true);
}

void
HdPrmanRenderDelegate::_Initialize()
{
    // Prepare list of render settings descriptors
    // TODO: With this approach some settings will need to be updated as the
    // defaults change in Renderman. Although these defaults are unlikely to
    // change we should either change how settings defaults are obtained or
    // automate using PRManOptions.args.
    _settingDescriptors.reserve(5);

    const std::string integrator = TfGetenv(
        "HD_PRMAN_INTEGRATOR", HdPrmanIntegratorTokens->PxrPathTracer);
    _settingDescriptors.push_back({
        std::string("Integrator"),
        HdPrmanRenderSettingsTokens->integratorName,
        VtValue(integrator)
    });

    if (TfGetEnvSetting(HD_PRMAN_ENABLE_QUICKINTEGRATE)) {
        const std::string interactiveIntegrator =
            HdPrmanIntegratorTokens->PxrDirectLighting;
        _settingDescriptors.push_back({
            std::string("Interactive Integrator"),
            HdPrmanRenderSettingsTokens->interactiveIntegrator,
            VtValue(interactiveIntegrator)
        });

        // If >0, the time in ms that we'll render quick output before switching
        // to path tracing
        _settingDescriptors.push_back({
            std::string("Interactive Integrator Timeout (ms)"),
            HdPrmanRenderSettingsTokens->interactiveIntegratorTimeout,
            VtValue(200)
        });
    }

#if _PRMANAPI_VERSION_MAJOR_ >= 26
    const int maxSamplesDefault = 0;
#else
    const int maxSamplesDefault = 64;
#endif
    const int maxSamples = TfGetenvInt("HD_PRMAN_MAX_SAMPLES",
                                       maxSamplesDefault);
    _settingDescriptors.push_back({
        std::string("Max Samples"),
        HdRenderSettingsTokens->convergedSamplesPerPixel,
        VtValue(maxSamples)
    });

#if _PRMANAPI_VERSION_MAJOR_ >= 26
    const float pixelVariance = 0.015f;
#else
    const float pixelVariance = 0.001f;
#endif
    _settingDescriptors.push_back({
        std::string("Variance Threshold"),
        HdRenderSettingsTokens->convergedVariance,
        VtValue(pixelVariance)
    });

    _settingDescriptors.push_back({
        std::string("Riley Variant"),
        HdPrmanRenderSettingsTokens->rileyVariant,
        VtValue(TfGetenv("RILEY_VARIANT"))
    });

    _settingDescriptors.push_back({
        std::string("Disable Motion Blur"),
        HdPrmanRenderSettingsTokens->disableMotionBlur,
        VtValue(false)
    });

    _PopulateDefaultSettings(_settingDescriptors);

    _renderParam->Begin(this);

    _resourceRegistry = std::make_shared<HdPrman_ResourceRegistry>(
        _renderParam);
}

HdPrmanRenderDelegate::~HdPrmanRenderDelegate() = default;

HdRenderSettingsMap
HdPrmanRenderDelegate::GetRenderSettingsMap() const
{
    return _settingsMap;
}

HdRenderSettingDescriptorList
HdPrmanRenderDelegate::GetRenderSettingDescriptors() const
{
    return _settingDescriptors;
}

HdRenderParam*
HdPrmanRenderDelegate::GetRenderParam() const
{
    return _renderParam.get();
}

VtDictionary
HdPrmanRenderDelegate::GetRenderStats() const
{
    VtDictionary stats;
    _renderParam->UpdateRenderStats(stats);
    return stats;
}

void
HdPrmanRenderDelegate::CommitResources(HdChangeTracker *tracker)
{
    // Do nothing
}

TfTokenVector const&
HdPrmanRenderDelegate::GetSupportedRprimTypes() const
{
    return SUPPORTED_RPRIM_TYPES;
}

TfTokenVector const&
HdPrmanRenderDelegate::GetSupportedSprimTypes() const
{
    return SUPPORTED_SPRIM_TYPES;
}

TfTokenVector const&
HdPrmanRenderDelegate::GetSupportedBprimTypes() const
{
    return SUPPORTED_BPRIM_TYPES;
}

HdResourceRegistrySharedPtr
HdPrmanRenderDelegate::GetResourceRegistry() const
{
    return _resourceRegistry;
}

HdRenderPassSharedPtr
HdPrmanRenderDelegate::CreateRenderPass(HdRenderIndex *index,
                                        HdRprimCollection const& collection)
{
    if (!_renderPass) {
        _renderPass = std::make_shared<HdPrman_RenderPass>(
            index, collection, _renderParam);
    }
    return _renderPass;
}

HdInstancer *
HdPrmanRenderDelegate::CreateInstancer(HdSceneDelegate *delegate,
                                       SdfPath const& id)
{
    return new HdPrmanInstancer(delegate, id);
}

void
HdPrmanRenderDelegate::DestroyInstancer(HdInstancer *instancer)
{
    delete instancer;
}

HdRprim *
HdPrmanRenderDelegate::CreateRprim(TfToken const& typeId,
                                    SdfPath const& rprimId)
{
    // Weirdly, Katana6 produces bboxes around lights
    // that render as meshes, so ignore them by name.
    if(rprimId.GetName() == _tokens->__FnKat_bbox) {
        return nullptr;
    }
    if (typeId == HdPrmanTokens->meshLightSourceMesh) {
        return new HdPrman_Mesh(rprimId, true /* isMeshLight */);
    } else if (typeId == HdPrmanTokens->meshLightSourceVolume) {
        return new HdPrman_Volume(rprimId, true /* isMeshLight */);
    } else if (typeId == HdPrimTypeTokens->mesh) {
        return new HdPrman_Mesh(rprimId, false /* isMeshLight */);
    } else if (typeId == HdPrimTypeTokens->basisCurves) {
        return new HdPrman_BasisCurves(rprimId);
#if PXR_VERSION >= 2208
    } if (typeId == HdPrimTypeTokens->cone) {
        return new HdPrman_Cone(rprimId);
    } if (typeId == HdPrimTypeTokens->cylinder) {
        return new HdPrman_Cylinder(rprimId);
    } if (typeId == HdPrimTypeTokens->sphere) {
        return new HdPrman_Sphere(rprimId);
#endif
    } else if (typeId == HdPrimTypeTokens->points) {
        return new HdPrman_Points(rprimId);
    } else if (typeId == HdPrimTypeTokens->volume) {
        return new HdPrman_Volume(rprimId, false /* isMeshLight */);
    } else {
        TF_CODING_ERROR("Unknown Rprim Type %s", typeId.GetText());
    }

    return nullptr;
}

void
HdPrmanRenderDelegate::DestroyRprim(HdRprim *rPrim)
{
    delete rPrim;
}

HdSprim *
HdPrmanRenderDelegate::CreateSprim(TfToken const& typeId,
                                    SdfPath const& sprimId)
{
    HdSprim* sprim = nullptr;
    if (typeId == HdPrimTypeTokens->camera) {
        sprim = new HdPrmanCamera(sprimId);
    } else if (typeId == HdPrimTypeTokens->material) {
        sprim = new HdPrmanMaterial(sprimId);
    } else if (typeId == HdPrimTypeTokens->coordSys) {
        sprim = new HdPrmanCoordSys(sprimId);
    } else if (typeId == HdPrimTypeTokens->lightFilter) {
        sprim = new HdPrmanLightFilter(sprimId, typeId);
    } else if (typeId == HdPrimTypeTokens->light ||
               typeId == HdPrimTypeTokens->distantLight ||
               typeId == HdPrimTypeTokens->domeLight ||
               typeId == HdPrimTypeTokens->rectLight ||
               typeId == HdPrimTypeTokens->diskLight ||
               typeId == HdPrimTypeTokens->cylinderLight ||
               typeId == HdPrimTypeTokens->sphereLight ||
#if PXR_VERSION <= 2305
               typeId == HdPrmanTokens->meshLight ||
#else
               typeId == HdPrimTypeTokens->meshLight ||
#endif
               typeId == HdPrimTypeTokens->pluginLight) {
        if(typeId == HdPrimTypeTokens->distantLight &&
           (sprimId.GetString().rfind(_tokens->DefaultMayaLight) !=
            std::string::npos)) {
            // The default maya distant light causes bad behavior in prman;
            // not sure why
        } else {
            sprim = new HdPrmanLight(sprimId, typeId);
        }

        // Disregard fallback prims in count.
        if (sprim && sprim->GetId() != SdfPath()) {
            _renderParam->IncreaseSceneLightCount();
        }
    } else if (typeId == HdPrimTypeTokens->extComputation) {
        sprim = new HdExtComputation(sprimId);
#if PXR_VERSION >= 2308
    } else if (typeId == HdPrimTypeTokens->integrator) {
        sprim = new HdPrman_Integrator(sprimId);
    } else if (typeId == HdPrimTypeTokens->sampleFilter) {
        sprim = new HdPrman_SampleFilter(sprimId);
    } else if (typeId == HdPrimTypeTokens->displayFilter) {
        sprim = new HdPrman_DisplayFilter(sprimId);
#endif
    } else {
        TF_CODING_ERROR("Unknown Sprim Type %s", typeId.GetText());
    }

    return sprim;
}

HdSprim *
HdPrmanRenderDelegate::CreateFallbackSprim(TfToken const& typeId)
{
    // For fallback sprims, create objects with an empty scene path.
    // They'll use default values and won't be updated by a scene delegate.
    if (typeId == HdPrimTypeTokens->camera) {
        return new HdPrmanCamera(SdfPath::EmptyPath());
    } else if (typeId == HdPrimTypeTokens->material) {
        return new HdPrmanMaterial(SdfPath::EmptyPath());
    } else if (typeId == HdPrimTypeTokens->coordSys) {
        return new HdPrmanCoordSys(SdfPath::EmptyPath());
    } else if (typeId == HdPrimTypeTokens->lightFilter) {
        return new HdPrmanLightFilter(SdfPath::EmptyPath(), typeId);
    } else if (typeId == HdPrimTypeTokens->light ||
               typeId == HdPrimTypeTokens->distantLight ||
               typeId == HdPrimTypeTokens->domeLight ||
               typeId == HdPrimTypeTokens->rectLight ||
               typeId == HdPrimTypeTokens->diskLight ||
               typeId == HdPrimTypeTokens->cylinderLight ||
               typeId == HdPrimTypeTokens->sphereLight ||
#if PXR_VERSION <= 2305
               typeId == HdPrmanTokens->meshLight ||
#else
               typeId == HdPrimTypeTokens->meshLight ||
#endif
               typeId == HdPrimTypeTokens->pluginLight) {
        return new HdPrmanLight(SdfPath::EmptyPath(), typeId);
    } else if (typeId == HdPrimTypeTokens->extComputation) {
        return new HdExtComputation(SdfPath::EmptyPath());
#if PXR_VERSION >= 2308
    } else if (typeId == HdPrimTypeTokens->integrator) {
        return new HdPrman_Integrator(SdfPath::EmptyPath());
    } else if (typeId == HdPrimTypeTokens->sampleFilter) {
        return new HdPrman_SampleFilter(SdfPath::EmptyPath());
    } else if (typeId == HdPrimTypeTokens->displayFilter) {
        return new HdPrman_DisplayFilter(SdfPath::EmptyPath());
#endif
    } else {
        TF_CODING_ERROR("Unknown Sprim Type %s", typeId.GetText());
    }

    return nullptr;
}

void
HdPrmanRenderDelegate::DestroySprim(HdSprim *sprim)
{
    if(dynamic_cast<HdPrmanLight*>(sprim)) {
        // Disregard fallback prims in count.
        if (sprim->GetId() != SdfPath()) {
            _renderParam->DecreaseSceneLightCount();
        }
    }
    delete sprim;
}

HdBprim *
HdPrmanRenderDelegate::CreateBprim(
    TfToken const& typeId,
    SdfPath const& bprimId)
{
    if (typeId == _tokens->openvdbAsset
#ifndef HDPRMAN_DISABLE_FIELD3D
        || typeId == _tokens->field3dAsset
#endif
        ) {
        return new HdPrman_Field(typeId, bprimId);
    } else if (typeId == HdPrimTypeTokens->renderBuffer) {
        return new HdPrmanRenderBuffer(bprimId);
#if PXR_VERSION >= 2308
    } else if (typeId == HdPrimTypeTokens->renderSettings) {
        return new HdPrman_RenderSettings(bprimId);
#endif
    } else {
        TF_CODING_ERROR("Unknown Bprim Type %s", typeId.GetText());
    }
    return nullptr;
}

HdBprim *
HdPrmanRenderDelegate::CreateFallbackBprim(TfToken const& typeId)
{
    if (typeId == _tokens->openvdbAsset
#ifndef HDPRMAN_DISABLE_FIELD3D
        || typeId == _tokens->field3dAsset
#endif
        ) {
        return new HdPrman_Field(typeId, SdfPath::EmptyPath());
    } else if (typeId == HdPrimTypeTokens->renderBuffer) {
        return new HdPrmanRenderBuffer(SdfPath::EmptyPath());
#if PXR_VERSION >= 2308
    } else if (typeId == HdPrimTypeTokens->renderSettings) {
        return new HdPrman_RenderSettings(SdfPath::EmptyPath());
#endif
    } else {
        TF_CODING_ERROR("Unknown Bprim Type %s", typeId.GetText());
    }
    return nullptr;
}

void
HdPrmanRenderDelegate::DestroyBprim(HdBprim *bPrim)
{
    delete bPrim;
}

HdAovDescriptor
HdPrmanRenderDelegate::GetDefaultAovDescriptor(
    TfToken const& name) const
{
    if (name == HdAovTokens->color) {
        return HdAovDescriptor(
            HdFormatFloat32Vec4,
            false,
            VtValue(GfVec4f(0.0f)));
    } else if (name == HdAovTokens->depth) {
        return HdAovDescriptor(HdFormatFloat32, false, VtValue(1.0f));
    } else if (name == HdAovTokens->primId ||
               name == HdAovTokens->instanceId ||
               name == HdAovTokens->elementId) {
        return HdAovDescriptor(HdFormatInt32, false, VtValue(-1));
    }
    return HdAovDescriptor(
        HdFormatFloat32Vec3,
        false,
        VtValue(GfVec3f(0.0f)));
}

TfToken
HdPrmanRenderDelegate::GetMaterialBindingPurpose() const
{
    return HdTokens->full;
}

TfTokenVector
HdPrmanRenderDelegate::GetMaterialRenderContexts() const
{
#ifdef PXR_MATERIALX_SUPPORT_ENABLED
    return {_tokens->ri, _tokens->mtlxRenderContext};
#else
    return {_tokens->ri};
#endif
}

TfTokenVector
HdPrmanRenderDelegate::GetShaderSourceTypes() const
{
    return HdPrmanMaterial::GetShaderSourceTypes();
}

#if HD_API_VERSION > 46
TfTokenVector
HdPrmanRenderDelegate::GetRenderSettingsNamespaces() const
{
#if PXR_VERSION <= 2403
    return {_tokens->ri, _tokens->outputsRi};
#else
    return {_tokens->ri};
#endif
}
#endif

#if HD_API_VERSION >= 60
HdContainerDataSourceHandle
HdPrmanRenderDelegate::GetCapabilities() const
{
    static const HdContainerDataSourceHandle result =
        HdRenderCapabilitiesSchema::Builder()
            .SetMotionBlur(
                HdRetainedTypedSampledDataSource<bool>::New(true))
            .Build();
    return result;
}
#endif

void
HdPrmanRenderDelegate::SetRenderSetting(TfToken const &key,
                                        VtValue const &value)
{
    HdRenderDelegate::SetRenderSetting(key, value);

    if(key == _tokens->renderCameraPath)
    {
        // Need to know the name of the render camera as soon as possible
        // so that as cameras are processed (directly after render settings),
        // the shutter of the active camera can be passed to riley,
        // prior to handling any geometry.
        SdfPath camPath = value.UncheckedGet<SdfPath>();
        _renderParam->GetCameraContext().SetCameraPath(camPath);
        _renderParam->GetCameraContext().MarkCameraInvalid(camPath);
        HdRenderIndex *renderIndex = GetRenderIndex();
        if(renderIndex) {
            // Needed to trigger call to
            // param->SetRileyShutterIntervalFromCameraContextCameraPath
            // from HdPrmanCamera::Sync.

            renderIndex->GetChangeTracker().MarkSprimDirty(
                camPath, HdChangeTracker::DirtyParams);
        }
    }
}

bool
HdPrmanRenderDelegate::IsStopSupported() const
{
    if (IsInteractive()) {
        return true;
    }
    return false;
}

bool
HdPrmanRenderDelegate::IsStopped() const
{
    if (IsInteractive()) {
        return !_renderParam->IsRendering();
    }
    return true;
}

bool
HdPrmanRenderDelegate::Stop(bool blocking)
{
    if (IsInteractive()) {
        _renderParam->StopRender(blocking);
        return !_renderParam->IsRendering();
    }
    return true;
}

bool
HdPrmanRenderDelegate::Restart()
{
    if (IsInteractive()) {
        // Next call into HdPrman_RenderPass::_Execute will do a StartRender
        _renderParam->sceneVersion++;
        return true;
    }
    return false;
}

bool
HdPrmanRenderDelegate::Pause()
{
    if (IsInteractive()) {
        _renderParam->StopRender();
    }
    return true;
}

bool
HdPrmanRenderDelegate::Resume()
{
    if (IsInteractive()) {
        // Indicate that render should start
        // at next HdxPrman_RenderPass::_Execute
        if (!_renderParam->IsRendering()) {
            _renderParam->sceneVersion++;
        }
    }
    return true;
}

HdRenderIndex*
HdPrmanRenderDelegate::GetRenderIndex() const
{
    if (_renderPass) {
        return _renderPass->GetRenderIndex();
    }
    return nullptr;
}

#if HD_API_VERSION >= 55

////////////////////////////////////////////////////////////////////////////
///
/// Hydra 2.0 API
///
////////////////////////////////////////////////////////////////////////////

void
HdPrmanRenderDelegate::SetTerminalSceneIndex(
    const HdSceneIndexBaseRefPtr &terminalSceneIndex)
{
#ifdef HDPRMAN_USE_SCENE_INDEX_OBSERVER
    if (terminalSceneIndex) {
        if (!_rileySceneIndices) {
            _rileySceneIndices =
                std::make_unique<_RileySceneIndices>(
                    terminalSceneIndex, _renderParam.get());
        }
    }
#endif
}

void
HdPrmanRenderDelegate::Update()
{
#ifdef HDPRMAN_USE_SCENE_INDEX_OBSERVER
    if (_rileySceneIndices) {
        _rileySceneIndices->Update();
    }
#endif
}

#if HD_API_VERSION >= 71
bool
HdPrmanRenderDelegate::IsParallelSyncEnabled(const TfToken &primType) const
{
    // The prim types below have been reviewed for Sync thread safety.
    //
    // Notable exceptions include integrator, renderSettings,
    // volume, and lights.  These exceptions are generally due
    // to interaction with HdChangeTracker state.
    return
        _enableParallelPrimSync && (
        primType == HdPrimTypeTokens->camera ||
        primType == HdPrimTypeTokens->coordSys ||
        primType == HdPrimTypeTokens->displayFilter ||
        primType == HdPrimTypeTokens->lightFilter ||
        primType == HdPrimTypeTokens->material ||
        primType == HdPrimTypeTokens->sampleFilter);
}
#endif

#endif

PXR_NAMESPACE_CLOSE_SCOPE
