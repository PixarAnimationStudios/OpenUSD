//
// Copyright 2026 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/pxr.h"
#include "pxr/base/tf/token.h"
#include "pxr/usd/sdf/path.h"
#include "pxr/usd/usd/prim.h"
#include "pxr/usd/usd/stage.h"
#include "pxr/usd/usdLod/distanceHeuristic.h"
#include "pxr/usd/usdLod/heuristic.h"
#include "pxr/usd/usdLod/overrideAPI.h"
#include "pxr/usd/usdLod/rootAPI.h"
#include "pxr/usd/usdLod/screenSizeHeuristic.h"
#include "pxr/usd/usdLod/tokens.h"
#include "pxr/usdValidation/usdLodValidators/validatorTokens.h"
#include "pxr/usdValidation/usdValidation/error.h"
#include "pxr/usdValidation/usdValidation/registry.h"
#include "pxr/usdValidation/usdValidation/validator.h"

#include <set>
#include <string>

PXR_NAMESPACE_USING_DIRECTIVE

TF_DEFINE_PRIVATE_TOKENS(_tokens,
    ((usdLodValidatorsPlugin, "usdLodValidators"))
);

// Helper used by all per-prim error assertions.
void
ValidatePrimError(const UsdValidationError &error,
                  const TfToken &expectedErrorIdentifier,
                  const SdfPathVector expectedPrimPaths,
                  const std::string &expectedErrorMsg,
                  UsdValidationErrorType expectedErrorType
                      = UsdValidationErrorType::Error)
{
    TF_AXIOM(error.GetIdentifier() == expectedErrorIdentifier);
    TF_AXIOM(error.GetType() == expectedErrorType);
    TF_AXIOM(error.GetSites().size() == expectedPrimPaths.size());
    for (size_t i = 0; i < error.GetSites().size(); ++i) {
        const UsdValidationErrorSite &site = error.GetSites()[i];
        TF_AXIOM(site.IsValid());
        TF_AXIOM(site.IsPrim());
        TF_AXIOM(site.GetPrim().GetPath() == expectedPrimPaths[i]);
    }
    TF_AXIOM(error.GetMessage() == expectedErrorMsg);
}

// Verify the expected set of validators is registered for this plugin.
void
TestUsdLodValidators()
{
    const std::set<TfToken> expectedNames = {
        UsdLodValidatorNameTokens->heuristicLodDomain,
        UsdLodValidatorNameTokens->distanceHeuristicThresholds,
        UsdLodValidatorNameTokens->screenSizeHeuristicThresholds,
        UsdLodValidatorNameTokens->lodOverrideApiMode,
        UsdLodValidatorNameTokens->lodRootApiHeuristics,
    };

    const UsdValidationRegistry &registry
        = UsdValidationRegistry::GetInstance();

    const UsdValidationValidatorMetadataVector metadata
        = registry.GetValidatorMetadataForPlugin(
            _tokens->usdLodValidatorsPlugin);

    TF_AXIOM(metadata.size() == 5);

    std::set<TfToken> registeredNames;
    for (const UsdValidationValidatorMetadata &m : metadata) {
        registeredNames.insert(m.name);
    }
    TF_AXIOM(registeredNames == expectedNames);
}

void
TestHeuristicLodDomainValidator()
{
    UsdValidationRegistry &registry = UsdValidationRegistry::GetInstance();
    const UsdValidationValidator *validator
        = registry.GetOrLoadValidatorByName(
            UsdLodValidatorNameTokens->heuristicLodDomain);
    TF_AXIOM(validator);

    UsdStageRefPtr stage = UsdStage::CreateInMemory();

    // Valid: lod:domain is set.
    const UsdLodDistanceHeuristic distHeuristic
        = UsdLodDistanceHeuristic::Define(stage, SdfPath("/DistHeuristic"));
    distHeuristic.GetLodDomainAttr().Set(UsdLodTokens->imaging);
    {
        const UsdValidationErrorVector errors
            = validator->Validate(distHeuristic.GetPrim());
        TF_AXIOM(errors.empty());
    }

    // Invalid: lod:domain is empty (default for a new prim).
    const UsdLodScreenSizeHeuristic ssHeuristic
        = UsdLodScreenSizeHeuristic::Define(stage, SdfPath("/SsHeuristic"));
    {
        const UsdValidationErrorVector errors
            = validator->Validate(ssHeuristic.GetPrim());
        TF_AXIOM(errors.size() == 1u);

        const TfToken expectedId(
            "usdLodValidators:HeuristicLodDomain.LodDomainEmpty");
        ValidatePrimError(
            errors[0], expectedId, { SdfPath("/SsHeuristic") },
            "LODHeuristic </SsHeuristic> has an empty lod:domain.");
    }
}

