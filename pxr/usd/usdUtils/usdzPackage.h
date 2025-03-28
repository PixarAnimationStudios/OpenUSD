//
// Copyright 2023 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_USD_USD_UTILS_USDZ_PACKAGE_H
#define PXR_USD_USD_UTILS_USDZ_PACKAGE_H

/// \file usdUtils/usdzPackage.h

#include "pxr/pxr.h"
#include "pxr/usd/sdf/layer.h"
#include "pxr/usd/usdUtils/api.h"

#include <string>
#include <vector>

PXR_NAMESPACE_OPEN_SCOPE

class SdfAssetPath;

/// Creates a USDZ package containing the specified asset, identified by its 
/// \p assetPath. The created package will include a localized version of the 
/// asset itself and all of its external dependencies. Any anonymous layers that
/// are encountered during dependency discovery will be serialized into the
/// resulting package. Due to localization, the packaged layers might be 
/// modified to have different asset paths.
///
/// You can optionally specify a different package-internal name for the first
/// layer of the asset by specifying \p firstLayerName. By default,
/// \p firstLayerName is empty, meaning that the original name is preserved.
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
/// 
/// \sa UsdUtilsCreateNewARKitUsdzPackage()
USDUTILS_API
bool
UsdUtilsCreateNewUsdzPackage(
    const SdfAssetPath& assetPath,
    const std::string& usdzFilePath,
    const std::string& firstLayerName=std::string(),
    bool editLayersInPlace = false);

/// Similar to UsdUtilsCreateNewUsdzPackage, this function packages all of the 
/// dependencies of the given asset. Assets targeted at the initial usdz 
/// implementation in ARKit operate under greater constraints than usdz files 
/// for more general 'in house' uses, and this option attempts to ensure that
/// these constraints are honored; this may involve more transformations to the 
/// data, which may cause loss of features such as VariantSets. Any anonymous 
/// layers that are encountered during dependency discovery will be serialized 
/// into the resulting package.
///
/// If \p firstLayerName is specified, it is modified to have the ".usdc" 
/// extension, as required by the initial usdz implementation in ARKit.
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
/// 
/// \sa UsdUtilsCreateNewUsdzPackage()
USDUTILS_API
bool
UsdUtilsCreateNewARKitUsdzPackage(
    const SdfAssetPath &assetPath,
    const std::string &usdzFilePath,
    const std::string &firstLayerName=std::string(),
    bool editLayersInplace = false);

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_USD_USD_UTILS_USDZ_PACKAGE_H
