//
// Copyright 2024 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/usd/ar/packageUtils.h"
#include "pxr/usd/usdUtils/dependencies.h"
#include "pxr/usd/usdUtils/userProcessingFunc.h"
#include "pxr/usdValidation/usdUtilsValidators/validatorTokens.h"
#include "pxr/usdValidation/usdValidation/error.h"
#include "pxr/usdValidation/usdValidation/registry.h"

#include <pxr/usd/sdf/fileFormat.h>

PXR_NAMESPACE_OPEN_SCOPE

static UsdValidationErrorVector
_PackageEncapsulationValidator(const UsdStagePtr &usdStage)
{
    UsdValidationErrorVector errors;

    const SdfLayerRefPtr &rootLayer = usdStage->GetRootLayer();
    const bool isPackage = [](const SdfLayerHandle &layer) {
        return layer->GetFileFormat()->IsPackage()
            || ArIsPackageRelativePath(layer->GetIdentifier());
    }(rootLayer);

    if (!isPackage) {
        return errors;
    }

    SdfLayerRefPtrVector layers;
    std::vector<std::basic_string<char>> assets, unresolvedPaths;
    const SdfAssetPath &path = SdfAssetPath(rootLayer->GetIdentifier());

    UsdUtilsComputeAllDependencies(path, &layers, &assets, &unresolvedPaths,
                                   nullptr);

    const std::string &realPath = rootLayer->GetRealPath();
    const std::string &packagePath
        = ArIsPackageRelativePath(rootLayer->GetIdentifier())
        ? ArSplitPackageRelativePathOuter(realPath).first
        : realPath;

    if (!packagePath.empty()) {
        for (const SdfLayerRefPtr &referencedLayer : layers) {
            if (!referencedLayer) {
                errors.emplace_back(
                    UsdUtilsValidationErrorNameTokens->invalidLayerInPackage,
                    UsdValidationErrorType::Error,
                    UsdValidationErrorSites {
                        UsdValidationErrorSite(rootLayer, SdfPath()) },
                    "Found invalid layer reference in package. This could be "
                    "due to a layer that failed to load or a layer that is not "
                    "a valid layer to be bundled in a package.");
                continue;
            }
            const std::string &realPath = referencedLayer->GetRealPath();

            // We don't want to validate in-memory or session layers
            // since these layers will not have a real path, we skip here
            if (realPath.empty()) {
                continue;
            }

            if (!TfStringStartsWith(realPath, packagePath)) {
                errors.emplace_back(
                    UsdUtilsValidationErrorNameTokens->layerNotInPackage,
                    UsdValidationErrorType::Warn,
                    UsdValidationErrorSites { UsdValidationErrorSite(
                        rootLayer, referencedLayer->GetDefaultPrimAsPath()) },
                    TfStringPrintf(
                        ("Found referenced layer '%s' that does not belong to "
                         "the package '%s'."),
                        referencedLayer->GetIdentifier().c_str(),
                        packagePath.c_str()));
            }
        }

        for (const std::string &asset : assets) {
            if (!TfStringStartsWith(asset, packagePath)) {
                errors.emplace_back(
                    UsdUtilsValidationErrorNameTokens->assetNotInPackage,
                    UsdValidationErrorType::Warn,
                    UsdValidationErrorSites {
                        UsdValidationErrorSite(rootLayer, SdfPath(asset)) },
                    TfStringPrintf(
                        ("Found asset reference '%s' that does not belong to "
                         "the package '%s'."),
                        asset.c_str(), packagePath.c_str()));
            }
        }
    }

    return errors;
}

static UsdValidationErrorVector
_PrimTypeValidator(const UsdPrim& usdPrim) {

    const std::vector<std::string> allowedPrimTypeNames = {"", "Scope",
        "Xform", "Shader", "Material", "Mesh", "Sphere", "Cube", "Cylinder",
        "Cone", "Capsule", "GeomSubset", "Points", "SkelRoot", "Skeleton",
        "SkelAnimation", "BlendShape", "SpatialAudio", "PhysicsScene",
        "Preliminary_ReferenceImage", "Preliminary_Text",
        "Preliminary_Trigger"};

    const std::string primTypeName = usdPrim.GetTypeName();

    if (std::find(allowedPrimTypeNames.begin(), allowedPrimTypeNames.end(),
        primTypeName) == allowedPrimTypeNames.end() &&
        !(TfStringStartsWith(primTypeName, "RealityKit")))
    {
        return {
            UsdValidationError{
                UsdUtilsValidationErrorNameTokens->unsupportedPrimType,
                UsdValidationErrorType::Error,
                UsdValidationErrorSites {
                    UsdValidationErrorSite(
                        usdPrim.GetStage(), usdPrim.GetPath())
                },
                TfStringPrintf(
                    ("Prim <%s> has an unsupported type '%s'."),
                    usdPrim.GetPath().GetText(), primTypeName.c_str())
            }
        };
    }

    return {};
}

TF_REGISTRY_FUNCTION(UsdValidationRegistry)
{
    UsdValidationRegistry &registry = UsdValidationRegistry::GetInstance();

    registry.RegisterPluginValidator(
        UsdUtilsValidatorNameTokens->packageEncapsulationValidator,
        _PackageEncapsulationValidator);

    registry.RegisterPluginValidator(
        UsdUtilsValidatorNameTokens->primTypeValidator,
        _PrimTypeValidator);
}

PXR_NAMESPACE_CLOSE_SCOPE
