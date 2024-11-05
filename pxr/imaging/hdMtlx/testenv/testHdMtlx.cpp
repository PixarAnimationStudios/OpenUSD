//
// Copyright 2024 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/base/tf/errorMark.h"
#include "pxr/usd/usd/stage.h"
#include "pxr/usd/usd/primRange.h"
#include "pxr/usd/usdShade/material.h"
#include "pxr/imaging/hd/dataSourceMaterialNetworkInterface.h"
#include "pxr/imaging/hd/renderIndex.h"
#include "pxr/imaging/hd/unitTestNullRenderDelegate.h"
#include "pxr/imaging/hdMtlx/hdMtlx.h"
#include "pxr/usdImaging/usdImaging/dataSourceMaterial.h"
#include "pxr/usdImaging/usdImaging/sceneIndices.h"
#include "pxr/usdImaging/usdImaging/stageSceneIndex.h"

#include <MaterialXFormat/XmlIo.h>

#include <iostream>

PXR_NAMESPACE_USING_DIRECTIVE

namespace mx = MaterialX;

class TestStageGlobals : public UsdImagingDataSourceStageGlobals {
public:
  TestStageGlobals() : _time(0) {}
  ~TestStageGlobals() override = default;

  UsdTimeCode GetTime() const override { return UsdTimeCode(_time); }

  void FlagAsTimeVarying(const SdfPath &hydraPath,
                         const HdDataSourceLocator &locator) const override {
    _timeVarying[hydraPath].insert(locator);
  }

  void FlagAsAssetPathDependent(const SdfPath &usdPath) const override {
    _assetPathDependent.insert(usdPath);
  }

  HdDataSourceLocatorSet const &
  GetTimeVaryingLocators(SdfPath const &hydraPath) const {
    return _timeVarying[hydraPath];
  }

  std::set<SdfPath> const &GetAssetPathDependents() const {
    return _assetPathDependent;
  }

private:
  double _time;
  mutable std::map<SdfPath, HdDataSourceLocatorSet> _timeVarying;
  mutable std::set<SdfPath> _assetPathDependent;
};

// element predicate to remove includes added by importing standard library
bool elementPredicate(mx::ConstElementPtr elem)
{
  if (elem->hasSourceUri())
  {
    return false;
  }
  return true;
}


void TestHdMtlx(const std::string& inputFilename,
                const std::string& outputFilenamePrefix) {
    UsdStageRefPtr stage = UsdStage::Open(inputFilename);

    UsdTimeCode frame(0);
    Hd_UnitTestNullRenderDelegate renderDelegate;
    std::unique_ptr<HdRenderIndex> renderIndex(
        HdRenderIndex::New(&renderDelegate, HdDriverVector()));

    UsdImagingCreateSceneIndicesInfo info;
    info.stage = stage;
    const UsdImagingSceneIndices sceneIndices = UsdImagingCreateSceneIndices(info);
    UsdImagingStageSceneIndexRefPtr stageSceneIndex = sceneIndices.stageSceneIndex;
    stageSceneIndex->SetTime(frame);

    // NOTE: this makes assumptions based on scene index emulation and will
    //       need to be updated when HdRenderIndex no longer uses the
    //       emulated legacy APIs.
    auto registeredSceneIndexNames = HdSceneIndexNameRegistry::GetInstance().GetRegisteredNames();
    if (registeredSceneIndexNames.size() != 1) {
        std::cerr << "expecting 1 registered scene index (via emulation) and found "
                  << registeredSceneIndexNames.size() << " instead." << std::endl;
        return;
    }

    HdSceneIndexBaseRefPtr sceneIndex =
        HdSceneIndexNameRegistry::GetInstance().GetNamedSceneIndex(
            registeredSceneIndexNames[0]);

    if (!sceneIndex) {
        std::cerr << "registered scene index is null." << std::endl;
        return;
    }

    std::string prefix = outputFilenamePrefix;
    if (prefix.empty()) {
      prefix = inputFilename;
    }

    for (const auto& prim : stage->TraverseAll()) {
        if (!prim.IsA<UsdShadeMaterial>()) {
            continue;
        }

        auto mtlSdfPath = prim.GetPath();
        std::string outputFilename = prefix+mtlSdfPath.GetName()+".mtlx";

        TestStageGlobals stageGlobals;
        auto imgPrim = UsdImagingDataSourceMaterialPrim::New(
            mtlSdfPath, stage->GetPrimAtPath(mtlSdfPath), stageGlobals);

        auto mtlDataSource = imgPrim->Get(TfToken("material"));
        HdContainerDataSourceHandle mtlDataSourceContainer =
            HdContainerDataSource::Cast(mtlDataSource);
        auto mtlxDataSource = mtlDataSourceContainer->Get(TfToken("mtlx"));
        HdContainerDataSourceHandle mtlxDataSourceContainer =
            HdContainerDataSource::Cast(mtlxDataSource);

        HdSceneIndexPrim hdPrim = sceneIndex->GetPrim(mtlSdfPath);

        auto hdNetwork = HdDataSourceMaterialNetworkInterface(
            mtlSdfPath, mtlxDataSourceContainer, hdPrim.dataSource);

        auto terminalNames = hdNetwork.GetTerminalNames();
        if (terminalNames.empty()) {
            std::cerr << "No terminals defined." << std::endl;
            return;
        }

        auto terminalNode = hdNetwork.GetTerminalConnection(terminalNames[0]);
        if (!terminalNode.first) {
            std::cerr << "No terminal node." << std::endl;
            return;
        }

        const mx::DocumentPtr &stdLibraries = HdMtlxStdLibraries();
        TfToken terminalNodeName = terminalNode.second.upstreamNodeName;
        TfTokenVector terminalNodeConnectionNames =
            hdNetwork.GetNodeInputConnectionNames(terminalNodeName);

        HdMtlxTexturePrimvarData *mxHdData = nullptr;

        MaterialX::DocumentPtr doc =
            HdMtlxCreateMtlxDocumentFromHdMaterialNetworkInterface(
                &hdNetwork, terminalNodeName, terminalNodeConnectionNames,
                stdLibraries, mxHdData);

        mx::XmlWriteOptions writeOptions;
        writeOptions.elementPredicate = [](mx::ConstElementPtr elem) -> bool {
            // skip writing all includes for brevity
            if (elem->hasSourceUri()) {
                return false;
            }
            return true;
        };

        mx::writeToXmlFile(doc, outputFilename, &writeOptions);
    }
}

