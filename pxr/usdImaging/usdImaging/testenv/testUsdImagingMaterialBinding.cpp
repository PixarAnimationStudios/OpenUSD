//
// Copyright 2024 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/base/tf/errorMark.h"

#include "pxr/base/gf/vec2i.h"
#include "pxr/base/tf/getenv.h"

#include "pxr/base/vt/dictionary.h"

#include "pxr/imaging/hd/renderIndex.h"
#include "pxr/imaging/hd/unitTestNullRenderDelegate.h"

#include "pxr/usdImaging/usdImaging/sceneIndices.h"
#include "pxr/usdImaging/usdImaging/stageSceneIndex.h"

#include "pxr/usd/usdShade/materialBindingAPI.h"
#include "pxr/usd/usd/stage.h"

#include <fstream>
#include <iostream>

PXR_NAMESPACE_USING_DIRECTIVE

namespace {

VtValue
_SanitiseValue(const VtValue& value)
{
    if (value.IsHolding<SdfAssetPath>()) {
        const SdfAssetPath assetPath = value.UncheckedGet<SdfAssetPath>();

        // Special case for openvdb assets, which use pointer addresses for
        // keys, which will change on each test run.
        if (TfStringStartsWith(
                assetPath.GetResolvedPath(), "openvdb-stream://")) {
            static const SdfAssetPath sanitisedAssetPath(
                "openvdb-stream://<MEMORY_ADDRESS>",
                "openvdb-stream://<MEMORY_ADDRESS>");
            return VtValue(sanitisedAssetPath);
        }

        // "Un-resolve" asset paths, just because their resolved paths might
        // vary based on whether they were run locally or remotely.
        return VtValue(
            SdfAssetPath(assetPath.GetAssetPath(), assetPath.GetAssetPath()));
    }

    return value;
}

VtValue _GetValueForDS(const HdDataSourceBaseHandle& dataSource);

std::vector<VtValue>
_GetValueForVector(const HdVectorDataSourceHandle& dataSource)
{
    std::vector<VtValue> ret;
    if (dataSource) {
        for (size_t i = 0; i < dataSource->GetNumElements(); ++i) {
            ret.push_back(_GetValueForDS(dataSource->GetElement(i)));
        }
    }
    return ret;
}

VtDictionary
_GetDictForDS(const HdContainerDataSourceHandle& dataSource)
{
    VtDictionary dict;
    if (dataSource) {
        for (const auto& name : dataSource->GetNames()) {
            HdDataSourceBaseHandle child = dataSource->Get(name);
            dict[name.GetString()] = _GetValueForDS(child);
        }
    }

    return dict;
}

VtValue
_GetValueForDS(const HdDataSourceBaseHandle& dataSource)
{
    if (auto container
        = std::dynamic_pointer_cast<HdContainerDataSource>(dataSource)) {
        return VtValue(_GetDictForDS(container));
    }
    else if (
        auto vector
        = std::dynamic_pointer_cast<HdVectorDataSource>(dataSource)) {
        return VtValue(_GetValueForVector(vector));
    }
    else if (
        auto sampled
        = std::dynamic_pointer_cast<HdSampledDataSource>(dataSource)) {
        VtValue value = sampled->GetValue(0.0f);
        return _SanitiseValue(value);
    }
    return VtValue("UNKNOWN TYPE");
}

VtDictionary
_GetDictForPrim(const HdSceneIndexBaseRefPtr& sceneIndex, const SdfPath& path)
{
    VtDictionary children;
    for (const auto& childPrim : sceneIndex->GetChildPrimPaths(path)) {
        children[childPrim.GetName()] = _GetDictForPrim(sceneIndex, childPrim);
    }

    HdSceneIndexPrim prim = sceneIndex->GetPrim(path);
    VtDictionary attributes = _GetDictForDS(prim.dataSource);

    VtDictionary dict;
    dict["type"] = prim.primType.GetString();
    if (!children.empty()) {
        dict["children"] = std::move(children);
    }
    if (!attributes.empty()) {
        dict["attributes"] = std::move(attributes);
    }
    return dict;
}

bool
_HydraDumpForUsdStage(
    const UsdStagePtr& stage,
    const SdfPath& sceneGraphPath,
    const UsdTimeCode& frame,
    VtDictionary* out)
{
    Hd_UnitTestNullRenderDelegate renderDelegate;
    std::unique_ptr<HdRenderIndex> renderIndex(
        HdRenderIndex::New(&renderDelegate, HdDriverVector()));
    
    UsdImagingCreateSceneIndicesInfo info;
    info.stage = stage;
    const UsdImagingSceneIndices sceneIndices =
        UsdImagingCreateSceneIndices(info);
    UsdImagingStageSceneIndexRefPtr stageSceneIndex =
        sceneIndices.stageSceneIndex;
    stageSceneIndex->SetTime(frame);
    HdSceneIndexBaseRefPtr finalSceneIndex = sceneIndices.finalSceneIndex;
    renderIndex->InsertSceneIndex(finalSceneIndex, SdfPath("/"));

    // NOTE: this makes assumptions based on scene index emulation and will
    //       need to be updated when HdRenderIndex no longer uses the
    //       emulated legacy APIs.
    auto registeredSceneIndexNames
        = HdSceneIndexNameRegistry::GetInstance().GetRegisteredNames();

    if (registeredSceneIndexNames.size() != 1) {
        std::cerr
            << "expecting 1 registered scene index (via emulation) and found "
            << registeredSceneIndexNames.size() << " instead." << std::endl;
        return false;
    }

    HdSceneIndexBaseRefPtr sceneIndex
        = HdSceneIndexNameRegistry::GetInstance().GetNamedSceneIndex(
            registeredSceneIndexNames[0]);

    if (!sceneIndex) {
        std::cerr << "registered scene index is null." << std::endl;
        return false;
    }
    /*
    _RecordingSceneIndexObserver observer;
    sceneIndex->AddObserver(HdSceneIndexObserverPtr(&observer));
    */

    VtDictionary dict = _GetDictForPrim(sceneIndex, sceneGraphPath);
    if (out) {
        std::swap(*out, dict);
    }
    return true;
}

};

