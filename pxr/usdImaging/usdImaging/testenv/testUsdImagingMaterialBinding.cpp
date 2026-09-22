//
// Copyright 2024 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/base/tf/errorMark.h"

#include "pxr/imaging/hd/instanceProxyViewSceneIndex.h"
#include "pxr/imaging/hd/materialBindingSchema.h"
#include "pxr/imaging/hd/materialBindingsSchema.h"
#include "pxr/imaging/hd/tokens.h"

#include "pxr/usdImaging/usdImaging/sceneIndices.h"
#include "pxr/usdImaging/usdImaging/stageSceneIndex.h"

#include "pxr/usd/usdShade/materialBindingAPI.h"
#include "pxr/usd/usd/stage.h"

#include <cstdlib> // EXIT_FAILURE, EXIT_SUCCESS
#include <iostream>

PXR_NAMESPACE_USING_DIRECTIVE

static SdfPath
_ComputeUsdShadeBoundMaterial(UsdStagePtr stage, const SdfPath& geomPath)
{
    // first, compute the binding that we would get from UsdShade.
    auto matBinding = UsdShadeMaterialBindingAPI::Get(stage, geomPath);
    UsdShadeMaterial boundMat = matBinding.ComputeBoundMaterial();
    return boundMat.GetPath();
}

static SdfPath
_ComputeHydraBoundMaterial(
    HdSceneIndexBaseRefPtr sceneIndex, const SdfPath& geomPath)
{
    HdSceneIndexPrim prim = sceneIndex->GetPrim(geomPath);
    HdMaterialBindingsSchema materialBindings =
        HdMaterialBindingsSchema::GetFromParent(prim.dataSource);

    // The scenes used in this test don't have any purpose-specific bindings ...
    HdMaterialBindingSchema binding =
        materialBindings.GetMaterialBinding(
            HdMaterialBindingsSchemaTokens->allPurpose);
    HdPathDataSourceHandle pathDS = binding.GetPath();
    if (!TF_VERIFY(pathDS)) {
        return SdfPath();
    }
    return pathDS->GetTypedValue(0.0f);
}

static void
TestMaterialBindings(
    const std::string& usdaFile,
    const std::vector<SdfPath>& paths)
{
    UsdStageRefPtr stage = UsdStage::Open(usdaFile);

    // Create the UsdImaging scene index graph. Also wire up an instance proxy
    // view scene index to allow querying of instance proxy scene index prims.
    UsdImagingCreateSceneIndicesInfo info;
    info.stage = stage;
    const UsdImagingSceneIndices sceneIndices =
        UsdImagingCreateSceneIndices(info);
    HdSceneIndexBaseRefPtr proxyViewSceneIndex =
        HdInstanceProxyViewSceneIndex::New(sceneIndices.finalSceneIndex);
    sceneIndices.stageSceneIndex->SetTime(UsdTimeCode(0));

    for (const SdfPath& path : paths) {
        // Compute the binding that we would get from UsdShade.
        const SdfPath usdShadeBoundPath
            = _ComputeUsdShadeBoundMaterial(stage, path);

        const bool isInstanceProxy =
            stage->GetPrimAtPath(path).IsInstanceProxy();

        HdSceneIndexBaseRefPtr sceneIndexToQuery =
            isInstanceProxy
            ? proxyViewSceneIndex
            : sceneIndices.finalSceneIndex;

        // Compute what we're computing from UsdImaging/Hydra.
        const SdfPath hydraBoundMaterialPath
            = _ComputeHydraBoundMaterial(sceneIndexToQuery, path);

        std::cout << "Geom: " << path
                  << ", UsdShade bound material: " << usdShadeBoundPath
                  << ", Hydra bound material: " << hydraBoundMaterialPath
                  << std::endl;

        // Safe to compare the paths if instancing isn't involved.
        if (!isInstanceProxy) {
            TF_VERIFY(usdShadeBoundPath == hydraBoundMaterialPath);
        } else {
            // Compare just the leaf name because of the namespace mangling
            // from instance aggregation and prototype propagation, but also
            // confirm that a material prim exists at the resolved path.
            TF_VERIFY(usdShadeBoundPath.GetName()
                        == hydraBoundMaterialPath.GetName());
            const auto hdPrim =
                sceneIndices.finalSceneIndex->GetPrim(hydraBoundMaterialPath);
            TF_VERIFY(hdPrim && hdPrim.primType == HdPrimTypeTokens->material);
        }
    }
}

int
main(int argc, char* argv[])
{
    const auto usage = [&]() {
        std::cerr << "Usage: " << argv[0]
                  << " -stage <usda_file>"
                  << " -validateBindingsFor <prim_path> [<prim_path> ...]\n";
    };

    if (argc < 5
        || std::string(argv[1]) != "-stage"
        || std::string(argv[3]) != "-validateBindingsFor") {
        usage();
        return EXIT_FAILURE;
    }

    const std::string stageFile = argv[2];
    std::vector<SdfPath> paths;
    for (int i = 4; i < argc; ++i) {
        paths.emplace_back(argv[i]);
    }

    TfErrorMark mark;

    TestMaterialBindings(stageFile, paths);

    if (TF_VERIFY(mark.IsClean())) {
        std::cout << "OK\n";
    }
    else {
        std::cout << "FAILED\n";
    }

    return EXIT_SUCCESS;
}
