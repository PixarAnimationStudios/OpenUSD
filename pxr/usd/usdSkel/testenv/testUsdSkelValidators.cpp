//
// Copyright 2024 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/pxr.h"
#include "pxr/base/tf/token.h"
#include "pxr/usd/sdr/registry.h"
#include "pxr/usd/usd/validationRegistry.h"
#include "pxr/usd/usd/validationError.h"
#include "pxr/usd/usd/validator.h"
#include <pxr/usd/usdSkel/validatorTokens.h>
#include <pxr/usd/usdSkel/root.h>
#include <pxr/usd/usdSkel/bindingAPI.h>
#include <pxr/usd/usdGeom/mesh.h>
#include <pxr/usd/usdGeom/primvarsAPI.h>

#include <algorithm>

PXR_NAMESPACE_USING_DIRECTIVE

void
TestUsdSkelBindingAPIChecker()
{
    // Verify validator exists
    UsdValidationRegistry &registry = UsdValidationRegistry::GetInstance();
    const UsdValidator *validator = registry.GetOrLoadValidatorByName(
            UsdSkelValidatorNameTokens->skelBindingApiAppliedChecker);
    TF_AXIOM(validator);

    // Create Stage and mesh with a skel binding property
    UsdStageRefPtr usdStage = UsdStage::CreateNew("/Users/andrewbeers/test.usda");
    UsdGeomMesh mesh = UsdGeomMesh::Define(usdStage, SdfPath("/SkelRoot/Mesh"));
    UsdGeomPrimvarsAPI primvarsApi(mesh);
    UsdGeomPrimvar jointIndicesPrimvar = primvarsApi.CreatePrimvar(TfToken("skel:jointIndices"), SdfValueTypeNames->IntArray, UsdGeomTokens->vertex);
    jointIndicesPrimvar.Set(VtIntArray{0, 1, 2});

    UsdValidationErrorVector errors = validator->Validate(mesh.GetPrim());

    // test and verify error for not having a skel binding api applied
    TF_AXIOM(errors.size() == 1);
    TF_AXIOM(errors[0].GetType() == UsdValidationErrorType::Error);
    TF_AXIOM(errors[0].GetSites().size() == 1);
    TF_AXIOM(errors[0].GetSites()[0].IsValid());
    TF_AXIOM(errors[0].GetSites()[0].IsPrim());
    TF_AXIOM(errors[0].GetSites()[0].GetPrim().GetPath() ==
             SdfPath("/SkelRoot/Mesh"));
    std::string expectedErrorMsg = "Found a UsdSkelBinding property (primvars:skel:jointIndices)" \
                                    ", but no SkelBindingAPI applied on the prim </SkelRoot/Mesh>." \
                                    "(fails 'SkelBindingAPIAppliedChecker')";
    TF_AXIOM(errors[0].GetMessage() == expectedErrorMsg);

    // apply skel binding api
    UsdSkelBindingAPI skelBindingApi = UsdSkelBindingAPI::Apply(mesh.GetPrim());

    errors = validator->Validate(mesh.GetPrim());

    // test and verify error for not having a skel root parenting the mesh
    TF_AXIOM(errors.size() == 1);
    TF_AXIOM(errors[0].GetType() == UsdValidationErrorType::Error);
    TF_AXIOM(errors[0].GetSites().size() == 1);
    TF_AXIOM(errors[0].GetSites()[0].IsValid());
    TF_AXIOM(errors[0].GetSites()[0].IsPrim());
    TF_AXIOM(errors[0].GetSites()[0].GetPrim().GetPath() ==
             SdfPath("/SkelRoot/Mesh"));
    expectedErrorMsg = "UsdSkelBindingAPI applied on prim: </SkelRoot/Mesh>, " \
                "which is not of type SkelRoot or is not rooted at a prim " \
                "of type SkelRoot, as required by the UsdSkel schema.(fails 'SkelBindingAPIAppliedChecker')";
    const std::string message = errors[0].GetMessage();
    TF_AXIOM(errors[0].GetMessage() == expectedErrorMsg);

    // add skel root as parent
    UsdSkelRoot skelRoot = UsdSkelRoot::Define(usdStage, SdfPath("/SkelRoot"));
    errors = validator->Validate(mesh.GetPrim());

    // verify 0 errors
    TF_AXIOM(errors.size() == 0);
}

int
main()
{
    TestUsdSkelBindingAPIChecker();
    printf("OK\n");
    return EXIT_SUCCESS;
};