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
#include "pxr/usd/usdRender/settings.h"
#include "pxr/usd/usdRi/tokens.h"
#include "pxr/usd/usdShade/tokens.h"

#include "pxr/usdValidation/usdValidation/error.h"
#include "pxr/usdValidation/usdValidation/registry.h"
#include "pxr/usdValidation/usdValidation/timeRange.h"
#include "rmanUsdValidators/validatorTokens.h"

#include <algorithm>
#include <unordered_map>
#include <vector>

PXR_NAMESPACE_OPEN_SCOPE

TF_DEFINE_PRIVATE_TOKENS(
    _terminalsTokens,
    (PxrRenderTerminalsAPI)
    (RenderSettings)
    ((RiIntegratorRel, "ri:integrator"))
    ((OutputsRiIntegratorAttr, "outputs:ri:integrator"))
    ((OutputsRiDisplayFiltersAttr, "outputs:ri:displayFilters"))
    ((OutputsRiSampleFiltersAttr, "outputs:ri:sampleFilters"))
);

static UsdValidationErrorVector
_PxrRenderTerminalsApiRelationships(
    const UsdPrim &usdPrim, 
    const UsdValidationTimeRange &/*timeRange*/)
{
    if (!usdPrim.IsA<UsdRenderSettings>()
        || !usdPrim.HasAPI(_terminalsTokens->PxrRenderTerminalsAPI)) {
        return {};
    }

    // Only produce errors if the registered schema has the updated
    // relationships.
    const UsdSchemaRegistry& reg = UsdSchemaRegistry::GetInstance();
    const UsdPrimDefinition* rsDef = reg.FindConcretePrimDefinition(
        _terminalsTokens->RenderSettings);
    const TfTokenVector& rsPropNames = rsDef->GetPropertyNames();
    const bool rsSchemaHasRelationships = std::find(rsPropNames.begin(),
        rsPropNames.end(), _terminalsTokens->RiIntegratorRel)
        != rsPropNames.end();
    if (!rsSchemaHasRelationships) {
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
    for (const UsdProperty& prop : properties) {
        UsdAttribute attr = prop.As<UsdAttribute>();
        if (!attr || !attr.HasAuthoredConnections()) {
            continue;
        }
        const TfToken propName = attr.GetName();

        for (const TfToken& attrName : unsupportedTerminalsAttrs) {
            if (propName == attrName) {
                errors.emplace_back(
                    RmanUsdValidatorsErrorNameTokens->invalidRenderTerminalsAttr,
                    UsdValidationErrorType::Warn,
                    UsdValidationErrorSites { UsdValidationErrorSite(
                        usdPrim.GetStage(), usdPrim.GetPath()) },
                    TfStringPrintf(("Found a PxrRenderTerminalsAPI "
                                    "unsupported attribute (%s) that should "
                                    "instead be a relationship; see the "
                                    "schema for more info."),
                                    propName.GetText()));
            }
        }
    }

    return errors;
}

TF_REGISTRY_FUNCTION(UsdValidationRegistry)
{
    UsdValidationRegistry &registry = UsdValidationRegistry::GetInstance();

    registry.RegisterPluginValidator(
        RmanUsdValidatorsNameTokens->PxrRenderTerminalsAPIRelationships,
        _PxrRenderTerminalsApiRelationships);
}

PXR_NAMESPACE_CLOSE_SCOPE
