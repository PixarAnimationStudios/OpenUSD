//
// Copyright 2026 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/base/tf/stringUtils.h"
#include "pxr/base/tf/token.h"
#include "pxr/base/vt/array.h"
#include "pxr/usd/usd/prim.h"
#include "pxr/usd/usd/relationship.h"
#include "pxr/usd/usdLod/distanceHeuristic.h"
#include "pxr/usd/usdLod/heuristic.h"
#include "pxr/usd/usdLod/overrideAPI.h"
#include "pxr/usd/usdLod/rootAPI.h"
#include "pxr/usd/usdLod/screenSizeHeuristic.h"
#include "pxr/usd/usdLod/tokens.h"
#include "pxr/usdValidation/usdLodValidators/validatorTokens.h"
#include "pxr/usdValidation/usdValidation/error.h"
#include "pxr/usdValidation/usdValidation/registry.h"
#include "pxr/usdValidation/usdValidation/timeRange.h"

#include <algorithm>

PXR_NAMESPACE_OPEN_SCOPE

// Validates that the lod:domain attribute on any UsdLodHeuristic prim is
// non-empty.
static UsdValidationErrorVector
_HeuristicLodDomainValidator(const UsdPrim &prim,
                             const UsdValidationTimeRange & /*timeRange*/)
{
    const UsdLodHeuristic heuristic(prim);
    if (!heuristic) {
        return {};
    }

    TfToken domain;
    heuristic.GetLodDomainAttr().Get(&domain);
    if (!domain.IsEmpty()) {
        return {};
    }

    return { UsdValidationError(
        UsdLodValidationErrorNameTokens->lodDomainEmpty,
        UsdValidationErrorType::Error,
        UsdValidationErrorSites {
            UsdValidationErrorSite(prim.GetStage(), prim.GetPath()) },
        TfStringPrintf("LODHeuristic <%s> has an empty lod:domain.",
                       prim.GetPath().GetText())) };
}

// Validates threshold ordering for UsdLodDistanceHeuristic prims.
// thresholds must be strictly increasing; blendThresholds (if present)
// must satisfy thresholds[i] <= blendThresholds[i] <= thresholds[i+1].
static UsdValidationErrorVector
_DistanceHeuristicThresholdsValidator(
    const UsdPrim &prim,
    const UsdValidationTimeRange & /*timeRange*/)
{
    const UsdLodDistanceHeuristic heuristic(prim);
    if (!heuristic) {
        return {};
    }

    VtFloatArray thresholds;
    heuristic.GetThresholdsAttr().Get(&thresholds);

    VtFloatArray blendThresholds;
    heuristic.GetBlendThresholdsAttr().Get(&blendThresholds);

    UsdValidationErrorVector errors;

    const int thresholdLimit = int(thresholds.size()) - 1;
    for (int i = 0; i < thresholdLimit; ++i) {
        if (thresholds[i+1] <= thresholds[i]) {
            errors.emplace_back(
                UsdLodValidationErrorNameTokens->thresholdsNotStrictlyIncreasing,
                UsdValidationErrorType::Error,
                UsdValidationErrorSites {
                    UsdValidationErrorSite(prim.GetStage(), prim.GetPath()) },
                TfStringPrintf(
                    "LODDistanceHeuristic <%s>: thresholds[%d] is not "
                    "strictly increasing.",
                    prim.GetPath().GetText(), i + 1));
        }
    }

    if (!blendThresholds.empty()) {
        const int blendLimit = int(std::min(thresholds.size(),
                                            blendThresholds.size()));
        for (int i = 0; i < blendLimit; ++i) {
            if (blendThresholds[i] < thresholds[i]) {
                errors.emplace_back(
                    UsdLodValidationErrorNameTokens->blendThresholdBelowThreshold,
                    UsdValidationErrorType::Error,
                    UsdValidationErrorSites {
                        UsdValidationErrorSite(
                            prim.GetStage(), prim.GetPath()) },
                    TfStringPrintf(
                        "LODDistanceHeuristic <%s>: blendThresholds[%d] "
                        "(%g) < thresholds[%d] (%g).",
                        prim.GetPath().GetText(), i,
                        blendThresholds[i], i, thresholds[i]));
            }
        }
        for (int i = 0; i < blendLimit - 1; ++i) {
            if (blendThresholds[i] > thresholds[i + 1]) {
                errors.emplace_back(
                    UsdLodValidationErrorNameTokens
                        ->blendThresholdAboveNextThreshold,
                    UsdValidationErrorType::Error,
                    UsdValidationErrorSites {
                        UsdValidationErrorSite(
                            prim.GetStage(), prim.GetPath()) },
                    TfStringPrintf(
                        "LODDistanceHeuristic <%s>: blendThresholds[%d] "
                        "(%g) > thresholds[%d] (%g).",
                        prim.GetPath().GetText(), i,
                        blendThresholds[i], i + 1, thresholds[i + 1]));
            }
        }
    }

    return errors;
}

