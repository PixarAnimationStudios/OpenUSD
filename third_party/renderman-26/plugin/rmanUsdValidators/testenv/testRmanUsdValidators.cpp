//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/usdValidation/usdValidation/error.h"
#include "pxr/usdValidation/usdValidation/registry.h"
#include "pxr/usd/usd/schemaRegistry.h"
#include "pxr/usd/usdRender/settings.h"

PXR_NAMESPACE_USING_DIRECTIVE

TF_DEFINE_PRIVATE_TOKENS(_tokens,
    ((rmanUsdValidatorsPlugin, "rmanUsdValidators"))
    ((PxrRenderTerminalsAPIRelationships,                                     \
      "rmanUsdValidators:PxrRenderTerminalsAPIRelationships"))
);

void
TestRmanUsdValidators()
{
    // This should be updated with every new validator added with the
    // UsdValidators keyword.
    const std::set<TfToken> expectedRmanUsdValidatorNames = {
        _tokens->PxrRenderTerminalsAPIRelationships,
    };

    const UsdValidationRegistry &registry
        = UsdValidationRegistry::GetInstance();

    // Since other validators can be registered with the same keywords,
    // our validators registered in usdValidators are/may be a subset of the
    // entire set.
    std::set<TfToken> validatorMetadataNameSet;

    UsdValidationValidatorMetadataVector metadata
        = registry.GetValidatorMetadataForPlugin(
            _tokens->rmanUsdValidatorsPlugin);
    TF_AXIOM(metadata.size() == 1);
    for (const UsdValidationValidatorMetadata &m: metadata) {
        validatorMetadataNameSet.insert(m.name);
    }

    TF_AXIOM(validatorMetadataNameSet == expectedRmanUsdValidatorNames);
}

void
TestPxrRenderTerminalsAPIRelationships()
{
    // Verify that the validator exists.
    UsdValidationRegistry &registry = UsdValidationRegistry::GetInstance();
    const UsdValidationValidator *validator = registry.GetOrLoadValidatorByName(
        _tokens->PxrRenderTerminalsAPIRelationships);
    TF_AXIOM(validator);

    // Create Stage and RenderSettings with invalid connection attributes
    UsdStageRefPtr usdStage = UsdStage::CreateInMemory();
    UsdRenderSettings rs = UsdRenderSettings::Define(
        usdStage, SdfPath("/RenderSettings"));
    UsdPrim rsPrim = rs.GetPrim();

    UsdAttribute attr1 = rsPrim.CreateAttribute(
        TfToken("outputs:ri:sampleFilters"), SdfValueTypeNames->Token);
    attr1.SetConnections({ SdfPath("/Foo") });

    UsdValidationErrorVector errors = validator->Validate(rsPrim);
    const TfToken renderSettings = TfToken("RenderSettings");
    const TfToken integratorRel = TfToken("ri:integrator");

    const UsdSchemaRegistry& reg = UsdSchemaRegistry::GetInstance();
    const UsdPrimDefinition* rsDef = reg.FindConcretePrimDefinition(
        renderSettings);
    const TfTokenVector& rsPropNames = rsDef->GetPropertyNames();
    bool rsSchemaHasRelationships = std::find(rsPropNames.begin(),
        rsPropNames.end(), integratorRel)
        != rsPropNames.end();

    if (rsSchemaHasRelationships) {
        TfToken expectedErrorIdentifier(
            "rmanUsdValidators:PxrRenderTerminalsAPIRelationships."
            "invalidRenderTerminalsAttr");
        // Verify the errors for having certain terminal connections
        // on RenderSettings are present.
        TF_AXIOM(errors.size() == 1);
        TF_AXIOM(errors[0].GetIdentifier() == expectedErrorIdentifier);
        TF_AXIOM(errors[0].GetType() == UsdValidationErrorType::Warn);
        TF_AXIOM(errors[0].GetSites().size() == 1);
        TF_AXIOM(errors[0].GetSites()[0].IsValid());
        TF_AXIOM(errors[0].GetSites()[0].IsPrim());
        TF_AXIOM(errors[0].GetSites()[0].GetPrim().GetPath()
                == SdfPath("/RenderSettings"));
        std::string expectedErrorMsg = ("Found a PxrRenderTerminalsAPI "
            "unsupported attribute (outputs:ri:sampleFilters) that "
            "should instead be a relationship; see the schema for more info.");
        TF_AXIOM(errors[0].GetMessage() == expectedErrorMsg);
        
        // Verify all errors are gone without authored terminal connections.
        attr1.ClearConnections();
        errors = validator->Validate(rsPrim);
        TF_AXIOM(errors.empty());
    } else {
        TF_AXIOM(errors.empty());
    }
}

int
main()
{
    TestRmanUsdValidators();
    TestPxrRenderTerminalsAPIRelationships();
    printf("OK\n");
    return EXIT_SUCCESS;
};

