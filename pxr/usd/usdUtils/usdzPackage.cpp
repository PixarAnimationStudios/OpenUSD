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
#include "pxr/usd/usdUtils/assetLocalizationDelegate.h"
#include "pxr/usd/usdUtils/dependencies.h"
#include "pxr/usd/usdUtils/debugCodes.h"
#include "pxr/usd/usdUtils/usdzPackage.h"

#include "pxr/usd/ar/packageUtils.h"
#include "pxr/usd/ar/resolver.h"
#include "pxr/usd/sdf/assetPath.h"
#include "pxr/usd/sdf/layerUtils.h"
#include "pxr/usd/sdf/types.h"
#include "pxr/usd/usd/stage.h"
#include "pxr/usd/usd/usdFileFormat.h"
#include "pxr/usd/usd/usdcFileFormat.h"
#include "pxr/usd/usd/zipFile.h"
#include "pxr/usd/usdShade/udimUtils.h"

#include "pxr/base/tf/fileUtils.h"
#include "pxr/base/tf/pathUtils.h"
#include "pxr/base/trace/trace.h"

#include <map>
#include <stack>
#include <vector>

PXR_NAMESPACE_OPEN_SCOPE

class UsdUtils_DirectoryRemapper {
public:
    UsdUtils_DirectoryRemapper() : _nextDirectoryNum(0) { }

    // Remap the given file path by replacing the directory with a
    // unique, artifically generated name. The generated directory name
    // will be reused if the original directory is seen again on a
    // subsequent call to Remap.
    std::string Remap(const std::string& filePath)
    {
        if (ArIsPackageRelativePath(filePath)) {
            std::pair<std::string, std::string> packagePath = 
                ArSplitPackageRelativePathOuter(filePath);
            return ArJoinPackageRelativePath(
                Remap(packagePath.first), packagePath.second);
        }

        const std::string pathName = TfGetPathName(filePath);
        if (pathName.empty()) {
            return filePath;
        }

        const std::string baseName = TfGetBaseName(filePath);
        
        auto insertStatus = _oldToNewDirectory.insert({pathName, ""});
        if (insertStatus.second) {
            insertStatus.first->second = 
                TfStringPrintf("%zu", _nextDirectoryNum++);
        }
        
        return TfStringCatPaths(insertStatus.first->second, baseName);
    }

private:
    size_t _nextDirectoryNum;
    std::unordered_map<std::string, std::string> _oldToNewDirectory;
};

class UsdUtils_UsdzPackageBuilder {
public:

    // Sets the original file path for this asset.
    // This is used with creating an ARKit usdz package as there may be some
    // processing done on the source asset file before the final package is
    // created. The path specified should be resolved by AR.
    inline void SetOriginalRootFilePath(const std::string &origRootFilePath)
    {
        _origRootFilePath = origRootFilePath;
    }

    // Sets a list of dependencies to skip during packaging.
    // The paths contained in this array should be resolved by AR.
    inline void SetDependenciesToSkip(
        const std::vector<std::string> &dependenciesToSkip) 
    {
        _dependenciesToSkip = dependenciesToSkip;
    }

    // Processes the asset, updating the layers and performing directory
    // remapping.
    bool Build(const SdfAssetPath& assetPath, const std::string &firstLayerName)
    {
        namespace Placeholders = std::placeholders;
        const std::string assetPathStr = assetPath.GetAssetPath();

        _rootFilePath = ArGetResolver().Resolve(assetPathStr);
        if (_rootFilePath.empty()) {
            return false;
        }

        _rootLayer =SdfLayer::FindOrOpen(assetPathStr);
        if (!_rootLayer) {
            return false;
        }

        // Change the first layer's name if requested.
        _firstLayerName = firstLayerName.empty() 
            ? TfGetBaseName(_rootLayer->GetRealPath()) 
            : firstLayerName;

        auto binding = std::bind(
            &UsdUtils_UsdzPackageBuilder::_ProcessDependency, 
            this, Placeholders::_1, Placeholders::_2, 
            Placeholders::_3, Placeholders::_4);

        UsdUtils_WritableLocalizationDelegate delegate(binding);
        UsdUtils_LocalizationContext context(&delegate);
        context.SetMetadataFilteringEnabled(true);
        context.SetDependenciesToSkip(_dependenciesToSkip);
        context.Process(_rootLayer);

        return true;
    }