static SdfPath
_ComputeUsdShadeBoundMaterial(UsdStagePtr stage, const SdfPath& geomPath)
{
    // first, compute the binding that we would get from UsdShade.
    auto matBinding = UsdShadeMaterialBindingAPI::Get(stage, geomPath);
    UsdShadeMaterial boundMat = matBinding.ComputeBoundMaterial();
    return boundMat.GetPath();
}

static VtValue
_GetFromDict(const VtDictionary& dict, const std::vector<std::string>& keys)
{
    const VtDictionary* curr = &dict;
    for (auto it = keys.begin(); it != keys.end(); ++it) {
        const std::string& key = *it;
        auto found = curr->find(key);
        if (found == curr->end()) {
            break;
        }

        const VtValue& foundValue = found->second;
        if (it + 1 == keys.end()) {
            // if we're at the last key, we return the value.
            return foundValue;
        }
        else {
            if (!foundValue.IsHolding<VtDictionary>()) {
                break;
            }

            curr = &(foundValue.UncheckedGet<VtDictionary>());
        }
    }
    return VtValue();
}

static SdfPath
_ComputeHydraBoundMaterial(UsdStagePtr stage, const SdfPath& geomPath)
{
    UsdTimeCode frame(0);
    VtDictionary dict;
    ::_HydraDumpForUsdStage(stage, geomPath, frame, &dict);
    VtValue materialBindingData
        = _GetFromDict(dict, { "attributes", "materialBindings" });
    TF_VERIFY(materialBindingData.IsHolding<VtDictionary>());
    const VtDictionary& materialBindingDict
        = materialBindingData.UncheckedGet<VtDictionary>();
    auto found = materialBindingDict.find("");
    TF_VERIFY(found != materialBindingDict.end());
    const VtValue& defaultBindingValue = found->second;
    TF_VERIFY(defaultBindingValue.IsHolding<VtDictionary>());
    const VtDictionary& defaultBindingDict
        = defaultBindingValue.UncheckedGet<VtDictionary>();
    auto pathIt = defaultBindingDict.find("path");
    TF_VERIFY(pathIt != defaultBindingDict.end());
    const VtValue& defaultBindingPath = pathIt->second;
    TF_VERIFY(defaultBindingPath.IsHolding<SdfPath>());
    return defaultBindingPath.UncheckedGet<SdfPath>();
}

static void
TestMaterialBinding()
{
    UsdStageRefPtr stage = UsdStage::Open("model.usda");
    const SdfPath path("/Model/Geom/Parent/mesh");

    // Compute the binding that we would get from UsdShade.
    const SdfPath usdShadeBoundPath
        = _ComputeUsdShadeBoundMaterial(stage, path);

    // Compute what we're computing from UsdImaging.
    const SdfPath hydraBoundMaterialPath
        = _ComputeHydraBoundMaterial(stage, path);

    // Test that they match.
    TF_VERIFY(usdShadeBoundPath == hydraBoundMaterialPath);
}

int
main()
{
    TfErrorMark mark;

    TestMaterialBinding();

    if (TF_VERIFY(mark.IsClean())) {
        std::cout << "OK" << std::endl;
    }
    else {
        std::cout << "FAILED" << std::endl;
    }
}