// Validates threshold ordering for UsdLodScreenSizeHeuristic prims.
// thresholds must be strictly decreasing; blendThresholds (if present)
// must satisfy thresholds[i+1] <= blendThresholds[i] <= thresholds[i].
// projectionMethod must be a recognized token.
// extent (if present) must have size 0 or 2.
static UsdValidationErrorVector
_ScreenSizeHeuristicThresholdsValidator(
    const UsdPrim &prim,
    const UsdValidationTimeRange & /*timeRange*/)
{
    const UsdLodScreenSizeHeuristic heuristic(prim);
    if (!heuristic) {
        return {};
    }

    VtFloatArray thresholds;
    heuristic.GetThresholdsAttr().Get(&thresholds);

    VtFloatArray blendThresholds;
    heuristic.GetBlendThresholdsAttr().Get(&blendThresholds);

    UsdValidationErrorVector errors;

    const int thresholdLimit = int(thresholds.size()) - 1;
    for (int i = 0; i < thresholdLimit; ++i) {
        if (thresholds[i+1] >= thresholds[i]) {
            errors.emplace_back(
                UsdLodValidationErrorNameTokens
                    ->thresholdsNotStrictlyDecreasing,
                UsdValidationErrorType::Error,
                UsdValidationErrorSites {
                    UsdValidationErrorSite(prim.GetStage(), prim.GetPath()) },
                TfStringPrintf(
                    "LODScreenSizeHeuristic <%s>: thresholds[%d] is not "
                    "strictly decreasing.",
                    prim.GetPath().GetText(), i + 1));
        }
    }

    if (!blendThresholds.empty()) {
        const int blendLimit = int(std::min(thresholds.size(),
                                            blendThresholds.size()));
        for (int i = 0; i < blendLimit; ++i) {
            if (blendThresholds[i] > thresholds[i]) {
                errors.emplace_back(
                    UsdLodValidationErrorNameTokens->blendThresholdAboveThreshold,
                    UsdValidationErrorType::Error,
                    UsdValidationErrorSites {
                        UsdValidationErrorSite(
                            prim.GetStage(), prim.GetPath()) },
                    TfStringPrintf(
                        "LODScreenSizeHeuristic <%s>: blendThresholds[%d] "
                        "(%g) > thresholds[%d] (%g).",
                        prim.GetPath().GetText(), i,
                        blendThresholds[i], i, thresholds[i]));
            }
        }
        for (int i = 0; i < blendLimit - 1; ++i) {
            if (blendThresholds[i] < thresholds[i + 1]) {
                errors.emplace_back(
                    UsdLodValidationErrorNameTokens
                        ->blendThresholdBelowNextThreshold,
                    UsdValidationErrorType::Error,
                    UsdValidationErrorSites {
                        UsdValidationErrorSite(
                            prim.GetStage(), prim.GetPath()) },
                    TfStringPrintf(
                        "LODScreenSizeHeuristic <%s>: blendThresholds[%d] "
                        "(%g) < thresholds[%d] (%g).",
                        prim.GetPath().GetText(), i,
                        blendThresholds[i], i + 1, thresholds[i + 1]));
            }
        }
    }

    TfToken projectionMethod;
    heuristic.GetProjectionMethodAttr().Get(&projectionMethod);
    if (!projectionMethod.IsEmpty() &&
        projectionMethod != UsdLodTokens->projectedSphere &&
        projectionMethod != UsdLodTokens->projectedExtent)
    {
        errors.emplace_back(
            UsdLodValidationErrorNameTokens->invalidProjectionMethod,
            UsdValidationErrorType::Error,
            UsdValidationErrorSites {
                UsdValidationErrorSite(prim.GetStage(), prim.GetPath()) },
            TfStringPrintf(
                "LODScreenSizeHeuristic <%s>: projectionMethod '%s' is "
                "not a recognized value (expected 'projectedSphere' or "
                "'projectedExtent').",
                prim.GetPath().GetText(), projectionMethod.GetText()));
    }

    VtVec3fArray extent;
    heuristic.GetExtentAttr().Get(&extent);
    if (extent.size() != 0 && extent.size() != 2) {
        errors.emplace_back(
            UsdLodValidationErrorNameTokens->invalidExtentSize,
            UsdValidationErrorType::Error,
            UsdValidationErrorSites {
                UsdValidationErrorSite(prim.GetStage(), prim.GetPath()) },
            TfStringPrintf(
                "LODScreenSizeHeuristic <%s>: extent has %zu element(s) "
                "but must be empty or have exactly 2 elements.",
                prim.GetPath().GetText(), extent.size()));
    }

    return errors;
}