    // Creates the usdz archive from all discovered dependencies of the asset.
    bool Write(const std::string &usdzFilePath)
    {
        _usdzFilePath = usdzFilePath;

        // Set of all the packaged files.
        std::unordered_set<std::string> packagedFiles;
        UsdZipFileWriter writer = UsdZipFileWriter::CreateNew(_usdzFilePath);
        bool success = true;

        packagedFiles.insert(_firstLayerName);
        _AddLayerToPackage(&writer, _rootLayer, _firstLayerName);

        for (const auto & layerDep : _layersToCopy) {

            if (!packagedFiles.insert(layerDep.second).second) {
                TF_WARN("A file already exists at path \"%s\" in the package. "
                    "Skipping export of dependency @%s@.",  
                    layerDep.second.c_str(), layerDep.first.c_str());
                continue;
            }

            const SdfLayerRefPtr layer = SdfLayer::FindOrOpen(layerDep.first);
            success &= _AddLayerToPackage(&writer, layer, layerDep.second);
        }

        for (const auto & fileDep : _filesToCopy) {
            if (!packagedFiles.insert(fileDep.second).second) {
                TF_WARN("A file already exists at path \"%s\" in the package. "
                    "Skipping export of dependency @%s@.", 
                    fileDep.second.c_str(), fileDep.first.c_str());
                continue;
            }

            success &= _AddAssetToPackage(&writer, fileDep.first, fileDep.second);
        }

        return writer.Save() && success;
    }

private:

    // Main callback for dependency processing. facilites the directory
    // remapping and package path generation.
    std::string _ProcessDependency( 
        const SdfLayerRefPtr &layer, 
        const std::string &assetPath,
        const std::vector<std::string>& dependencies,
        UsdUtilsDependencyType dependencyType)
    {
        std::string anchoredPath = 
            SdfComputeAssetPathRelativeToLayer(layer, assetPath);
        bool isRelativeOutput;
        std::string remappedPath = 
            _RemapAssetPath(assetPath, layer, &isRelativeOutput);
        std::string packagePath = remappedPath;
        if (isRelativeOutput) {
            // If it's a relative path, construct the full path relative to
            // the final (destination) location of the reference-containing 
            // file. This is only applicable if the path is a filesystem path
            const auto containingLayerInfo = 
                _layersToCopy.find(layer->GetIdentifier());

            if (containingLayerInfo != _layersToCopy.end()) {
                packagePath = TfNormPath(TfStringCatPaths(
                    TfGetPathName(containingLayerInfo->second), assetPath));
            }
        }

        // Add all dependencies to package
        std::string destDirectory = TfGetPathName(packagePath);

        for (const auto &dependency : dependencies) {
            const std::string dependencyAnchored = 
                SdfComputeAssetPathRelativeToLayer(layer, dependency);
            const std::string dependencyPackage = TfNormPath(
                TfStringCatPaths(destDirectory, TfGetBaseName(dependency)));
            
            if (UsdStage::IsSupportedFile(dependencyAnchored)) {
                _layersToCopy[dependencyAnchored] = dependencyPackage;
            } else {
                _filesToCopy.emplace_back(
                    std::make_pair(dependencyAnchored, dependencyPackage));
            }
        }

        
        return remappedPath;
    }

