//
// Copyright 2026 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
//
#include "pxr/pxr.h"
#include "pxr/base/tf/token.h"
#include "pxr/usd/sdf/path.h"
#include "pxr/usd/usd/prim.h"
#include "pxr/usd/usd/stage.h"
#include "pxr/usd/usdGeom/scope.h"
#include "pxr/usd/usdLux/distantLight.h"
#include "pxr/usd/usdLux/domeLight.h"
#include "pxr/usd/usdLux/lightFilter.h"
#include "pxr/usd/usdLux/portalLight.h"
#include "pxr/usd/usdLux/sphereLight.h"
#include "pxr/usd/usdShade/materialBindingAPI.h"
#include "pxr/usd/usdShade/shader.h"
#include "pxr/usdValidation/usdLuxValidators/validatorTokens.h"
#include "pxr/usdValidation/usdValidation/error.h"
#include "pxr/usdValidation/usdValidation/registry.h"
#include "pxr/usdValidation/usdValidation/validator.h"

#include <set>
#include <string>

PXR_NAMESPACE_USING_DIRECTIVE

TF_DEFINE_PRIVATE_TOKENS(_tokens,
    ((usdLuxValidatorsPlugin, "usdLuxValidators"))
);

void
TestUsdLuxValidators()
{
    // This should be updated with every new validator added with the
    // UsdLuxValidators keyword.
    const std::set<TfToken> expectedUsdLuxValidatorNames
        = { UsdLuxValidatorNameTokens->encapsulationValidator };

    const UsdValidationRegistry &registry
        = UsdValidationRegistry::GetInstance();

    // Since other validators can be registered with the same keywords,
    // our validators registered in usdLux are/may be a subset of the
    // entire set.
    std::set<TfToken> validatorMetadataNameSet;

    UsdValidationValidatorMetadataVector metadata
        = registry.GetValidatorMetadataForPlugin(
            _tokens->usdLuxValidatorsPlugin);

    TF_AXIOM(metadata.size() == 1);

    for (const UsdValidationValidatorMetadata &m : metadata) {
        validatorMetadataNameSet.insert(m.name);
    }

    TF_AXIOM(validatorMetadataNameSet == expectedUsdLuxValidatorNames);
}

void ValidatePrimError(const UsdValidationError &error,
        const TfToken& expectedErrorIdentifier,
        const SdfPathVector expectedPrimPaths,
        const std::string& expectedErrorMsg,
        UsdValidationErrorType expectedErrorType =
        UsdValidationErrorType::Error)
{
    TF_AXIOM(error.GetIdentifier() == expectedErrorIdentifier);
    TF_AXIOM(error.GetType() == expectedErrorType);
    TF_AXIOM(error.GetSites().size() == expectedPrimPaths.size());
    for (size_t index = 0; index < error.GetSites().size(); ++index) {
        const UsdValidationErrorSite& errorSite = error.GetSites()[index];
        TF_AXIOM(errorSite.IsValid());
        TF_AXIOM(errorSite.IsPrim());
        TF_AXIOM(errorSite.GetPrim().GetPath() == expectedPrimPaths[index]);
    }
    TF_AXIOM(error.GetMessage() == expectedErrorMsg);
}

