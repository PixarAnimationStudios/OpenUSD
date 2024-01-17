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
#ifndef PXR_USD_USD_UTILS_ASSET_LOCALIZATION_PACKAGE
#define PXR_USD_USD_UTILS_ASSET_LOCALIZATION_PACKAGE

#include "pxr/pxr.h"
#include "pxr/usd/sdf/layer.h"
#include "pxr/usd/usdUtils/api.h"

#include "pxr/usd/usdUtils/assetLocalizationDelegate.h"

#include <string>
#include <vector>

/// \file usdUtils/assetLocalizationPackage.h

PXR_NAMESPACE_OPEN_SCOPE

class UsdUtils_DirectoryRemapper {
public:
    UsdUtils_DirectoryRemapper() : _nextDirectoryNum(0) { }

    // Remap the given file path by replacing the directory with a
    // unique, artifically generated name. The generated directory name
    // will be reused if the original directory is seen again on a
    // subsequent call to Remap.
    std::string Remap(const std::string& filePath);

private:
    size_t _nextDirectoryNum;
    std::unordered_map<std::string, std::string> _oldToNewDirectory;
};

class UsdUtils_AssetLocalizationPackage
{
public:
    UsdUtils_AssetLocalizationPackage()
    : _delegate(std::bind(
            &UsdUtils_AssetLocalizationPackage::_ProcessDependency, this, 
            std::placeholders::_1, std::placeholders::_2, 
            std::placeholders::_3, std::placeholders::_4))
    {}

    // Sets the original file path for this asset.
    // The path specified should be resolved by AR.
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

    // Controls whether layers are edited in place
    // Refer to UsdUtils_WritableLocalizationDelegate::SetEditLayersInPlace
    inline void SetEditLayersInPlace(bool editLayersInPlace) {
        _delegate.SetEditLayersInPlace(editLayersInPlace);
    }

    virtual bool Build(
        const SdfAssetPath& assetPath, 
        const std::string &firstLayerName);

    virtual bool Write(const std::string &packagePath);

    // Remap the path to an artifically-constructed one so that the source 
    // directory structure isn't embedded in the final package.
    // Otherwise, sensitive information (e.g. usernames, movie titles...) in
    // directory names may be inadvertently leaked in the package.
    inline std::string RemapPath(const std::string &path) {
        return _directoryRemapper.Remap(path);
    }

protected:
    virtual 
    std::string  _RemapAssetPath(
        const SdfLayerRefPtr &layer, 
        const std::string &assetPath,
        bool isRelativePath);
    
    virtual
    bool _WriteToPackage(
        const std::string& source,
        const std::string& dest) = 0;

private:
    std::string _ProcessAssetPath(
        const SdfLayerRefPtr &layer, 
        const std::string &refPath, 
        bool *isRelativePathOut);

    bool _AddLayerToPackage(
        SdfLayerRefPtr sourceLayer, 
        const std::string &destPath);

    bool _AddAssetToPackage(
        const std::string &srcPath,
        const std::string &destPath);

    std::string _ProcessDependency( 
        const SdfLayerRefPtr &layer, 
        const std::string &assetPath,
        const std::vector<std::string> &dependencies,
        UsdUtilsDependencyType dependencyType);

    std::string 
    _AddDependenciesToPackage( 
        const SdfLayerRefPtr &layer, 
        const std::string &assetPath,
        const std::vector<std::string> &dependencies);

    void
    _AddDependencyToPackage(
        const SdfLayerRefPtr &layer, 
        const std::string dependency,
        const std::string destDirectory);

    SdfLayerRefPtr _rootLayer;
    
    // The resolved path of the root usd layer
    std::string _rootFilePath;

    // The original root file path...used for ARkit packages
    std::string _origRootFilePath;

    UsdUtils_WritableLocalizationDelegate _delegate;

    std::string _packagePath;

    // User supplied first layer override name
    std::string _firstLayerName;

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

    UsdUtils_DirectoryRemapper _directoryRemapper;
};


PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_USD_USD_UTILS_ASSET_LOCALIZATION_PACKAGE
