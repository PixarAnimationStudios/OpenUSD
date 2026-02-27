//
// Copyright 2024 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/usdValidation/usdValidation/error.h"
#include "pxr/usdValidation/usdValidation/registry.h"
#include "pxr/usdValidation/usdValidation/timeRange.h"
#include "pxr/usdValidation/usdValidation/validator.h"
#include "pxr/usdValidation/usdValidation/validatorTokens.h"

#include "pxr/usd/usd/attribute.h"

PXR_NAMESPACE_OPEN_SCOPE

static UsdValidationErrorVector
_GetCompositionErrors(const UsdStagePtr &usdStage, 
                      const UsdValidationTimeRange &/*timeRange*/)
{
    UsdValidationErrorVector errors;
    const PcpErrorVector pcpErrors = usdStage->GetCompositionErrors();
    errors.reserve(pcpErrors.size());
    for (const PcpErrorBasePtr &pcpError : pcpErrors) {
        UsdValidationErrorSites errorSites
            = { UsdValidationErrorSite(usdStage, pcpError->rootSite.path) };
        errors.emplace_back(UsdValidationErrorNameTokens->compositionError,
                            UsdValidationErrorType::Error,
                            std::move(errorSites), pcpError->ToString());
    }
    return errors;
}

static UsdValidationErrorVector
_GetStageMetadataErrors(const UsdStagePtr &usdStage, 
                        const UsdValidationTimeRange &/*timeRange*/)
{
    UsdValidationErrorVector errors;
    if (!usdStage->GetDefaultPrim()) {
        errors.emplace_back(
            UsdValidationErrorNameTokens->missingDefaultPrim,
            UsdValidationErrorType::Error,
            UsdValidationErrorSites {
                UsdValidationErrorSite(usdStage, SdfPath::AbsoluteRootPath()) },
            TfStringPrintf("Stage with root layer <%s> has an invalid or "
                           "missing defaultPrim.",
                           usdStage->GetRootLayer()->GetIdentifier().c_str()));
    }

    return errors;
}

static UsdValidationErrorVector
_GetAttributeTypeMismatchErrors(const UsdPrim &usdPrim, 
                               const UsdValidationTimeRange &/*timeRange*/)
{
    UsdValidationErrorVector errors;
    for (const UsdAttribute &attr : usdPrim.GetAttributes()) {
        const SdfValueTypeName attrType = attr.GetTypeName();
        for (const SdfPropertySpecHandle &spec : attr.GetPropertyStack()) {
            if (spec->GetTypeName() != attrType) {
                const UsdValidationErrorSites attributeErrorSites = 
                    { UsdValidationErrorSite(spec->GetLayer(), attr.GetPath()) };
                errors.emplace_back(
                    UsdValidationErrorNameTokens->attributeTypeMismatch,
                    UsdValidationErrorType::Error, attributeErrorSites,
                    TfStringPrintf("Type mismatch for attribute <%s>. "
                                   "Expected attribute type is '%s' but "
                                   "defined as '%s' in layer <%s>.",
                                   attr.GetPath().GetText(),
                                   attrType.GetAsToken().GetText(),
                                   spec->GetTypeName().GetAsToken().GetText(),
                                   spec->GetLayer()->GetIdentifier().c_str()));
            }
        }
    }
    return errors;
}

TF_REGISTRY_FUNCTION(UsdValidationRegistry)
{
    UsdValidationRegistry &registry = UsdValidationRegistry::GetInstance();
    registry.RegisterPluginValidator(
        UsdValidatorNameTokens->compositionErrorTest, _GetCompositionErrors);
    registry.RegisterPluginValidator(
        UsdValidatorNameTokens->stageMetadataChecker, _GetStageMetadataErrors);
    registry.RegisterPluginValidator(
        UsdValidatorNameTokens->attributeTypeMismatch, 
        _GetAttributeTypeMismatchErrors);
}

PXR_NAMESPACE_CLOSE_SCOPE
