//
// Copyright 2023 Pixar
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
///
/// \file usdUtils/usdzPackage.cpp
#include "pxr/pxr.h"
#include "pxr/usd/usdUtils/assetLocalization.h"
#include "pxr/usd/usdUtils/dependencies.h"
#include "pxr/usd/usdUtils/debugCodes.h"

#include "pxr/usd/ar/packageUtils.h"
#include "pxr/usd/ar/resolver.h"
#include "pxr/usd/sdf/assetPath.h"
#include "pxr/usd/sdf/layerUtils.h"
#include "pxr/usd/sdf/types.h"
#include "pxr/usd/usd/stage.h"
#include "pxr/usd/usd/usdFileFormat.h"
#include "pxr/usd/usd/usdcFileFormat.h"
#include "pxr/usd/usd/zipFile.h"

#include "pxr/base/tf/fileUtils.h"
#include "pxr/base/tf/pathUtils.h"
#include "pxr/base/trace/trace.h"

#include <stack>
#include <vector>

PXR_NAMESPACE_OPEN_SCOPE

// Returns a relative path for fullDestPath, relative to the given destination 
// directory (destDir).
static 
std::string 
_GetDestRelativePath(const std::string &fullDestPath, 
                     const std::string &destDir)
{
    std::string destPath = fullDestPath;
    // fullDestPath won't start with destDir if destDir is a relative path, 
    // relative to CWD.
    if (TfStringStartsWith(destPath, destDir)) {
        destPath = destPath.substr(destDir.length());
    }
    return destPath;
}

static 
bool
_CreateNewUsdzPackage(const SdfAssetPath &assetPath,
                      const std::string &usdzFilePath,
                      const std::string &firstLayerName,
                      const std::string &origRootFilePath=std::string(),
                      const std::vector<std::string> &dependenciesToSkip
                            =std::vector<std::string>())
{
    TF_DEBUG(USDUTILS_CREATE_USDZ_PACKAGE).Msg("Creating USDZ package at '%s' "
        "containing asset @%s@.\n", usdzFilePath.c_str(), 
        assetPath.GetAssetPath().c_str());

    std::string destDir = TfGetPathName(usdzFilePath);
    destDir = destDir.empty() ? "./" : destDir;
    UsdUtils_AssetLocalizer localizer(assetPath, destDir, 
                            /* enableMetadataFiltering */ true,
                            firstLayerName, 
                            origRootFilePath, 
                            dependenciesToSkip);

    auto &layerExportMap = localizer.GetLayerExportMap();
    auto &fileCopyMap = localizer.GetFileCopyMap();

    if (layerExportMap.empty() && fileCopyMap.empty()) {
        return false;
    }

    // Set of all the packaged files.
    std::unordered_set<std::string> packagedFiles;

    const std::string tmpDirPath = ArchGetTmpDir();

    UsdZipFileWriter writer = UsdZipFileWriter::CreateNew(usdzFilePath);

    auto &resolver = ArGetResolver();
    // This returns true of src and dest have the same file extension.
    const auto extensionsMatch = [&resolver](const std::string &src, 
                                             const std::string &dest) {
        return resolver.GetExtension(src) == resolver.GetExtension(dest);
    };

    bool firstLayer = true;
    bool success = true;
    for (auto &layerAndDestPath : layerExportMap) {
        const auto &layer = layerAndDestPath.first;
        std::string destPath = _GetDestRelativePath(
                layerAndDestPath.second, destDir);

        // Change the first layer's name if requested.
        if (firstLayer && !firstLayerName.empty()) {
            const std::string pathName = TfGetPathName(destPath);
            destPath = TfStringCatPaths(pathName, firstLayerName);
            firstLayer = false;
        }

        if (!packagedFiles.insert(destPath).second) {
            TF_WARN("A file already exists at path \"%s\" in the package. "
                "Skipping export of layer @%s@.", destPath.c_str(), 
                layer->GetIdentifier().c_str());
            continue;
        }

        TF_DEBUG(USDUTILS_CREATE_USDZ_PACKAGE).Msg(
            ".. adding layer @%s@ to package at path '%s'.\n", 
            layer->GetIdentifier().c_str(), destPath.c_str());

        // If the layer is a package or if it's inside a package, copy the 
        // entire package. We could extract the package and copy only the 
        // dependencies, but this could get very complicated.
        if (layer->GetFileFormat()->IsPackage() ||
             ArIsPackageRelativePath(layer->GetIdentifier())) {
            std::string packagePath = ArSplitPackageRelativePathOuter(
                    layer->GetRealPath()).first;
            std::string destPackagePath = ArSplitPackageRelativePathOuter(
                    destPath).first;
            if (!packagePath.empty()) {
                std::string inArchivePath = writer.AddFile(packagePath,
                        destPackagePath);
                if (inArchivePath.empty()) {
                    success = false;
                }
            }
        } else if (!layer->IsDirty() && 
                   extensionsMatch(layer->GetRealPath(), destPath)) {
            // If the layer hasn't been modified from its persistent 
            // representation and if its extension isn't changing in the 
            // package, then simply copy it over from its real-path (i.e. 
            // location on disk). This preserves any existing comments in the 
            // file (which will be lost if we were to export all layers before 
            // adding them to to the package).
            std::string inArchivePath = writer.AddFile(layer->GetRealPath(), 
                    destPath);
            if (inArchivePath.empty()) {
                success = false;
            }
        } else {
            // If the layer has been modified or needs to be modified, then we 
            // need to export it to a temporary file before adding it to the 
            // package.
            SdfFileFormat::FileFormatArguments args;

            const SdfFileFormatConstPtr fileFormat = 
                    SdfFileFormat::FindByExtension(
                        SdfFileFormat::GetFileExtension(destPath));

            if (TfDynamic_cast<UsdUsdFileFormatConstPtr>(fileFormat)) {
                args[UsdUsdFileFormatTokens->FormatArg] = 
                        UsdUsdFileFormat::GetUnderlyingFormatForLayer(
                            *get_pointer(layer));
            }
            
            std::string tmpLayerExportPath = TfStringCatPaths(tmpDirPath, 
                    TfGetBaseName(destPath));
            layer->Export(tmpLayerExportPath, /*comment*/ "", args);

            std::string inArchivePath = writer.AddFile(tmpLayerExportPath, 
                    destPath);

            if (inArchivePath.empty()) {
                // XXX: Should we discard the usdz file and return early here?
                TF_WARN("Failed to add temporary layer at '%s' to the package "
                    "at path '%s'.", tmpLayerExportPath.c_str(), 
                    usdzFilePath.c_str());
                success = false;
            } else {
                // The file has been added to the package successfully. We can 
                // delete it now.
                TfDeleteFile(tmpLayerExportPath);
            }
        }
    }

    for (auto &fileSrcAndDestPath : fileCopyMap) {
        const std::string &srcPath = fileSrcAndDestPath.first;
        const std::string destPath = _GetDestRelativePath(
                fileSrcAndDestPath.second, destDir);
        TF_DEBUG(USDUTILS_CREATE_USDZ_PACKAGE).Msg(
            ".. adding file '%s' to package at path '%s'.\n", 
            srcPath.c_str(), destPath.c_str());

        if (!packagedFiles.insert(destPath).second) {
            TF_WARN("A file already exists at path \"%s\" in the package. "
                "Skipping copy of file \"%s\".", destPath.c_str(), 
                srcPath.c_str());
            continue;
        }

        // If the file is a package or inside a package, copy the
        // entire package. We could extract the package and copy only the 
        // dependencies, but this could get very complicated.
        if (ArIsPackageRelativePath(destPath)) {
            std::string packagePath = ArSplitPackageRelativePathOuter(
                    srcPath).first;
            std::string destPackagePath = ArSplitPackageRelativePathOuter(
                    destPath).first;
            if (!packagePath.empty()) {
                std::string inArchivePath = writer.AddFile(packagePath,
                    destPackagePath);
                if (inArchivePath.empty()) {
                    success = false;
                }
            }
        }
        else {
            std::string inArchivePath = writer.AddFile(srcPath, destPath);
            if (inArchivePath.empty()) {
                // XXX: Should we discard the usdz file and return early here?
                TF_WARN("Failed to add file '%s' to the package at path '%s'.",
                    srcPath.c_str(), usdzFilePath.c_str());
                success = false;
            }
        }
    }

    return writer.Save() && success;
}