    std::string _RemapAssetPath(const std::string &refPath,
        const SdfLayerRefPtr &layer, bool *isRelativePathOut)
    {
        auto &resolver = ArGetResolver();

        const bool isContextDependentPath =
            resolver.IsContextDependentPath(refPath);

        // We want to maintain relative paths where possible to keep localized
        // assets as close as possible to their original layout. However, we
        // skip this for context-dependent paths because those must be resolved
        // to determine what asset is being referred to.
        if (!isContextDependentPath) {
            // We determine if refPath is relative by creating identifiers with
            // and without the anchoring layer and seeing if they're the same.
            // If they aren't, then refPath depends on the anchor, so we assume
            // it's relative.
            const std::string anchored = 
                resolver.CreateIdentifier(refPath, layer->GetResolvedPath());
            const std::string unanchored =
                resolver.CreateIdentifier(refPath);
            const bool isRelativePath = (anchored != unanchored);

            if (isRelativePath) {
                // Asset localization is rooted at the location of the root
                // layer. If this relative path points somewhere outside that
                // location (e.g., a relative path like "../foo.jpg"). there 
                // will be nowhere to put this asset in the localized asset
                // structure. In that case, we need to remap this path. 
                // Otherwise, we can keep the relative asset path as-is.
                const ArResolvedPath resolvedRefPath = 
                    resolver.Resolve(anchored);
                const bool refPathIsOutsideAssetLocation = 
                    !TfStringStartsWith(
                        TfNormPath(TfGetPathName(resolvedRefPath)),
                        TfNormPath(TfGetPathName(_rootFilePath)));

                if (!refPathIsOutsideAssetLocation) {
                    if (isRelativePathOut) {
                        *isRelativePathOut = true;
                    }

                    // Return relative paths unmodified.
                    return refPath;
                }
            }
        }

        if (isRelativePathOut) {
            *isRelativePathOut = false;
        }

        std::string result = refPath;
        if (isContextDependentPath) {
            // Absolutize the search path, to avoid collisions resulting from 
            // the same search path resolving to different paths in different 
            // resolver contexts.
            const std::string refAssetPath = 
                    SdfComputeAssetPathRelativeToLayer(layer, refPath);
            const std::string refFilePath = resolver.Resolve(refAssetPath);

            const bool resolveOk = !refFilePath.empty();

            if (resolveOk) {
                result = refFilePath;
            } else {
                // Failed to resolve, hence retain the reference as is.
                result = refAssetPath;
            }
        }

        // Normalize paths compared below to account for path format differences.
        const std::string layerPath = 
            TfNormPath(layer->GetRealPath());
        result = TfNormPath(result);
        std::string rootFilePath = TfNormPath(_rootFilePath);
        std::string origRootFilePath = TfNormPath(_origRootFilePath);

        bool resultPointsToRoot = ((result == rootFilePath) || 
                                (result == origRootFilePath));
        // If this is a self-reference, then remap to a relative path that
        // points to the file itself.
        if (result == layerPath) {
            // If this is a self-reference in the root layer and we're renaming 
            // the root layer, simply set the reference path to point to the 
            // renamed root layer.
            return resultPointsToRoot && !_firstLayerName.empty() ? 
                _firstLayerName : TfGetBaseName(result);
        }

        // References to the original (unflattened) root file need to be 
        // remapped to point to the new root file.
        if (resultPointsToRoot && layerPath == rootFilePath) {
            return !_firstLayerName.empty() 
                ? _firstLayerName 
                : TfGetBaseName(result);
        }

        // Result is now an absolute or a repository path. Simply strip off the 
        // leading slashes to make it relative.
        // Strip off any drive letters.
        if (result.size() >= 2 && result[1] == ':') {
            result.erase(0, 2);
        }

        // Strip off any initial slashes.
        result = TfStringTrimLeft(result, "/");

        // Remap the path to an artifically-constructed one so that the source 
        // directory structure isn't embedded in the final .usdz file.
        // Otherwise, sensitive information (e.g. usernames, movie titles...) in
        // directory names may be inadvertently leaked in the .usdz file.
        return _directoryRemapper.Remap(result);
    }

