//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/base/tf/stringUtils.h"
#include "pxr/base/tf/token.h"
#include "pxr/base/tf/staticTokens.h"
#include "pxr/usd/usd/prim.h"
#include "pxr/usd/usd/attribute.h"
#include "pxr/usd/usd/property.h"
#include "pxr/usd/usd/relationship.h"
#include "pxr/usd/usd/schemaRegistry.h"
#include "pxr/usd/usdGeom/camera.h"
#include "pxr/usd/usdRender/settings.h"
#include "pxr/usd/usdRi/tokens.h"
#include "pxr/usd/usdShade/tokens.h"

#include "pxr/usdValidation/usdValidation/error.h"
#include "pxr/usdValidation/usdValidation/fixer.h"
#include "pxr/usdValidation/usdValidation/registry.h"
#include "pxr/usdValidation/usdValidation/timeRange.h"
#include "rmanUsdValidators/validatorTokens.h"

#include <algorithm>
#include <unordered_map>
#include <vector>

PXR_NAMESPACE_OPEN_SCOPE

TF_DEFINE_PRIVATE_TOKENS(
    _terminalsTokens,
    (PxrCameraProjectionAPI)
    (PxrRenderTerminalsAPI)
    (RenderSettings)
    ((RiProjectionRel, "ri:projection"))
    ((OutputsRiIntegratorAttr, "outputs:ri:integrator"))
    ((OutputsRiDisplayFiltersAttr, "outputs:ri:displayFilters"))
    ((OutputsRiSampleFiltersAttr, "outputs:ri:sampleFilters"))
);

const std::vector<UsdValidationFixer>
_PxrRenderTerminalsApiRelationshipsFixers() 
{
    std::vector<UsdValidationFixer> fixers;

    FixerCanApplyFn fixerCanApplyFn = 
        [](const UsdValidationError &error, const UsdEditTarget &editTarget,
           const UsdTimeCode &/*timeCode*/) -> bool {
            if (!editTarget.IsValid() || !editTarget.GetLayer()) {
                return false;
            }
            if (error.GetSites().size() != 1) {
                // Must have one and only one error site to fix
                return false;
            }
            const UsdValidationErrorSite &site = error.GetSites().front();
            if (!site.IsValid() || !site.IsPrim()) {
                return false;
            }
            UsdPrim prim = site.GetPrim();
            return prim.IsA<UsdRenderSettings>()
                && prim.HasAPI(_terminalsTokens->PxrRenderTerminalsAPI);
        };

    FixerImplFn fixerImplFn = 
        [](const UsdValidationError &error, const UsdEditTarget &editTarget,
           const UsdTimeCode &/*timeCode*/) -> bool {
            if (!editTarget.IsValid() || !editTarget.GetLayer()) {
                return false;
            }
            if (error.GetSites().size() != 1) {
                // Must have one and only one error site to fix
                return false;
            }
            const UsdValidationErrorSite &site = error.GetSites()[0];
            if (!site.IsValid() || !site.IsPrim()) {
                return false;
            }
            UsdPrim prim = site.GetPrim();

            const std::vector<TfToken> unsupportedTerminalsAttrs = {
                _terminalsTokens->OutputsRiIntegratorAttr,
                _terminalsTokens->OutputsRiDisplayFiltersAttr,
                _terminalsTokens->OutputsRiSampleFiltersAttr};

            for (const TfToken& attrName : unsupportedTerminalsAttrs) {
                UsdAttribute attr = prim.GetAttribute(attrName);
                if (!attr || !attr.HasAuthoredConnections()) {
                    continue;
                }
                const TfToken relName = 
                    TfToken(SdfPath::StripPrefixNamespace(
                        attrName.GetString(),
                        UsdShadeTokens->outputs.GetString()).first);
                prim.CreateRelationship(relName);
                std::vector<SdfPath> targets;
                attr.GetConnections(&targets);
                UsdRelationship rel = prim.GetRelationship(relName);
                rel.SetTargets(targets);
                attr.ClearConnections();
            }

            return true;
        };

    fixers.emplace_back(
        TfToken("ConvertRenderTerminalsAttrsToRels"),
        "Converts PxrRenderTerminalsAPI unsupported attributes to relationships.",
        fixerImplFn, fixerCanApplyFn, TfTokenVector{}, 
        RmanUsdValidatorsErrorNameTokens->invalidRenderTerminalsAttr);

    return fixers;
}

static UsdValidationErrorVector
_PxrRenderTerminalsApiRelationships(
    const UsdPrim &usdPrim, 
    const UsdValidationTimeRange &/*timeRange*/)
{
    if (!usdPrim.IsA<UsdRenderSettings>()
        || !usdPrim.HasAPI(_terminalsTokens->PxrRenderTerminalsAPI)) {
        return {};
    }

    UsdValidationErrorVector errors;
    
    const std::vector<TfToken> unsupportedTerminalsAttrs = {
        _terminalsTokens->OutputsRiIntegratorAttr,
        _terminalsTokens->OutputsRiDisplayFiltersAttr,
        _terminalsTokens->OutputsRiSampleFiltersAttr};

    const std::string prefix = 
        UsdShadeTokens->outputs.GetString() +
        UsdRiTokens->renderContext.GetString();

    const std::vector<UsdProperty> properties
        = usdPrim.GetPropertiesInNamespace(prefix);

    for (const TfToken& attrName : unsupportedTerminalsAttrs) {
        const UsdAttribute attr = usdPrim.GetAttribute(attrName);
        if (attr && attr.HasAuthoredConnections()) {
            errors.emplace_back(
                RmanUsdValidatorsErrorNameTokens->invalidRenderTerminalsAttr,
                UsdValidationErrorType::Warn,
                UsdValidationErrorSites { UsdValidationErrorSite(
                    usdPrim.GetStage(), usdPrim.GetPath()) },
                TfStringPrintf(("Found a PxrRenderTerminalsAPI "
                               "unsupported attribute (%s) that should "
                               "instead be a relationship; see the "
                               "schema for more info."),
                               attrName.GetText()));
        }
    }

    return errors;
}

