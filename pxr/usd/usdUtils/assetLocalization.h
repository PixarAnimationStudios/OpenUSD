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
#include "pxr/usd/usdUtils/dependencies.h"
#include "pxr/usd/usdUtils/assetLocalizationDelegate.h"

#include <functional>
#include <memory>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

PXR_NAMESPACE_OPEN_SCOPE



// Recursively computes the given asset's dependencies.
// This class is responsible for handling layer traversal and asset path
// discovery only.  As asset paths are discovered, they are handed off to the
// context's delegate where all specific processing logic lives.
class UsdUtils_LocalizationContext {
public:
    // Enum class representing the external reference types that must be 
    // included  in the search for external dependencies.
    enum class ReferenceType {
        // Include only references that affect composition.
        CompositionOnly, 

        // Include all external references including asset-valued attributes
        // and non-composition metadata containing SdfAssetPath values.
        All              
    };

public:
    UsdUtils_LocalizationContext(UsdUtils_LocalizationDelegate* delegate)
        :_delegate(delegate)
        {}

    // Begins recursive dependency analysis on the supplied layer
    bool Process(const SdfLayerRefPtr& layer);

    // Returns the root layer of the asset
    SdfLayerRefPtr GetRootLayer() const { 
        return _rootLayer; 
    } 

    // Toggles metadata filtering.  When active, non-relevant metadata keys will
    // be ignored. Refer to _ShouldFilterAssetPath implementation.
    inline void SetMetadataFilteringEnabled(bool filteringEnabled) 
    {
        _metadataFilteringEnabled = filteringEnabled;
    }

    // Sets whether all layer dependencies should be recursively traversed.
    // When this is false, only direct asset dependencies of the root
    // asset layer will be processed.
    inline void SetRecurseLayerDependencies(bool recurseLayerDependencies)
    {
        _recurseLayerDependencies = recurseLayerDependencies;
    }

    // Sets the reference types that will be included for processing.
    // \sa ReferenceType
    inline void SetRefTypesToInclude(ReferenceType refTypesToInclude )
    {
        _refTypesToInclude = refTypesToInclude;
    }

    /// Sets a list of dependencies to skip during packaging.
    /// The paths contained in this array should be fully resolved.
    inline void SetDependenciesToSkip(
        const std::vector<std::string> &dependenciesToSkip) 
    {
        _dependenciesToSkip = 
            std::unordered_set<std::string>(dependenciesToSkip.begin(), 
                                            dependenciesToSkip.end());
    }

private:
    void _ProcessLayer(const SdfLayerRefPtr& layer);
    void _ProcessSublayers(const SdfLayerRefPtr&  layer);
    void _ProcessMetadata(const SdfLayerRefPtr&  layer,
                          const SdfPrimSpecHandle &primSpec);
    void _ProcessPayloads(const SdfLayerRefPtr&  layer,
                          const SdfPrimSpecHandle &primSpec);
    void _ProcessProperties(const SdfLayerRefPtr&  layer,
                            const SdfPrimSpecHandle &primSpec);
    void _ProcessReferences(const SdfLayerRefPtr&  layer,
                            const SdfPrimSpecHandle &primSpec);
    bool _ShouldFilterAssetPath(const std::string &key,
                                bool processingMetadata);

    void _ProcessAssetValue(const SdfLayerRefPtr&  layer, 
                               const std::string &key,
                               const VtValue &val,
                               bool processingMetadata = false);
    void _ProcessAssetValue(const SdfLayerRefPtr&  layer, 
                             const VtValue &val,
                             bool processingMetadata = false);

    // Searches for udim tiles associated with the given asset path.
    static std::vector<std::string> _GetUdimTiles(const SdfLayerRefPtr& layer,
                                           const std::string &assetPath);

    // Discovers all dependencies for the supplied asset path
    static std::vector<std::string> _GetDependencies(const SdfLayerRefPtr& layer,
                                           const std::string &assetPath);
    
    // Searches for the clips of a given templated string
    // XXX: this method is currently only implemented for filesystem paths and
    // uses globbing to find the clips.
    static std::vector<std::string> _GetTemplatedClips(const SdfLayerRefPtr& layer,
                                                const std::string &assetPath);

    // Enqueues a dependency into the LIFO processing queue
    void _EnqueueDependency(const SdfLayerRefPtr layer, 
                           const std::string &assetPath);

    // Determines if a value needs to be processed by the delegate. Dictionaries
    // are always considered because they main contain asset path values.
    static bool _ValueTypeIsRelevant(const VtValue &val);

private:
    UsdUtils_LocalizationDelegate* _delegate;

    SdfLayerRefPtr _rootLayer;

    std::vector<std::string> _pathsToProcess;

    // holds a list of assets that needs to be recursively processed
    // this queue is used in order to preserve the processing order of
    // the previous localization code.
    std::vector<std::string> _queue;

    // holds the list of paths that have already been processed
    std::unordered_set<std::string> _encounteredPaths;

    ReferenceType _refTypesToInclude = ReferenceType::All;

    // Specifies if layer dependencies should be followed when processing the
    // root asset.
    bool _recurseLayerDependencies = true;

    // Specifies if metadata filtering should be enabled
    bool _metadataFilteringEnabled = false;

    // user supplied list of dependencies that will be skipped when 
    // processing the asset
    std::unordered_set<std::string> _dependenciesToSkip;
};

void UsdUtils_ExtractExternalReferences(
    const std::string& filePath,
    const UsdUtils_LocalizationContext::ReferenceType refTypesToInclude,
    std::vector<std::string>* subLayers,
    std::vector<std::string>* references,
    std::vector<std::string>* payloads);

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_USD_USD_UTILS_ASSET_LOCALIZATION_H
