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
#include "pxr/base/tf/enum.h"
#include "pxr/base/tf/safeTypeCompare.h"
#include "pxr/base/tf/stringUtils.h"
#include "pxr/base/ts/spline.h"

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

// Return errors for the property spec \p spec:
//  - attributeSpecValueTypeMismatch for each authored default, time sample, or
//    spline value that does not hold the spec's declared type, and
//  - attributeTypeMismatch, only if \p attr is valid: if \p spec is not an
//    attribute spec, or if its declared type differs from \p expectedType.
// \p attr is the composed attribute that \p spec contributes to, and is used to
// name that attribute in the error messages when applicable.
static UsdValidationErrorVector
_ReportAttributeSpecTypeAndValueTypeMismatch(
    const SdfPropertySpecHandle &spec,
    const UsdAttribute &attr = UsdAttribute(),
    const SdfValueTypeName &expectedType = SdfValueTypeName())
{
    // A non-attribute spec in an attribute's property stack means layers
    // disagree on the kind of property.
    if (attr && spec && spec->GetSpecType() != SdfSpecTypeAttribute) {
        return { UsdValidationError(
            UsdValidationErrorNameTokens->attributeTypeMismatch,
            UsdValidationErrorType::Error,
            UsdValidationErrorSites {
                UsdValidationErrorSite(spec->GetLayer(), spec->GetPath()) },
            TfStringPrintf(
                "Type mismatch for attribute <%s>: spec <%s> in layer <%s> is "
                "not an attribute spec (spec type '%s').",
                attr.GetPath().GetText(),
                spec->GetPath().GetText(),
                spec->GetLayer()->GetIdentifier().c_str(),
                TfEnum::GetDisplayName(spec->GetSpecType()).c_str())) };
    }

    // No spec to work off of, or no typeName on the spec, or not attribute spec
    if (!spec || !spec->GetTypeName() ||
            (spec->GetSpecType() != SdfSpecTypeAttribute)) {
        return {};
    }

    UsdValidationErrorVector errors;
    const SdfValueTypeName specType = spec->GetTypeName();
    const SdfLayerHandle layer = spec->GetLayer();
    const SdfPath specPath = spec->GetPath();

    // If an expected type is provided we also check it against the spec's
    // typeName, and report an error if they don't match.
    if (attr && expectedType && specType != expectedType) {
        errors.emplace_back(
            UsdValidationErrorNameTokens->attributeTypeMismatch,
            UsdValidationErrorType::Error,
            UsdValidationErrorSites {
                UsdValidationErrorSite(layer, specPath) },
            TfStringPrintf(
                "Type mismatch for attribute <%s>: spec <%s> in layer <%s> is "
                "defined as '%s' but the expected type is '%s'.",
                attr.GetPath().GetText(),
                specPath.GetText(),
                layer->GetIdentifier().c_str(),
                specType.GetAsToken().GetText(),
                expectedType.GetAsToken().GetText()));
    }

    auto addValueTypeMismatchError =
        [&attr, &errors, &specPath, &specType, &layer](
            const std::string &foundValueType, const std::string &where) {

        // include attribute path in the error message if an attr is provided
        const std::string composedAttrErrorText =
            (attr ? TfStringPrintf("of composed attribute <%s> ",
                                    attr.GetPath().GetText()) : "");

        errors.emplace_back(
            UsdValidationErrorNameTokens->attributeSpecValueTypeMismatch,
            UsdValidationErrorType::Error,
            UsdValidationErrorSites {
                UsdValidationErrorSite(layer, specPath) },
            TfStringPrintf("Attribute Spec <%s> %sis declared as '%s' (%s) in "
                           "layer <%s>, but its authored %s holds a value of "
                           "type '%s'.",
                           specPath.GetText(),
                           composedAttrErrorText.c_str(),
                           specType.GetAsToken().GetText(),
                           specType.GetType().GetTypeName().c_str(),
                           layer->GetIdentifier().c_str(),
                           where.c_str(),
                           foundValueType.c_str()));
    };

    // Check that each authored default, time sample, and spline value holds
    // the spec's declared type.

    // Only when _IsTriviallyValidValueType can't vouch for a value do we fetch
    // it in full and defer to CanRepresent.
    if (!_IsTriviallyValidValueType(specType, layer->GetFieldTypeid(
            specPath, SdfFieldKeys->Default))) {
        const VtValue value = layer->GetField(specPath, SdfFieldKeys->Default);
        if (!specType.CanRepresent(value)) {
            addValueTypeMismatchError(value.GetTypeName(), "default value");
        }
    }

    // time samples
    for (const double time : layer->ListTimeSamplesForPath(specPath)) {
        if (_IsTriviallyValidValueType(specType, layer->QueryTimeSampleTypeid(
                specPath, time))) {
            continue;
        }
        VtValue value;
        layer->QueryTimeSample(specPath, time, &value);
        if (!specType.CanRepresent(value)) {
            addValueTypeMismatchError(value.GetTypeName(), TfStringPrintf(
                "time sample at %s", TfStringify(time).c_str()));
        }
    }
    // splines
    TsSpline spline;
    if (layer->HasField(specPath, SdfFieldKeys->Spline, &spline)) {
        if (spline.GetValueType() != specType.GetType()) {
            addValueTypeMismatchError(
                spline.GetValueType().GetTypeName(), "spline");
        }
    }

    return errors;
}

static UsdValidationErrorVector
_GetAttributeTypeMismatchErrors(const UsdPrim &usdPrim, 
                               const UsdValidationTimeRange &/*timeRange*/)
{
    UsdValidationErrorVector errors;
    for (const UsdAttribute &attr : usdPrim.GetAttributes()) {
        const SdfValueTypeName expectedType = attr.GetTypeName();
        for (const SdfPropertySpecHandle &spec : attr.GetPropertyStack()) {
            const UsdValidationErrorVector specErrors =
                _ReportAttributeSpecTypeAndValueTypeMismatch(
                    spec, attr, expectedType);
            errors.insert(errors.end(), specErrors.begin(), specErrors.end());
        }
    }
    return errors;
}

static UsdValidationErrorVector
_GetAttributeSpecValueTypeMismatchErrors(const SdfLayerHandle &layer)
{
    UsdValidationErrorVector errors;

    layer->Traverse(SdfPath::AbsoluteRootPath(), [&](const SdfPath &path) {
        const UsdValidationErrorVector specErrors =
            _ReportAttributeSpecTypeAndValueTypeMismatch(
                layer->GetAttributeAtPath(path));
        errors.insert(errors.end(), specErrors.begin(), specErrors.end());
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
        UsdValidatorNameTokens->attributeSpecValueTypeMismatch,
        _GetAttributeSpecValueTypeMismatchErrors);
}

PXR_NAMESPACE_CLOSE_SCOPE
