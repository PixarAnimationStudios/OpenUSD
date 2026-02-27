//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/pxr.h"
#include "pxr/base/tf/staticTokens.h"
#include "pxr/base/tf/stringUtils.h"
#include "pxr/base/vt/value.h"
#include "pxr/usd/sdr/shaderMetadataHelpers.h"
#include "pxr/usd/sdr/shaderPropertyMetadata.h"

#include <algorithm>

PXR_NAMESPACE_OPEN_SCOPE

TF_DEFINE_PUBLIC_TOKENS(SdrPropertyMetadata, SDR_PROPERTY_METADATA_TOKENS);
TF_DEFINE_PUBLIC_TOKENS(SdrPropertyRole, SDR_PROPERTY_ROLE_TOKENS);
TF_DEFINE_PUBLIC_TOKENS(SdrPropertyTokens, SDR_PROPERTY_TOKENS);

namespace {

using FromLegacyFn = std::function<VtValue (std::string)>;
using ToLegacyFn = std::function<std::string (VtValue)>;
using FromLegacyFnMap =
    std::unordered_map<TfToken, FromLegacyFn, TfToken::HashFunctor>;
using ToLegacyFnMap =
    std::unordered_map<TfToken, ToLegacyFn, TfToken::HashFunctor>;
using DefaultsMap =
    std::unordered_map<TfToken, VtValue, TfToken::HashFunctor>;


std::string _LegacyStringFromString(const VtValue& value) {
    TF_VERIFY(value.IsHolding<std::string>());
    return value.UncheckedGet<std::string>();
}

std::string _LegacyStringFromTfToken(const VtValue& value) {
    TF_VERIFY(value.IsHolding<TfToken>());
    return value.UncheckedGet<TfToken>().GetString();
}

std::string _LegacyStringFromInt(const VtValue& value) {
    TF_VERIFY(value.IsHolding<int>());
    return TfStringify(value);
}

std::string _LegacyStringFromSdrTokenVec(const VtValue& value) {
    TF_VERIFY(value.IsHolding<SdrTokenVec>());

    const SdrTokenVec& tokenVec = value.UncheckedGet<SdrTokenVec>();
    const SdrStringVec stringVec(tokenVec.begin(), tokenVec.end());
    return TfStringJoin(stringVec, "|");
}

std::string _LegacyStringFromBool(const VtValue& value) {
    TF_VERIFY(value.IsHolding<bool>());

    return value.UncheckedGet<bool>() ? "1": "0";
}

VtValue _LegacyStringToString(const std::string& value) {
    return VtValue(value);
}

VtValue _LegacyStringToTfToken(const std::string& value) {
    return VtValue(TfToken(value));
}

VtValue _LegacyStringToSdrTokenVec(const std::string& value) {
    SdrTokenVec tokenized;
    for (const std::string& item : TfStringSplit(value, "|")) {
        tokenized.push_back(TfToken(item));
    }
    return VtValue(tokenized);
}

VtValue _LegacyStringToBool(const std::string& value) {
    // See ShaderMetadataHelpers::IsTruthy
    std::string boolStr = value;
    std::transform(boolStr.begin(), boolStr.end(), boolStr.begin(), ::tolower);
    if (boolStr == "0" || boolStr == "false" || boolStr == "f") {
        return VtValue(false);
    }
    return VtValue(true);
}

VtValue _LegacyStringToInt(const std::string& value) {
    try {
        return VtValue(std::stoi(value));
    } catch (...) {
        TF_WARN("_LegacyStringToInt: unable to parse "
                "int from given string '%s', setting 0", value.c_str());
        return VtValue(0);
    }
}

const FromLegacyFnMap& _GetFromLegacyFnMap() {
    static const FromLegacyFnMap _fromLegacyFns = {
        {SdrPropertyMetadata->Label, _LegacyStringToTfToken},
        {SdrPropertyMetadata->Help, _LegacyStringToString},
        {SdrPropertyMetadata->Page, _LegacyStringToTfToken},
        {SdrPropertyMetadata->RenderType, _LegacyStringToString},
        {SdrPropertyMetadata->Role, _LegacyStringToString},
        {SdrPropertyMetadata->Widget, _LegacyStringToTfToken},
        {SdrPropertyMetadata->IsDynamicArray, _LegacyStringToBool},
        {SdrPropertyMetadata->TupleSize, _LegacyStringToInt},
        {SdrPropertyMetadata->Connectable, _LegacyStringToBool},
        {SdrPropertyMetadata->ShownIf, _LegacyStringToString},
        {SdrPropertyMetadata->ValidConnectionTypes,
            _LegacyStringToSdrTokenVec},
        {SdrPropertyMetadata->IsAssetIdentifier, _LegacyStringToBool},
        {SdrPropertyMetadata->ImplementationName, _LegacyStringToString},
        {SdrPropertyMetadata->SdrUsdDefinitionType, _LegacyStringToTfToken},
        {SdrPropertyMetadata->DefaultInput, _LegacyStringToBool},
        {SdrPropertyMetadata->Colorspace, _LegacyStringToTfToken}
    };
    return _fromLegacyFns;
}

const ToLegacyFnMap& _GetToLegacyFnMap() {
    static const ToLegacyFnMap _toLegacyFns = {
        {SdrPropertyMetadata->Label, _LegacyStringFromTfToken},
        {SdrPropertyMetadata->Help, _LegacyStringFromString},
        {SdrPropertyMetadata->Page, _LegacyStringFromTfToken},
        {SdrPropertyMetadata->RenderType, _LegacyStringFromString},
        {SdrPropertyMetadata->Role, _LegacyStringFromString},
        {SdrPropertyMetadata->Widget, _LegacyStringFromTfToken},
        {SdrPropertyMetadata->IsDynamicArray, _LegacyStringFromBool},
        {SdrPropertyMetadata->TupleSize, _LegacyStringFromInt},
        {SdrPropertyMetadata->Connectable, _LegacyStringFromBool},
        {SdrPropertyMetadata->ShownIf, _LegacyStringFromString},
        {SdrPropertyMetadata->ValidConnectionTypes,
            _LegacyStringFromSdrTokenVec},
        {SdrPropertyMetadata->IsAssetIdentifier, _LegacyStringFromBool},
        {SdrPropertyMetadata->ImplementationName, _LegacyStringFromString},
        {SdrPropertyMetadata->SdrUsdDefinitionType, _LegacyStringFromTfToken},
        {SdrPropertyMetadata->DefaultInput, _LegacyStringFromBool},
        {SdrPropertyMetadata->Colorspace, _LegacyStringFromTfToken}
    };
    return _toLegacyFns;
}

// When a client attempts to set a "named metadata" item with
// SdrShaderPropertyMetadata::SetItem (bypassing the named metadata API e.g.
// SetLabel), this map helps enforce that these metadata items have the
// expected types.
const DefaultsMap& _GetDefaultsMap() {
    static const DefaultsMap _defaults = {
        {SdrPropertyMetadata->Label, VtValue(TfToken(""))},
        {SdrPropertyMetadata->Help, VtValue(std::string(""))},
        {SdrPropertyMetadata->Page, VtValue(TfToken(""))},
        {SdrPropertyMetadata->RenderType, VtValue(std::string(""))},
        {SdrPropertyMetadata->Role, VtValue(std::string())},
        {SdrPropertyMetadata->Widget, VtValue(TfToken(""))},
        {SdrPropertyMetadata->IsDynamicArray, VtValue(false)},
        {SdrPropertyMetadata->TupleSize, VtValue(0)},
        {SdrPropertyMetadata->Connectable, VtValue(false)},
        {SdrPropertyMetadata->ShownIf, VtValue(std::string(""))},
        {SdrPropertyMetadata->ValidConnectionTypes, VtValue(SdrTokenVec())},
        {SdrPropertyMetadata->IsAssetIdentifier, VtValue(false)},
        {SdrPropertyMetadata->ImplementationName, VtValue(std::string())},
        {SdrPropertyMetadata->SdrUsdDefinitionType, VtValue(TfToken(""))},
        {SdrPropertyMetadata->DefaultInput, VtValue(false)},
        {SdrPropertyMetadata->Colorspace, VtValue(TfToken(""))}
    };
    return _defaults;
}

} // anonymous namespace

