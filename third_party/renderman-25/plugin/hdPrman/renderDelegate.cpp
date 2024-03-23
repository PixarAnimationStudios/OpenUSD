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
#include "hdPrman/renderDelegate.h"
#include "hdPrman/basisCurves.h"
#include "hdPrman/camera.h"
#include "hdPrman/cone.h"
#include "hdPrman/cylinder.h"
#include "hdPrman/sphere.h"
#include "hdPrman/renderParam.h"
#include "hdPrman/renderBuffer.h"
#include "hdPrman/renderSettings.h"
#include "hdPrman/integrator.h"
#include "hdPrman/sampleFilter.h"
#include "hdPrman/displayFilter.h"
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
#include "hdPrman/terminalSceneIndexObserver.h"
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
#if HD_API_VERSION >= 60
#include "pxr/imaging/hd/renderCapabilitiesSchema.h"
#include "pxr/imaging/hd/retainedDataSource.h"
#endif

#include "pxr/base/tf/envSetting.h"
#include "pxr/base/tf/getenv.h"

PXR_NAMESPACE_OPEN_SCOPE

#ifdef HDPRMAN_USE_SCENE_INDEX_OBSERVER

TF_DEFINE_ENV_SETTING(HD_PRMAN_EXPERIMENTAL_RILEY_SCENE_INDEX_OBSERVER, false,
                      "Enable scene index observer calling the Riley API for "
                      "the prims in the terminal scene index. This is scene "
                      "index observer is the first step towards a future "
                      "Hydra 2.0 implementation. "
                      "See HdPrmanRenderDelegate::_RileySceneIndices for more.");

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
    ((mtlxRenderContext, "mtlx"))
    (renderCameraPath)
);

TF_DEFINE_PUBLIC_TOKENS(HdPrmanRenderSettingsTokens,
    HDPRMAN_RENDER_SETTINGS_TOKENS);

TF_DEFINE_PUBLIC_TOKENS(HdPrmanExperimentalRenderSpecTokens,
    HDPRMAN_EXPERIMENTAL_RENDER_SPEC_TOKENS);

TF_DEFINE_PUBLIC_TOKENS(HdPrmanIntegratorTokens,
    HDPRMAN_INTEGRATOR_TOKENS);

TF_DEFINE_PUBLIC_TOKENS(HdPrmanRenderProductTokens,
    HDPRMAN_RENDER_PRODUCT_TOKENS);

TF_DEFINE_PUBLIC_TOKENS(HdPrmanAovSettingsTokens,
    HDPRMAN_AOV_SETTINGS_TOKENS);


const TfTokenVector HdPrmanRenderDelegate::SUPPORTED_RPRIM_TYPES =
{
    HdPrimTypeTokens->cone,
    HdPrimTypeTokens->cylinder,
    HdPrimTypeTokens->sphere,
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
#if PXR_VERSION < 2302
    HdPrmanTokens->meshLight,
#else
    HdPrimTypeTokens->meshLight,
#endif
    HdPrimTypeTokens->pluginLight,
    HdPrimTypeTokens->extComputation,
    HdPrimTypeTokens->coordSys,
    HdPrimTypeTokens->integrator,
    HdPrimTypeTokens->sampleFilter,
    HdPrimTypeTokens->displayFilter,
};

