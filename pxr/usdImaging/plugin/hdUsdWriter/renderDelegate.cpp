//
// Copyright (c) 2022-2024, NVIDIA CORPORATION.
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

#include "pxr/usdImaging/plugin/hdUsdWriter/camera.h"
#include "pxr/usdImaging/plugin/hdUsdWriter/curves.h"
#include "pxr/usdImaging/plugin/hdUsdWriter/instancer.h"
#include "pxr/usdImaging/plugin/hdUsdWriter/light.h"
#include "pxr/usdImaging/plugin/hdUsdWriter/material.h"
#include "pxr/usdImaging/plugin/hdUsdWriter/mesh.h"
#include "pxr/usdImaging/plugin/hdUsdWriter/openvdbasset.h"
#include "pxr/usdImaging/plugin/hdUsdWriter/points.h"
#include "pxr/usdImaging/plugin/hdUsdWriter/renderPass.h"
#include "pxr/usdImaging/plugin/hdUsdWriter/utils.h"
#include "pxr/usdImaging/plugin/hdUsdWriter/volume.h"
#include "pxr/usdImaging/plugin/hdUsdWriter/renderDelegate.h"

#include "pxr/imaging/hd/bprim.h"
#include "pxr/imaging/hd/rprim.h"
#include "pxr/imaging/hd/sprim.h"
#include "pxr/usd/usd/stage.h"
#include "pxr/usdImaging/usdVolImaging/tokens.h"

#include <chrono>
#include <fstream>
#include <iostream>
#include <thread>

PXR_NAMESPACE_OPEN_SCOPE

namespace
{

template <typename T>
void _WritePrimitives(const UsdStagePtr &stage,
    HdUsdWriterRenderDelegate::PrimMap<T>& primitives)
{
    for (const auto& [path, prim] : primitives)
    {
        TF_UNUSED(path);
        auto id = prim->GetId().GetAsString();
        if (id.rfind("/_UsdImaging_HdUsdWriterRendererPlugin_", 0) != 0)
        {
            prim->SerializeToUsd(stage);
        }
    }
}

template <typename T>
bool _EraseElement(const SdfPath& id, HdUsdWriterRenderDelegate::PrimMap<T>& primitives)
{
    return primitives.erase(id) > 0;
}

template <typename T0, typename... T>
bool _EraseElement(const SdfPath& id,
    HdUsdWriterRenderDelegate::PrimMap<T0>& primitives0,
    HdUsdWriterRenderDelegate::PrimMap<T>&... primitives)
{
    return _EraseElement(id, primitives0) || _EraseElement(id, std::forward<decltype(primitives)>(primitives)...);
}

template <typename HYDRA_TYPE, typename... T>
void _EraseElement(HYDRA_TYPE* prim, HdUsdWriterRenderDelegate::PrimMap<T>&... primitives)
{
    const auto& id = prim->GetId();
    _EraseElement(id, std::forward<decltype(primitives)>(primitives)...);
    delete prim;
}

bool _TrySaveLayer(const SdfLayerHandle& layer)
{
    std::string layerAsString;
    if (!layer->ExportToString(&layerAsString))
    {
        TF_WARN("Unexpected failure exporting layer %s.",  layer->GetIdentifier().c_str());
        return false;
    }

    auto filepath = layer->GetRealPath();
    std::ofstream ofs(filepath, std::ofstream::trunc);
    if (!ofs.good())
    {
        TF_WARN("Unexpected failure opening output file %s.", filepath.c_str());
        return false;
    }

    ofs << layerAsString;
    ofs.close();
    return true;
}

void _TrySave(const UsdStagePtr& stage)
{
    if (!_TrySaveLayer(stage->GetRootLayer()))
    {
        TF_WARN("Attempt to save root layer failed. Saving stage as a last resort.");
        std::this_thread::sleep_for(std::chrono::milliseconds(250));
        stage->Save();
    }
}
}

TF_DEFINE_PRIVATE_TOKENS(
    _tokens,
    (materialBindingPurpose)
    (materialRenderContexts)
    (shaderSourceTypes)
    (writeExtent)
    ((mtlxRenderContext, "mtlx"))
);

