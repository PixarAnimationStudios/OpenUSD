//
// Copyright 2026 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/base/tf/stringUtils.h"
#include "pxr/base/tf/token.h"
#include "pxr/usd/usd/prim.h"
#include "pxr/usd/usd/schemaRegistry.h"
#include "pxr/usd/usdLux/lightAPI.h"
#include "pxr/usd/usdLux/lightFilter.h"
#include "pxr/usd/usdShade/connectableAPI.h"
#include "pxr/usd/usdShade/material.h"
#include "pxr/usdValidation/usdLuxValidators/validatorTokens.h"
#include "pxr/usdValidation/usdValidation/error.h"
#include "pxr/usdValidation/usdValidation/registry.h"
#include "pxr/usdValidation/usdValidation/timeRange.h"

PXR_NAMESPACE_OPEN_SCOPE

static UsdValidationErrorVector
_EncapsulationValidator(const UsdPrim &usdPrim, 
                        const UsdValidationTimeRange &/*timeRange*/)
{
    // Encapsulation rules for connections are only relevant for UsdLuxLight and
    // UsdLuxLightFilter prims, so we only need to run this validator for those
    // prims.
    if (! (usdPrim.HasAPI<UsdLuxLightAPI>() || 
           usdPrim.IsA<UsdLuxLightFilter>())) {
        return {};
    }

    const UsdShadeConnectableAPI &connectable = UsdShadeConnectableAPI(usdPrim);

    if (!connectable) {
        return {};
    }

    const UsdPrim &parentPrim = usdPrim.GetParent();

    if (!parentPrim || parentPrim.IsPseudoRoot()) {
        return {};
    }

    UsdShadeConnectableAPI parentConnectable
        = UsdShadeConnectableAPI(parentPrim);

    UsdValidationErrorVector errors;

    // If usdPrim is a light, that is, it has a LightAPI applied, then it must
    // not have a connectable parent, lights can not be scoped within a 
    // UsdShadeNodeGraph, or other Connectable / container prims. Except Light
    // themselves, that is parent prims with LightAPI applied! For example, we
    // expect PortalLight prims to be parented inside DomeLight prims.
    if (usdPrim.HasAPI<UsdLuxLightAPI>() && parentConnectable
        && !parentPrim.HasAPI<UsdLuxLightAPI>()) {
        errors.emplace_back(
            UsdLuxValidationErrorNameTokens->lightUnderConnectable,
            UsdValidationErrorType::Error,
            UsdValidationErrorSites {
                UsdValidationErrorSite(usdPrim.GetStage(), usdPrim.GetPath()) },
            TfStringPrintf("Light %s <%s> cannot have a connectable parent %s "
                           "<%s>.", usdPrim.GetTypeName().GetText(), 
                           usdPrim.GetPath().GetText(),
                           parentPrim.GetTypeName().GetText(),
                           parentPrim.GetPath().GetText()));
    }

    // LightFilter is a connectable container prim, but can not contain lights.
    // Note that lights within a light filter will be caught by the above check.
    // Also note that other non-usdLux connectable prims like UsdShadeMaterial,
    // must be checked here, as UsdShadeMaterial validator can not control
    // downsteam schemas like UsdLuxLightFilter.
    if (usdPrim.IsA<UsdLuxLightFilter>() && 
             parentPrim.IsA<UsdShadeMaterial>()) {
        errors.emplace_back(
            UsdLuxValidationErrorNameTokens->lightFilterUnderLightFilter,
            UsdValidationErrorType::Error,
            UsdValidationErrorSites {
                UsdValidationErrorSite(usdPrim.GetStage(), usdPrim.GetPath()) },
            TfStringPrintf("LightFilter <%s> cannot have a parent <%s> that "
                           "is a UsdShadeMaterial.",
                           usdPrim.GetPath().GetText(),
                           parentPrim.GetPath().GetText()));
    }

    // Generic UsdLux (mimics UsdShade) connectable encapsulation rules:
    if (parentConnectable && !parentConnectable.IsContainer()) {
        // It is a violation of the encapsulation rule which enforces
        // encapsulation of connectable prims under a Container-type
        // connectable prim.
        errors.emplace_back(
            UsdLuxValidationErrorNameTokens->connectableInNonContainer,
            UsdValidationErrorType::Error,
            UsdValidationErrorSites {
                UsdValidationErrorSite(usdPrim.GetStage(), usdPrim.GetPath()) },
            TfStringPrintf("Connectable %s <%s> cannot reside "
                           "under a non-Container Connectable %s",
                           usdPrim.GetTypeName().GetText(),
                           usdPrim.GetPath().GetText(),
                           parentPrim.GetTypeName().GetText()));
    } else if (!parentConnectable) {
        std::function<void(const UsdPrim &)> _VerifyValidAncestor
            = [&](const UsdPrim &currentAncestor) -> void {
            if (!currentAncestor || currentAncestor.IsPseudoRoot()) {
                return;
            }
            const UsdShadeConnectableAPI &ancestorConnectable
                = UsdShadeConnectableAPI(currentAncestor);
            if (ancestorConnectable) {
                // it's only OK to have a non-connectable parent if all
                // the rest of your ancestors are also non-connectable.
                // The error message we give is targeted at the most common
                // infraction, using Scope or other grouping prims inside
                // a Container like a Material
                errors.emplace_back(
                    UsdLuxValidationErrorNameTokens
                        ->invalidConnectableHierarchy,
                    UsdValidationErrorType::Error,
                    UsdValidationErrorSites { UsdValidationErrorSite(
                        usdPrim.GetStage(), usdPrim.GetPath()) },
                    TfStringPrintf("Connectable %s <%s> can only have "
                                   "Connectable Container ancestors up to %s "
                                   "ancestor <%s>, but its parent %s is a %s.",
                                   usdPrim.GetTypeName().GetText(),
                                   usdPrim.GetPath().GetText(),
                                   currentAncestor.GetTypeName().GetText(),
                                   currentAncestor.GetPath().GetText(),
                                   parentPrim.GetName().GetText(),
                                   parentPrim.GetTypeName().GetText()));
                return;
            }
            _VerifyValidAncestor(currentAncestor.GetParent());
        };
        _VerifyValidAncestor(parentPrim.GetParent());
    }

    return errors;
}

TF_REGISTRY_FUNCTION(UsdValidationRegistry)
{
    UsdValidationRegistry &registry = UsdValidationRegistry::GetInstance();

    registry.RegisterPluginValidator(
        UsdLuxValidatorNameTokens->encapsulationValidator,
        _EncapsulationValidator);
}

PXR_NAMESPACE_CLOSE_SCOPE
