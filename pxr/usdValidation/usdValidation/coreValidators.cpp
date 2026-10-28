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
#include "pxr/usd/usd/attributeLimits.h"
#include "pxr/usd/usd/primDefinition.h"
#include "pxr/usd/sdf/schema.h"

#include "pxr/base/gf/traits.h"
#include "pxr/base/gf/vec2d.h"
#include "pxr/base/gf/vec2f.h"
#include "pxr/base/gf/vec2h.h"
#include "pxr/base/gf/vec2i.h"
#include "pxr/base/gf/vec3d.h"
#include "pxr/base/gf/vec3f.h"
#include "pxr/base/gf/vec3h.h"
#include "pxr/base/gf/vec3i.h"
#include "pxr/base/gf/vec4d.h"
#include "pxr/base/gf/vec4f.h"
#include "pxr/base/gf/vec4h.h"
#include "pxr/base/gf/vec4i.h"
#include "pxr/base/tf/stringUtils.h"
#include "pxr/base/vt/array.h"

#include <algorithm>
#include <optional>
#include <string>
#include <vector>

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

// Return the default time, if timeRange includes it, followed by the
// attribute's time samples within timeRange's interval.
static std::vector<UsdTimeCode>
_GetTimeCodesToValidate(const UsdAttribute &attr,
                        const UsdValidationTimeRange &timeRange)
{
    std::vector<UsdTimeCode> timeCodes;
    if (timeRange.IncludesTimeCodeDefault()) {
        timeCodes.push_back(UsdTimeCode::Default());
    }
    std::vector<double> times;
    attr.GetTimeSamplesInInterval(timeRange.GetInterval(), &times);
    timeCodes.insert(timeCodes.end(), times.begin(), times.end());
    return timeCodes;
}

static std::string
_GetTimeCodeDescription(const UsdTimeCode &timeCode)
{
    if (timeCode.IsDefault()) {
        return std::string();
    }
    return TfStringPrintf(" at time %s",
                          TfStringify(timeCode.GetValue()).c_str());
}

static VtValue
_GetSchemaFallbackValue(const UsdAttribute &attr)
{
    VtValue fallback;
    const UsdPrimDefinition::Attribute attrDef = attr.GetPrim()
        .GetPrimDefinition().GetAttributeDefinition(attr.GetName());
    if (attrDef) {
        attrDef.GetFallbackValue(&fallback);
    }
    return fallback;
}

template <class T>
static bool
_IsNan(const T &value)
{
    return value != value;
}

// Return whether a and b are equal, comparing GfVec values component-wise and
// treating NaN as equal to NaN.
template <class T>
static bool
_IsSameValue(const T &a, const T &b)
{
    if constexpr (GfIsGfVec<T>::value) {
        for (size_t i = 0; i < T::dimension; ++i) {
            if (!_IsSameValue(a[i], b[i])) {
                return false;
            }
        }
        return true;
    } else {
        return a == b || (_IsNan(a) && _IsNan(b));
    }
}

// Return whether value lies within [minimum, maximum], comparing GfVec values
// component-wise. A null bound is absent. NaN never lies within the limits.
template <class T>
static bool
_IsWithinLimits(const T &value, const T *minimum, const T *maximum)
{
    if constexpr (GfIsGfVec<T>::value) {
        for (size_t i = 0; i < T::dimension; ++i) {
            if ((minimum && !((*minimum)[i] <= value[i])) ||
                (maximum && !(value[i] <= (*maximum)[i]))) {
                return false;
            }
        }
        return true;
    } else {
        return (!minimum || *minimum <= value) &&
               (!maximum || value <= *maximum);
    }
}

