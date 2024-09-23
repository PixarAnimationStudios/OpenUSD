//
// Copyright 2024 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/usd/usd/validationRegistry.h"
#include "pxr/usd/usd/validationError.h"
#include <pxr/usd/usdSkel/validatorTokens.h>
#include <pxr/usd/usdSkel/root.h>
#include <pxr/usd/usdSkel/bindingAPI.h>
#include <pxr/usd/usdGeom/mesh.h>
#include <pxr/usd/usdGeom/primvarsAPI.h>

PXR_NAMESPACE_USING_DIRECTIVE

void
TestUsdSkelValidators()
{
    // This should be updated with every new validator added with
    // UsdSkelValidators keyword.
    UsdValidationRegistry &registry = UsdValidationRegistry::GetInstance();
    UsdValidatorMetadataVector metadata =
            registry.GetValidatorMetadataForKeyword(
                    UsdSkelValidatorKeywordTokens->UsdSkelValidators);
    // Since other validators can be registered with a UsdSkelValidators
    // keyword, our validators registered in usdSkel are a subset of the entire
    // set.
    std::set<TfToken> validatorMetadataNameSet;
    for (const UsdValidatorMetadata &metadata : metadata) {
        validatorMetadataNameSet.insert(metadata.name);
    }

    const std::set<TfToken> expectedValidatorNames =
            {UsdSkelValidatorNameTokens->skelBindingApiAppliedValidator,
             UsdSkelValidatorNameTokens->skelBindingApiValidator};

    TF_AXIOM(std::includes(validatorMetadataNameSet.begin(),
                           validatorMetadataNameSet.end(),
                           expectedValidatorNames.begin(),
                           expectedValidatorNames.end()));
}

void
TestUsdSkelBindingApiAppliedValidator()
{
    // Verify that the validator exists.
    UsdValidationRegistry &registry = UsdValidationRegistry::GetInstance();
    const UsdValidator *validator = registry.GetOrLoadValidatorByName(
            UsdSkelValidatorNameTokens->skelBindingApiAppliedValidator);
    TF_AXIOM(validator);

    // Create Stage and mesh with a skel binding property
    UsdStageRefPtr usdStage = UsdStage::CreateInMemory();
    UsdGeomMesh mesh = UsdGeomMesh::Define(usdStage, SdfPath("/SkelRoot/Mesh"));
    UsdGeomPrimvarsAPI primvarsApi(mesh);
    UsdGeomPrimvar jointIndicesPrimvar = primvarsApi.CreatePrimvar(
        TfToken("skel:jointIndices"), SdfValueTypeNames->IntArray, 
        UsdGeomTokens->vertex);
    jointIndicesPrimvar.Set(VtIntArray{0, 1, 2});

    UsdValidationErrorVector errors = validator->Validate(mesh.GetPrim());

    TfToken expectedErrorIdentifier(
        "usdSkel:SkelBindingApiAppliedValidator.MissingSkelBindingAPI");
    // Verify the error for not having the SkelBindingAPI schema applied is 
    // present.
    TF_AXIOM(errors.size() == 1);
    TF_AXIOM(errors[0].GetIdentifier() == expectedErrorIdentifier);
    TF_AXIOM(errors[0].GetType() == UsdValidationErrorType::Error);
    TF_AXIOM(errors[0].GetSites().size() == 1);
    TF_AXIOM(errors[0].GetSites()[0].IsValid());
    TF_AXIOM(errors[0].GetSites()[0].IsPrim());
    TF_AXIOM(errors[0].GetSites()[0].GetPrim().GetPath() ==
             SdfPath("/SkelRoot/Mesh"));
    std::string expectedErrorMsg = ("Found a UsdSkelBinding property "
                                    "(primvars:skel:jointIndices), but no "
                                    "SkelBindingAPI applied on the prim "
                                    "</SkelRoot/Mesh>.");
    TF_AXIOM(errors[0].GetMessage() == expectedErrorMsg);

    // Apply the SkelBindingAPI.
    UsdSkelBindingAPI skelBindingApi = UsdSkelBindingAPI::Apply(mesh.GetPrim());
    errors = validator->Validate(mesh.GetPrim());
    // Verify all errors are gone
    TF_AXIOM(errors.empty());

    const UsdValidator *skelBindingApiValidator = 
        registry.GetOrLoadValidatorByName(
            UsdSkelValidatorNameTokens->skelBindingApiValidator);
    errors = skelBindingApiValidator->Validate(mesh.GetPrim());

    expectedErrorIdentifier = TfToken(
        "usdSkel:SkelBindingApiValidator.InvalidSkelBindingAPIApply");
    // Verify the error for not having a SkelRoot parenting a prim with the 
    // SkelBindingAPI applied.
    TF_AXIOM(errors.size() == 1);
    TF_AXIOM(errors[0].GetIdentifier() == expectedErrorIdentifier);
    TF_AXIOM(errors[0].GetType() == UsdValidationErrorType::Error);
    TF_AXIOM(errors[0].GetSites().size() == 1);
    TF_AXIOM(errors[0].GetSites()[0].IsValid());
    TF_AXIOM(errors[0].GetSites()[0].IsPrim());
    TF_AXIOM(errors[0].GetSites()[0].GetPrim().GetPath() ==
             SdfPath("/SkelRoot/Mesh"));
    expectedErrorMsg = ("UsdSkelBindingAPI applied on prim: </SkelRoot/Mesh>, "
                "which is not of type SkelRoot or is not rooted at a prim "
                "of type SkelRoot, as required by the UsdSkel schema.");
    const std::string message = errors[0].GetMessage();
    TF_AXIOM(errors[0].GetMessage() == expectedErrorMsg);

    // Add SkelRoot as a parent to the mesh.
    UsdSkelRoot skelRoot = UsdSkelRoot::Define(usdStage, SdfPath("/SkelRoot"));
    errors = skelBindingApiValidator->Validate(mesh.GetPrim());

    // Verify all errors are gone
    TF_AXIOM(errors.empty());
}

int
main()
{
    TestUsdSkelValidators();
    TestUsdSkelBindingApiAppliedValidator();
    printf("OK\n");
    return EXIT_SUCCESS;
};
