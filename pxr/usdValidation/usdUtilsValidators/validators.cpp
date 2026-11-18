//
// Copyright 2024 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/usd/ar/packageUtils.h"
#include "pxr/usd/ar/resolver.h"
#include "pxr/usd/sdf/zipFile.h"
#include "pxr/usd/usdUtils/dependencies.h"
#include "pxr/usd/usdUtils/userProcessingFunc.h"
#include "pxr/usdValidation/usdValidation/validatorTokens.h"
#include "pxr/usdValidation/usdGeomValidators/validatorTokens.h"
#include "pxr/usdValidation/usdLodValidators/validatorTokens.h"
#include "pxr/usdValidation/usdLuxValidators/validatorTokens.h"
#include "pxr/usdValidation/usdMediaValidators/validatorTokens.h"
#include "pxr/usdValidation/usdPhysicsValidators/validatorTokens.h"
#include "pxr/usdValidation/usdShadeValidators/validatorTokens.h"
#include "pxr/usdValidation/usdSkelValidators/validatorTokens.h"
#include "pxr/usdValidation/usdUtilsValidators/validatorTokens.h"
#include "pxr/usdValidation/usdValidation/error.h"
#include "pxr/usdValidation/usdValidation/registry.h"
#include "pxr/usdValidation/usdValidation/timeRange.h"

#include "pxr/usd/sdf/fileFormat.h"

#include <algorithm>
#include <string>

PXR_NAMESPACE_OPEN_SCOPE

static UsdValidationErrorVector
_PackageEncapsulationValidator(
    const UsdStagePtr &usdStage, 
    const UsdValidationTimeRange &/*timeRange*/)
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
    std::vector<std::string> assets, unresolvedPaths;
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
            const std::string &refRealPath = referencedLayer->GetRealPath();

            // We don't want to validate in-memory or session layers
            // since these layers will not have a real path, we skip here
            if (refRealPath.empty()) {
                continue;
            }

            if (!TfStringStartsWith(refRealPath, packagePath)) {
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
                        "Found asset reference '%s' that does not belong to "
                         "the package '%s'.",
                        asset.c_str(), packagePath.c_str()));
            }
        }
    }

    return errors;
}

static UsdValidationErrorVector
_FileExtensionValidator(const UsdStagePtr& usdStage,
                        const UsdValidationTimeRange& /*timeRange*/) 
{
    UsdValidationErrorVector errors;

    const std::set<TfToken> validExtensions = {TfToken("usda"),
        TfToken("usdc"), TfToken("usd"), TfToken("usdz"), TfToken("png"),
        TfToken("jpg"), TfToken("jpeg"), TfToken("exr"), TfToken("avif"),
        TfToken("m4a"), TfToken("mp3"), TfToken("wav")};

    const SdfLayerHandle& rootLayer = usdStage->GetRootLayer();
    const SdfZipFile& zipFile = SdfZipFile::Open(rootLayer->GetRealPath());

    const std::vector<std::string> fileNames =
        std::vector<std::string>(zipFile.begin(), zipFile.end());

    bool didWarnAboutUsdPmc = false;

    for (const std::string& fileName : fileNames)
    {
        const std::string extension = ArGetResolver().GetExtension(fileName);

        if (extension == "pmc") {
            if (didWarnAboutUsdPmc) {
                continue;
            }

            errors.emplace_back(
                UsdValidationError {
                    UsdUtilsValidationErrorNameTokens->containsPmcFile,
                    UsdValidationErrorType::Warn,
                    UsdValidationErrorSites {
                        UsdValidationErrorSite(
                                rootLayer, SdfPath(rootLayer->GetIdentifier()))
                    },
                    "This package contains a file with the 'pmc' extension. "
                    "The UsdPmc plugin is required in order to read files of "
                    "this type. This plugin is not part of the default USD "
                    "build and therefore a client may not be able to read it."
                });
            didWarnAboutUsdPmc = true;
        } else if (std::find(validExtensions.begin(), validExtensions.end(),
            extension) == validExtensions.end()) 
        {
            errors.emplace_back(
                UsdValidationError {
                    UsdUtilsValidationErrorNameTokens->
                    unsupportedFileExtensionInPackage,
                    UsdValidationErrorType::Error,
                    UsdValidationErrorSites {
                        UsdValidationErrorSite(
                                rootLayer, SdfPath(rootLayer->GetIdentifier()))
                    },
                    TfStringPrintf("File '%s' in package '%s' has an unknown "
                                   "unsupported extension '%s'.",
                                   fileName.c_str(),
                                   rootLayer->GetIdentifier().c_str(),
                                   extension.c_str())
                });
        }
    }

    return errors;
}