// Validates that lod:override:mode is a recognized token, and that
// lod:override:index is readable when mode is indexedLOD.
static UsdValidationErrorVector
_LodOverrideApiModeValidator(const UsdPrim &prim,
                             const UsdValidationTimeRange & /*timeRange*/)
{
    if (!prim.HasAPI<UsdLodOverrideAPI>()) {
        return {};
    }

    const UsdLodOverrideAPI overrideAPI(prim);

    TfToken mode;
    if (!overrideAPI.GetLodOverrideModeAttr().Get(&mode)) {
        return {};
    }

    static const TfToken allowedModes[] = {
        UsdLodTokens->inherited,
        UsdLodTokens->noOverride,
        UsdLodTokens->indexedLOD,
        UsdLodTokens->noLOD,
        UsdLodTokens->allLOD,
    };
    const bool modeIsValid = std::find(
        std::begin(allowedModes), std::end(allowedModes), mode)
        != std::end(allowedModes);

    UsdValidationErrorVector errors;

    if (!modeIsValid) {
        errors.emplace_back(
            UsdLodValidationErrorNameTokens->invalidOverrideMode,
            UsdValidationErrorType::Error,
            UsdValidationErrorSites {
                UsdValidationErrorSite(prim.GetStage(), prim.GetPath()) },
            TfStringPrintf(
                "LODOverrideAPI prim <%s>: lod:override:mode '%s' is not "
                "a recognized value.",
                prim.GetPath().GetText(), mode.GetText()));
    } else if (mode == UsdLodTokens->indexedLOD) {
        float index = 0.0f;
        if (!overrideAPI.GetLodOverrideIndexAttr().Get(&index)) {
            errors.emplace_back(
                UsdLodValidationErrorNameTokens->unreadableOverrideIndex,
                UsdValidationErrorType::Warn,
                UsdValidationErrorSites {
                    UsdValidationErrorSite(prim.GetStage(), prim.GetPath()) },
                TfStringPrintf(
                    "LODOverrideAPI prim <%s>: lod:override:mode is "
                    "'indexedLOD' but lod:override:index cannot be read.",
                    prim.GetPath().GetText()));
        }
    }

    return errors;
}

// Validates that every target of lod:heuristics resolves to a prim that
// IsA UsdLodHeuristic.
static UsdValidationErrorVector
_LodRootApiHeuristicsValidator(const UsdPrim &prim,
                               const UsdValidationTimeRange & /*timeRange*/)
{
    if (!prim.HasAPI<UsdLodRootAPI>()) {
        return {};
    }

    const UsdLodRootAPI rootAPI(prim);
    SdfPathVector targets;
    rootAPI.GetLodHeuristicsRel().GetTargets(&targets);

    UsdValidationErrorVector errors;

    for (const SdfPath &targetPath : targets) {
        const UsdPrim targetPrim = prim.GetStage()->GetPrimAtPath(targetPath);
        if (!targetPrim || !targetPrim.IsA<UsdLodHeuristic>()) {
            errors.emplace_back(
                UsdLodValidationErrorNameTokens->invalidHeuristicTarget,
                UsdValidationErrorType::Error,
                UsdValidationErrorSites {
                    UsdValidationErrorSite(prim.GetStage(), prim.GetPath()) },
                TfStringPrintf(
                    "LODRootAPI prim <%s>: lod:heuristics target <%s> "
                    "does not resolve to a UsdLodHeuristic prim.",
                    prim.GetPath().GetText(), targetPath.GetText()));
        }
    }

    return errors;
}

TF_REGISTRY_FUNCTION(UsdValidationRegistry)
{
    UsdValidationRegistry &registry = UsdValidationRegistry::GetInstance();

    registry.RegisterPluginValidator(
        UsdLodValidatorNameTokens->heuristicLodDomain,
        _HeuristicLodDomainValidator);

    registry.RegisterPluginValidator(
        UsdLodValidatorNameTokens->distanceHeuristicThresholds,
        _DistanceHeuristicThresholdsValidator);

    registry.RegisterPluginValidator(
        UsdLodValidatorNameTokens->screenSizeHeuristicThresholds,
        _ScreenSizeHeuristicThresholdsValidator);

    registry.RegisterPluginValidator(
        UsdLodValidatorNameTokens->lodOverrideApiMode,
        _LodOverrideApiModeValidator);

    registry.RegisterPluginValidator(
        UsdLodValidatorNameTokens->lodRootApiHeuristics,
        _LodRootApiHeuristicsValidator);
}

PXR_NAMESPACE_CLOSE_SCOPE
