//
// Copyright 2024 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/base/arch/systemInfo.h"
#include "pxr/base/tf/pathUtils.h"
#include "pxr/usd/usdGeom/sphere.h"
#include <pxr/usd/usdLux/sphereLight.h>
#include "pxr/usdValidation/usdUtilsValidators/validatorTokens.h"
#include "pxr/usdValidation/usdValidation/error.h"
#include "pxr/usdValidation/usdValidation/registry.h"
#include "pxr/usdValidation/usdValidation/validator.h"

#include <array>
#include <filesystem>

PXR_NAMESPACE_USING_DIRECTIVE

TF_DEFINE_PRIVATE_TOKENS(_tokens,
    ((usdUtilsValidatorsPlugin, "usdUtilsValidators"))
);

static void
TestUsdUsdzValidators()
{
    // This should be updated with every new validator added with
    // UsdUtilsValidators keyword.
    UsdValidationRegistry &registry = UsdValidationRegistry::GetInstance();
    UsdValidationValidatorMetadataVector metadata
        = registry.GetValidatorMetadataForPlugin(
            _tokens->usdUtilsValidatorsPlugin);
    TF_AXIOM(metadata.size() == 2);
    // Since other validators can be registered with a UsdUtilsValidators
    // keyword, our validators registered in usd are a subset of the entire
    // set.
    std::set<TfToken> validatorMetadataNameSet;
    for (const UsdValidationValidatorMetadata &metadata : metadata) {
        validatorMetadataNameSet.insert(metadata.name);
    }

    const std::set<TfToken> expectedValidatorNames
        = { UsdUtilsValidatorNameTokens->packageEncapsulationValidator,
         UsdUtilsValidatorNameTokens->primTypeValidator };

    TF_AXIOM(validatorMetadataNameSet == expectedValidatorNames);
}

static void
TestPackageEncapsulationValidator()
{
    UsdValidationRegistry &registry = UsdValidationRegistry::GetInstance();

    // Verify the validator exists
    const UsdValidationValidator *validator = registry.GetOrLoadValidatorByName(
        UsdUtilsValidatorNameTokens->packageEncapsulationValidator);

    TF_AXIOM(validator);

    // Load the pre-created usdz stage with paths to a layer and asset
    // that are not included in the package, but exist
    const UsdStageRefPtr &stage = UsdStage::Open("fail.usdz");

    UsdValidationErrorVector errors = validator->Validate(stage);

    // Verify both the layer & asset errors are present
    TF_AXIOM(errors.size() == 2);

    // Note that we keep the referenced layer in normalized path to represent
    // the layer identifier, whereas the asset path is platform specific path,
    // as returned by UsdUtilsComputeAllDependencies
    const SdfLayerRefPtr &rootLayer = stage->GetRootLayer();
    const std::string &rootLayerIdentifier = rootLayer->GetIdentifier();
    const std::string realUsdzPath = rootLayer->GetRealPath();
    const std::string errorLayer
        = TfStringCatPaths(TfGetPathName(TfAbsPath(rootLayerIdentifier)),
                           "excludedDirectory/layer.usda");

    std::filesystem::path parentDir
        = std::filesystem::path(realUsdzPath).parent_path();
    const std::string errorAsset
        = (parentDir / "excludedDirectory" / "image.jpg").string();

    std::array<std::string, 2> expectedErrorMessages
        = { TfStringPrintf(("Found referenced layer '%s' that does not "
                            "belong to the package '%s'."),
                           errorLayer.c_str(), realUsdzPath.c_str()),
            TfStringPrintf(("Found asset reference '%s' that does not belong"
                            " to the package '%s'."),
                           errorAsset.c_str(), realUsdzPath.c_str()) };

    std::array<TfToken, 2> expectedErrorIdentifiers = {
        TfToken(
            "usdUtilsValidators:PackageEncapsulationValidator.LayerNotInPackage"),
        TfToken(
            "usdUtilsValidators:PackageEncapsulationValidator.AssetNotInPackage")
    };

    for (size_t i = 0; i < errors.size(); ++i) {
        TF_AXIOM(errors[i].GetIdentifier() == expectedErrorIdentifiers[i]);
        TF_AXIOM(errors[i].GetType() == UsdValidationErrorType::Warn);
        TF_AXIOM(errors[i].GetSites().size() == 1);
        TF_AXIOM(!errors[i].GetSites()[0].GetLayer().IsInvalid());
        TF_AXIOM(errors[i].GetMessage() == expectedErrorMessages[i]);
    }

    // Load the pre-created usdz stage with relative paths to both a reference
    // and an asset that are included in the package.
    const UsdStageRefPtr &passStage = UsdStage::Open("pass.usdz");

    errors = validator->Validate(passStage);

    // Verify the errors are gone
    TF_AXIOM(errors.empty());
}

static void
TestPrimTypeValidator()
{
    UsdValidationRegistry& registry = UsdValidationRegistry::GetInstance();

    // Verify the validator exists
    const UsdValidationValidator *validator = registry.GetOrLoadValidatorByName(
            UsdUtilsValidatorNameTokens->primTypeValidator);

    TF_AXIOM(validator);

    const UsdStageRefPtr& stage = UsdStage::CreateInMemory();

    // Create an unsupported prim
    const UsdLuxSphereLight sphereLight = UsdLuxSphereLight::Define(stage,
        SdfPath("/SphereLight"));
    const UsdPrim sphereLightPrim = sphereLight.GetPrim();

    UsdValidationErrorVector errors = validator->Validate(sphereLightPrim);

    // Verify unsupported prim error is present
    TfToken expectedErrorIdentifier(
        "usdUtilsValidators:PrimTypeValidator.UnsupportedPrimType");
    TF_AXIOM(errors.size() == 1);
    TF_AXIOM(errors[0].GetIdentifier() == expectedErrorIdentifier);
    TF_AXIOM(errors[0].GetType() == UsdValidationErrorType::Error);
    TF_AXIOM(errors[0].GetSites().size() == 1);
    TF_AXIOM(errors[0].GetSites()[0].IsValid());
    TF_AXIOM(errors[0].GetSites()[0].IsPrim());
    TF_AXIOM(errors[0].GetSites()[0].GetPrim().GetPath() ==
             SdfPath("/SphereLight"));
    std::string expectedErrorMsg = ("Prim </SphereLight> has an unsupported "
                                    "type 'SphereLight'.");
    TF_AXIOM(errors[0].GetMessage() == expectedErrorMsg);

    const UsdGeomSphere sphere = UsdGeomSphere::Define(stage,
        SdfPath("/Sphere"));
    const UsdPrim spherePrim = sphere.GetPrim();

    errors = validator->Validate(spherePrim);

    // Verify there are no errors on a supported prim type
    TF_AXIOM(errors.empty());
}

int
main()
{
    TestUsdUsdzValidators();
    TestPackageEncapsulationValidator();
    TestPrimTypeValidator();

    return EXIT_SUCCESS;
}