static
UsdValidationErrorVector
_MissingReferenceValidator(const UsdStagePtr& usdStage,
                           const UsdValidationTimeRange &/*timeRange*/) 
{
    const SdfLayerRefPtr& rootLayer = usdStage->GetRootLayer();

    SdfLayerRefPtrVector layers;
    std::vector<std::string> unresolvedPaths;
    const SdfAssetPath& path = SdfAssetPath(rootLayer->GetIdentifier());

    UsdUtilsComputeAllDependencies(path, &layers, nullptr /*assets*/, 
                                   &unresolvedPaths, nullptr);

    UsdValidationErrorVector errors;
    for(const std::string &unresolvedPath : unresolvedPaths)
    {
        errors.emplace_back(
            UsdUtilsValidationErrorNameTokens->unresolvableDependency,
            UsdValidationErrorType::Error,
            UsdValidationErrorSites {
                UsdValidationErrorSite(
                    rootLayer, SdfPath(unresolvedPath))
            },
            TfStringPrintf(
            ("Found unresolvable external dependency "
                "'%s'."), unresolvedPath.c_str())
        );
    }

    return errors;
}

static UsdValidationErrorVector
_GetUsdzPackageErrors(const SdfLayerHandle &rootLayer)
{
    const SdfZipFile zipFile = SdfZipFile::Open(
            rootLayer->GetRealPath().c_str());
    if (!zipFile) {
        return {};
    }

    const std::string packagePath = ArSplitPackageRelativePathOuter(
            rootLayer->GetIdentifier()).first;

    UsdValidationErrorVector errors;
    for(auto it = zipFile.begin(); it != zipFile.end(); ++it)
    {
        const SdfZipFile::FileInfo &fileInfo = it.GetFileInfo();
        if (fileInfo.compressionMethod != 0)
        {
            const std::string &fileName = *it;
            errors.emplace_back(
                UsdUtilsValidationErrorNameTokens->compressionDetected,
                UsdValidationErrorType::Error,
                UsdValidationErrorSites {
                    UsdValidationErrorSite(
                            rootLayer, SdfPath(fileName))
                },
                TfStringPrintf(
                    "File '%s' in package '%s' has compression. Compression "
                    "method is '%u', actual size is %lu. Uncompressed size "
                    "is %lu.", fileName.c_str(), packagePath.c_str(), 
                    fileInfo.compressionMethod, fileInfo.size, 
                    fileInfo.uncompressedSize)
            );
        }

        if (fileInfo.encrypted)
        {
            const std::string &fileName = *it;
            errors.emplace_back(
                UsdUtilsValidationErrorNameTokens->encryptionDetected,
                UsdValidationErrorType::Error,
                UsdValidationErrorSites {
                    UsdValidationErrorSite(
                            rootLayer, SdfPath(fileName))
                },
                TfStringPrintf("File '%s' in package '%s' is encrypted.",
                    fileName.c_str(), packagePath.c_str())
            );
        }

        if (fileInfo.dataOffset % 64 != 0)
        {
            const std::string &fileName = *it;
            errors.emplace_back(
                UsdUtilsValidationErrorNameTokens->byteMisalignment,
                UsdValidationErrorType::Error,
                UsdValidationErrorSites {
                    UsdValidationErrorSite(
                            rootLayer, SdfPath(fileName))
                },
                TfStringPrintf("File '%s' in package '%s' has an invalid "
                    "offset %zu.", fileName.c_str(), packagePath.c_str(),
                    fileInfo.dataOffset)
            );
        }
    }

    return errors;
}

static UsdValidationErrorVector
_RootPackageValidator(const UsdStagePtr& usdStage,
                           const UsdValidationTimeRange &/*timeRange*/)
{
    const SdfLayerHandle &rootLayer = usdStage->GetRootLayer();
    if (!rootLayer->GetFileFormat()->IsPackage())
    {
        return {};
    }
    return _GetUsdzPackageErrors(rootLayer);
}

