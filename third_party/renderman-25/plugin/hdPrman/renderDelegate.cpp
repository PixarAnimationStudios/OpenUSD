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
#include "hdPrman/paramsSetter.h"
#include "hdPrman/points.h"
#include "hdPrman/resourceRegistry.h"
#include "hdPrman/tokens.h"
#include "hdPrman/volume.h"

#include "pxr/imaging/hd/bprim.h"
#include "pxr/imaging/hd/camera.h"
#include "pxr/imaging/hd/extComputation.h"
#include "pxr/imaging/hd/rprim.h"
#include "pxr/imaging/hd/sprim.h"
#include "pxr/imaging/hd/tokens.h"

#include "pxr/base/tf/envSetting.h"
#include "pxr/base/tf/getenv.h"

PXR_NAMESPACE_OPEN_SCOPE

extern TfEnvSetting<bool> HD_PRMAN_ENABLE_QUICKINTEGRATE;

TF_DEFINE_PRIVATE_TOKENS(
    _tokens,
    (openvdbAsset)
    (field3dAsset)
    (ri)
    ((outputsRi, "outputs:ri"))
    ((mtlxRenderContext, "mtlx"))
    (prmanParams) /* XXX currently duplicated whereever used as to not yet */
                 /* establish a formal convention */
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
    HdPrimTypeTokens->meshLight,
    HdPrimTypeTokens->pluginLight,
    HdPrimTypeTokens->extComputation,
    HdPrimTypeTokens->coordSys,
    HdPrimTypeTokens->integrator,
    HdPrimTypeTokens->sampleFilter,
    HdPrimTypeTokens->displayFilter,
    _tokens->prmanParams,
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

    // 64 samples is RenderMan default
    int maxSamples = TfGetenvInt("HD_PRMAN_MAX_SAMPLES", 64);

    float pixelVariance = 0.001f;

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
    bool isMeshLight = false;
    if (typeId == HdPrmanTokens->meshLightSourceMesh) {
        isMeshLight = true;
        return new HdPrman_Mesh(rprimId, isMeshLight);
    } else if (typeId == HdPrmanTokens->meshLightSourceVolume) {
        isMeshLight = true;
        return new HdPrman_Volume(rprimId, isMeshLight);
    } else if (typeId == HdPrimTypeTokens->mesh) {
        return new HdPrman_Mesh(rprimId, isMeshLight);
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
        return new HdPrman_Volume(rprimId, isMeshLight);
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
               typeId == HdPrimTypeTokens->meshLight ||
               typeId == HdPrimTypeTokens->pluginLight) {
        sprim = new HdPrmanLight(sprimId, typeId);

        // Disregard fallback prims in count.
        if (sprim->GetId() != SdfPath()) {
            _renderParam->IncreaseSceneLightCount();
        }
    } else if (typeId == HdPrimTypeTokens->extComputation) {
        sprim = new HdExtComputation(sprimId);
    
    } else if (typeId == _tokens->prmanParams) {
        sprim = new HdPrmanParamsSetter(sprimId);
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
               typeId == HdPrimTypeTokens->meshLight ||
               typeId == HdPrimTypeTokens->pluginLight) {
        return new HdPrmanLight(SdfPath::EmptyPath(), typeId);
    } else if (typeId == HdPrimTypeTokens->extComputation) {
        return new HdExtComputation(SdfPath::EmptyPath());
    } else if (typeId == _tokens->prmanParams) {
        return new HdPrmanParamsSetter(SdfPath::EmptyPath());
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
    return {_tokens->ri, _tokens->outputsRi};
}
#endif

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

PXR_NAMESPACE_CLOSE_SCOPE
