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
#include "hdPrman/basisCurves.h"
#include "hdPrman/camera.h"
#include "hdPrman/context.h"
#include "hdPrman/coordSys.h"
#include "hdPrman/instancer.h"
#include "hdPrman/light.h"
#include "hdPrman/lightFilter.h"
#include "hdPrman/material.h"
#include "hdPrman/mesh.h"
#include "hdPrman/points.h"
#include "hdPrman/renderDelegate.h"
#include "hdPrman/renderParam.h"
#include "hdPrman/renderPass.h"
#include "hdPrman/volume.h"

#include "pxr/imaging/hd/tokens.h"
#include "pxr/imaging/hd/bprim.h"
#include "pxr/imaging/hd/camera.h"
#include "pxr/imaging/hd/extComputation.h"
#include "pxr/imaging/hd/rprim.h"
#include "pxr/imaging/hd/sprim.h"

#include "pxr/base/tf/getenv.h"

PXR_NAMESPACE_OPEN_SCOPE
 
TF_DEFINE_PRIVATE_TOKENS(
    _tokens,
    (openvdbAsset)
    (field3dAsset)
);

TF_DEFINE_PUBLIC_TOKENS(HdPrmanIntegratorTokens,
    HDPRMAN_INTEGRATOR_TOKENS);

const TfTokenVector HdPrmanRenderDelegate::SUPPORTED_RPRIM_TYPES =
{
    HdPrimTypeTokens->mesh,
    HdPrimTypeTokens->basisCurves,
    HdPrimTypeTokens->points,
    HdPrimTypeTokens->volume,
};

const TfTokenVector HdPrmanRenderDelegate::SUPPORTED_SPRIM_TYPES =
{
    HdPrimTypeTokens->camera,
    HdPrimTypeTokens->material,
    HdPrimTypeTokens->distantLight,
    HdPrimTypeTokens->domeLight,
    HdPrimTypeTokens->lightFilter,
    HdPrimTypeTokens->rectLight,
    HdPrimTypeTokens->diskLight,
    HdPrimTypeTokens->cylinderLight,
    HdPrimTypeTokens->sphereLight,
    HdPrimTypeTokens->extComputation,
    HdPrimTypeTokens->coordSys,
};

const TfTokenVector HdPrmanRenderDelegate::SUPPORTED_BPRIM_TYPES =
{
    _tokens->openvdbAsset,
    _tokens->field3dAsset
};

HdPrmanRenderDelegate::HdPrmanRenderDelegate(
    std::shared_ptr<HdPrman_Context> context) : _context(context)
{
    _Initialize();
}

HdPrmanRenderDelegate::HdPrmanRenderDelegate(
    std::shared_ptr<HdPrman_Context> context,
    HdRenderSettingsMap const& settingsMap)
    : HdRenderDelegate(settingsMap), _context(context)
{
    _Initialize();
}

void
HdPrmanRenderDelegate::_Initialize()
{
    _renderParam = std::make_shared<HdPrman_RenderParam>(_context);

    // Initialize default resource registry. HdxPrman may override this with its
    // resource registry during HdxPrmanRenderDelegate::_Initialize.
    _resourceRegistry = std::make_shared<HdResourceRegistry>();

    std::string integrator = HdPrmanIntegratorTokens->PxrPathTracer;
    const std::string interactiveIntegrator = 
        HdPrmanIntegratorTokens->PxrDirectLighting;
    std::string integratorEnv = TfGetenv("HDX_PRMAN_INTEGRATOR");
    if (!integratorEnv.empty())
        integrator = integratorEnv;

    int maxSamples = 1024;
    int maxSamplesEnv = TfGetenvInt("HDX_PRMAN_MAX_SAMPLES", 0);
    if (maxSamplesEnv != 0)
        maxSamples = maxSamplesEnv;

    float pixelVariance = 0.001f;

    _settingDescriptors.resize(5);

    _settingDescriptors[0] = { 
        std::string("Integrator"),
        HdPrmanRenderSettingsTokens->integratorName,
        VtValue(integrator) 
    };

    _settingDescriptors[1] = {
        std::string("Interactive Integrator"),
        HdPrmanRenderSettingsTokens->interactiveIntegrator,
        VtValue(interactiveIntegrator)
    };

    // If >0, the time in ms that we'll render quick output before switching
    // to path tracing
    _settingDescriptors[2] = {
        std::string("Interactive Integrator Timeout (ms)"),
        HdPrmanRenderSettingsTokens->interactiveIntegratorTimeout,
        VtValue(200)
    };

    _settingDescriptors[3] = { std::string("Max Samples"),
        HdRenderSettingsTokens->convergedSamplesPerPixel,
        VtValue(maxSamples) };

    _settingDescriptors[4] = { std::string("Variance Threshold"),
        HdRenderSettingsTokens->convergedVariance,
        VtValue(pixelVariance) };

    _PopulateDefaultSettings(_settingDescriptors);
}