void
TestDistanceHeuristicThresholdsValidator()
{
    UsdValidationRegistry &registry = UsdValidationRegistry::GetInstance();
    const UsdValidationValidator *validator
        = registry.GetOrLoadValidatorByName(
            UsdLodValidatorNameTokens->distanceHeuristicThresholds);
    TF_AXIOM(validator);

    UsdStageRefPtr stage = UsdStage::CreateInMemory();
    const UsdLodDistanceHeuristic heuristic
        = UsdLodDistanceHeuristic::Define(stage, SdfPath("/Dist"));
    heuristic.GetLodDomainAttr().Set(UsdLodTokens->imaging);

    // Valid: strictly increasing thresholds, valid blendThresholds.
    heuristic.GetThresholdsAttr().Set(
        VtFloatArray({ 10.0f, 50.0f, 100.0f }));
    heuristic.GetBlendThresholdsAttr().Set(
        VtFloatArray({ 15.0f, 60.0f, 110.0f }));
    {
        const UsdValidationErrorVector errors
            = validator->Validate(heuristic.GetPrim());
        TF_AXIOM(errors.empty());
    }

    // Invalid: thresholds not strictly increasing.
    heuristic.GetThresholdsAttr().Set(
        VtFloatArray({ 10.0f, 10.0f, 100.0f }));
    heuristic.GetBlendThresholdsAttr().Set(VtFloatArray());
    {
        const UsdValidationErrorVector errors
            = validator->Validate(heuristic.GetPrim());
        TF_AXIOM(errors.size() == 1u);

        const TfToken expectedId(
            "usdLodValidators:DistanceHeuristicThresholds"
            ".ThresholdsNotStrictlyIncreasing");
        ValidatePrimError(
            errors[0], expectedId, { SdfPath("/Dist") },
            "LODDistanceHeuristic </Dist>: thresholds[1] is not "
            "strictly increasing.");
    }

    // Invalid: blendThreshold below corresponding threshold.
    heuristic.GetThresholdsAttr().Set(
        VtFloatArray({ 10.0f, 50.0f, 100.0f }));
    heuristic.GetBlendThresholdsAttr().Set(
        VtFloatArray({ 5.0f, 60.0f, 110.0f }));
    {
        const UsdValidationErrorVector errors
            = validator->Validate(heuristic.GetPrim());
        TF_AXIOM(errors.size() == 1u);

        const TfToken expectedId(
            "usdLodValidators:DistanceHeuristicThresholds"
            ".BlendThresholdBelowThreshold");
        ValidatePrimError(
            errors[0], expectedId, { SdfPath("/Dist") },
            "LODDistanceHeuristic </Dist>: blendThresholds[0] (5) "
            "< thresholds[0] (10).");
    }

    // Invalid: blendThreshold above next threshold.
    heuristic.GetThresholdsAttr().Set(
        VtFloatArray({ 10.0f, 50.0f, 100.0f }));
    heuristic.GetBlendThresholdsAttr().Set(
        VtFloatArray({ 15.0f, 60.0f, 110.0f }));
    // Make thresholds[1] < blendThresholds[0]
    heuristic.GetThresholdsAttr().Set(
        VtFloatArray({ 10.0f, 12.0f, 100.0f }));
    heuristic.GetBlendThresholdsAttr().Set(
        VtFloatArray({ 15.0f, 20.0f, 110.0f }));
    {
        const UsdValidationErrorVector errors
            = validator->Validate(heuristic.GetPrim());
        TF_AXIOM(errors.size() == 1u);

        const TfToken expectedId(
            "usdLodValidators:DistanceHeuristicThresholds"
            ".BlendThresholdAboveNextThreshold");
        ValidatePrimError(
            errors[0], expectedId, { SdfPath("/Dist") },
            "LODDistanceHeuristic </Dist>: blendThresholds[0] (15) "
            "> thresholds[1] (12).");
    }
}

