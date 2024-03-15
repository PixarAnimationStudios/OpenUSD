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

#include "pxr/pxr.h"

#include "pxr/base/tf/fileUtils.h"
#include "pxr/base/tf/pathUtils.h"

#include "pxr/usd/sdf/layerUtils.h"
#include "pxr/usd/ar/packageUtils.h"
#include "pxr/usd/ar/resolver.h"
#include "pxr/usd/usd/stage.h"
#include "pxr/usd/usd/usdFileFormat.h"
#include "pxr/usd/usdUtils/assetLocalization.h"
#include "pxr/usd/usdUtils/assetLocalizationDelegate.h"
#include "pxr/usd/usdUtils/assetLocalizationPackage.h"
#include "pxr/usd/usdUtils/debugCodes.h"

#include <unordered_set>

PXR_NAMESPACE_OPEN_SCOPE

std::string 
UsdUtils_DirectoryRemapper::Remap(
    const std::string& filePath)
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

bool 
UsdUtils_AssetLocalizationPackage::Build(
    const SdfAssetPath& assetPath, 
    const std::string &firstLayerName)
{
    const std::string assetPathStr = assetPath.GetAssetPath();

    _rootFilePath = ArGetResolver().Resolve(assetPathStr);
    if (_rootFilePath.empty()) {
        TF_WARN("Failed to resolve asset path: %s", assetPathStr.c_str());
        return false;
    }

    _rootLayer = SdfLayer::FindOrOpen(assetPathStr);
    if (!_rootLayer) {
        TF_WARN("Failed to find or open root asset layer: %s", 
            assetPathStr.c_str());
        return false;
    }

    // Change the first layer's name if requested.
    _firstLayerName = firstLayerName.empty() 
        ? TfGetBaseName(_rootLayer->GetRealPath()) 
        : firstLayerName;

    UsdUtils_LocalizationContext context(&_delegate);
    context.SetMetadataFilteringEnabled(true);
    context.SetDependenciesToSkip(_dependenciesToSkip);

    return context.Process(_rootLayer);
}

bool 
UsdUtils_AssetLocalizationPackage::Write(
    const std::string &packagePath)
{
    _packagePath = packagePath;
    // Set of all the packaged files.
    std::unordered_set<std::string> packagedFiles;
    bool success = true;

    packagedFiles.insert(_firstLayerName);
    _AddLayerToPackage(_rootLayer, _firstLayerName);

    for (const auto & layerDep : _layersToCopy) {

        if (!packagedFiles.insert(layerDep.second).second) {
            TF_WARN("A file already exists at path \"%s\" in the package. "
                "Skipping export of dependency @%s@.",  
                layerDep.second.c_str(), layerDep.first.c_str());
            continue;
        }

        const SdfLayerRefPtr &layerToAdd = SdfLayer::FindOrOpen(layerDep.first);
        if (!layerToAdd) {
            TF_WARN("Unable to open layer at path \"%s\" while writing package."
                " Skipping export of dependency @%s@.",  
                layerDep.first.c_str(), layerDep.second.c_str());
            continue;
        }

        success &= _AddLayerToPackage(layerToAdd, layerDep.second);
    }

    for (const auto & fileDep : _filesToCopy) {
        if (!packagedFiles.insert(fileDep.second).second) {
            TF_WARN("A file already exists at path \"%s\" in the package. "
                "Skipping export of dependency @%s@.", 
                fileDep.second.c_str(), fileDep.first.c_str());
            continue;
        }

        success &= _AddAssetToPackage(fileDep.first, fileDep.second);
    }

    return success;
}

UsdUtilsDependencyInfo
UsdUtils_AssetLocalizationPackage::_ProcessDependency( 
    const SdfLayerRefPtr &layer, 
    const UsdUtilsDependencyInfo &depInfo,
    UsdUtils_DependencyType dependencyType)
{
    if (_userProcessingFunc) {
        UsdUtilsDependencyInfo processedInfo = _userProcessingFunc(
            layer, depInfo);

        // if the user processing func returned empty string then this
        // asset should be removed from the source layer
        if (processedInfo.GetAssetPath().empty()) {
            return UsdUtilsDependencyInfo();
        }

        return _AddDependenciesToPackage(
            layer, processedInfo);
    }

    return _AddDependenciesToPackage(
        layer, depInfo);
}