SdrShaderPropertyMetadata::SdrShaderPropertyMetadata(
    const SdrTokenMap& legacyMetadata)
{
    for (const auto& kv : legacyMetadata) {
        const TfToken& key = kv.first;
        const std::string& value = kv.second;
        const FromLegacyFnMap& _fromLegacyFns =_GetFromLegacyFnMap();
        const auto it = _fromLegacyFns.find(key);
        if (it == _fromLegacyFns.end()) {
            // If there's no legacy converter, set a VtValue
            // holding a string.
            SetItem(key, VtValue(value));
        } else {
            VtValue val = it->second(value);
            SetItem(key, val);
        }

    }
}

bool
SdrShaderPropertyMetadata::HasItem(const TfToken& key) const
{
    return _items.find(key) != _items.end();
}


void
SdrShaderPropertyMetadata::SetItem(
    const TfToken& key,
    const VtValue& value)
{
    if (value.IsEmpty()) {
        TF_CODING_ERROR("SetItem: Cannot set empty VtValue.");
        return;
    }

    const DefaultsMap& _defaults = _GetDefaultsMap();
    const auto it = _defaults.find(key);
    if (it == _defaults.end()) {
        _items[key] = value;
    } else {
        const VtValue cast = VtValue::CastToTypeOf(value, it->second);
        if (cast.IsEmpty()) {
            TF_CODING_ERROR("SetItem: Unable to convert given value"
                            "to the registered type for key %s, "
                            "item not set", key.GetText());
        } else {
            _items[key] = cast;
        }
    }
}

