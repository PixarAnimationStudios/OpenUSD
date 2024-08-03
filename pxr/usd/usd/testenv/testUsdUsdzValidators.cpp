#include "pxr/usd/usd/validator.h"
#include "pxr/usd/usd/validationError.h"
#include "pxr/usd/usd/validatorTokens.h"
#include "pxr/usd/usd/validationRegistry.h"

#include <iostream>

PXR_NAMESPACE_USING_DIRECTIVE

static
void
TestUsdUsdzValidators()
{
    // This should be updated with every new validator added with
    // UsdUsdzValidators keyword.
    UsdValidationRegistry &registry = UsdValidationRegistry::GetInstance();
    UsdValidatorMetadataVector metadata =
            registry.GetValidatorMetadataForKeyword(
                    UsdValidatorKeywordTokens->UsdUsdzValidators);
    // Since other validators can be registered with a UsdUsdzValidators
    // keyword, our validators registered in usd are a subset of the entire
    // set.
    std::set<TfToken> validatorMetadataNameSet;
    for (const UsdValidatorMetadata &metadata : metadata) {
        validatorMetadataNameSet.insert(metadata.name);
    }

    const std::set<TfToken> expectedValidatorNames =
            {UsdValidatorNameTokens->usdzPackageEncapsulationValidator};

    TF_AXIOM(std::includes(validatorMetadataNameSet.begin(),
                           validatorMetadataNameSet.end(),
                           expectedValidatorNames.begin(),
                           expectedValidatorNames.end()));
}

static
void
TestPackageEncapsulationValidator()
{
    // Get validator
    UsdValidationRegistry &registry = UsdValidationRegistry::GetInstance();

    // Verify validator
    const UsdValidator *validator = registry.GetOrLoadValidatorByName(
            UsdValidatorNameTokens->usdzPackageEncapsulationValidator);

    TF_AXIOM(validator);

    // load invalid root layer usdz
    SdfLayerRefPtr layer = SdfLayer::CreateAnonymous();

    // validate and verify errors
    auto errors = validator->Validate(layer);

    // remove invalid reference? change reference to be valid? load different valid reference?

    // validate errors are gone
    TF_AXIOM(errors.size() == 0);
}

int
main()
{
    TestUsdUsdzValidators();
    TestPackageEncapsulationValidator();

    std::cout << "OK\n";
}