int main(int argc, char *argv[]) {
    TfErrorMark mark;

    if (argc == 1) {
        // standard cmake unit test run
        // We are intentionally loading usd files here that hold UsdShade Materials
        // representing MaterialX, instead of loading .mtlx files, because the UsdMtlx
        // plugin can only create UsdShade network for the current MaterialX library
        // version.

        // Test a case where no config:mtlx:version is specified.
        TestHdMtlx("standard_surface_no_mtlx_version.usda",
                   "out_");

        TestHdMtlx("usd_preview_surface_carpaint.usda",
                   "out_");

        // Test a version where MaterialX actually has to mutate the data - switch
        // used to only have 5 inputs but now has 10, so previously a 'which' of anything
        // 5 or over acted like a 0, the MaterialX upgrade modifies those values to 0
        TestHdMtlx("switch_example.usda",
                   "out_");

        // This atan2 test currently fails because HdMtlxCreateMtlxDocumentFromHdMaterialNetworkInterface()
        // is unable to determine the correct type of the input ports, because their names have changed and
        // thus the MaterialX lookup fails.
        // I think we might need to update/extend _GetInputType() in hdMtlx.cpp to derive the correct MaterialX
        // type using the data from HdNetwork - but perhaps not enough data is stored there currently?
//
//      TestHdMtlx("atan2_example.usda",
//                 "out_");

    } else {
        // ad-hoc testing allows for passing options on the command line.

        std::string inputFilename = "";
        std::string outputFilenamePrefix = "";

        for (int i = 0; i < argc; ++i) {
            const std::string arg(argv[i]);

            if (arg == "--inputFilename") {
                inputFilename = mx::FilePath(argv[++i]);
            } else if (arg == "--outputFilenamePrefix") {
                outputFilenamePrefix = mx::FilePath(argv[++i]);
            }
        }

        if (inputFilename.empty() ) {
            std::cerr << "--inputFilename is required.";
            return EXIT_FAILURE;
        }

        TestHdMtlx(inputFilename, outputFilenamePrefix);
    }

    TF_VERIFY(mark.IsClean());

    if (mark.IsClean()) {
        std::cout << "OK" << std::endl;
        return EXIT_SUCCESS;
    } else {
        std::cout << "FAILED" << std::endl;
        return EXIT_FAILURE;
    }
}