    bool _AddLayerToPackage(UsdZipFileWriter *writer, 
        const SdfLayerRefPtr layer, const std::string &destPath)
    {
        TF_DEBUG(USDUTILS_CREATE_USDZ_PACKAGE).Msg(
            ".. adding layer @%s@ to package at path '%s'.\n", 
            layer->GetIdentifier().c_str(), destPath.c_str());

                
        // This returns true of src and dest have the same file extension.
        const auto extensionsMatch = [](const std::string &src, 
                                        const std::string &dest) {
            auto &resolver = ArGetResolver();
            return resolver.GetExtension(src) == resolver.GetExtension(dest);
        };

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
                std::string inArchivePath = writer->AddFile(packagePath,
                        destPackagePath);
                if (inArchivePath.empty()) {
                    return false;
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
            std::string inArchivePath = writer->AddFile(layer->GetRealPath(), 
                    destPath);
            if (inArchivePath.empty()) {
                return false;
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
            
            const std::string tmpDirPath = ArchGetTmpDir();
            std::string tmpLayerExportPath = TfStringCatPaths(tmpDirPath, 
                    TfGetBaseName(destPath));
            layer->Export(tmpLayerExportPath, /*comment*/ "", args);

            std::string inArchivePath = writer->AddFile(tmpLayerExportPath, 
                    destPath);

            if (inArchivePath.empty()) {
                // XXX: Should we discard the usdz file and return early here?
                TF_WARN("Failed to add temporary layer at '%s' to the package "
                    "at path '%s'.", tmpLayerExportPath.c_str(), 
                    _usdzFilePath.c_str());
                return false;
            } else {
                // The file has been added to the package successfully. We can 
                // delete it now.
                TfDeleteFile(tmpLayerExportPath);
            }
        }

        return true;
    }

    bool _AddAssetToPackage(UsdZipFileWriter *writer,const std::string &srcPath,
        const std::string &destPath)
    {
        TF_DEBUG(USDUTILS_CREATE_USDZ_PACKAGE).Msg(
            ".. adding file '%s' to package at path '%s'.\n", 
            srcPath.c_str(), destPath.c_str());

        // If the file is a package or inside a package, copy the
        // entire package. We could extract the package and copy only the 
        // dependencies, but this could get very complicated.
        if (ArIsPackageRelativePath(destPath)) {
            std::string packagePath = ArSplitPackageRelativePathOuter(
                    srcPath).first;
            std::string destPackagePath = ArSplitPackageRelativePathOuter(
                    destPath).first;
            if (!packagePath.empty()) {
                std::string inArchivePath = writer->AddFile(packagePath,
                    destPackagePath);
                if (inArchivePath.empty()) {
                    return false;
                }
            }
        }
        else {
            std::string inArchivePath = writer->AddFile(srcPath, destPath);
            if (inArchivePath.empty()) {
                // XXX: Should we discard the usdz file and return early here?
                TF_WARN("Failed to add file '%s' to the package at path '%s'.",
                    srcPath.c_str(), _usdzFilePath.c_str());
                return false;
            }
        }

        return true;
    }

private:
    SdfLayerRefPtr _rootLayer;
    UsdUtils_DirectoryRemapper _directoryRemapper;

    // User supplied first layer override name
    std::string _firstLayerName;

    // The original root file path...used for ARkit packages
    std::string _origRootFilePath;

    // The resolved path of the root usd layer
    std::string _rootFilePath;

    // The output path that the USDZ package will be writtent o
    std::string _usdzFilePath;

    // List of dependencies to skip during packaging.
    std::vector<std::string> _dependenciesToSkip;

    // Maps Layer's anchored path to package path
    // This lookup is handy for determining package paths of assets with
    // relative paths
    std::map<std::string, std::string> _layersToCopy;

    // List of non layer dependencies to copy into the package.  Each element
    // is a pair consisting of the source path and package path
    using FileToCopy = std::pair<std::string, std::string>;
    std::vector<FileToCopy> _filesToCopy;
};

static 
bool
_CreateNewUsdzPackage(const SdfAssetPath &assetPath,
                      const std::string &usdzFilePath,
                      const std::string &firstLayerName,
                      const std::string &origRootFilePath=std::string(),
                      const std::vector<std::string> &dependenciesToSkip
                            =std::vector<std::string>())
{
    UsdUtils_UsdzPackageBuilder builder;
    builder.SetOriginalRootFilePath(origRootFilePath);
    builder.SetDependenciesToSkip(dependenciesToSkip);

    if (!builder.Build(assetPath, firstLayerName)) {
        return false;
    }

    return builder.Write(usdzFilePath);
}

bool
UsdUtilsCreateNewUsdzPackage(
    const SdfAssetPath& assetPath,
    const std::string& usdzFilePath,
    const std::string& firstLayerName)
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