const TfTokenVector HdUsdWriterRenderDelegate::SUPPORTED_RPRIM_TYPES = { HdPrimTypeTokens->mesh, HdPrimTypeTokens->basisCurves,
                                                                    HdPrimTypeTokens->points, HdPrimTypeTokens->volume };

const TfTokenVector HdUsdWriterRenderDelegate::SUPPORTED_SPRIM_TYPES = {
    HdPrimTypeTokens->camera, HdPrimTypeTokens->material,
    // Sprims Lights
    HdPrimTypeTokens->simpleLight, HdPrimTypeTokens->cylinderLight, HdPrimTypeTokens->diskLight,
    HdPrimTypeTokens->distantLight, HdPrimTypeTokens->domeLight, HdPrimTypeTokens->lightFilter,
    HdPrimTypeTokens->rectLight, HdPrimTypeTokens->sphereLight
};
const TfTokenVector HdUsdWriterRenderDelegate::SUPPORTED_BPRIM_TYPES = { UsdVolImagingTokens->openvdbAsset };


HdUsdWriterRenderDelegate::HdUsdWriterRenderDelegate() : HdRenderDelegate()
{
    _Initialize();    
}

HdUsdWriterRenderDelegate::HdUsdWriterRenderDelegate(HdRenderSettingsMap const& settingsMap) : HdRenderDelegate(settingsMap)
{
    _Initialize();
}

void HdUsdWriterRenderDelegate::_Initialize()
{
    _resourceRegistry = std::make_shared<HdResourceRegistry>();

    // Plugins that need to extend behavior can call SetTypeForPrimFactory with their own derived classes
    SetTypeForPrimFactory(HdPrimTypeTokens->mesh, [](SdfPath rPrimId, bool writeExtent) { return new HdUsdWriterMesh(rPrimId, writeExtent); });
    SetTypeForPrimFactory(HdPrimTypeTokens->basisCurves, [](SdfPath rPrimId, bool writeExtent) { return new HdUsdWriterBasisCurves(rPrimId); });
    SetTypeForPrimFactory(HdPrimTypeTokens->points, [](SdfPath rPrimId, bool writeExtent) { return new HdUsdWriterPoints(rPrimId); });
    SetTypeForPrimFactory(HdPrimTypeTokens->volume, [](SdfPath rPrimId, bool writeExtent) { return new HdUsdWriterVolume(rPrimId); });

    SetTypeForPrimFactory(HdPrimTypeTokens->camera, [](TfToken typeId, SdfPath sPrimId) { return new HdUsdWriterCamera(sPrimId); });
    SetTypeForPrimFactory(HdPrimTypeTokens->material, [](TfToken typeId, SdfPath sPrimId) { return new HdUsdWriterMaterial(sPrimId); });
    static const auto lightTokens = std::vector<TfToken>
    {
        HdPrimTypeTokens->cylinderLight, HdPrimTypeTokens->diskLight,
        HdPrimTypeTokens->distantLight, HdPrimTypeTokens->domeLight,
        HdPrimTypeTokens->rectLight, HdPrimTypeTokens->sphereLight
    };
    for (const auto& lightToken : lightTokens)
    {
        SetTypeForPrimFactory(lightToken, [](TfToken typeId, SdfPath sPrimId) { return new HdUsdWriterLight(typeId, sPrimId); });
    }

    SetTypeForPrimFactory(UsdVolImagingTokens->openvdbAsset, [](SdfPath bPrimId) { return new HdUsdWriterOpenvdbAsset(bPrimId); });

    SetTypeForPrimFactory(HdPrimTypeTokens->instancer, [](HdSceneDelegate* delegate, SdfPath instancerId) { return new HdUsdWriterInstancer(delegate, instancerId); });

    // Initialize the settings and settings descriptors.
    _settingDescriptors = {
        HdRenderSettingDescriptor{
            "Set the material binding purpose",
            _tokens->materialBindingPurpose,
            VtValue(HdTokens->preview) },
        HdRenderSettingDescriptor{
            "Set the material render contexts",
            _tokens->materialRenderContexts,
            VtValue(TfTokenVector {_tokens->mtlxRenderContext}) },
        HdRenderSettingDescriptor{
            "Set the shader source types",
            _tokens->shaderSourceTypes,
            VtValue(TfTokenVector()) },
        HdRenderSettingDescriptor{
            "Set whether to write extents",
            _tokens->writeExtent,
            VtValue(false) }
    };

    _PopulateDefaultSettings(_settingDescriptors);
}

