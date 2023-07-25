//
// Copyright 2016 Pixar
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
/// \file usdUtils/dependencies.cpp
#include "pxr/pxr.h"
#include "pxr/usd/usdUtils/assetLocalization.h"
#include "pxr/usd/usdUtils/dependencies.h"
#include "pxr/usd/usdUtils/debugCodes.h"
#include "pxr/usd/sdf/assetPath.h"

#include "pxr/base/trace/trace.h"

#include <stack>

PXR_NAMESPACE_OPEN_SCOPE


// XXX: don't even know if it's important to distinguish where
// these asset paths are coming from..  if it's not important, maybe this
// should just go into Sdf's _GatherPrimAssetReferences?  if it is important,
// we could also have another function that takes 3 vectors.
void
UsdUtilsExtractExternalReferences(
    const std::string& filePath,
    std::vector<std::string>* subLayers,
    std::vector<std::string>* references,
    std::vector<std::string>* payloads)
{
    TRACE_FUNCTION();
    UsdUtils_ExtractExternalReferences(filePath, 
        UsdUtils_FileAnalyzer::ReferenceType::All, 
        subLayers, references, payloads);
}



bool
UsdUtilsComputeAllDependencies(const SdfAssetPath &assetPath,
                               std::vector<SdfLayerRefPtr> *layers,
                               std::vector<std::string> *assets,
                               std::vector<std::string> *unresolvedPaths)
{
    // We are not interested in localizing here, hence pass in the empty string
    // for destination directory.
    UsdUtils_AssetLocalizer localizer(assetPath, 
                              /* destDir */ std::string(), 
                              /* enableMetadataFiltering */ true);

    // Clear the vectors before we start.
    layers->clear();
    assets->clear();
    
    // Reserve space in the vectors.
    layers->reserve(localizer.GetLayerExportMap().size());
    assets->reserve(localizer.GetFileCopyMap().size());

    for (auto &layerAndDestPath : localizer.GetLayerExportMap()) {
        layers->push_back(layerAndDestPath.first);
    }

    for (auto &srcAndDestPath : localizer.GetFileCopyMap()) {
        assets->push_back(srcAndDestPath.first);
    }

    *unresolvedPaths = localizer.GetUnresolvedAssetPaths();

    // Return true if one or more layers or assets were added  to the results.
    return !layers->empty() || !assets->empty();
}

void 
UsdUtilsModifyAssetPaths(
        const SdfLayerHandle& layer,
        const UsdUtilsModifyAssetPathFn& modifyFn)
{
    UsdUtils_FileAnalyzer(layer,
        UsdUtils_FileAnalyzer::ReferenceType::All,
        /* enableMetadataFiltering*/ false, 
        [&modifyFn](const std::string& assetPath, 
                    const SdfLayerRefPtr& layer) { 
            return modifyFn(assetPath);
        }
    );
}

PXR_NAMESPACE_CLOSE_SCOPE