HdPrmanRenderDelegate::~HdPrmanRenderDelegate()
{
    // TODO: let someone else tear down the context? renderer plugin?
    //_context->End();
    _context.reset();
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
        _renderPass = HdRenderPassSharedPtr(
            new HdPrman_RenderPass(index, collection));
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
    if (typeId == HdPrimTypeTokens->mesh) {
        return new HdPrman_Mesh(rprimId);
    } else if (typeId == HdPrimTypeTokens->basisCurves) {
        return new HdPrman_BasisCurves(rprimId);
    } else if (typeId == HdPrimTypeTokens->points) {
        return new HdPrman_Points(rprimId);
    } else if (typeId == HdPrimTypeTokens->volume) {
        return new HdPrman_Volume(rprimId);
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
    if (typeId == HdPrimTypeTokens->camera) {
        return new HdPrmanCamera(sprimId);
    } else if (typeId == HdPrimTypeTokens->material) {
        return new HdPrmanMaterial(sprimId);
    } else if (typeId == HdPrimTypeTokens->coordSys) {
        return new HdPrmanCoordSys(sprimId);
    } else if (typeId == HdPrimTypeTokens->lightFilter) {
        return new HdPrmanLightFilter(sprimId, typeId);
    } else if (typeId == HdPrimTypeTokens->distantLight ||
               typeId == HdPrimTypeTokens->domeLight ||
               typeId == HdPrimTypeTokens->rectLight ||
               typeId == HdPrimTypeTokens->diskLight ||
               typeId == HdPrimTypeTokens->cylinderLight ||
               typeId == HdPrimTypeTokens->sphereLight) {
        return new HdPrmanLight(sprimId, typeId);
    } else if (typeId == HdPrimTypeTokens->extComputation) {
        return new HdExtComputation(sprimId);
    } else {
        TF_CODING_ERROR("Unknown Sprim Type %s", typeId.GetText());
    }

    return nullptr;
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
    } else if (typeId == HdPrimTypeTokens->distantLight ||
               typeId == HdPrimTypeTokens->domeLight ||
               typeId == HdPrimTypeTokens->rectLight ||
               typeId == HdPrimTypeTokens->diskLight ||
               typeId == HdPrimTypeTokens->cylinderLight ||
               typeId == HdPrimTypeTokens->sphereLight) {
        return new HdPrmanLight(SdfPath::EmptyPath(), typeId);
    } else if (typeId == HdPrimTypeTokens->extComputation) {
        return new HdExtComputation(SdfPath::EmptyPath());
    } else {
        TF_CODING_ERROR("Unknown Sprim Type %s", typeId.GetText());
    }

    return nullptr;
}

void
HdPrmanRenderDelegate::DestroySprim(HdSprim *sPrim)
{
    delete sPrim;
}

HdBprim *
HdPrmanRenderDelegate::CreateBprim(TfToken const& typeId,
                                    SdfPath const& bprimId)
{
    if (typeId == _tokens->openvdbAsset ||
        typeId == _tokens->field3dAsset) {
        return new HdPrman_Field(typeId, bprimId);
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

TfToken
HdPrmanRenderDelegate::GetMaterialBindingPurpose() const
{
    return HdTokens->full;
}

TfToken
HdPrmanRenderDelegate::GetMaterialNetworkSelector() const
{
    static const TfToken ri("ri");
    return ri;
}

TfTokenVector
HdPrmanRenderDelegate::GetShaderSourceTypes() const
{
    return HdPrmanMaterial::GetShaderSourceTypes();
}

void
HdPrmanRenderDelegate::SetRenderSetting(TfToken const &key, 
                                        VtValue const &value)
{
    // update settings version only if a setting actually changed
    auto it = _settingsMap.find(key);
    if (it != _settingsMap.end()) {
        if (value != it->second) {
            _settingsVersion++;
        }
    } else {
        _settingsVersion++;
    }

    _settingsMap[key] = value;

    if (TfDebug::IsEnabled(HD_RENDER_SETTINGS)) {
        std::cout << "Render Setting [" << key << "] = " << value << std::endl;
    }
}

PXR_NAMESPACE_CLOSE_SCOPE
