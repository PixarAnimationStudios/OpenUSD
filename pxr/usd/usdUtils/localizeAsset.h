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
#include "pxr/usd/sdf/layer.h"
#include "pxr/usd/usdUtils/api.h"
#include "pxr/usd/usdUtils/userProcessingFunc.h"

#include <string>

#ifndef PXR_USD_USD_UTILS_LOCALIZE_ASSET
#define PXR_USD_USD_UTILS_LOCALIZE_ASSET

/// \file usdUtils/localizeAsset.h

PXR_NAMESPACE_OPEN_SCOPE

class SdfAssetPath;

/// Creates a localized version of the asset identified by \p assetPath and all
/// of its external dependencies in the directory specified by
/// \p localizationDirectory. Any anonymous layers that are encountered
/// during dependency discovery will be serialized into the resulting package.
/// Due to localization, the packaged layers might be modified to have different
/// asset paths.
///
/// The \p editLayersInPlace parameter controls the strategy used for managing 
/// changes to layers (including the root layer and all transitive layer 
/// dependencies) that occur during the package creation process.  When 
/// \p editLayersInPlace is false, a temporary, anonymous copy of each 
/// modified layer is created and written into the package. This has the 
/// advantage of leaving source layers untouched at the expense of creating a 
/// copy of each modified layer in memory for the duration of this function.
///
/// When \p editLayersInPlace is set to true, layers are modified in-place and 
/// not reverted or persisted once the package has been created. In this case, 
/// there is no overhead of creating copies of each modified layer.  If you have
/// UsdStages open during the function call that reference the layers being 
/// modified, you may receive warnings or composition errors.  While these 
/// errors will not affect the resulting package adversely, it is strongly 
/// recommended that this function is run in isolation after any source 
/// UsdStages have been closed.
///
/// If a function is provided for the \p processingFunc parameter, it will be
/// invoked on every asset path that is discovered during localization. This
/// allows you to inject your own logic into the process. Refer to
/// \ref UsdUtilsDependencyInfo for additional information.
/// 
/// Returns true if the package was created successfully.
/// 
/// \note Clients of this function must take care of configuring the asset 
/// resolver context before invoking the function. To create a default 
/// resolver context, use \ref CreateDefaultContextForAsset() with the 
/// asset path.
/// 
/// \note If the given asset has a dependency on a directory (i.e. an external 
/// reference to a directory path), the dependency is ignored and the contents 
/// of the directory are not included in the created package. 
USDUTILS_API
bool
UsdUtilsLocalizeAsset(
    const SdfAssetPath& assetPath,
    const std::string& localizationDirectory,
    bool editLayersInPlace = false,
    std::function<UsdUtilsProcessingFunc> processingFunc = 
        std::function<UsdUtilsProcessingFunc>());

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_USD_USD_UTILS_LOCALIZE_ASSET