// Return whether value equals the fallback or lies within the limits, or an
// empty optional if the value type is not one of T, Ts. The minimum, maximum
// and fallback values are ignored unless they hold the same type as value.
template <class T, class... Ts>
static std::optional<bool>
_IsAllowedByHardLimitsAs(const VtValue &value,
                         const VtValue &minimum,
                         const VtValue &maximum,
                         const VtValue &fallback)
{
    if (value.IsHolding<T>()) {
        const T &typedValue = value.UncheckedGet<T>();
        if (fallback.IsHolding<T>() &&
            _IsSameValue(typedValue, fallback.UncheckedGet<T>())) {
            return true;
        }
        return _IsWithinLimits(
            typedValue,
            minimum.IsHolding<T>() ? &minimum.UncheckedGet<T>() : nullptr,
            maximum.IsHolding<T>() ? &maximum.UncheckedGet<T>() : nullptr);
    }
    if constexpr (sizeof...(Ts) > 0) {
        return _IsAllowedByHardLimitsAs<Ts...>(
            value, minimum, maximum, fallback);
    } else {
        return std::nullopt;
    }
}

static std::optional<bool>
_IsAllowedByHardLimits(const VtValue &value,
                       const VtValue &minimum,
                       const VtValue &maximum,
                       const VtValue &fallback)
{
    return _IsAllowedByHardLimitsAs<
        unsigned char, int, unsigned int, int64_t, uint64_t,
        GfHalf, float, double,
        GfVec2i, GfVec2h, GfVec2f, GfVec2d,
        GfVec3i, GfVec3h, GfVec3f, GfVec3d,
        GfVec4i, GfVec4h, GfVec4f, GfVec4d>(
            value, minimum, maximum, fallback);
}

// Validates that authored attribute values lie within the attribute's hard
// limits, at the default time and at each time sample in timeRange. Soft
// limits, spline values, and values that are not scalar numbers or GfVecs
// (including arrays) are not validated.
static UsdValidationErrorVector
_GetAttributeHardLimitsErrors(const UsdPrim &usdPrim,
                              const UsdValidationTimeRange &timeRange)
{
    UsdValidationErrorVector errors;
    for (const UsdAttribute &attr : usdPrim.GetAuthoredAttributes()) {
        const UsdAttributeLimits limits = attr.GetHardLimits();
        VtValue minimum = limits.GetMinimum();
        VtValue maximum = limits.GetMaximum();
        if (minimum.IsEmpty() && maximum.IsEmpty()) {
            continue;
        }

        // Conform the limits to the attribute's value type, as
        // UsdAttributeLimits::Validate does. Limits that cannot be conformed
        // are invalid, and are not validated against.
        const VtValue typeDefault = attr.GetTypeName().GetDefaultValue();
        if (!minimum.IsEmpty()) {
            minimum = VtValue::CastToTypeOf(minimum, typeDefault);
            if (minimum.IsEmpty()) {
                continue;
            }
        }
        if (!maximum.IsEmpty()) {
            maximum = VtValue::CastToTypeOf(maximum, typeDefault);
            if (maximum.IsEmpty()) {
                continue;
            }
        }

        // A schema fallback outside the hard limits is a deliberate sentinel
        // value, otherwise every prim that does not author the attribute
        // would be invalid.
        const VtValue fallback = _GetSchemaFallbackValue(attr);
        for (const UsdTimeCode &timeCode :
                _GetTimeCodesToValidate(attr, timeRange)) {
            VtValue value;
            if (!attr.Get(&value, timeCode) ||
                value.GetType() != typeDefault.GetType()) {
                continue;
            }
            const std::optional<bool> isAllowed =
                _IsAllowedByHardLimits(value, minimum, maximum, fallback);
            if (!isAllowed) {
                // The value type has no ordering that limits can apply to.
                break;
            }
            if (*isAllowed) {
                continue;
            }
            const UsdValidationErrorSites attributeErrorSites = {
                UsdValidationErrorSite(usdPrim.GetStage(), attr.GetPath()) };
            errors.emplace_back(
                UsdValidationErrorNameTokens->attributeOutsideHardLimits,
                UsdValidationErrorType::Error, attributeErrorSites,
                TfStringPrintf("Value %s%s for attribute <%s> is outside its "
                               "hard limits [%s, %s].",
                               TfStringify(value).c_str(),
                               _GetTimeCodeDescription(timeCode).c_str(),
                               attr.GetPath().GetText(),
                               minimum.IsEmpty() ? "-inf" :
                                   TfStringify(minimum).c_str(),
                               maximum.IsEmpty() ? "inf" :
                                   TfStringify(maximum).c_str()));
            // Report only the first value outside the hard limits.
            break;
        }
    }
    return errors;
}

