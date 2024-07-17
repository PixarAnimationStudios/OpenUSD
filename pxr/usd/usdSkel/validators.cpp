//
// Copyright 2024 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/usd/usd/prim.h"
#include "pxr/usd/usd/schemaRegistry.h"
#include "pxr/usd/usd/validationError.h"
#include "pxr/usd/usd/validationRegistry.h"
#include "pxr/usd/usd/validator.h"
#include "pxr/base/tf/token.h"
#include "pxr/base/tf/stringUtils.h"
#include "pxr/usd/usdSkel/validatorTokens.h"
#include "pxr/usd/usdSkel/bindingAPI.h"

#include <algorithm>
#include <unordered_map>

PXR_NAMESPACE_OPEN_SCOPE

static
UsdValidationErrorVector
_SkelBindingApiChecker(const UsdPrim &usdPrim)
{
    UsdValidationErrorVector errors;

    if (!usdPrim.HasAPI<UsdSkelBindingAPI>()){
        std::vector<TfToken> primPropertyNames = usdPrim.GetPropertyNames();
        static const std::unordered_set<TfToken, TfHash> skelPropertyNames = []() {
            UsdSchemaRegistry& usdSchemaRegistry = UsdSchemaRegistry::GetInstance();
            std::unique_ptr<UsdPrimDefinition> primDef = usdSchemaRegistry.BuildComposedPrimDefinition(TfToken(), {TfToken("SkelBindingAPI")});
            std::vector<TfToken> vectorOfNames = primDef->GetPropertyNames();
            return std::unordered_set<TfToken, TfHash>(vectorOfNames.begin(), vectorOfNames.end());
        }();
 
        for (const TfToken &primToken : primPropertyNames){
            if (skelPropertyNames.find(primToken) == skelPropertyNames.end()){
                continue;
            }
            errors.emplace_back(
                    UsdValidationErrorType::Error,
                    UsdValidationErrorSites{
                            UsdValidationErrorSite(usdPrim.GetStage(),
                                                   usdPrim.GetPath())
                    },
                    TfStringPrintf(("Found a UsdSkelBinding property (%s), "
                                    "but no SkelBindingAPI applied on the prim <%s>."),
                                   primToken.GetText(), usdPrim.GetPath().GetText()));
        }
    }
    else {
        if (usdPrim.GetTypeName() != UsdSkelTokens->SkelRoot) {
            UsdPrim parentPrim = usdPrim.GetParent();
            while (parentPrim && !parentPrim.IsPseudoRoot()) {
                if (parentPrim.GetTypeName() != UsdSkelTokens->SkelRoot) {
                    parentPrim = parentPrim.GetParent();
                }
                else {
                    return errors;
                }
            }
            errors.emplace_back(
                    UsdValidationErrorType::Error,
                    UsdValidationErrorSites{
                            UsdValidationErrorSite(usdPrim.GetStage(),
                                                   usdPrim.GetPath())
                    },
                    TfStringPrintf(("UsdSkelBindingAPI applied on prim: <%s>, "
                            "which is not of type SkelRoot or is not rooted at a prim "
                            "of type SkelRoot, as required by the UsdSkel schema."),
                                   usdPrim.GetPath().GetText()));
        }
    }

    return errors;
}

TF_REGISTRY_FUNCTION(UsdValidationRegistry)
{
    UsdValidationRegistry &registry = UsdValidationRegistry::GetInstance();

    registry.RegisterPluginValidator(
            UsdSkelValidatorNameTokens->skelBindingApiAppliedChecker,
            _SkelBindingApiChecker);
}

PXR_NAMESPACE_CLOSE_SCOPE