HdUsdWriterRenderDelegate::~HdUsdWriterRenderDelegate()
{

}

TfTokenVector const& HdUsdWriterRenderDelegate::GetSupportedRprimTypes() const
{
    return SUPPORTED_RPRIM_TYPES;
}

TfTokenVector const& HdUsdWriterRenderDelegate::GetSupportedSprimTypes() const
{
    return SUPPORTED_SPRIM_TYPES;
}

TfTokenVector const& HdUsdWriterRenderDelegate::GetSupportedBprimTypes() const
{
    return SUPPORTED_BPRIM_TYPES;
}

HdResourceRegistrySharedPtr HdUsdWriterRenderDelegate::GetResourceRegistry() const
{
    return _resourceRegistry;
}

void HdUsdWriterRenderDelegate::CommitResources(HdChangeTracker* tracker)
{
}

HdRenderPassSharedPtr HdUsdWriterRenderDelegate::CreateRenderPass(HdRenderIndex* index, HdRprimCollection const& collection)
{
    return HdRenderPassSharedPtr(new HdUsdWriterRenderPass(index, collection));
}

HdRprim* HdUsdWriterRenderDelegate::CreateRprim(TfToken const& typeId, SdfPath const& rprimId)
{
    _createdPaths.insert(rprimId);

    if (typeId == HdPrimTypeTokens->mesh)
    {
        HdUsdWriterMesh* mesh = static_cast<HdUsdWriterMesh*>(std::get<RprimFactoryLambda>(_primFactoryMap[typeId])(rprimId, _writeExtent));
        _meshes[rprimId] = mesh;
        return mesh;
    }
    else if (typeId == HdPrimTypeTokens->basisCurves)
    {
        HdUsdWriterBasisCurves* curves = static_cast<HdUsdWriterBasisCurves*>(std::get<RprimFactoryLambda>(_primFactoryMap[typeId])(rprimId, _writeExtent));
        _curves[rprimId] = curves;
        return curves;
    }
    else if (typeId == HdPrimTypeTokens->points)
    {
        HdUsdWriterPoints* points = static_cast<HdUsdWriterPoints*>(std::get<RprimFactoryLambda>(_primFactoryMap[typeId])(rprimId, _writeExtent));
        _points[rprimId] = points;
        return points;
    }
    else if (typeId == HdPrimTypeTokens->volume)
    {
        HdUsdWriterVolume* volume = static_cast<HdUsdWriterVolume*>(std::get<RprimFactoryLambda>(_primFactoryMap[typeId])(rprimId, _writeExtent));
        _volumes[rprimId] = volume;
        return volume;
    }
    
    TF_CODING_ERROR("Unknown Rprim type=%s id=%s", typeId.GetText(), rprimId.GetAsString().c_str());
    return nullptr;
}

void HdUsdWriterRenderDelegate::DestroyRprim(HdRprim* rPrim)
{
    _destroyedPaths.insert(rPrim->GetId());
    if (!rPrim->GetInstancerId().IsEmpty())
    {
        if (auto search = _instancers.find(rPrim->GetInstancerId()); search != _instancers.end())
        {
            search->second->RemoveInstancedPrim(rPrim->GetId());
        }
        // else: instancer can be already destroyed when multiple prototypes use the same instancer
    }
    _EraseElement(rPrim, _meshes, _curves, _points, _volumes);
}

HdSprim* HdUsdWriterRenderDelegate::CreateSprim(TfToken const& typeId, SdfPath const& sprimId)
{
    _createdPaths.insert(sprimId);

    if (HdPrimTypeTokens->camera == typeId)
    {
        HdUsdWriterCamera* camera = static_cast<HdUsdWriterCamera*>(std::get<SprimFactoryLambda>(_primFactoryMap[typeId])(typeId, sprimId));
        _cameras[sprimId] = camera;        
        return camera;
    }

    if (HdPrimTypeTokens->material == typeId)
    {
        HdUsdWriterMaterial* material = static_cast<HdUsdWriterMaterial*>(std::get<SprimFactoryLambda>(_primFactoryMap[typeId])(typeId, sprimId));
        _materials[sprimId] = material;
        return material;
    }

    if (HdPrimTypeTokens->cylinderLight == typeId || HdPrimTypeTokens->diskLight == typeId ||
        HdPrimTypeTokens->distantLight == typeId || HdPrimTypeTokens->domeLight == typeId ||
        HdPrimTypeTokens->rectLight == typeId || HdPrimTypeTokens->sphereLight == typeId)
    {
        HdUsdWriterLight* light = static_cast<HdUsdWriterLight*>(std::get<SprimFactoryLambda>(_primFactoryMap[typeId])(typeId, sprimId));
        _lights[sprimId] = light;
        return light;
    }

    TF_WARN("Unknown Sprim type=%s id=%s", typeId.GetText(), sprimId.GetAsString().c_str());
    return nullptr;
}

