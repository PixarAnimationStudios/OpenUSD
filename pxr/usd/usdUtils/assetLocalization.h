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
#ifndef PXR_USD_USD_UTILS_ASSET_LOCALIZATION_H
#define PXR_USD_USD_UTILS_ASSET_LOCALIZATION_H

/// \file usdUtils/assetLocalization.h


#include "pxr/pxr.h"
#include "pxr/usd/sdf/layer.h"
#include "pxr/usd/usdUtils/api.h"

#include <string>
#include <unordered_map>
#include <vector>

PXR_NAMESPACE_OPEN_SCOPE

class UsdUtils_FileAnalyzer {
public:
    // Enum class representing the type of dependency.
    enum class DependencyType {
        Reference,
        Sublayer,
        Payload
    };

    // Enum class representing the external reference types that must be included 
    // in the search for external dependencies.
    enum class ReferenceType {
        // Include only references that affect composition.
        CompositionOnly, 

        // Include all external references including asset-valued attributes
        // and non-composition metadata containing SdfAssetPath values.
        All              
    };
    
    // The asset remapping function's signature. 
    // It takes a given asset path and the layer it was found in and returns
    // the corresponding remapped path.
    // 
    // The layer is used to resolve the asset path in cases where the given 
    // asset path is a search path or a relative path. 
    using RemapAssetPathFunc = std::function<std::string 
            (const std::string &assetPath, 
             const SdfLayerRefPtr &layer)>;

    // Takes the asset path and the type of dependency it is and does some 
    // arbitrary processing (like enumerating dependencies).
    using ProcessAssetPathFunc = std::function<void 
            (const std::string &assetPath, 
             const SdfLayerRefPtr &layer,
             const DependencyType &DependencyType)>;

    // Attempts to open the file at \p referencePath and analyzes its external 
    // dependencies.  Opening the layer using this non-resolved path ensures
    // that layer metadata is correctly set.  If the file cannot be opened then
    // the analyzer simply retains the \p resolvedPath for later use.
    // 
    // For each dependency that is detected, the provided (optional) callback 
    // functions are invoked. 
    // 
    // \p processPathFunc is invoked first with the raw (un-remapped) path. 
    // Then \p remapPathFunc is invoked. 
    UsdUtils_FileAnalyzer(const std::string &referencePath,
                  const std::string &resolvedPath,
                  UsdUtils_FileAnalyzer::ReferenceType refTypesToInclude=
                        UsdUtils_FileAnalyzer::ReferenceType::All,
                  bool enableMetadataFiltering = false,
                  const RemapAssetPathFunc &remapPathFunc={},
                  const ProcessAssetPathFunc &processPathFunc={});

    // overload version of the above constructor that takes a \c layer instead
    // of a filePath.
    UsdUtils_FileAnalyzer(const SdfLayerHandle& layer,
                  UsdUtils_FileAnalyzer::ReferenceType refTypesToInclude=
                        UsdUtils_FileAnalyzer::ReferenceType::All,
                  bool enableMetadataFiltering = false,
                  const RemapAssetPathFunc &remapPathFunc={},
                  const ProcessAssetPathFunc &processPathFunc={});

    // Returns the path to the file on disk that is being analyzed.
    inline const std::string &GetFilePath() const {
        return _filePath;
    }

    // Returns the SdfLayer associated with the file being analyzed.
    inline const SdfLayerRefPtr &GetLayer() const {
        return _layer;
    }

private:
    // This function returns a boolean value indicating whether an asset path
    // should be processed.  Currently this is used when processing metadata
    // to filter out specific keys
    using ShouldProcessAssetValueFunc = std::function<bool
            (const std::string &key)>;

    // Open the layer, updates references to point to relative or search paths
    // and accumulates all references.
    void _AnalyzeDependencies();

    // This adds the given raw reference path to the list of all referenced 
    // paths. It also returns the remapped reference path, so client code
    // can update the source reference to point to the remapped path.
    std::string _ProcessDependency(const std::string &rawRefPath,
                                   const DependencyType &DependencyType);

    // Invokes the path remapping function if one has been supplied, otherwise
    // returns the passed in raw reference path
    std::string _RemapDependency(const std::string &rawRefPath);

    // Processes any sublayers in the SdfLayer associated with the file.
    void _ProcessSublayers();

    // Processes all payloads on the given primSpec.
    void _ProcessPayloads(const SdfPrimSpecHandle &primSpec);

    // Processes prim metadata.
    void _ProcessMetadata(const SdfPrimSpecHandle &primSpec);

    // Processes metadata on properties.
    void _ProcessProperties(const SdfPrimSpecHandle &primSpec);

    // Processes all references on the given primSpec.
    void _ProcessReferences(const SdfPrimSpecHandle &primSpec);

    // Returns the given VtValue with any asset paths remapped to point to 
    // destination-relative path.
    VtValue _UpdateAssetValue(const VtValue &val);

    // Returns the given VtValue with any asset paths remapped to point to 
    // destination-relative path.
    // This overload supports filtering asset path processing based on their
    // key. The processing callback is not invoked for filtered keys, however
    // the remapping callback is invoked in all cases.
    VtValue _UpdateAssetValue(const std::string &key, 
                              const VtValue &val, 
                              const ShouldProcessAssetValueFunc shouldProcessFunc);

    // Callback function that's passed into SdfPayloadsProxy::ModifyItemEdits()
    // or SdfReferencesProxy::ModifyItemEdits() to update all payloads or 
    // references.
    template <class RefOrPayloadType, DependencyType DEP_TYPE>
    boost::optional<RefOrPayloadType> _RemapRefOrPayload(
        const RefOrPayloadType &refOrPayload);