static UsdValidationErrorVector
_UsdzPackageValidator(const UsdStagePtr& usdStage,
                           const UsdValidationTimeRange &/*timeRange*/)
{
    SdfLayerRefPtrVector layers;
    const SdfLayerHandle &rootLayer = usdStage->GetRootLayer();
    const SdfAssetPath &path = SdfAssetPath(rootLayer->GetIdentifier());

    UsdUtilsComputeAllDependencies(path, &layers, nullptr /*assets*/, 
                                   nullptr /*unresolvedPaths*/, 
                                   nullptr /*processingFunc*/);

    const std::string &realPath = rootLayer->GetRealPath();
    const std::string &packagePath
        = ArIsPackageRelativePath(rootLayer->GetIdentifier())
            ? ArSplitPackageRelativePathOuter(realPath).first
            : realPath;

    UsdValidationErrorVector errors;

    if (!packagePath.empty())
    {
        for (const SdfLayerRefPtr &referencedLayer : layers)
        {
            if (!referencedLayer)
            {
                // Invalid reference layer found; PackageEncapsulationValidator
                // will catch this, so we can skip this layer here.
                continue;
            }
            UsdValidationErrorVector layerErrors = _GetUsdzPackageErrors(
                    referencedLayer);
            errors.insert(
                errors.end(), std::make_move_iterator(layerErrors.begin()),
                std::make_move_iterator(layerErrors.end()));
        }
    }

    return errors;
}

static void
_RegisterVanillaUsdValidatorSuite(UsdValidationRegistry &registry)
{
    // Include every validator from the core and schema domain validator
    // plugins, except the ones listed in excludedNames.
    const std::vector<const std::vector<TfToken> *> domainTokens = {
        &UsdValidatorNameTokens->allTokens,
        &UsdGeomValidatorNameTokens->allTokens,
        &UsdLodValidatorNameTokens->allTokens,
        &UsdLuxValidatorNameTokens->allTokens,
        &UsdMediaValidatorNameTokens->allTokens,
        &UsdPhysicsValidatorNameTokens->allTokens,
        &UsdShadeValidatorNameTokens->allTokens,
        &UsdSkelValidatorNameTokens->allTokens,
        &UsdUtilsValidatorNameTokens->allTokens
    };

    const TfTokenVector excludedNames = {
        // Asset checks for stages intended to be referenced into aggregate
        // stages.
        UsdValidatorNameTokens->stageMetadataChecker,
        UsdGeomValidatorNameTokens->stageMetadataChecker,
        // UsdzPackageValidator already checks the root layer's package, along
        // with every other package in the stage's dependencies.
        UsdUtilsValidatorNameTokens->rootPackageValidator,
        // This suite itself.
        UsdUtilsValidatorNameTokens->vanillaUsdValidatorSuite
    };

    TfTokenVector validatorNames;
    for (const std::vector<TfToken> *tokens : domainTokens) {
        for (const TfToken &name : *tokens) {
            if (std::find(excludedNames.begin(), excludedNames.end(), name)
                == excludedNames.end()) {
                validatorNames.push_back(name);
            }
        }
    }

    const std::vector<const UsdValidationValidator *> containedValidators =
        registry.GetOrLoadValidatorsByName(validatorNames);

    registry.RegisterPluginValidatorSuite(
        UsdUtilsValidatorNameTokens->vanillaUsdValidatorSuite,
        containedValidators);
}

TF_REGISTRY_FUNCTION(UsdValidationRegistry)
{
    UsdValidationRegistry &registry = UsdValidationRegistry::GetInstance();

    registry.RegisterPluginValidator(
        UsdUtilsValidatorNameTokens->packageEncapsulationValidator,
        _PackageEncapsulationValidator);

    registry.RegisterPluginValidator(
            UsdUtilsValidatorNameTokens->fileExtensionValidator,
            _FileExtensionValidator);

    registry.RegisterPluginValidator(
       UsdUtilsValidatorNameTokens->missingReferenceValidator, 
        _MissingReferenceValidator);

    registry.RegisterPluginValidator(
            UsdUtilsValidatorNameTokens->rootPackageValidator,
            _RootPackageValidator);

    registry.RegisterPluginValidator(
            UsdUtilsValidatorNameTokens->usdzPackageValidator,
            _UsdzPackageValidator);

    _RegisterVanillaUsdValidatorSuite(registry);
}

PXR_NAMESPACE_CLOSE_SCOPE