HdSprim* HdUsdWriterRenderDelegate::CreateFallbackSprim(TfToken const& typeId)
{
    TF_WARN("Creating unknown fallback sprim type=%s", typeId.GetText());
    return nullptr;
}

void HdUsdWriterRenderDelegate::DestroySprim(HdSprim* sPrim)
{
    if (sPrim == nullptr)
        return;
    _destroyedPaths.insert(sPrim->GetId());
    _EraseElement(sPrim, _lights, _materials, _cameras);
}

HdBprim* HdUsdWriterRenderDelegate::CreateBprim(TfToken const& typeId, SdfPath const& bprimId)
{
    _createdPaths.insert(bprimId);

    if (typeId == UsdVolImagingTokens->openvdbAsset)
    {
        HdUsdWriterOpenvdbAsset* openvdbAsset = static_cast<HdUsdWriterOpenvdbAsset*>(std::get<BprimFactoryLambda>(_primFactoryMap[typeId])(bprimId));
        _openvdbAssets[bprimId] = openvdbAsset;
        return openvdbAsset;
    }
    TF_WARN("Unknown Bprim type=%s id=%s", typeId.GetText(), bprimId.GetAsString().c_str());

    return nullptr;
}

HdBprim* HdUsdWriterRenderDelegate::CreateFallbackBprim(TfToken const& typeId)
{
    TF_WARN("Creating unknown fallback bprim type=%s", typeId.GetText());
    return nullptr;
}

void HdUsdWriterRenderDelegate::DestroyBprim(HdBprim* bPrim)
{
    if (bPrim == nullptr)
        return;
    _destroyedPaths.insert(bPrim->GetId());
    _EraseElement(bPrim, _openvdbAssets);
}

HdInstancer* HdUsdWriterRenderDelegate::CreateInstancer(HdSceneDelegate* delegate, SdfPath const& id)
{
    HdUsdWriterInstancer* instancer = static_cast<HdUsdWriterInstancer*>(
        std::get<InstancerFactoryLambda>(_primFactoryMap[HdPrimTypeTokens->instancer])(delegate, id));
    _instancers[id] = instancer;
    return instancer;
}

void HdUsdWriterRenderDelegate::DestroyInstancer(HdInstancer* instancer)
{
    if (instancer == nullptr)
        return;
    _destroyedPaths.insert(instancer->GetId());
    _EraseElement(instancer, _instancers);
}

HdRenderParam* HdUsdWriterRenderDelegate::GetRenderParam() const
{
    return nullptr;
}

HdRenderSettingDescriptorList
HdUsdWriterRenderDelegate::GetRenderSettingDescriptors() const
{
    return _settingDescriptors;
}