void
TestScreenSizeHeuristicThresholdsValidator()
{
    UsdValidationRegistry &registry = UsdValidationRegistry::GetInstance();
    const UsdValidationValidator *validator
        = registry.GetOrLoadValidatorByName(
            UsdLodValidatorNameTokens->screenSizeHeuristicThresholds);
    TF_AXIOM(validator);

    UsdStageRefPtr stage = UsdStage::CreateInMemory();
    const UsdLodScreenSizeHeuristic heuristic
        = UsdLodScreenSizeHeuristic::Define(stage, SdfPath("/SS"));
    heuristic.GetLodDomainAttr().Set(UsdLodTokens->imaging);

    // Valid: strictly decreasing thresholds, valid blendThresholds,
    // valid projectionMethod, valid extent.
    heuristic.GetThresholdsAttr().Set(
        VtFloatArray({ 0.25f, 0.10f, 0.025f }));
    heuristic.GetBlendThresholdsAttr().Set(
        VtFloatArray({ 0.20f, 0.08f, 0.020f }));
    heuristic.GetProjectionMethodAttr().Set(UsdLodTokens->projectedSphere);
    heuristic.GetExtentAttr().Set(
        VtVec3fArray({ GfVec3f(-1,-1,-1), GfVec3f(1,1,1) }));
    {
        const UsdValidationErrorVector errors
            = validator->Validate(heuristic.GetPrim());
        TF_AXIOM(errors.empty());
    }

    // Invalid: thresholds not strictly decreasing.
    heuristic.GetThresholdsAttr().Set(
        VtFloatArray({ 0.25f, 0.25f, 0.025f }));
    heuristic.GetBlendThresholdsAttr().Set(VtFloatArray());
    {
        const UsdValidationErrorVector errors
            = validator->Validate(heuristic.GetPrim());
        TF_AXIOM(errors.size() == 1u);

        const TfToken expectedId(
            "usdLodValidators:ScreenSizeHeuristicThresholds"
            ".ThresholdsNotStrictlyDecreasing");
        ValidatePrimError(
            errors[0], expectedId, { SdfPath("/SS") },
            "LODScreenSizeHeuristic </SS>: thresholds[1] is not "
            "strictly decreasing.");
    }

    // Invalid: blendThreshold above corresponding threshold.
    heuristic.GetThresholdsAttr().Set(
        VtFloatArray({ 0.25f, 0.10f, 0.025f }));
    heuristic.GetBlendThresholdsAttr().Set(
        VtFloatArray({ 0.30f, 0.08f, 0.020f }));
    {
        const UsdValidationErrorVector errors
            = validator->Validate(heuristic.GetPrim());
        TF_AXIOM(errors.size() == 1u);

        const TfToken expectedId(
            "usdLodValidators:ScreenSizeHeuristicThresholds"
            ".BlendThresholdAboveThreshold");
        ValidatePrimError(
            errors[0], expectedId, { SdfPath("/SS") },
            "LODScreenSizeHeuristic </SS>: blendThresholds[0] (0.3) "
            "> thresholds[0] (0.25).");
    }

    // Invalid: blendThreshold below next threshold.
    // blendThresholds[1]=0.021 < thresholds[2]=0.025 violates the rule
    // that blendThresholds[i] >= thresholds[i+1].
    heuristic.GetThresholdsAttr().Set(
        VtFloatArray({ 0.25f, 0.10f, 0.025f }));
    heuristic.GetBlendThresholdsAttr().Set(
        VtFloatArray({ 0.20f, 0.021f, 0.020f }));
    {
        const UsdValidationErrorVector errors
            = validator->Validate(heuristic.GetPrim());
        TF_AXIOM(errors.size() == 1u);

        const TfToken expectedId(
            "usdLodValidators:ScreenSizeHeuristicThresholds"
            ".BlendThresholdBelowNextThreshold");
        ValidatePrimError(
            errors[0], expectedId, { SdfPath("/SS") },
            "LODScreenSizeHeuristic </SS>: blendThresholds[1] (0.021) "
            "< thresholds[2] (0.025).");
    }

    // Restore valid thresholds before testing other attributes.
    heuristic.GetThresholdsAttr().Set(
        VtFloatArray({ 0.25f, 0.10f, 0.025f }));
    heuristic.GetBlendThresholdsAttr().Set(VtFloatArray());

    // Invalid: unrecognized projectionMethod.
    heuristic.GetProjectionMethodAttr().Set(TfToken("badMethod"));
    {
        const UsdValidationErrorVector errors
            = validator->Validate(heuristic.GetPrim());
        TF_AXIOM(errors.size() == 1u);

        const TfToken expectedId(
            "usdLodValidators:ScreenSizeHeuristicThresholds"
            ".InvalidProjectionMethod");
        ValidatePrimError(
            errors[0], expectedId, { SdfPath("/SS") },
            "LODScreenSizeHeuristic </SS>: projectionMethod 'badMethod' "
            "is not a recognized value (expected 'projectedSphere' or "
            "'projectedExtent').");
    }
    heuristic.GetProjectionMethodAttr().Set(UsdLodTokens->projectedSphere);

    // Invalid: extent with 1 element.
    heuristic.GetExtentAttr().Set(
        VtVec3fArray({ GfVec3f(-1,-1,-1) }));
    {
        const UsdValidationErrorVector errors
            = validator->Validate(heuristic.GetPrim());
        TF_AXIOM(errors.size() == 1u);

        const TfToken expectedId(
            "usdLodValidators:ScreenSizeHeuristicThresholds"
            ".InvalidExtentSize");
        ValidatePrimError(
            errors[0], expectedId, { SdfPath("/SS") },
            "LODScreenSizeHeuristic </SS>: extent has 1 element(s) "
            "but must be empty or have exactly 2 elements.");
    }
}