UsdUtilsDependencyInfo 
UsdUtils_AssetLocalizationPackage::_AddDependenciesToPackage( 
    const SdfLayerRefPtr &layer, 
    const UsdUtilsDependencyInfo &depInfo)
{
    // If there are no dependencies then there is no need for remapping
    if (depInfo.GetAssetPath().empty()) {
        return depInfo;
    }

    bool isRelativeOutput;
    const std::string remappedPath = 
        _ProcessAssetPath(layer, depInfo.GetAssetPath(), &isRelativeOutput);
    
    std::string packagePath = remappedPath;
    if (isRelativeOutput) {
        // If it's a relative path, construct the full path relative to
        // the final (destination) location of the reference-containing 
        // file. This is only applicable if the path is a filesystem path
        const auto containingLayerInfo = 
            _layersToCopy.find(layer->GetIdentifier());

        if (containingLayerInfo != _layersToCopy.end()) {
            packagePath = TfNormPath(TfStringCatPaths(TfGetPathName(
                containingLayerInfo->second), depInfo.GetAssetPath()));
        }
    }

    // Add all dependencies to package
    const std::string destDirectory = TfGetPathName(packagePath);

    if (depInfo.GetDependencies().empty()) {
        _AddDependencyToPackage(layer, depInfo.GetAssetPath(), destDirectory);
    }
    else {
        for (const auto &dependency : depInfo.GetDependencies()) {
            _AddDependencyToPackage(layer, dependency, destDirectory);
        }
    }

    return {remappedPath, depInfo.GetDependencies()};
}

void
UsdUtils_AssetLocalizationPackage::_AddDependencyToPackage(
    const SdfLayerRefPtr &layer, 
    const std::string &dependency,
    const std::string &destDirectory)
{
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

static bool _PathIsURIResolvable(const std::string & path) {
    size_t uriEnd = path.find(':');
    if (uriEnd == std::string::npos) {
        return false;
    }

    std::string scheme = path.substr(0, uriEnd);
    const auto& registeredSchemes = ArGetRegisteredURISchemes();
    return std::binary_search(
        registeredSchemes.begin(), registeredSchemes.end(), scheme);
}

std::string 
UsdUtils_AssetLocalizationPackage::_ProcessAssetPath(
    const SdfLayerRefPtr &layer, 
    const std::string &refPath, 
    bool *isRelativePathOut)
{
    auto &resolver = ArGetResolver();

    const bool isContextDependentPath =
        resolver.IsContextDependentPath(refPath);

    // We want to maintain relative paths where possible to keep localized
    // assets as close as possible to their original layout. However, we
    // skip this for context-dependent paths because those must be resolved
    // to determine what asset is being referred to.
    //
    // Due to the open ended nature of URI based paths, there may not be a
    // straightforward way to map them to a filesystem directory structure so
    // we will always send them down the remap path.
    if (!isContextDependentPath && !_PathIsURIResolvable(refPath)) {
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
            // Note: if we are unable to resolve the anchored path we will not 
            // consider it outside the asset location. For example, we would
            // like to preserve relative clip template paths for matching.
            const ArResolvedPath resolvedRefPath = 
                resolver.Resolve(anchored);
            const bool refPathIsOutsideAssetLocation = 
                !resolvedRefPath.empty() && !TfStringStartsWith(
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

bool
UsdUtils_AssetLocalizationPackage::_AddLayerToPackage(
    SdfLayerRefPtr sourceLayer,
    const std::string &destPath)
{
    SdfLayerConstHandle layer = 
        _delegate.GetLayerUsedForWriting(sourceLayer);
    TF_DEBUG(USDUTILS_CREATE_PACKAGE).Msg(
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
            if (!_WriteToPackage(packagePath, destPackagePath)) {
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
        if (!_WriteToPackage(layer->GetRealPath(), destPath)) {
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

        if (!_WriteToPackage(tmpLayerExportPath, destPath)) {
            // XXX: Should we abort package creation and return early here?
            TF_WARN("Failed to add temporary layer at '%s' to the package "
                "at path '%s'.", tmpLayerExportPath.c_str(), 
                _packagePath.c_str());
            return false;
        } else {
            // XXX: This is here to work around an issue with exporting
            // anonymous layers that are backed by crate files.  Temporary
            // layers used for layer modifications need to be cleared to
            // prevent a mapped file descriptor from being held after
            // export to temporary file.
            _delegate.ClearLayerUsedForWriting(sourceLayer);
            TfDeleteFile(tmpLayerExportPath);
        }
    }

    return true;
}

bool
UsdUtils_AssetLocalizationPackage::_AddAssetToPackage(
    const std::string &srcPath,
    const std::string &destPath)
{
    TF_DEBUG(USDUTILS_CREATE_PACKAGE).Msg(
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
            if (!_WriteToPackage(packagePath, destPackagePath)) {
                return false;
            }
        }
    }
    else {
        if (!_WriteToPackage(srcPath, destPath)) {
            // XXX: Should we abort package creation and return early here?
            TF_WARN("Failed to add file '%s' to the package at path '%s'.",
                srcPath.c_str(), _packagePath.c_str());
            return false;
        }
    }

    return true;
}

std::string 
UsdUtils_AssetLocalizationPackage::_RemapAssetPath(
    const SdfLayerRefPtr &layer, 
    const std::string &refPath, 
    bool isRelativePath)
{
    if (isRelativePath) {
        return refPath;
    }

    return RemapPath(refPath);
}

PXR_NAMESPACE_CLOSE_SCOPE
