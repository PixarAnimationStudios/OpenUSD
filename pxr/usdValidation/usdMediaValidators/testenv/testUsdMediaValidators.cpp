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
#include "pxr/usd/usd/editContext.h"
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
#include <string>

PXR_NAMESPACE_USING_DIRECTIVE

TF_DEFINE_PRIVATE_TOKENS(_tokens,
    ((usdMediaValidatorsPlugin, "usdMediaValidators"))
);

static const UsdValidationValidator *
_GetValidator(const TfToken &name)
{
    const UsdValidationValidator *validator =
        UsdValidationRegistry::GetInstance().GetOrLoadValidatorByName(name);
    TF_AXIOM(validator);
    return validator;
}

static TfToken
_ErrorId(const TfToken &validatorName, const TfToken &errorName)
{
    return TfToken(validatorName.GetString() + "." + errorName.GetString());
}

void
TestUsdMediaValidatorsRegistered()
{
    const UsdValidationValidatorMetadataVector metadata =
        UsdValidationRegistry::GetInstance().GetValidatorMetadataForPlugin(
            _tokens->usdMediaValidatorsPlugin);

    std::set<TfToken> names;
    for (const UsdValidationValidatorMetadata &m : metadata) {
        names.insert(m.name);
    }
    TF_AXIOM(names == std::set<TfToken>({
        UsdMediaValidatorNameTokens->shadowedOrDuplicateAuthorship,
        UsdMediaValidatorNameTokens->authorshipInputsPaired }));
}

void
TestShadowedOrDuplicateCleanPrim()
{
    const UsdValidationValidator *validator = _GetValidator(
        UsdMediaValidatorNameTokens->shadowedOrDuplicateAuthorship);

    UsdStageRefPtr stage = UsdStage::CreateInMemory();
    UsdPrim prim = stage->DefinePrim(SdfPath("/Bunny"));
    UsdMediaAuthorshipAPI::Apply(prim, TfToken("hunyuan3d"));

    TF_AXIOM(validator->Validate(prim).empty());
}

// An explicit apiSchemas list on a referencing prim can shadow an
// application coming from the reference target.
void
TestShadowedApplicationIsFlagged()
{
    const UsdValidationValidator *validator = _GetValidator(
        UsdMediaValidatorNameTokens->shadowedOrDuplicateAuthorship);

    SdfLayerRefPtr asset = SdfLayer::CreateAnonymous("asset.usda");
    UsdStageRefPtr assetStage = UsdStage::Open(asset);
    UsdPrim rock = assetStage->DefinePrim(SdfPath("/RockLarge"));
    UsdMediaAuthorshipAPI::Apply(rock, TfToken("rockmaker"));
    assetStage->SetDefaultPrim(rock);

    SdfLayerRefPtr root = SdfLayer::CreateAnonymous("scene.usda");
    UsdStageRefPtr stage = UsdStage::Open(root);
    UsdPrim referencing = stage->DefinePrim(SdfPath("/World/Rock_1"));
    referencing.GetReferences().AddReference(asset->GetIdentifier());

    root->GetPrimAtPath(SdfPath("/World/Rock_1"))->SetInfo(
        UsdTokens->apiSchemas,
        VtValue(SdfTokenListOp::CreateExplicit(
            { TfToken("AuthorshipAPI:layout") })));

    const UsdValidationErrorVector errors = validator->Validate(referencing);
    TF_AXIOM(errors.size() == 1u);
    TF_AXIOM(errors[0].GetIdentifier() == _ErrorId(
        UsdMediaValidatorNameTokens->shadowedOrDuplicateAuthorship,
        UsdMediaValidationErrorNameTokens->shadowedAuthorshipApplication));
    TF_AXIOM(errors[0].GetType() == UsdValidationErrorType::Warn);
}

// The same instance applied in two contributing layers is one composed
// record, but two contributing specs.
void
TestDuplicateApplicationIsFlagged()
{
    const UsdValidationValidator *validator = _GetValidator(
        UsdMediaValidatorNameTokens->shadowedOrDuplicateAuthorship);

    SdfLayerRefPtr weak = SdfLayer::CreateAnonymous("weak.usda");
    UsdStageRefPtr weakStage = UsdStage::Open(weak);
    UsdMediaAuthorshipAPI::Apply(
        weakStage->DefinePrim(SdfPath("/Bunny")), TfToken("blender"));

    SdfLayerRefPtr strong = SdfLayer::CreateAnonymous("strong.usda");
    strong->InsertSubLayerPath(weak->GetIdentifier());
    UsdStageRefPtr stage = UsdStage::Open(strong);
    UsdPrim prim = stage->GetPrimAtPath(SdfPath("/Bunny"));
    UsdMediaAuthorshipAPI::Apply(prim, TfToken("blender"));

    const UsdValidationErrorVector errors = validator->Validate(prim);
    TF_AXIOM(errors.size() == 1u);
    TF_AXIOM(errors[0].GetIdentifier() == _ErrorId(
        UsdMediaValidatorNameTokens->shadowedOrDuplicateAuthorship,
        UsdMediaValidationErrorNameTokens->duplicateAuthorshipApplication));
}

