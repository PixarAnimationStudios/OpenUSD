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
#include "pxr/usd/sdr/shaderNodeMetadata.h"

PXR_NAMESPACE_OPEN_SCOPE

TF_DEFINE_PUBLIC_TOKENS(SdrNodeMetadata, SDR_NODE_METADATA_TOKENS);
TF_DEFINE_PUBLIC_TOKENS(SdrNodeContext, SDR_NODE_CONTEXT_TOKENS);
TF_DEFINE_PUBLIC_TOKENS(SdrNodeRole, SDR_NODE_ROLE_TOKENS);

namespace {
// The following _LegacyStringTo* and _LegacyStringFrom* functions are helpers
// to get/set values from legacy metadata. These can be removed once the
// deprecated SdrTokenMap metadata is completely removed in favor of
// SdrShaderNodeMetadata.

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

std::string _LegacyStringFromSdrStringVec(const VtValue& value) {
    TF_VERIFY(value.IsHolding<SdrStringVec>());

    const SdrStringVec& stringVec = value.UncheckedGet<SdrStringVec>();
    return TfStringJoin(stringVec, "|");
}

VtValue _LegacyStringToString(const std::string& value) {
    return VtValue(value);
}

VtValue _LegacyStringToTfToken(const std::string& value) {
    return VtValue(TfToken(value));
}

VtValue _LegacyStringToSdrUsdEncodingVersion(const std::string& value) {
    try {
        return VtValue(std::stoi(value));
    } catch (...) {
        // SdrShaderNode expects that bogus values are defaulted to -1
        // for SdrUsdEncodingVersion.
        TF_WARN("_LegacyStringToSdrUsdEncodingVersion: unable to parse "
                "int from given string '%s', setting -1", value.c_str());
        return VtValue(-1);
    }
}

VtValue _LegacyStringToSdrTokenVec(const std::string& value) {
    SdrTokenVec tokenized;
    for (const std::string& item : TfStringSplit(value, "|")) {
        tokenized.push_back(TfToken(item));
    }
    return VtValue(tokenized);
}

VtValue _LegacyStringToSdrStringVec(const std::string& value) {
    return VtValue(TfStringSplit(value, "|"));
}

const FromLegacyFnMap& _GetFromLegacyFnMap() {
    static const FromLegacyFnMap _fromLegacyFns = {
        {SdrNodeMetadata->Label, _LegacyStringToTfToken},
        {SdrNodeMetadata->Category, _LegacyStringToTfToken},
        {SdrNodeMetadata->Role, _LegacyStringToTfToken},
        {SdrNodeMetadata->Help, _LegacyStringToString},
        {SdrNodeMetadata->Departments, _LegacyStringToSdrTokenVec},
        {SdrNodeMetadata->Pages, _LegacyStringToSdrTokenVec},
        {SdrNodeMetadata->OpenPages, _LegacyStringToSdrTokenVec},
        {SdrNodeMetadata->Primvars, _LegacyStringToSdrStringVec},
        {SdrNodeMetadata->ImplementationName, _LegacyStringToString},
        {SdrNodeMetadata->SdrUsdEncodingVersion,
            _LegacyStringToSdrUsdEncodingVersion},
        {SdrNodeMetadata->SdrDefinitionNameFallbackPrefix,
            _LegacyStringToString}
    };
    return _fromLegacyFns;
}

const ToLegacyFnMap& _GetToLegacyFnMap() {
    static const ToLegacyFnMap _toLegacyFns = {
        {SdrNodeMetadata->Label, _LegacyStringFromTfToken},
        {SdrNodeMetadata->Category, _LegacyStringFromTfToken},
        {SdrNodeMetadata->Role, _LegacyStringFromTfToken},
        {SdrNodeMetadata->Help, _LegacyStringFromString},
        {SdrNodeMetadata->Departments, _LegacyStringFromSdrTokenVec},
        {SdrNodeMetadata->Pages, _LegacyStringFromSdrTokenVec},
        {SdrNodeMetadata->OpenPages, _LegacyStringFromSdrTokenVec},
        {SdrNodeMetadata->Primvars, _LegacyStringFromSdrStringVec},
        {SdrNodeMetadata->ImplementationName, _LegacyStringFromString},
        {SdrNodeMetadata->SdrUsdEncodingVersion, _LegacyStringFromInt},
        {SdrNodeMetadata->SdrDefinitionNameFallbackPrefix,
            _LegacyStringFromString}
    };
    return _toLegacyFns;
}

// When a client attempts to set a "named metadata" item with
// SdrShaderNodeMetadata::SetItem (bypassing the named metadata API e.g. SetLabel),
// _defaults helps enforce that these metadata items have the expected types.
const DefaultsMap& _GetDefaultsMap() {
    static const DefaultsMap _defaults = {
        {SdrNodeMetadata->Label, VtValue(TfToken(""))},
        {SdrNodeMetadata->Category, VtValue(TfToken(""))},
        {SdrNodeMetadata->Role, VtValue(TfToken(""))},
        {SdrNodeMetadata->Help, VtValue(std::string(""))},
        {SdrNodeMetadata->Departments, VtValue(SdrTokenVec())},
        {SdrNodeMetadata->Pages, VtValue(SdrTokenVec())},
        {SdrNodeMetadata->OpenPages, VtValue(SdrTokenVec())},
        {SdrNodeMetadata->PagesShownIf, VtValue(SdrTokenMap())},
        {SdrNodeMetadata->Primvars, VtValue(SdrStringVec())},
        {SdrNodeMetadata->ImplementationName, VtValue(std::string(""))},
        {SdrNodeMetadata->SdrUsdEncodingVersion, VtValue(int(-1))},
        {SdrNodeMetadata->SdrDefinitionNameFallbackPrefix,
            VtValue(std::string(""))}
    };
    return _defaults;
}
} // anonymous namespace