const TfTokenVector HdPrmanRenderDelegate::SUPPORTED_BPRIM_TYPES =
{
    HdPrimTypeTokens->renderBuffer,
    HdPrimTypeTokens->renderSettings,
    _tokens->openvdbAsset,
    _tokens->field3dAsset,
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

HdPrmanRenderDelegate::HdPrmanRenderDelegate(
    HdRenderSettingsMap const& settingsMap)
  : HdRenderDelegate(settingsMap)
{
    std::string rileyVariant = _ToLower(
              GetRenderSetting<std::string>(
                  HdPrmanRenderSettingsTokens->rileyVariant,
                  TfGetenv("RILEY_VARIANT")));

    std::string xpuDevices = GetRenderSetting<std::string>(
        HdPrmanRenderSettingsTokens->xpuDevices, std::string());

    _renderParam = std::make_unique<HdPrman_RenderParam>(
        this, rileyVariant, xpuDevices, _GetExtraArgs(settingsMap));

    _Initialize();
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
    std::string integrator = HdPrmanIntegratorTokens->PxrPathTracer;
    std::string integratorEnv = TfGetenv("HD_PRMAN_INTEGRATOR");
    if (!integratorEnv.empty()) {
        integrator = integratorEnv;
    }
 
    const int maxSamples = 64; // 64 samples is RenderMan default
    const float pixelVariance = 0.001f;

    // Prepare list of render settings descriptors
    _settingDescriptors.reserve(5);

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

    _settingDescriptors.push_back({
        std::string("Max Samples"),
        HdRenderSettingsTokens->convergedSamplesPerPixel,
        VtValue(maxSamples)
    });

    _settingDescriptors.push_back({
        std::string("Variance Threshold"),
        HdRenderSettingsTokens->convergedVariance,
        VtValue(pixelVariance)
    });

    _settingDescriptors.push_back({
        std::string("Riley variant"),
        HdPrmanRenderSettingsTokens->rileyVariant,
        VtValue(TfGetenv("RILEY_VARIANT"))
    });

    _settingDescriptors.push_back({
        std::string("Disable motion blur"),
        HdPrmanRenderSettingsTokens->disableMotionBlur,
        VtValue(false)});

    _PopulateDefaultSettings(_settingDescriptors);

    _renderParam->Begin(this);

    _resourceRegistry = std::make_shared<HdPrman_ResourceRegistry>(
        _renderParam);
}

HdPrmanRenderDelegate::~HdPrmanRenderDelegate()
{
    _renderParam.reset();
}

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
    if (typeId == HdPrmanTokens->meshLightSourceMesh) {
        return new HdPrman_Mesh(rprimId, true /* isMeshLight */);
    } else if (typeId == HdPrmanTokens->meshLightSourceVolume) {
        return new HdPrman_Volume(rprimId, true /* isMeshLight */);
    } else if (typeId == HdPrimTypeTokens->mesh) {
        return new HdPrman_Mesh(rprimId, false /* isMeshLight */);
    } else if (typeId == HdPrimTypeTokens->basisCurves) {
        return new HdPrman_BasisCurves(rprimId);
    } if (typeId == HdPrimTypeTokens->cone) {
        return new HdPrman_Cone(rprimId);
    } if (typeId == HdPrimTypeTokens->cylinder) {
        return new HdPrman_Cylinder(rprimId);
    } if (typeId == HdPrimTypeTokens->sphere) {
        return new HdPrman_Sphere(rprimId);
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
#if PXR_VERSION < 2302
               typeId == HdPrmanTokens->meshLight ||
#else
               typeId == HdPrimTypeTokens->meshLight ||
#endif
               typeId == HdPrimTypeTokens->pluginLight) {
        sprim = new HdPrmanLight(sprimId, typeId);

        // Disregard fallback prims in count.
        if (sprim->GetId() != SdfPath()) {
            _renderParam->IncreaseSceneLightCount();
        }
    } else if (typeId == HdPrimTypeTokens->extComputation) {
        sprim = new HdExtComputation(sprimId);
    } else if (typeId == HdPrimTypeTokens->integrator) {
        sprim = new HdPrman_Integrator(sprimId);
    } else if (typeId == HdPrimTypeTokens->sampleFilter) {
        sprim = new HdPrman_SampleFilter(sprimId);
    } else if (typeId == HdPrimTypeTokens->displayFilter) {
        sprim = new HdPrman_DisplayFilter(sprimId);
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
#if PXR_VERSION < 2302
               typeId == HdPrmanTokens->meshLight ||
#else
               typeId == HdPrimTypeTokens->meshLight ||
#endif
               typeId == HdPrimTypeTokens->pluginLight) {
        return new HdPrmanLight(SdfPath::EmptyPath(), typeId);
    } else if (typeId == HdPrimTypeTokens->extComputation) {
        return new HdExtComputation(SdfPath::EmptyPath());
    } else if (typeId == HdPrimTypeTokens->integrator) {
        return new HdPrman_Integrator(SdfPath::EmptyPath());
    } else if (typeId == HdPrimTypeTokens->sampleFilter) {
        return new HdPrman_SampleFilter(SdfPath::EmptyPath());
    } else if (typeId == HdPrimTypeTokens->displayFilter) {
        return new HdPrman_DisplayFilter(SdfPath::EmptyPath());
    } else {
        TF_CODING_ERROR("Unknown Sprim Type %s", typeId.GetText());
    }

    return nullptr;
}

