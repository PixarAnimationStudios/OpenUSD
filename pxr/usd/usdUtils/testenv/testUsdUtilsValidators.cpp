#include "pxr/usd/usd/validator.h"
#include "pxr/usd/usd/validationError.h"
#include "pxr/usd/usdUtils/validatorTokens.h"
#include "pxr/usd/usd/validationRegistry.h"
#include "pxr/base/arch/systemInfo.h"

#include <iostream>

PXR_NAMESPACE_USING_DIRECTIVE

static
void
TestUsdUsdzValidators()
{
    // This should be updated with every new validator added with
    // UsdUsdzValidators keyword.
    UsdValidationRegistry& registry = UsdValidationRegistry::GetInstance();
    UsdValidatorMetadataVector metadata =
            registry.GetValidatorMetadataForKeyword(
                    UsdUtilsValidatorKeywordTokens->UsdUtilsValidators);
    // Since other validators can be registered with a UsdUsdzValidators
    // keyword, our validators registered in usd are a subset of the entire
    // set.
    std::set<TfToken> validatorMetadataNameSet;
    for (const UsdValidatorMetadata &metadata : metadata) {
        validatorMetadataNameSet.insert(metadata.name);
    }

    const std::set<TfToken> expectedValidatorNames =
            {UsdUtilsValidatorNameTokens->packageEncapsulationValidator};

    TF_AXIOM(std::includes(validatorMetadataNameSet.begin(),
                           validatorMetadataNameSet.end(),
                           expectedValidatorNames.begin(),
                           expectedValidatorNames.end()));
}

static
void
TestPackageEncapsulationValidator()
{
    UsdValidationRegistry& registry = UsdValidationRegistry::GetInstance();

    // Verify the validator exists
    const UsdValidator *validator = registry.GetOrLoadValidatorByName(
            UsdUtilsValidatorNameTokens->packageEncapsulationValidator);

    TF_AXIOM(validator);

    const std::string testAssetDirectory = ArchGetCwd() + "/ctest/testUsdUtilsPackageEncapsulation/";

    // Load the pre-created usdz stage with fully qualified paths to both a reference and an asset
    const UsdStageRefPtr& stage = UsdStage::Open(testAssetDirectory + "fail.usdz");

    UsdValidationErrorVector errors = validator->Validate(stage);

    // Verify both the layer & asset errors are present
    TF_AXIOM(errors.size() == 2);
    const std::string& errorLayer = (testAssetDirectory + "0/layer.usda");
    const std::string& rootLayer = testAssetDirectory + "fail.usdz";
    const std::string& errorAsset = testAssetDirectory + "0/image.jpg";

    std::vector<std::string> expectedErrorMessages = {
            TfStringPrintf(("Found loaded layer '%s' that does not belong to the package '%s'."),
                           errorLayer.c_str(), rootLayer.c_str()),
            TfStringPrintf(("Found asset reference '%s' that does not belong to the package '%s'."),
                           errorAsset.c_str(), rootLayer.c_str())
    };

    for (size_t i = 0; i < errors.size(); ++i)
    {
        TF_AXIOM(errors[i].GetType() == UsdValidationErrorType::Error);
        TF_AXIOM(errors[i].GetSites().size() == 1);
        TF_AXIOM(!errors[i].GetSites()[0].GetLayer().IsInvalid());
        TF_AXIOM(errors[i].GetMessage() == expectedErrorMessages[i]);
    }

    // Load the pre-created usdz stage with relative paths to both a reference and an asset.
    const UsdStageRefPtr &passStage = UsdStage::Open(testAssetDirectory + "pass.usdz");
    
    errors = validator->Validate(passStage);

    // Verify the errors are gone
    TF_AXIOM(errors.size() == 0);
}

int
main()
{
    TestUsdUsdzValidators();
    TestPackageEncapsulationValidator();

    std::cout << "OK\n";
}