bool HdUsdWriterRenderDelegate::SerializeToUsd(const std::string& filename)
{
    auto stage = UsdStage::CreateNew(filename);
    if (!stage || !stage->GetPseudoRoot() || !stage->GetRootLayer() || !stage->GetRootLayer()->PermissionToEdit() ||
        !stage->GetRootLayer()->PermissionToSave())
    {
        TF_CODING_ERROR("Failed to create writable UsdStage %s", filename.c_str());
        return false;
    }

    _WritePrimitives(stage, _meshes);
    _WritePrimitives(stage, _curves);
    _WritePrimitives(stage, _points);
    _WritePrimitives(stage, _volumes);
    _WritePrimitives(stage, _openvdbAssets);
    _WritePrimitives(stage, _lights);
    _WritePrimitives(stage, _instancers);
    _WritePrimitives(stage, _materials);
    _WritePrimitives(stage, _cameras);

    if (!_destroyedPaths.empty())
    {
        VtDictionary deletedPrimMd;
        deletedPrimMd["HdDestroyedPrim"] = true;
        std::vector<SdfPath> destroyedPrims;
        for (const auto& path : _destroyedPaths)
        {
            if (path.IsPrimPath() && _createdPaths.find(path) == _createdPaths.end())
            {
                destroyedPrims.push_back(path);
            }
        }

        // Sort paths and override prims first, the de-activate in second pass as we can't create prims under disabled
        // prims
        std::sort(destroyedPrims.begin(), destroyedPrims.end());
        for (const auto& path : destroyedPrims)
        {
            auto prim = stage->OverridePrim(HdUsdWriterGetFlattenPrototypePath(path));
            if (prim)
            {
                prim.SetCustomData(deletedPrimMd);
            }
        }
        for (const auto& path : destroyedPrims)
        {
            auto prim = stage->GetPrimAtPath(HdUsdWriterGetFlattenPrototypePath(path));
            if (prim)
            {
                prim.SetActive(false);
            }
        }
    }
    _destroyedPaths.clear();
    _createdPaths.clear();

    _TrySave(stage);
    return true;
}

void HdUsdWriterRenderDelegate::SetMaterialBindingPurpose(const TfToken& materialBindingPurpose)
{
    SetRenderSetting(_tokens->materialBindingPurpose, VtValue(materialBindingPurpose));
}

void HdUsdWriterRenderDelegate::SetMaterialRenderContexts(const TfTokenVector& materialRenderContexts)
{
    SetRenderSetting(_tokens->materialRenderContexts, VtValue(materialRenderContexts));
}

void HdUsdWriterRenderDelegate::SetShaderSourceTypes(const TfTokenVector& shaderSourceTypes)
{
    SetRenderSetting(_tokens->shaderSourceTypes, VtValue(shaderSourceTypes));
}

void HdUsdWriterRenderDelegate::SetWriteExtent(bool writeExtent)
{
    SetRenderSetting(_tokens->writeExtent, VtValue(writeExtent));
}

TfToken HdUsdWriterRenderDelegate::GetMaterialBindingPurpose() const
{
    return GetRenderSetting<TfToken>(_tokens->materialBindingPurpose, HdTokens->preview);
}

TfTokenVector HdUsdWriterRenderDelegate::GetMaterialRenderContexts() const
{
    return GetRenderSetting<TfTokenVector>(_tokens->materialRenderContexts, TfTokenVector());
}

TfTokenVector HdUsdWriterRenderDelegate::GetShaderSourceTypes() const
{
    return GetRenderSetting<TfTokenVector>(_tokens->shaderSourceTypes, TfTokenVector());
}

bool HdUsdWriterRenderDelegate::GetWriteExtent() const
{
    return GetRenderSetting<bool>(_tokens->writeExtent, false);
}

bool HdUsdWriterRenderDelegate::InvokeCommand(const TfToken &command, const HdCommandArgs &args)
{
    if (command == "SerializeToUsd")
    {
        return SerializeToUsd(args.find("outputPath")->second.Get<std::string>());
    }
    else if (command == "SetMaterialBindingPurpose")
    {
        SetMaterialBindingPurpose(TfToken(args.find("materialBindingPurpose")->second.Get<std::string>()));
        return true;
    }
    else if (command == "SetMaterialRenderContexts")
    {
        SetMaterialRenderContexts(args.find("materialRenderContexts")->second.Get<TfTokenVector>());
        return true;
    }
    else if (command == "SetShaderSourceTypes")
    {
        SetShaderSourceTypes(args.find("shaderSourceTypes")->second.Get<TfTokenVector>());
        return true;
    }
    else if (command == "SetWriteExtent")
    {
        SetWriteExtent(args.find("setWriteExtent")->second.Get<bool>());
        return true;
    }
    TF_WARN("Unknown command %s", command.GetText());
    return false;
}

void HdUsdWriterRenderDelegate::SetTypeForPrimFactory(TfToken typeId, PrimFactory constructorLambda)
{
    _primFactoryMap[typeId] = constructorLambda;
}

PXR_NAMESPACE_CLOSE_SCOPE
