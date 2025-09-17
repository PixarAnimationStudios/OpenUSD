//
// Copyright 2023 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
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
/// \ref UsdUtilsDependencyInfo for general information on user processing
/// functions.  If an asset path is ignored in the processing function, it will
/// be removed from the layer and excluded from the localized package.  Paths
/// that are modified will have their updated value written back into the
/// localized layer. Paths that are added to the dependencies array during
/// processing will be included in the resulting localized asset.
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