void
HdPrmanRenderDelegate::DestroySprim(HdSprim *sprim)
{
    // Disregard fallback prims in count.
    if (sprim->GetId() != SdfPath()) {
        _renderParam->DecreaseSceneLightCount();
    }
    delete sprim;
}

HdBprim *
HdPrmanRenderDelegate::CreateBprim(
    TfToken const& typeId,
    SdfPath const& bprimId)
{
    if (typeId == _tokens->openvdbAsset ||
        typeId == _tokens->field3dAsset) {
        return new HdPrman_Field(typeId, bprimId);
    } else if (typeId == HdPrimTypeTokens->renderBuffer) {
        return new HdPrmanRenderBuffer(bprimId);
    } else if (typeId == HdPrimTypeTokens->renderSettings) {
        return new HdPrman_RenderSettings(bprimId);
    } else {
        TF_CODING_ERROR("Unknown Bprim Type %s", typeId.GetText());
    }
    return nullptr;
}

HdBprim *
HdPrmanRenderDelegate::CreateFallbackBprim(TfToken const& typeId)
{
    if (typeId == _tokens->openvdbAsset ||
        typeId == _tokens->field3dAsset) {
        return new HdPrman_Field(typeId, SdfPath::EmptyPath());
    } else if (typeId == HdPrimTypeTokens->renderBuffer) {
        return new HdPrmanRenderBuffer(SdfPath::EmptyPath());
    } else if (typeId == HdPrimTypeTokens->renderSettings) {
        return new HdPrman_RenderSettings(SdfPath::EmptyPath());
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
    if (IsInteractive()) {
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
    return HdAovDescriptor();
}

TfToken
HdPrmanRenderDelegate::GetMaterialBindingPurpose() const
{
    return HdTokens->full;
}

#if HD_API_VERSION < 41
TfToken
HdPrmanRenderDelegate::GetMaterialNetworkSelector() const
{
    return _tokens->ri;
}
#else
TfTokenVector
HdPrmanRenderDelegate::GetMaterialRenderContexts() const
{
#ifdef PXR_MATERIALX_SUPPORT_ENABLED
    return {_tokens->ri, _tokens->mtlxRenderContext};
#else
    return {_tokens->ri};
#endif
}
#endif

TfTokenVector
HdPrmanRenderDelegate::GetShaderSourceTypes() const
{
    return HdPrmanMaterial::GetShaderSourceTypes();
}

#if HD_API_VERSION > 46
TfTokenVector
HdPrmanRenderDelegate::GetRenderSettingsNamespaces() const
{
    return {_tokens->ri};
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
            renderIndex->GetChangeTracker().MarkSprimDirty(
                camPath, HdChangeTracker::DirtyParams);
            renderIndex->GetChangeTracker().MarkAllRprimsDirty(
                HdChangeTracker::DirtyPoints);
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
    if (!_terminalObserver) {
        _terminalObserver =
            std::make_unique<HdPrman_TerminalSceneIndexObserver>(
                _renderParam, terminalSceneIndex);
    }

#ifdef HDPRMAN_USE_SCENE_INDEX_OBSERVER
    if (terminalSceneIndex) {
        if (TfGetEnvSetting(HD_PRMAN_EXPERIMENTAL_RILEY_SCENE_INDEX_OBSERVER)) {
            if (!_rileySceneIndices) {
                _rileySceneIndices =
                    std::make_unique<_RileySceneIndices>(
                        terminalSceneIndex, _renderParam.get());
            }
        }
    }
#endif
}

void
HdPrmanRenderDelegate::Update()
{
#ifdef HDPRMAN_USE_SCENE_INDEX_OBSERVER
    if (_rileySceneIndices) {
        // We need to set some paths before any riley Create call can
        // be issued - otherwise, we get a crash.
        //
        // TODO: There should be a designated prim in the scene index
        // to communicate the global riley options.
        //
        _renderParam->SetRileyOptions();

        _rileySceneIndices->Update();
    }
#endif

    if (!_terminalObserver) {
        TF_CODING_ERROR("Invalid terminal scene index observer.");
        return;
    }

    _terminalObserver->Update();
}

#endif

PXR_NAMESPACE_CLOSE_SCOPE
