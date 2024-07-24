//
// Copyright 2023 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
///
/// \file usdUtils/usdzPackage.cpp
#include "pxr/pxr.h"
#include "pxr/usd/usdUtils/assetLocalization.h"
#include "pxr/usd/usdUtils/assetLocalizationPackage.h"
#include "pxr/usd/usdUtils/debugCodes.h"
#include "pxr/usd/usdUtils/usdzPackage.h"

#include "pxr/usd/ar/resolver.h"
#include "pxr/usd/sdf/assetPath.h"
#include "pxr/usd/usd/stage.h"
#include "pxr/usd/usd/usdcFileFormat.h"
#include "pxr/usd/usd/zipFile.h"

#include "pxr/base/tf/fileUtils.h"
#include "pxr/base/tf/pathUtils.h"
#include "pxr/base/trace/trace.h"

PXR_NAMESPACE_OPEN_SCOPE

class UsdUtils_UsdzPackageBuilder : public UsdUtils_AssetLocalizationPackage
{
public:
    // Creates the usdz archive from all discovered dependencies of the asset.
    bool 
    Write(
        const std::string &usdzFilePath) override
    {
        _writer = UsdZipFileWriter::CreateNew(usdzFilePath);

        const bool success = 
            UsdUtils_AssetLocalizationPackage::Write(usdzFilePath);

        return success && _writer.Save();
    }

protected:
    bool
    _WriteToPackage(
        const std::string& source,
        const std::string& dest) override
    {
        std::string inPackagePath = _writer.AddFile(source, dest);

        return !inPackagePath.empty();
    }

    private:
        UsdZipFileWriter _writer;
};

static 
bool
_CreateNewUsdzPackage(const SdfAssetPath &assetPath,
                      const std::string &usdzFilePath,
                      const std::string &firstLayerName,
                      const std::string &origRootFilePath=std::string(),
                      const std::vector<std::string> &dependenciesToSkip =
                          std::vector<std::string>(),
                      bool editLayersInPlace = false)
{
    UsdUtils_UsdzPackageBuilder builder;
    builder.SetOriginalRootFilePath(origRootFilePath);
    builder.SetDependenciesToSkip(dependenciesToSkip);
    builder.SetEditLayersInPlace(editLayersInPlace);

    if (!builder.Build(assetPath, firstLayerName)) {
        return false;
    }

    return builder.Write(usdzFilePath);
}

bool
UsdUtilsCreateNewUsdzPackage(
    const SdfAssetPath& assetPath,
    const std::string& usdzFilePath,
    const std::string& firstLayerName,
    bool editLayersInPlace)
{
    TRACE_FUNCTION();
    
    return _CreateNewUsdzPackage(
        assetPath,
        usdzFilePath,
        firstLayerName,
        /*origRootFilePath*/ "",
        /*origRootFilePath*/ {},
        editLayersInPlace);
}

bool
UsdUtilsCreateNewARKitUsdzPackage(
    const SdfAssetPath &assetPath,
    const std::string &inUsdzFilePath,
    const std::string &firstLayerName,
    bool editLayersInPlace)
{
    TRACE_FUNCTION();

    auto &resolver = ArGetResolver();

    std::string usdzFilePath = ArchNormPath(inUsdzFilePath);

    const std::string resolvedPath = resolver.Resolve(assetPath.GetAssetPath());
    if (resolvedPath.empty()) {
        return false;
    }
    
    // Check if the given asset has external dependencies that participate in 
    // the composition of the stage.
    std::vector<std::string> sublayers, references, payloads;

    UsdUtils_ExtractExternalReferences(resolvedPath, 
        UsdUtils_LocalizationContext::ReferenceType::CompositionOnly,
        &sublayers, &references, &payloads);

    // Ensure that the root layer has the ".usdc" extension.
    std::string targetBaseName = firstLayerName.empty() ? 
        TfGetBaseName(assetPath.GetAssetPath()) : firstLayerName;
    const std::string &fileExt = resolver.GetExtension(targetBaseName);
    bool renamingRootLayer = false;
    if (fileExt != UsdUsdcFileFormatTokens->Id) {
        renamingRootLayer = true;
        targetBaseName = targetBaseName.substr(0, targetBaseName.rfind(".")+1) +  
                UsdUsdcFileFormatTokens->Id.GetString();
    }

    // If there are no external dependencies needed for composition, we can 
    // invoke the regular packaging function.
    if (sublayers.empty() && references.empty() && payloads.empty()) {
        if (renamingRootLayer) {
            return _CreateNewUsdzPackage(
                    assetPath, 
                    usdzFilePath, 
                    /*firstLayerName*/ targetBaseName, 
                    /*origRootFilePath*/ resolvedPath,
                    /*dependenciesToSkip*/ {resolvedPath},
                    editLayersInPlace);
        } else {
            return _CreateNewUsdzPackage(
                assetPath, 
                usdzFilePath, 
                /*firstLayerName*/ targetBaseName, 
                /*origRootFilePath*/ resolvedPath,
                /*dependenciesToSkip*/ {},
                editLayersInPlace);
        }
    }

    TF_WARN("The given asset '%s' contains one or more composition arcs "
        "referencing external USD files. Flattening it to a single .usdc file "
        "before packaging. This will result in loss of features such as "
        "variantSets and all asset references to be absolutized.", 
        assetPath.GetAssetPath().c_str());

    const auto &usdStage = UsdStage::Open(resolvedPath);
    const std::string tmpFileName = 
            ArchMakeTmpFileName(targetBaseName, ".usdc");

    TF_DEBUG(USDUTILS_CREATE_PACKAGE).Msg(
        "Flattening asset @%s@ located at '%s' to temporary layer at "
        "path '%s'.\n", assetPath.GetAssetPath().c_str(), resolvedPath.c_str(), 
        tmpFileName.c_str());

    if (!usdStage->Export(tmpFileName, /*addSourceFileComment*/ false)) {
        TF_WARN("Failed to flatten and export the USD stage '%s'.", 
            UsdDescribe(usdStage).c_str());
        return false;
    }

    bool success = _CreateNewUsdzPackage(
        /*assetPath*/ SdfAssetPath(tmpFileName), 
        usdzFilePath, 
        /* firstLayerName */ targetBaseName,
        /* origRootFilePath*/ resolvedPath,
        /*dependenciesToSkip*/ {resolvedPath},
        editLayersInPlace);

    if (success) {
        TfDeleteFile(tmpFileName);
    } else {
        TF_WARN("Failed to create a .usdz package from temporary, flattened "
            "layer '%s'.", tmpFileName.c_str());;
    }

    return success;
}


PXR_NAMESPACE_CLOSE_SCOPE
