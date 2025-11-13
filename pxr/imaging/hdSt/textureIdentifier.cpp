//
// Copyright 2020 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/imaging/hdSt/textureIdentifier.h"

#include "pxr/imaging/hdSt/subtextureIdentifier.h"

PXR_NAMESPACE_OPEN_SCOPE

static
std::unique_ptr<const HdStSubtextureIdentifier>
_CloneSubtextureId(
        std::unique_ptr<const HdStSubtextureIdentifier> const &subtextureId) {
    if (subtextureId) {
        return subtextureId->Clone();
    }
    return nullptr;
}

HdStTextureIdentifier::HdStTextureIdentifier() = default;

HdStTextureIdentifier::HdStTextureIdentifier(
    const TfToken &filePath,
    const VtValue &fallback,
    bool defaultToFallback)
  : _filePath(filePath),
    _fallback(fallback),
    _defaultToFallback(defaultToFallback)
{
}

HdStTextureIdentifier::HdStTextureIdentifier(
    const TfToken &filePath,
    std::unique_ptr<const HdStSubtextureIdentifier> &&subtextureId,
    const VtValue &fallback,
    bool defaultToFallback)
  : _filePath(filePath),
    _fallback(fallback),
    _defaultToFallback(defaultToFallback),
    _subtextureId(std::move(subtextureId))
{
}

HdStTextureIdentifier::HdStTextureIdentifier(
    const HdStTextureIdentifier &textureId)
  : _filePath(textureId._filePath),
    _fallback(textureId._fallback),
    _defaultToFallback(textureId._defaultToFallback),
    _subtextureId(_CloneSubtextureId(textureId._subtextureId))
{
}

HdStTextureIdentifier &
HdStTextureIdentifier::operator=(HdStTextureIdentifier &&textureId) = default;

HdStTextureIdentifier &
HdStTextureIdentifier::operator=(const HdStTextureIdentifier &textureId)
{
    _filePath = textureId._filePath;
    _fallback = textureId._fallback;
    _defaultToFallback = textureId._defaultToFallback;
    _subtextureId = _CloneSubtextureId(textureId._subtextureId);

    return *this;
}

HdStTextureIdentifier::~HdStTextureIdentifier() = default;

static
std::pair<bool, HdStTextureIdentifier::ID>
_OptionalSubidentifierHash(const HdStTextureIdentifier &id)
{
    if (const HdStSubtextureIdentifier * subId = id.GetSubtextureIdentifier()) {
        return {true, TfHash()(*subId)};
    }
    return {false, 0};
}

bool
HdStTextureIdentifier::operator==(const HdStTextureIdentifier &other) const
{
    return
        _filePath == other._filePath &&
        _fallback == other._fallback &&
        _defaultToFallback == other._defaultToFallback &&
        _OptionalSubidentifierHash(*this) == _OptionalSubidentifierHash(other);
}

bool
HdStTextureIdentifier::operator!=(const HdStTextureIdentifier &other) const
{
    return !(*this == other);
}

size_t
hash_value(const HdStTextureIdentifier &id)
{
    if (const HdStSubtextureIdentifier * const subId =
                                    id.GetSubtextureIdentifier()) {
        return TfHash::Combine(id.GetFilePath(), id.GetFallback(),
            id.ShouldDefaultToFallback(), *subId);
    } else {
        return TfHash::Combine(id.GetFilePath(), id.GetFallback(),
            id.ShouldDefaultToFallback());
    }
}

PXR_NAMESPACE_CLOSE_SCOPE