const std::vector<UsdValidationFixer>
_PxrCameraProjectionApiRelationshipsFixers()
{
    std::vector<UsdValidationFixer> fixers;

    FixerCanApplyFn fixerCanApplyFn =
        [](const UsdValidationError &error, const UsdEditTarget &editTarget,
           const UsdTimeCode &/*timeCode*/) -> bool {
            if (!editTarget.IsValid() || !editTarget.GetLayer()) {
                return false;
            }
            if (error.GetSites().size() != 1) {
                return false;
            }
            const UsdValidationErrorSite& site = error.GetSites().front();
            if (!site.IsValid() || !site.IsPrim()) {
                return false;
            }
            UsdPrim prim = site.GetPrim();
            return prim.IsA<UsdGeomCamera>() &&
                prim.HasAPI(_terminalsTokens->PxrCameraProjectionAPI);
        };

    FixerImplFn fixerImplFn =
        [](const UsdValidationError &error, const UsdEditTarget &editTarget,
           const UsdTimeCode &/*timeCode*/) -> bool {
            if (!editTarget.IsValid() || !editTarget.GetLayer()) {
                return false;
            }
            if (error.GetSites().size() != 1) {
                return false;
            }
            const UsdValidationErrorSite& site = error.GetSites()[0];
            if (!site.IsValid() || !site.IsPrim()) {
                return false;
            }
            UsdPrim prim = site.GetPrim();

            UsdAttribute attr = prim.GetAttribute(
                TfToken(UsdShadeTokens->outputs.GetString() +
                        _terminalsTokens->RiProjectionRel.GetString()));
            if (!attr || !attr.HasAuthoredConnections()) {
                return false;
            }

            prim.CreateRelationship(_terminalsTokens->RiProjectionRel);
            std::vector<SdfPath> targets;
            attr.GetConnections(&targets);
            UsdRelationship rel = prim.GetRelationship(
                _terminalsTokens->RiProjectionRel);
            rel.SetTargets(targets);
            attr.ClearConnections();
            return true;
        };

    fixers.emplace_back(
        TfToken("ConvertPxrCameraProjectionAttrToRel"),
        "Convert PxrCameraProjectionAPI unsupported attribute to relationship",
        fixerImplFn, fixerCanApplyFn, TfTokenVector{},
        RmanUsdValidatorsErrorNameTokens->invalidCameraProjectionAttr);

    return fixers;
}

static UsdValidationErrorVector
_PxrCameraProjectionApiRelationships(
    const UsdPrim &usdPrim, 
    const UsdValidationTimeRange &/*timeRange*/)
{
    if (!usdPrim.IsA<UsdGeomCamera>()
        || !usdPrim.HasAPI(_terminalsTokens->PxrCameraProjectionAPI)) {
        return {};
    }

    // Only produce errors if the registered schema has the updated
    // relationships.
    const UsdSchemaRegistry& reg = UsdSchemaRegistry::GetInstance();
    const UsdPrimDefinition* camProjAPIDef = reg.FindAppliedAPIPrimDefinition(
        _terminalsTokens->PxrCameraProjectionAPI);

    using UsdPropertyDefinition = UsdPrimDefinition::Property;
    const UsdPropertyDefinition camProjAPIRelDef = 
        camProjAPIDef->GetPropertyDefinition(
            _terminalsTokens->RiProjectionRel);
    if (!camProjAPIRelDef) {
        return {};
    }

    UsdValidationErrorVector errors;

    const UsdAttribute attr = usdPrim.GetAttribute(
        TfToken(UsdShadeTokens->outputs.GetString() + 
                _terminalsTokens->RiProjectionRel.GetString()));
    if (attr && attr.HasAuthoredConnections()) {
        errors.emplace_back(
            RmanUsdValidatorsErrorNameTokens->invalidCameraProjectionAttr,
            UsdValidationErrorType::Warn,
            UsdValidationErrorSites { UsdValidationErrorSite(
                usdPrim.GetStage(), usdPrim.GetPath()) },
            TfStringPrintf(
                ("Found a PxrCameraProjectionAPI unsupported attribute (%s) "
                 "that should instead be a relationship (ri:projection); see "
                 "the schema for more info."),
                 attr.GetName().GetText()));
    }

    return errors;
}

TF_REGISTRY_FUNCTION(UsdValidationRegistry)
{
    UsdValidationRegistry &registry = UsdValidationRegistry::GetInstance();

    registry.RegisterPluginValidator(
        RmanUsdValidatorsNameTokens->PxrRenderTerminalsAPIRelationships,
        _PxrRenderTerminalsApiRelationships,
        _PxrRenderTerminalsApiRelationshipsFixers());

    registry.RegisterPluginValidator(
        RmanUsdValidatorsNameTokens->PxrCameraProjectionAPIRelationships,
        _PxrCameraProjectionApiRelationships,
        _PxrCameraProjectionApiRelationshipsFixers());
}

PXR_NAMESPACE_CLOSE_SCOPE
