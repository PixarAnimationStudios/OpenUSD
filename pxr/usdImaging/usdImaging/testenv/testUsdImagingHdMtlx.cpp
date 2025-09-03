//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/imaging/hdMtlx/hdMtlx.h"

#include "pxr/imaging/hd/dataSourceMaterialNetworkInterface.h"
#include "pxr/imaging/hd/renderIndex.h"
#include "pxr/imaging/hd/unitTestNullRenderDelegate.h"
#include "pxr/imaging/hd/unitTestHelper.h"
#include "pxr/usdImaging/usdImaging/dataSourceMaterial.h"
#include "pxr/usdImaging/usdImaging/sceneIndices.h"
#include "pxr/usdImaging/usdImaging/stageSceneIndex.h"
#include "pxr/usdImaging/usdImaging/materialParamUtils.h"

#include "pxr/usd/usd/stage.h"
#include "pxr/usd/usd/primRange.h"
#include "pxr/usd/usdShade/material.h"

#include "pxr/base/tf/errorMark.h"

#include <MaterialXFormat/XmlIo.h>

#include <iostream>

PXR_NAMESPACE_USING_DIRECTIVE


TF_DEFINE_PRIVATE_TOKENS(
    _tokens,
    (mtlx)
    (material)
    ((mtlxVersion, "mtlx:version"))
);

namespace mx = MaterialX;

// NOTE: This class is copied from testUsdImagingDataSourceAttribute.cpp
class TestStageGlobals : public UsdImagingDataSourceStageGlobals {
public:
    TestStageGlobals() : _time(0) {}
    ~TestStageGlobals() override = default;

    UsdTimeCode GetTime() const override { return UsdTimeCode(_time); }

    void FlagAsTimeVarying(
        const SdfPath &hydraPath,
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

// Replace the given filename extension with 'mtlx'
static std::string 
_CreateMtlxFilename(std::string filename)
{
    size_t extPos = filename.find_last_of(".");
    return filename.replace(extPos, filename.length() - extPos, ".mtlx");
}

bool
TestHdMtlx(const std::string& inputFilename)
{
    UsdStageRefPtr stage = UsdStage::Open(inputFilename);
    UsdTimeCode frame(0);

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

    for (const auto& prim : stage->TraverseAll()) {
        if (!prim.IsA<UsdShadeMaterial>()) {
            continue;
        }

        // Get the MaterialX Material DataSource 
        const SdfPath materialPath = prim.GetPath();
        TestStageGlobals stageGlobals;
        auto imgPrimDs = UsdImagingDataSourceMaterialPrim::New(
            materialPath, stage->GetPrimAtPath(materialPath), stageGlobals);

        UsdImagingDataSourceMaterialHandle materialDs =
            UsdImagingDataSourceMaterial::Cast(
                imgPrimDs->Get(_tokens->material));
        HdContainerDataSourceHandle mtlxNetworkDs =
            HdContainerDataSource::Cast(materialDs->Get(_tokens->mtlx));

        // Get the HdMaterialNetwork
        HdSceneIndexPrim hdPrim = stageSceneIndex->GetPrim(materialPath);
        if (hdPrim.primType.IsEmpty()) {
            std::cout << " - No prim type for <" << materialPath << ">\n";
            return false;
        }
        HdDataSourceMaterialNetworkInterface hdNetInterfaceDs = 
            HdDataSourceMaterialNetworkInterface(
                materialPath, mtlxNetworkDs, hdPrim.dataSource);

        // Get the terminal node from the Material Network 
        TfTokenVector terminalNames = hdNetInterfaceDs.GetTerminalNames();
        if (terminalNames.empty()) {
            std::cout << "No terminals defined on <" << materialPath << ">.\n";
            return false;
        }
        const TfToken terminalType = terminalNames[0];
        const auto terminalNodeConn =
            hdNetInterfaceDs.GetTerminalConnection(terminalType);
        if (!terminalNodeConn.first) {
            std::cout << "No terminal node for type '" << terminalType << "'.\n";
            return false;
        }

        // Create MaterialX Document 
        const mx::DocumentPtr &stdLibraries = HdMtlxStdLibraries();
        TfToken terminalNodeName = terminalNodeConn.second.upstreamNodeName;
        TfTokenVector terminalNodeConnectionNames =
            hdNetInterfaceDs.GetNodeInputConnectionNames(terminalNodeName);

        HdMtlxTexturePrimvarData *mxHdData = nullptr;
        MaterialX::DocumentPtr mtlxDoc =
            HdMtlxCreateMtlxDocumentFromHdMaterialNetworkInterface(
                &hdNetInterfaceDs, terminalNodeName, 
                terminalNodeConnectionNames,
                stdLibraries, mxHdData);

        // Write out the MaterialX document
        mx::XmlWriteOptions mxWriteOptions;
        mxWriteOptions.elementPredicate = [](mx::ConstElementPtr elem) -> bool {
            // skip writing all includes for brevity
            return !elem->hasSourceUri();
        };

        const std::string outputFilename = _CreateMtlxFilename(inputFilename);
        mx::writeToXmlFile(mtlxDoc, outputFilename, &mxWriteOptions);
    }
    return true;
}


int main(int argc, char *argv[])
{
    std::string inputFilename = "";
    for (int i = 0; i < argc; ++i) {
        const std::string arg(argv[i]);

        if (arg == "--filename") {
            inputFilename = mx::FilePath(argv[++i]);
        }
    }

    if (inputFilename.empty() ) {
        std::cout << "--filename is required.";
        return EXIT_FAILURE;
    }

    TfErrorMark mark;
    bool success = TestHdMtlx(inputFilename);

    if (success && mark.IsClean()) {
        std::cout << "OK" << std::endl;
        return EXIT_SUCCESS;
    } else {
        std::cout << "FAILED" << std::endl;
        return EXIT_FAILURE;
    }
}
