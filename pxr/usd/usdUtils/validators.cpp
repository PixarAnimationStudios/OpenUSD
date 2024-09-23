//
// Copyright 2024 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include <pxr/usd/sdf/fileFormat.h>
#include "pxr/usd/usd/validationError.h"
#include "pxr/usd/usd/validationRegistry.h"
#include "pxr/usd/usdUtils/validatorTokens.h"
#include "pxr/usd/ar/packageUtils.h"
#include "pxr/usd/usdUtils/dependencies.h"
#include "pxr/usd/usdUtils/userProcessingFunc.h"

PXR_NAMESPACE_OPEN_SCOPE

static
UsdValidationErrorVector
_PackageEncapsulationValidator(const UsdStagePtr& usdStage) {
    UsdValidationErrorVector errors;

    const SdfLayerRefPtr& rootLayer = usdStage->GetRootLayer();
    const bool isPackage = [](const SdfLayerHandle& layer) 
    {
        return layer->GetFileFormat()->IsPackage() || 
               ArIsPackageRelativePath(layer->GetIdentifier());
    }(rootLayer);

    if (!isPackage) {
        return errors;
    }

    SdfLayerRefPtrVector layers;
    std::vector<std::basic_string<char>> assets, unresolvedPaths;
    const SdfAssetPath& path = SdfAssetPath(rootLayer->GetIdentifier());

    UsdUtilsComputeAllDependencies(path, &layers, &assets, &unresolvedPaths, 
                                   nullptr);

    const std::string& realPath = rootLayer->GetRealPath();
    const std::string& packagePath = ArIsPackageRelativePath(
        rootLayer->GetIdentifier()) ? 
        ArSplitPackageRelativePathOuter(realPath).first : realPath;

    if (!packagePath.empty()) {
        for (const SdfLayerRefPtr& referencedLayer : layers) {
            const std::string& realPath = referencedLayer->GetRealPath();

            // We don't want to validate in-memory or session layers
            // since these layers will not have a real path, we skip here
            if (realPath.empty()) {
                continue;
            }

            if (!TfStringStartsWith(realPath, packagePath)) {
                errors.emplace_back(
                    UsdUtilsValidationErrorNameTokens->layerNotInPackage,
                    UsdValidationErrorType::Warn,
                    UsdValidationErrorSites {
                        UsdValidationErrorSite(
                            rootLayer, referencedLayer->GetDefaultPrimAsPath())
                    },
                    TfStringPrintf(
                        ("Found referenced layer '%s' that does not belong to "
                         "the package '%s'."), 
                        referencedLayer->GetIdentifier().c_str(), 
                        packagePath.c_str())
                );
            }
        }

        for (const std::string& asset : assets) {
            if (!TfStringStartsWith(asset, packagePath)) {
                errors.emplace_back(
                    UsdUtilsValidationErrorNameTokens->assetNotInPackage,
                    UsdValidationErrorType::Warn,
                    UsdValidationErrorSites {
                        UsdValidationErrorSite(
                            rootLayer, SdfPath(asset))
                    },
                    TfStringPrintf(
                        ("Found asset reference '%s' that does not belong to "
                         "the package '%s'."), asset.c_str(), 
                        packagePath.c_str())
                );
            }
        }
    }

    return errors;
}

TF_REGISTRY_FUNCTION(UsdValidationRegistry)
{
    UsdValidationRegistry& registry = UsdValidationRegistry::GetInstance();
    registry.RegisterPluginValidator(
            UsdUtilsValidatorNameTokens->packageEncapsulationValidator, _PackageEncapsulationValidator);
}

PXR_NAMESPACE_CLOSE_SCOPE