bool
UsdUtilsCreateNewUsdzPackage(const SdfAssetPath &assetPath,
                             const std::string &usdzFilePath,
                             const std::string &firstLayerName)
{
    return _CreateNewUsdzPackage(assetPath, usdzFilePath, firstLayerName);
}

bool
UsdUtilsCreateNewARKitUsdzPackage(
    const SdfAssetPath &assetPath,
    const std::string &inUsdzFilePath,
    const std::string &firstLayerName)
{
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
        UsdUtils_FileAnalyzer::ReferenceType::CompositionOnly,
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
            return _CreateNewUsdzPackage(assetPath, usdzFilePath, 
                    /*firstLayerName*/ targetBaseName, 
                    /* origRootFilePath*/ resolvedPath,
                    /* dependenciesToSkip */ {resolvedPath});
        } else {
            return _CreateNewUsdzPackage(assetPath, usdzFilePath, 
                /*firstLayerName*/ targetBaseName, 
                /* origRootFilePath*/ resolvedPath);
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

    TF_DEBUG(USDUTILS_CREATE_USDZ_PACKAGE).Msg(
        "Flattening asset @%s@ located at '%s' to temporary layer at "
        "path '%s'.\n", assetPath.GetAssetPath().c_str(), resolvedPath.c_str(), 
        tmpFileName.c_str());

    if (!usdStage->Export(tmpFileName, /*addSourceFileComment*/ false)) {
        TF_WARN("Failed to flatten and export the USD stage '%s'.", 
            UsdDescribe(usdStage).c_str());
        return false;
    }

    bool success = _CreateNewUsdzPackage(SdfAssetPath(tmpFileName), 
        usdzFilePath, /* firstLayerName */ targetBaseName,
        /* origRootFilePath*/ resolvedPath,
        /*dependenciesToSkip*/ {resolvedPath});

    if (success) {
        TfDeleteFile(tmpFileName);
    } else {
        TF_WARN("Failed to create a .usdz package from temporary, flattened "
            "layer '%s'.", tmpFileName.c_str());;
    }

    return success;
}

PXR_NAMESPACE_CLOSE_SCOPE