void
TestUsdLuxEncapsulationRulesValidator()
{
    UsdValidationRegistry &registry = UsdValidationRegistry::GetInstance();
    const UsdValidationValidator *validator = registry.GetOrLoadValidatorByName(
        UsdLuxValidatorNameTokens->encapsulationValidator);
    TF_AXIOM(validator);

    UsdStageRefPtr usdStage = UsdStage::CreateInMemory();

    // Test a LightFilter under a connectable but non-container parent, which
    // should fail validation
    UsdShadeShader::Define(usdStage, SdfPath("/Shader"));
    const UsdLuxLightFilter &lightFilter = UsdLuxLightFilter::Define(
        usdStage, SdfPath("/Shader/LightFilter"));
    {
        const UsdValidationErrorVector errors = validator->Validate(
            lightFilter.GetPrim());
        TF_AXIOM(errors.size() == 1u);

        const TfToken expectedErrorIdentifier(
            "usdLuxValidators:EncapsulationRulesValidator.ConnectableInNonContainer");
        const SdfPath expectedPrimPath("/Shader/LightFilter");
        const std::string expectedErrorMsg
            = "Connectable LightFilter </Shader/LightFilter> cannot reside "
              "under a non-Container Connectable Shader";

        ValidatePrimError(
            errors[0], expectedErrorIdentifier, {expectedPrimPath},
            expectedErrorMsg);
    }

    // Test a LightFilter under a non-connectable parent, which is under a
    // connectable and container light.
    UsdLuxSphereLight::Define(usdStage, SdfPath("/SphereLight"));
    UsdGeomScope::Define(usdStage, SdfPath("/SphereLight/Scope"));
    const UsdLuxLightFilter &lightFilter2 = UsdLuxLightFilter::Define(
        usdStage, SdfPath("/SphereLight/Scope/LightFilter"));
    {
        const UsdValidationErrorVector errors = validator->Validate(
            lightFilter2.GetPrim());
        TF_AXIOM(errors.size() == 1u);

        const TfToken expectedErrorIdentifier(
            "usdLuxValidators:EncapsulationRulesValidator."
            "InvalidConnectableHierarchy");
        const SdfPath expectedPrimPath("/SphereLight/Scope/LightFilter");
        const std::string expectedErrorMsg
            = "Connectable LightFilter </SphereLight/Scope/LightFilter> can "
              "only have Connectable Container ancestors up to SphereLight "
              "ancestor </SphereLight>, but its parent Scope is a Scope.";

        ValidatePrimError(
            errors[0], expectedErrorIdentifier, {expectedPrimPath},
            expectedErrorMsg);
    }

    // But a LightFilter under a non-connectable parent, should be just fine.
    UsdGeomScope::Define(usdStage, SdfPath("/Scope"));
    const UsdLuxLightFilter &lightFilter4 = UsdLuxLightFilter::Define(
        usdStage, SdfPath("/Scope/LightFilter"));
    {
        const UsdValidationErrorVector errors = validator->Validate(
            lightFilter4.GetPrim());
        TF_AXIOM(errors.empty());
    }

    // Test a Light under another Light -- this is allowed as in the case of
    // DomeLight and PortalLight for example
    const UsdLuxDomeLight &domeLight = UsdLuxDomeLight::Define(
        usdStage, SdfPath("/DomeLight"));
    const UsdLuxPortalLight &portalLight = UsdLuxPortalLight::Define(
        usdStage, SdfPath("/DomeLight/PortalLight"));
    {
        const UsdValidationErrorVector errors = validator->Validate(
            domeLight.GetPrim());
        TF_AXIOM(errors.empty());
    }
    {
        const UsdValidationErrorVector errors = validator->Validate(
            portalLight.GetPrim());
        TF_AXIOM(errors.empty());
    }

    // Test a LightFilter under a connectable and container light parent, which 
    // should pass validation
    UsdLuxDistantLight::Define(usdStage, SdfPath("/DistantLight"));
    const UsdLuxLightFilter &lightFilter3 = UsdLuxLightFilter::Define(
        usdStage, SdfPath("/DistantLight/LightFilter"));
    {
        const UsdValidationErrorVector errors = validator->Validate(
            lightFilter3.GetPrim());
        TF_AXIOM(errors.empty());
    }

    // Test a Light under LightFilter
    const UsdLuxDistantLight &distantLight3 = UsdLuxDistantLight::Define(
        usdStage, SdfPath("/DistantLight/LightFilter/DistantLight"));
    {
        const UsdValidationErrorVector errors = validator->Validate(
            distantLight3.GetPrim());
        TF_AXIOM(errors.size() == 1u);

        const TfToken expectedErrorIdentifier(
            "usdLuxValidators:EncapsulationRulesValidator."
            "LightUnderConnectable");
        const SdfPath expectedPrimPath("/DistantLight/LightFilter/DistantLight");
        const std::string expectedErrorMsg
            = "Light DistantLight </DistantLight/LightFilter/DistantLight> "
              "cannot have a connectable parent LightFilter "
              "</DistantLight/LightFilter>.";

        ValidatePrimError(
            errors[0], expectedErrorIdentifier, {expectedPrimPath},
            expectedErrorMsg);
    }

    // Test a LightFilter under another LightFilter
    const UsdLuxLightFilter &lightFilter5 = UsdLuxLightFilter::Define(
        usdStage, SdfPath("/DistantLight/LightFilter/LightFilter"));
    {
        const UsdValidationErrorVector errors = validator->Validate(
            lightFilter5.GetPrim());
        TF_AXIOM(errors.empty());
    }

    // Test a LightFilter under a UsdShadeNodeGraph, which is a connectable and
    // container parent, should be fine
    UsdShadeNodeGraph::Define(usdStage, SdfPath("/NodeGraph"));
    const UsdLuxLightFilter &lightFilter7 = UsdLuxLightFilter::Define(
        usdStage, SdfPath("/NodeGraph/LightFilter"));
    {
        const UsdValidationErrorVector errors = validator->Validate(
            lightFilter7.GetPrim());
        TF_AXIOM(errors.empty());
    }

    // Test a LightFilter under a Material, which is a connectable and container
    UsdShadeMaterial::Define(usdStage, SdfPath("/Material"));
    const UsdLuxLightFilter &lightFilter6 = UsdLuxLightFilter::Define(
        usdStage, SdfPath("/Material/LightFilter"));
    {
        const UsdValidationErrorVector errors = validator->Validate(
            lightFilter6.GetPrim());
        TF_AXIOM(errors.size() == 1u);

        const TfToken expectedErrorIdentifier(
            "usdLuxValidators:EncapsulationRulesValidator."
            "LightFilterUnderLightFilter");
        const SdfPath expectedPrimPath("/Material/LightFilter");

        const std::string expectedErrorMsg
            = "LightFilter </Material/LightFilter> cannot have a parent "
              "</Material> that is a UsdShadeMaterial.";

        ValidatePrimError(
            errors[0], expectedErrorIdentifier, {expectedPrimPath},
            expectedErrorMsg);
    }
}

int
main()
{
    TestUsdLuxValidators();
    TestUsdLuxEncapsulationRulesValidator();

    return EXIT_SUCCESS;
}