void
TestLodOverrideApiModeValidator()
{
    UsdValidationRegistry &registry = UsdValidationRegistry::GetInstance();
    const UsdValidationValidator *validator
        = registry.GetOrLoadValidatorByName(
            UsdLodValidatorNameTokens->lodOverrideApiMode);
    TF_AXIOM(validator);

    UsdStageRefPtr stage = UsdStage::CreateInMemory();
    UsdPrim prim = stage->DefinePrim(SdfPath("/Override"));
    UsdLodOverrideAPI::Apply(prim);
    const UsdLodOverrideAPI overrideAPI(prim);

    // Valid: mode is "inherited" (default).
    overrideAPI.GetLodOverrideModeAttr().Set(UsdLodTokens->inherited);
    {
        const UsdValidationErrorVector errors = validator->Validate(prim);
        TF_AXIOM(errors.empty());
    }

    // Valid: mode is "indexedLOD" and index is readable.
    overrideAPI.GetLodOverrideModeAttr().Set(UsdLodTokens->indexedLOD);
    overrideAPI.GetLodOverrideIndexAttr().Set(1.0f);
    {
        const UsdValidationErrorVector errors = validator->Validate(prim);
        TF_AXIOM(errors.empty());
    }

    // Invalid: unrecognized mode token.
    overrideAPI.GetLodOverrideModeAttr().Set(TfToken("badMode"));
    {
        const UsdValidationErrorVector errors = validator->Validate(prim);
        TF_AXIOM(errors.size() == 1u);

        const TfToken expectedId(
            "usdLodValidators:LodOverrideApiMode.InvalidOverrideMode");
        ValidatePrimError(
            errors[0], expectedId, { SdfPath("/Override") },
            "LODOverrideAPI prim </Override>: lod:override:mode 'badMode' "
            "is not a recognized value.");
    }
}

void
TestLodRootApiHeuristicsValidator()
{
    UsdValidationRegistry &registry = UsdValidationRegistry::GetInstance();
    const UsdValidationValidator *validator
        = registry.GetOrLoadValidatorByName(
            UsdLodValidatorNameTokens->lodRootApiHeuristics);
    TF_AXIOM(validator);

    UsdStageRefPtr stage = UsdStage::CreateInMemory();
    UsdPrim root = stage->DefinePrim(SdfPath("/Root"));
    UsdLodRootAPI::Apply(root);
    const UsdLodRootAPI rootAPI(root);

    // Define a valid heuristic prim.
    const UsdLodDistanceHeuristic heuristic
        = UsdLodDistanceHeuristic::Define(stage, SdfPath("/Heuristic"));

    // Valid: target is a UsdLodHeuristic prim.
    rootAPI.GetLodHeuristicsRel().SetTargets(
        { SdfPath("/Heuristic") });
    {
        const UsdValidationErrorVector errors = validator->Validate(root);
        TF_AXIOM(errors.empty());
    }

    // Invalid: target path does not exist.
    rootAPI.GetLodHeuristicsRel().SetTargets(
        { SdfPath("/Missing") });
    {
        const UsdValidationErrorVector errors = validator->Validate(root);
        TF_AXIOM(errors.size() == 1u);

        const TfToken expectedId(
            "usdLodValidators:LodRootApiHeuristics"
            ".InvalidHeuristicTarget");
        ValidatePrimError(
            errors[0], expectedId, { SdfPath("/Root") },
            "LODRootAPI prim </Root>: lod:heuristics target </Missing> "
            "does not resolve to a UsdLodHeuristic prim.");
    }

    // Invalid: target exists but is not a UsdLodHeuristic.
    stage->DefinePrim(SdfPath("/NotAHeuristic"));
    rootAPI.GetLodHeuristicsRel().SetTargets(
        { SdfPath("/NotAHeuristic") });
    {
        const UsdValidationErrorVector errors = validator->Validate(root);
        TF_AXIOM(errors.size() == 1u);

        const TfToken expectedId(
            "usdLodValidators:LodRootApiHeuristics"
            ".InvalidHeuristicTarget");
        ValidatePrimError(
            errors[0], expectedId, { SdfPath("/Root") },
            "LODRootAPI prim </Root>: lod:heuristics target </NotAHeuristic> "
            "does not resolve to a UsdLodHeuristic prim.");
    }
}

int
main()
{
    TestUsdLodValidators();
    TestHeuristicLodDomainValidator();
    TestDistanceHeuristicThresholdsValidator();
    TestScreenSizeHeuristicThresholdsValidator();
    TestLodOverrideApiModeValidator();
    TestLodRootApiHeuristicsValidator();

    return EXIT_SUCCESS;
}
