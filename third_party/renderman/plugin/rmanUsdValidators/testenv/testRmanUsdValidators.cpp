//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/usdValidation/usdValidation/error.h"
#include "pxr/usdValidation/usdValidation/registry.h"
#include "pxr/usd/usd/schemaRegistry.h"
#include "pxr/usd/usdGeom/camera.h"
#include "pxr/usd/usdRender/settings.h"

PXR_NAMESPACE_USING_DIRECTIVE

TF_DEFINE_PRIVATE_TOKENS(_tokens,
    (PxrCameraProjectionAPI)
    ((rmanUsdValidatorsPlugin, "rmanUsdValidators"))
    ((PxrRenderTerminalsAPIRelationships,                                     \
      "rmanUsdValidators:PxrRenderTerminalsAPIRelationships"))                \
    ((PxrCameraProjectionAPIRelationships,                                    \
      "rmanUsdValidators:PxrCameraProjectionAPIRelationships"))               \
);

void
TestRmanUsdValidators()
{
    // This should be updated with every new validator added with the
    // UsdValidators keyword.
    const std::set<TfToken> expectedRmanUsdValidatorNames = {
        _tokens->PxrRenderTerminalsAPIRelationships,
        _tokens->PxrCameraProjectionAPIRelationships
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
    TF_AXIOM(metadata.size() == 2);
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
    UsdStageRefPtr usdStage = UsdStage::CreateNew("test.usda");
    UsdRenderSettings rs = UsdRenderSettings::Define(
        usdStage, SdfPath("/RenderSettings"));
    UsdPrim rsPrim = rs.GetPrim();

    UsdAttribute attr1 = rsPrim.CreateAttribute(
        TfToken("outputs:ri:sampleFilters"), SdfValueTypeNames->Token);
    attr1.SetConnections({ SdfPath("/Foo") });

    UsdValidationErrorVector errors = validator->Validate(rsPrim);
    const TfToken renderSettings = TfToken("RenderSettings");
    const TfToken integratorRel = TfToken("ri:integrator");

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
    
    // Verify all errors are gone after fix is applied.
    // We know there is only 1 fixer for the error.
    const UsdValidationFixer *fixer = errors[0].GetFixers()[0];
    UsdEditTarget editTarget = usdStage->GetEditTarget();

    if (fixer->CanApplyFix(errors[0], editTarget)) {
        bool fixApplied = fixer->ApplyFix(errors[0], editTarget);
        TF_AXIOM(fixApplied);
    } else {
        TF_AXIOM(false);
    }
    errors = validator->Validate(rsPrim);
    TF_AXIOM(errors.empty());
}

void
TestPxrCameraProjectionAPIRelationships()
{
    // Only run validation if we have the appropriate schema
    using UsdPropertyDefinition = UsdPrimDefinition::Property;
    const UsdSchemaRegistry& reg = UsdSchemaRegistry::GetInstance();
    const UsdPrimDefinition* camProjAPIDef = reg.FindAppliedAPIPrimDefinition(
        _tokens->PxrCameraProjectionAPI);
    if (!camProjAPIDef) {
        return;
    }

    const UsdPropertyDefinition camProjAPIRelDef = 
        camProjAPIDef->GetPropertyDefinition(
            TfToken("ri:projection"));
    if (!camProjAPIRelDef) {
        return;
    }

    // Now also check if PxrCameraProjectionAPI is applied to UsdGeomCamera
    UsdStageRefPtr usdStage = UsdStage::CreateNew("testCamera.usda");
    UsdGeomCamera camera = UsdGeomCamera::Define(
        usdStage, SdfPath("/Camera"));
    UsdPrim cameraPrim = camera.GetPrim();
    if (!cameraPrim.HasAPI(_tokens->PxrCameraProjectionAPI)) {
        return;
    }

    UsdValidationRegistry &registry = UsdValidationRegistry::GetInstance();
    const UsdValidationValidator *validator = registry.GetOrLoadValidatorByName(
        _tokens->PxrCameraProjectionAPIRelationships);
    TF_AXIOM(validator);

    UsdAttribute attr = cameraPrim.CreateAttribute(
        TfToken("outputs:ri:projection"), SdfValueTypeNames->Token);
    attr.SetConnections({ SdfPath("/SomeProjection") });

    UsdValidationErrorVector errors = validator->Validate(cameraPrim);

    TfToken expectedErrorIdentifier(
        "rmanUsdValidators:PxrCameraProjectionAPIRelationships."
        "invalidCameraProjectionAttr");

    TF_AXIOM(errors.size() == 1);
    TF_AXIOM(errors[0].GetIdentifier() == expectedErrorIdentifier);
    TF_AXIOM(errors[0].GetType() == UsdValidationErrorType::Warn);
    TF_AXIOM(errors[0].GetSites().size() == 1);
    TF_AXIOM(errors[0].GetSites()[0].IsValid());
    TF_AXIOM(errors[0].GetSites()[0].IsPrim());
    TF_AXIOM(errors[0].GetSites()[0].GetPrim().GetPath()
            == SdfPath("/Camera"));
    std::string expectedErrorMsg =
        "Found a PxrCameraProjectionAPI unsupported attribute "
        "(outputs:ri:projection) that should instead be a relationship "
        "(ri:projection); see the schema for more info.";
    TF_AXIOM(errors[0].GetMessage() == expectedErrorMsg);

    const UsdValidationFixer *fixer = errors[0].GetFixers()[0];
    UsdEditTarget editTarget = usdStage->GetEditTarget();
    if (fixer->CanApplyFix(errors[0], editTarget)) {
        bool fixApplied = fixer->ApplyFix(errors[0], editTarget);
        TF_AXIOM(fixApplied);
    } else {
        TF_AXIOM(false);
    }
    errors = validator->Validate(cameraPrim);
    TF_AXIOM(errors.empty());
}

int
main()
{
    TestRmanUsdValidators();
    TestPxrRenderTerminalsAPIRelationships();
    TestPxrCameraProjectionAPIRelationships();
    printf("OK\n");
    return EXIT_SUCCESS;
};