void
TestInputsPaired()
{
    const UsdValidationValidator *validator = _GetValidator(
        UsdMediaValidatorNameTokens->authorshipInputsPaired);
    const TfToken &name = UsdMediaValidatorNameTokens->authorshipInputsPaired;

    UsdStageRefPtr stage = UsdStage::CreateInMemory();
    UsdPrim prim = stage->DefinePrim(SdfPath("/Bunny"));
    UsdMediaAuthorshipAPI record =
        UsdMediaAuthorshipAPI::Apply(prim, TfToken("gen"));

    // Neither authored.
    TF_AXIOM(validator->Validate(prim).empty());

    // Names without values.
    record.CreateInputNamesAttr(VtValue(VtStringArray({"prompt", "seed"})));
    {
        const UsdValidationErrorVector errors = validator->Validate(prim);
        TF_AXIOM(errors.size() == 1u);
        TF_AXIOM(errors[0].GetIdentifier() == _ErrorId(name,
            UsdMediaValidationErrorNameTokens->unpairedAuthorshipInputs));
        TF_AXIOM(errors[0].GetType() == UsdValidationErrorType::Error);
    }

    // Mismatched lengths.
    record.CreateInputValuesAttr(VtValue(VtStringArray({"A fluffy bunny"})));
    {
        const UsdValidationErrorVector errors = validator->Validate(prim);
        TF_AXIOM(errors.size() == 1u);
        TF_AXIOM(errors[0].GetIdentifier() == _ErrorId(name,
            UsdMediaValidationErrorNameTokens->mismatchedAuthorshipInputs));
    }

    // Matched, in the same spec.
    record.GetInputValuesAttr().Set(
        VtStringArray({"A fluffy bunny", "1234"}));
    TF_AXIOM(validator->Validate(prim).empty());
}

// Matching lengths resolved from different layers can still be misaligned.
void
TestInputsFromDifferentLayers()
{
    const UsdValidationValidator *validator = _GetValidator(
        UsdMediaValidatorNameTokens->authorshipInputsPaired);

    SdfLayerRefPtr weak = SdfLayer::CreateAnonymous("weak.usda");
    UsdStageRefPtr weakStage = UsdStage::Open(weak);
    UsdMediaAuthorshipAPI weakRecord = UsdMediaAuthorshipAPI::Apply(
        weakStage->DefinePrim(SdfPath("/Bunny")), TfToken("gen"));
    weakRecord.CreateInputNamesAttr(VtValue(VtStringArray({"prompt"})));
    weakRecord.CreateInputValuesAttr(VtValue(VtStringArray({"a bunny"})));

    SdfLayerRefPtr strong = SdfLayer::CreateAnonymous("strong.usda");
    strong->InsertSubLayerPath(weak->GetIdentifier());
    UsdStageRefPtr stage = UsdStage::Open(strong);
    UsdPrim prim = stage->GetPrimAtPath(SdfPath("/Bunny"));
    UsdMediaAuthorshipAPI(prim, TfToken("gen"))
        .CreateInputNamesAttr(VtValue(VtStringArray({"seed"})));

    const UsdValidationErrorVector errors = validator->Validate(prim);
    TF_AXIOM(errors.size() == 1u);
    TF_AXIOM(errors[0].GetIdentifier() == _ErrorId(
        UsdMediaValidatorNameTokens->authorshipInputsPaired,
        UsdMediaValidationErrorNameTokens
            ->authorshipInputsFromDifferentSpecs));
    TF_AXIOM(errors[0].GetType() == UsdValidationErrorType::Warn);
}

int
main()
{
    TestUsdMediaValidatorsRegistered();
    TestShadowedOrDuplicateCleanPrim();
    TestShadowedApplicationIsFlagged();
    TestDuplicateApplicationIsFlagged();
    TestInputsPaired();
    TestInputsFromDifferentLayers();

    return EXIT_SUCCESS;
}