SdrShaderNodeMetadata::SdrShaderNodeMetadata(const SdrTokenMap& legacyMetadata)
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
            SetItem(key, it->second(value));
        }

    }
}

SdrShaderNodeMetadata::SdrShaderNodeMetadata(const VtDictionary& items) {
    // Run SetItem on each metadata item to process valid casts for named
    // metadata items.
    for (const auto& kv : items) {
        SetItem(TfToken(kv.first), kv.second);
    }
}

SdrShaderNodeMetadata::SdrShaderNodeMetadata(VtDictionary&& items) {
    // Run SetItem on each metadata item to process valid casts for named
    // metadata items.
    for (const auto& kv : items) {
        SetItem(TfToken(kv.first), kv.second);
    }
}

bool
SdrShaderNodeMetadata::HasItem(const TfToken& key) const
{
    return _items.find(key) != _items.end();
}

void
SdrShaderNodeMetadata::SetItem(
    const TfToken& key,
    const VtValue& value)
{
    if (value.IsEmpty()) {
        ClearItem(key);
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
            return;
        } else {
            _items[key] = cast;
        }
    }
}

VtValue
SdrShaderNodeMetadata::GetItemValue(const TfToken& key) const
{
    const auto it = _items.find(key);
    if (it != _items.end()) {
        return it->second;
    }

    return VtValue();
}

void
SdrShaderNodeMetadata::ClearItem(const TfToken& key)
{
    _items.erase(key);
}

SdrTokenMap
SdrShaderNodeMetadata::_EncodeLegacyMetadata() const
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
                // constructed a SdrShaderNodeMetadata with a custom value
                // not directly compatible with string. This indicates
                // that the user is aware of the metadata upgrade in general,
                // and should also use SdrShaderNode::GetMetadataObject
                // instead of the deprecated SdrShaderNode::GetMetadata to
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
#define SDR_NODE_METADATA_DEFINE_ITEM_API(name, T)              \
    bool SdrShaderNodeMetadata::Has##name() const               \
        { return HasItem(SdrNodeMetadata->name); }              \
    T SdrShaderNodeMetadata::Get##name() const                  \
        { return GetItemValueAs<T>(SdrNodeMetadata->name); }    \
    void SdrShaderNodeMetadata::Set##name(const T& v)           \
        { SetItem(SdrNodeMetadata->name, v); }                  \
    void SdrShaderNodeMetadata::Clear##name()                   \
        { ClearItem(SdrNodeMetadata->name); }

SDR_NODE_METADATA_DEFINE_ITEM_API(Label, TfToken)
SDR_NODE_METADATA_DEFINE_ITEM_API(Category, TfToken)
SDR_NODE_METADATA_DEFINE_ITEM_API(Role, TfToken)
SDR_NODE_METADATA_DEFINE_ITEM_API(Help, std::string)
SDR_NODE_METADATA_DEFINE_ITEM_API(Departments, SdrTokenVec)
SDR_NODE_METADATA_DEFINE_ITEM_API(Pages, SdrTokenVec)
SDR_NODE_METADATA_DEFINE_ITEM_API(OpenPages, SdrTokenVec)
SDR_NODE_METADATA_DEFINE_ITEM_API(PagesShownIf, SdrTokenMap)
SDR_NODE_METADATA_DEFINE_ITEM_API(Primvars, SdrStringVec)
SDR_NODE_METADATA_DEFINE_ITEM_API(ImplementationName, std::string)
SDR_NODE_METADATA_DEFINE_ITEM_API(SdrUsdEncodingVersion, int)
SDR_NODE_METADATA_DEFINE_ITEM_API(SdrDefinitionNameFallbackPrefix, std::string)

#undef SDR_METADATA_DEFINE_ITEM_API


PXR_NAMESPACE_CLOSE_SCOPE
