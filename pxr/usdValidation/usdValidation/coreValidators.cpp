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

#include "pxr/usd/sdf/attributeSpec.h"
#include "pxr/usd/sdf/layer.h"
#include "pxr/usd/sdf/schema.h"
#include "pxr/usd/sdf/types.h"
#include "pxr/usd/usd/attribute.h"
#include "pxr/base/tf/safeTypeCompare.h"
#include "pxr/base/tf/stringUtils.h"

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

// Return true if a value of type \p valueType needs no further checking on an
// attribute declared as \p typeName, i.e. if it is:
//  - exactly the declared type,
//  - void, meaning no value is authored (e.g. no default),
//  - a value block, or
//  - an animation block.
// This only needs the value's typeid, which crate files provide without
// unpacking the value.
static bool
_IsTriviallyValidValueType(const SdfValueTypeName &typeName,
                           const std::type_info &valueType)
{
    return TfSafeTypeCompare(valueType, typeName.GetType().GetTypeid()) ||
        TfSafeTypeCompare(valueType, typeid(void)) ||
        TfSafeTypeCompare(valueType, typeid(SdfValueBlock)) ||
        TfSafeTypeCompare(valueType, typeid(SdfAnimationBlock));
}

static UsdValidationErrorVector
_GetAttributeValueTypeMismatchErrors(const SdfLayerHandle &layer)
{
    UsdValidationErrorVector errors;

    auto addError = [&errors, &layer](const SdfAttributeSpecHandle &attr,
                                      const VtValue &value,
                                      const std::string &where) {
        errors.emplace_back(
            UsdValidationErrorNameTokens->attributeValueTypeMismatch,
            UsdValidationErrorType::Error,
            UsdValidationErrorSites {
                UsdValidationErrorSite(layer, attr->GetPath()) },
            TfStringPrintf("Attribute <%s> is declared as '%s' (%s) in "
                           "layer <%s>, but its authored %s holds a value of "
                           "type '%s'.",
                           attr->GetPath().GetText(),
                           attr->GetTypeName().GetAsToken().GetText(),
                           attr->GetTypeName().GetType().GetTypeName().c_str(),
                           layer->GetIdentifier().c_str(),
                           where.c_str(),
                           value.GetTypeName().c_str()));
    };

    // Only when _IsTriviallyValidValueType can't vouch for a value do we fetch
    // it in full and defer to CanRepresent.
    layer->Traverse(SdfPath::AbsoluteRootPath(), [&](const SdfPath &path) {
        const SdfAttributeSpecHandle attr = layer->GetAttributeAtPath(path);
        if (!attr || !attr->GetTypeName()) {
            return;
        }
        const SdfValueTypeName typeName = attr->GetTypeName();

        if (!_IsTriviallyValidValueType(typeName,
                layer->GetFieldTypeid(path, SdfFieldKeys->Default))) {
            const VtValue value = attr->GetDefaultValue();
            if (!typeName.CanRepresent(value)) {
                addError(attr, value, "default value");
            }
        }

        for (const double time : layer->ListTimeSamplesForPath(path)) {
            if (_IsTriviallyValidValueType(typeName,
                    layer->QueryTimeSampleTypeid(path, time))) {
                continue;
            }
            VtValue value;
            layer->QueryTimeSample(path, time, &value);
            if (!typeName.CanRepresent(value)) {
                addError(attr, value, TfStringPrintf(
                    "time sample at %s", TfStringify(time).c_str()));
            }
        }
    });
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
    registry.RegisterPluginValidator(
        UsdValidatorNameTokens->attributeValueTypeMismatch,
        _GetAttributeValueTypeMismatchErrors);
}

PXR_NAMESPACE_CLOSE_SCOPE