// Return the first token or string in value that is not one of allowedTokens,
// or an empty optional if there is none or value does not hold a token,
// string, or array of them.
static std::optional<std::string>
_FindDisallowedToken(const VtValue &value, const VtTokenArray &allowedTokens)
{
    const auto isAllowed = [&allowedTokens](const std::string &token) {
        return std::find(allowedTokens.cbegin(), allowedTokens.cend(), token)
            != allowedTokens.cend();
    };
    if (value.IsHolding<TfToken>()) {
        const std::string &token = value.UncheckedGet<TfToken>().GetString();
        if (!isAllowed(token)) {
            return token;
        }
    } else if (value.IsHolding<VtTokenArray>()) {
        for (const TfToken &token : value.UncheckedGet<VtTokenArray>()) {
            if (!isAllowed(token.GetString())) {
                return token.GetString();
            }
        }
    } else if (value.IsHolding<std::string>()) {
        const std::string &token = value.UncheckedGet<std::string>();
        if (!isAllowed(token)) {
            return token;
        }
    } else if (value.IsHolding<VtStringArray>()) {
        for (const std::string &token : value.UncheckedGet<VtStringArray>()) {
            if (!isAllowed(token)) {
                return token;
            }
        }
    }
    return std::nullopt;
}

// Validates that authored token, string, token[] and string[] attribute
// values are among the attribute's allowedTokens, at the default time and at
// each time sample in timeRange.
static UsdValidationErrorVector
_GetAttributeAllowedTokensErrors(const UsdPrim &usdPrim,
                                 const UsdValidationTimeRange &timeRange)
{
    UsdValidationErrorVector errors;
    for (const UsdAttribute &attr : usdPrim.GetAuthoredAttributes()) {
        VtTokenArray allowedTokens;
        if (!attr.GetMetadata(SdfFieldKeys->AllowedTokens, &allowedTokens) ||
            allowedTokens.empty()) {
            continue;
        }

        // A schema fallback that is not one of the allowed tokens is a
        // deliberate sentinel value, otherwise every prim that does not
        // author the attribute would be invalid.
        const VtValue fallback = _GetSchemaFallbackValue(attr);
        for (const UsdTimeCode &timeCode :
                _GetTimeCodesToValidate(attr, timeRange)) {
            VtValue value;
            if (!attr.Get(&value, timeCode) || value == fallback) {
                continue;
            }
            const std::optional<std::string> disallowedToken =
                _FindDisallowedToken(value, allowedTokens);
            if (!disallowedToken) {
                continue;
            }
            std::vector<std::string> quotedAllowedTokens;
            quotedAllowedTokens.reserve(allowedTokens.size());
            for (const TfToken &token : allowedTokens) {
                quotedAllowedTokens.push_back("'" + token.GetString() + "'");
            }
            const UsdValidationErrorSites attributeErrorSites = {
                UsdValidationErrorSite(usdPrim.GetStage(), attr.GetPath()) };
            errors.emplace_back(
                UsdValidationErrorNameTokens->attributeTokenNotAllowed,
                UsdValidationErrorType::Error, attributeErrorSites,
                TfStringPrintf("Value '%s'%s for attribute <%s> is not one of "
                               "its allowed tokens [%s].",
                               disallowedToken->c_str(),
                               _GetTimeCodeDescription(timeCode).c_str(),
                               attr.GetPath().GetText(),
                               TfStringJoin(quotedAllowedTokens, ", ")
                                   .c_str()));
            // Report only the first value that is not allowed.
            break;
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
    registry.RegisterPluginValidator(
        UsdValidatorNameTokens->attributeHardLimits,
        _GetAttributeHardLimitsErrors);
    registry.RegisterPluginValidator(
        UsdValidatorNameTokens->attributeAllowedTokens,
        _GetAttributeAllowedTokensErrors);
}

PXR_NAMESPACE_CLOSE_SCOPE
