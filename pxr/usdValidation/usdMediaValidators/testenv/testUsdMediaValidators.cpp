//
// Copyright 2026 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/pxr.h"
#include "pxr/base/tf/token.h"
#include "pxr/usd/sdf/layer.h"
#include "pxr/usd/sdf/listOp.h"
#include "pxr/usd/sdf/path.h"
#include "pxr/usd/sdf/primSpec.h"
#include "pxr/usd/usd/prim.h"
#include "pxr/usd/usd/references.h"
#include "pxr/usd/usd/stage.h"
#include "pxr/usd/usd/tokens.h"
#include "pxr/usd/usdMedia/authorshipAPI.h"
#include "pxr/usdValidation/usdMediaValidators/validatorTokens.h"
#include "pxr/usdValidation/usdValidation/error.h"
#include "pxr/usdValidation/usdValidation/registry.h"
#include "pxr/usdValidation/usdValidation/validator.h"

#include <cstdlib>
#include <set>

PXR_NAMESPACE_USING_DIRECTIVE

TF_DEFINE_PRIVATE_TOKENS(_tokens,
    ((usdMediaValidatorsPlugin, "usdMediaValidators"))
);

// Verify the expected validator is registered for this plugin.
void
TestUsdMediaValidatorsRegistered()
{
    const UsdValidationRegistry &registry
        = UsdValidationRegistry::GetInstance();

    const UsdValidationValidatorMetadataVector metadata
        = registry.GetValidatorMetadataForPlugin(
            _tokens->usdMediaValidatorsPlugin);

    TF_AXIOM(metadata.size() == 1);
    TF_AXIOM(metadata[0].name ==
             UsdMediaValidatorNameTokens->shadowedOrClobberedAuthorship);
}

void
TestCleanStageHasNoErrors()
{
    UsdValidationRegistry &registry = UsdValidationRegistry::GetInstance();
    const UsdValidationValidator *validator
        = registry.GetOrLoadValidatorByName(
            UsdMediaValidatorNameTokens->shadowedOrClobberedAuthorship);
    TF_AXIOM(validator);

    UsdStageRefPtr stage = UsdStage::CreateInMemory();
    UsdPrim prim = stage->DefinePrim(SdfPath("/Bunny"));
    UsdMediaAuthorshipAPI::Apply(prim, TfToken("hunyuan3d"));

    const UsdValidationErrorVector errors = validator->Validate(stage);
    TF_AXIOM(errors.empty());
}

// An explicit apiSchemas list on a referencing prim can shadow an
// application coming from the reference target.
void
TestShadowedApplicationIsFlagged()
{
    UsdValidationRegistry &registry = UsdValidationRegistry::GetInstance();
    const UsdValidationValidator *validator
        = registry.GetOrLoadValidatorByName(
            UsdMediaValidatorNameTokens->shadowedOrClobberedAuthorship);
    TF_AXIOM(validator);

    SdfLayerRefPtr asset = SdfLayer::CreateAnonymous("asset.usda");
    UsdStageRefPtr assetStage = UsdStage::Open(asset);
    UsdPrim rock = assetStage->DefinePrim(SdfPath("/RockLarge"));
    UsdMediaAuthorshipAPI::Apply(rock, TfToken("rockmaker"));
    assetStage->SetDefaultPrim(rock);

    SdfLayerRefPtr root = SdfLayer::CreateAnonymous("scene.usda");
    UsdStageRefPtr stage = UsdStage::Open(root);
    UsdPrim referencing = stage->DefinePrim(SdfPath("/World/Rock_1"));
    referencing.GetReferences().AddReference(asset->GetIdentifier());

    const SdfPrimSpecHandle primSpec =
        root->GetPrimAtPath(SdfPath("/World/Rock_1"));
    primSpec->SetInfo(
        UsdTokens->apiSchemas,
        VtValue(SdfTokenListOp::CreateExplicit(
            { TfToken("AuthorshipAPI:layout") })));

    const UsdValidationErrorVector errors = validator->Validate(stage);
    TF_AXIOM(errors.size() == 1u);
    const TfToken expectedId(
        "usdMediaValidators:ShadowedOrClobberedAuthorship"
        ".ShadowedAuthorshipApplication");
    TF_AXIOM(errors[0].GetIdentifier() == expectedId);
    TF_AXIOM(errors[0].GetType() == UsdValidationErrorType::Warn);
}

// The same instance name authored in two contributing layers is one
// composed record, but two contributing specs.
void
TestClobberedInstanceNameIsFlagged()
{
    UsdValidationRegistry &registry = UsdValidationRegistry::GetInstance();
    const UsdValidationValidator *validator
        = registry.GetOrLoadValidatorByName(
            UsdMediaValidatorNameTokens->shadowedOrClobberedAuthorship);
    TF_AXIOM(validator);

    SdfLayerRefPtr weak = SdfLayer::CreateAnonymous("weak.usda");
    UsdStageRefPtr weakStage = UsdStage::Open(weak);
    UsdPrim weakPrim = weakStage->DefinePrim(SdfPath("/Bunny"));
    UsdMediaAuthorshipAPI::Apply(weakPrim, TfToken("blender"));

    SdfLayerRefPtr strong = SdfLayer::CreateAnonymous("strong.usda");
    strong->InsertSubLayerPath(weak->GetIdentifier());
    UsdStageRefPtr stage = UsdStage::Open(strong);
    UsdPrim prim = stage->GetPrimAtPath(SdfPath("/Bunny"));
    UsdMediaAuthorshipAPI::Apply(prim, TfToken("blender"));

    const UsdValidationErrorVector errors = validator->Validate(stage);
    TF_AXIOM(errors.size() == 1u);
    const TfToken expectedId(
        "usdMediaValidators:ShadowedOrClobberedAuthorship"
        ".ClobberedAuthorshipInstanceName");
    TF_AXIOM(errors[0].GetIdentifier() == expectedId);
}

int
main()
{
    TestUsdMediaValidatorsRegistered();
    TestCleanStageHasNoErrors();
    TestShadowedApplicationIsFlagged();
    TestClobberedInstanceNameIsFlagged();

    return EXIT_SUCCESS;
}