VtValue
SdrShaderPropertyMetadata::GetItemValue(const TfToken& key) const
{
    const auto it = _items.find(key);
    if (it != _items.end()) {
        return it->second;
    }

    return VtValue();
}

void
SdrShaderPropertyMetadata::ClearItem(const TfToken& key)
{
    _items.erase(key);
}

SdrTokenMap
SdrShaderPropertyMetadata::_EncodeLegacyMetadata() const
{
    SdrTokenMap legacyMetadata;
    for (const auto& it : _items) {
        const TfToken key = TfToken(it.first);
        const VtValue& value = it.second;
        std::string encoded;

        const ToLegacyFnMap& _toLegacyFns = _GetToLegacyFnMap();
        const auto itFn = _toLegacyFns.find(key);
        if (itFn == _toLegacyFns.end()) {
            // We don't use TfStringify because not every data type
            // can be stringified generically. Try getting a string as
            // a fallback for unnamed metadata, but otherwise don't
            // add metadata items that don't have a legacy encoding.
            const VtValue cast = VtValue::Cast<std::string>(value);
            if (cast.IsEmpty()) {
                // Do nothing here. If we get to this point, the user has
                // constructed a SdrShaderPropertyMetadata with a custom value
                // not directly compatible with string. This indicates
                // that the user is aware of the metadata upgrade in general,
                // and should also use SdrShaderProperty::GetMetadataObject
                // instead of the deprecated SdrShaderProperty::GetMetadata to
                // get their custom value.
            } else {
                legacyMetadata[key] = cast.UncheckedGet<std::string>();
            }
        } else {
            legacyMetadata[key] = itFn->second(value);
        }
    }
    return legacyMetadata;
}

// Defines Has/Get/Set API for metadata key-value items
#define SDR_PROPERTY_METADATA_DEFINE_ITEM_API(name, T)            \
    bool SdrShaderPropertyMetadata::Has##name() const             \
        { return HasItem(SdrPropertyMetadata->name); }            \
    T SdrShaderPropertyMetadata::Get##name() const                \
        { return GetItemValueAs<T>(SdrPropertyMetadata->name); }  \
    void SdrShaderPropertyMetadata::Set##name(const T& v)         \
        { SetItem(SdrPropertyMetadata->name, v); }                \
    void SdrShaderPropertyMetadata::Clear##name()                 \
        { ClearItem(SdrPropertyMetadata->name); }
        
SDR_PROPERTY_METADATA_DEFINE_ITEM_API(Label, TfToken)
SDR_PROPERTY_METADATA_DEFINE_ITEM_API(Help, std::string)
SDR_PROPERTY_METADATA_DEFINE_ITEM_API(Page, TfToken)
SDR_PROPERTY_METADATA_DEFINE_ITEM_API(Role, std::string)
SDR_PROPERTY_METADATA_DEFINE_ITEM_API(RenderType, std::string)
SDR_PROPERTY_METADATA_DEFINE_ITEM_API(Widget, TfToken)
SDR_PROPERTY_METADATA_DEFINE_ITEM_API(IsDynamicArray, bool)
SDR_PROPERTY_METADATA_DEFINE_ITEM_API(TupleSize, int)
SDR_PROPERTY_METADATA_DEFINE_ITEM_API(Connectable, bool)
SDR_PROPERTY_METADATA_DEFINE_ITEM_API(ShownIf, std::string)
SDR_PROPERTY_METADATA_DEFINE_ITEM_API(ValidConnectionTypes, SdrTokenVec)
SDR_PROPERTY_METADATA_DEFINE_ITEM_API(IsAssetIdentifier, bool)
SDR_PROPERTY_METADATA_DEFINE_ITEM_API(ImplementationName, std::string)
SDR_PROPERTY_METADATA_DEFINE_ITEM_API(SdrUsdDefinitionType, TfToken)
SDR_PROPERTY_METADATA_DEFINE_ITEM_API(DefaultInput, bool)
SDR_PROPERTY_METADATA_DEFINE_ITEM_API(Colorspace, TfToken)

#undef SDR_METADATA_DEFINE_ITEM_API

PXR_NAMESPACE_CLOSE_SCOPE
