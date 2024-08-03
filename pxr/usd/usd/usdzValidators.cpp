//
// Copyright 2024 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include <pxr/usd/sdf/fileFormat.h>
#include "pxr/usd/usd/validationError.h"
#include "pxr/usd/usd/validationRegistry.h"
#include "pxr/usd/usd/validatorTokens.h"
#include "pxr/usd/ar/packageUtils.h"
#include "pxr/usd/usdUtils/dependencies.h"
#include "pxr/usd/usdUtils/userProcessingFunc.h"

PXR_NAMESPACE_OPEN_SCOPE

static
UsdValidationErrorVector
_PackageEncapsulationValidator(const SdfLayerHandle& layer) {
    UsdValidationErrorVector errors;

    const bool isPackage = [](const SdfLayerHandle& layer){
        return layer->GetFileFormat()->IsPackage() || ArIsPackageRelativePath(layer->GetIdentifier());
    }(layer);

    if (!isPackage){
        return errors;
    }

    std::vector<TfRefPtr<SdfLayer>> layers;
    std::vector<std::basic_string<char>> assets, unresolvedPaths;
    const SdfAssetPath &path = SdfAssetPath(layer->GetIdentifier());



    std::function<pxrInternal_v0_24__pxrReserved__::UsdUtilsDependencyInfo(
            const pxrInternal_v0_24__pxrReserved__::SdfLayerHandle &,
            const pxrInternal_v0_24__pxrReserved__::UsdUtilsDependencyInfo &)> processingFunc = [](const pxr::TfWeakPtr<pxr::SdfLayer>& layer, const pxr::UsdUtilsDependencyInfo& info) {
        // Define your processing function here
        return pxr::UsdUtilsDependencyInfo();
    };

    UsdUtilsComputeAllDependencies(path, &layers, &assets,
                                   &unresolvedPaths, processingFunc);

    auto realPath = layer->GetRealPath();
    const std::string packagePath = ArIsPackageRelativePath(layer->GetIdentifier()) ?
          ArSplitPackageRelativePathOuter(realPath).first :
          realPath;

    if (packagePath.length() > 0){
        for(const SdfLayerRefPtr& subLayer : layers){
            const auto realPath = subLayer->GetRealPath();
            if (!TfStringStartsWith(realPath, packagePath)){
                errors.emplace_back(
                        UsdValidationErrorType::Error,
                        UsdValidationErrorSites{
                                UsdValidationErrorSite(layer,
                                                       subLayer->GetDefaultPrimAsPath())
                        },
                        TfStringPrintf(("Found loaded layer '%s' that "
                                        "does not belong to the package '%s'."),
                                       subLayer->GetIdentifier().c_str(), packagePath.c_str())
               );
            }
        }

        for(const std::string& asset : assets){
            if (!TfStringStartsWith(asset, packagePath)){
                errors.emplace_back(
                        UsdValidationErrorType::Error,
                        UsdValidationErrorSites{
                                UsdValidationErrorSite(layer,
                                                       SdfPath(asset))
                        },
                        TfStringPrintf(("Found asset reference '%s' that "
                                        "does not belong to the package '%s'."),
                                       asset.c_str(), packagePath.c_str())
                );
            }
        }
    }

    return errors;
}

TF_REGISTRY_FUNCTION(UsdValidationRegistry)
{
    UsdValidationRegistry &registry = UsdValidationRegistry::GetInstance();
    registry.RegisterPluginValidator(
            UsdValidatorNameTokens->usdzPackageEncapsulationValidator, _PackageEncapsulationValidator);
}

PXR_NAMESPACE_CLOSE_SCOPE