    // Resolved path to the file.
    std::string _filePath;

    // SdfLayer corresponding to the file. This will be null for non-layer 
    // files.
    SdfLayerRefPtr _layer;

    // The types of references to include in the processing. 
    // If set to UsdUtils_FileAnalyzer::ReferenceType::CompositionOnly, 
    // non-composition related asset references (eg. property values, property
    // metadata and non-composition prim metadata) are ignored.
    UsdUtils_FileAnalyzer::ReferenceType _refTypesToInclude;

    // if true, will filter ignore path processing for specified metadata keys.
    bool _metadataFilteringEnabled;

    // Remap and process path callback functions.
    RemapAssetPathFunc _remapPathFunc;
    ProcessAssetPathFunc _processPathFunc;
};

class UsdUtils_AssetLocalizer {
public:
    using LayerAndDestPath = std::pair<SdfLayerRefPtr, std::string>;
    using SrcPathAndDestPath = std::pair<std::string, std::string>;
    using DestFilePathAndAnalyzer = std::pair<std::string, UsdUtils_FileAnalyzer>;
    using LayerDependenciesMap = std::unordered_map<SdfLayerRefPtr, 
            std::vector<std::string>, TfHash>;

    // Computes the given asset's dependencies recursively and determines
    // the information needed to localize the asset.
    // If \p destDir is empty, none of the asset layers are modified, allowing
    // this class to be used purely as a recursive dependency finder.
    // \p enableMetadataFiltering if true, will instruct FileAnalyzer to skip
    // processing asset paths that match a list of predefined names
    // \p firstLayerName if non-empty, holds desired the name of the root layer 
    // in the localized asset. 
    // 
    // If \p origRootFilePath is non-empty, it points to the original root layer 
    // of which \p assetPath is a flattened representation. This is by 
    // UsdUtilsCreateNewARKitUsdzPackage(), to point to the original 
    // (unflattened) asset with external dependencies.
    // 
    // \p dependenciesToSkip lists an optional set of dependencies that must be 
    // skipped in the created package. This list must contain fully resolved 
    // asset paths that must be skipped in the created package. It cannot 
    // contain the resolved root \p assetPath value itself. If a dependency
    // is skipped because it exists in the \p dependenciesToSkip list, none of 
    // the transitive dependencies referenced by the skipped dependency are 
    // processed and may be missing in the created package.
    UsdUtils_AssetLocalizer(const SdfAssetPath &assetPath,
                    const std::string &destDir,
                    bool enableMetadataFiltering,
                    const std::string &firstLayerName=std::string(),
                    const std::string &origRootFilePath=std::string(),
                    const std::vector<std::string> 
                        &dependenciesToSkip=std::vector<std::string>());

    // Get the list of layers to be localized along with their corresponding 
    // destination paths.
    inline const std::vector<LayerAndDestPath> &GetLayerExportMap() const {
        return _layerExportMap;
    }

    // Get the list of source files to be copied along with their corresponding 
    // destination paths.
    inline const std::vector<SrcPathAndDestPath> &GetFileCopyMap() const {
        return _fileCopyMap;
    }

    // Returns ths list of all unresolved references.
    inline const std::vector<std::string> GetUnresolvedAssetPaths() const {
        return _unresolvedAssetPaths;
    }

private:
    // This function will ensure that all tiles that match the UDIM identifier
    // contained in src path are correctly added to the file copy map
    void _ResolveUdimPaths(
        const std::string &srcFilePath,
        const std::string &destFilePath);

    // This will contain a mapping of SdfLayerRefPtr's mapped to their 
    // desination path inside the destination directory.
    std::vector<LayerAndDestPath> _layerExportMap;

    // This will contain a mapping of source file path to the corresponding 
    // desination file path.
    std::vector<SrcPathAndDestPath> _fileCopyMap;

    // A map of layers and their corresponding vector of raw external reference
    // paths.
    LayerDependenciesMap _layerDependenciesMap;

    // List of all the unresolvable asset paths.
    std::vector<std::string> _unresolvedAssetPaths;

    // Helper object for remapping paths to an artifically-generated path.
    class _DirectoryRemapper;

    // Remaps a given asset path (\p refPath) to be relative to the layer 
    // containing it (\p layer) for the purpose of localization.
    // \p dirRemapper should not be empty.
    // \p pathType is allowed to be a nullptr.
    // \p origRootFilePath should contain to the path to the original 
    // root file from which the asset at \p rootFilePath was created (possibly 
    // by flattening),
    // \p rootFilePath should contain a path to the resolved and localized root 
    // asset layer on disk.
    // \p firstLayerName if non-empty contains the final name of the asset's 
    // root layer.
    static std::string _RemapAssetPath(
        const std::string &refPath, 
        const SdfLayerRefPtr &layer,
        const std::string origRootFilePath, 
        const std::string rootFilePath, 
        const std::string &firstLayerName,
        _DirectoryRemapper *dirRemapper,
        bool *isRelativePath);
};

void UsdUtils_ExtractExternalReferences(
    const std::string& filePath,
    const UsdUtils_FileAnalyzer::ReferenceType &refTypesToInclude,
    std::vector<std::string>* subLayers,
    std::vector<std::string>* references,
    std::vector<std::string>* payloads);

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_USD_USD_UTILS_ASSET_LOCALIZATION_H
