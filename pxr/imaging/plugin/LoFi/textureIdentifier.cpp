//
// Copyright 2020 benmalartre
//
// Unlicensed 
//
#include "pxr/imaging/plugin/LoFi/textureIdentifier.h"

#include "pxr/imaging/plugin/LoFi/subtextureIdentifier.h"

PXR_NAMESPACE_OPEN_SCOPE

static
std::unique_ptr<const LoFiSubtextureIdentifier>
_CloneSubtextureId(
        std::unique_ptr<const LoFiSubtextureIdentifier> const &subtextureId) {
    if (subtextureId) {
        return subtextureId->Clone();
    }
    return nullptr;
}

LoFiTextureIdentifier::LoFiTextureIdentifier() = default;

LoFiTextureIdentifier::LoFiTextureIdentifier(
    const TfToken &filePath)
  : _filePath(filePath)
{
}

LoFiTextureIdentifier::LoFiTextureIdentifier(
    const TfToken &filePath,
    std::unique_ptr<const LoFiSubtextureIdentifier> &&subtextureId)
  : _filePath(filePath),
    _subtextureId(std::move(subtextureId))
{
}

LoFiTextureIdentifier::LoFiTextureIdentifier(
    const LoFiTextureIdentifier &textureId)
  : _filePath(textureId._filePath),
    _subtextureId(_CloneSubtextureId(textureId._subtextureId))
{
}

LoFiTextureIdentifier &
LoFiTextureIdentifier::operator=(LoFiTextureIdentifier &&textureId) = default;

LoFiTextureIdentifier &
LoFiTextureIdentifier::operator=(const LoFiTextureIdentifier &textureId)
{
    _filePath = textureId._filePath;
    _subtextureId = _CloneSubtextureId(textureId._subtextureId);

    return *this;
}

LoFiTextureIdentifier::~LoFiTextureIdentifier() = default;

static
std::pair<bool, LoFiTextureIdentifier::ID>
_OptionalSubidentifierHash(const LoFiTextureIdentifier &id)
{
    if (const LoFiSubtextureIdentifier * subId = id.GetSubtextureIdentifier()) {
        return {true, TfHash()(*subId)};
    }
    return {false, 0};
}

bool
LoFiTextureIdentifier::operator==(const LoFiTextureIdentifier &other) const
{
    return
        _filePath == other._filePath &&
        _OptionalSubidentifierHash(*this) == _OptionalSubidentifierHash(other);
}

bool
LoFiTextureIdentifier::operator!=(const LoFiTextureIdentifier &other) const
{
    return !(*this == other);
}

size_t
hash_value(const LoFiTextureIdentifier &id)
{
    if (const LoFiSubtextureIdentifier * const subId =
                                    id.GetSubtextureIdentifier()) {
        return TfHash::Combine(id.GetFilePath(), *subId);
    } else {
        return TfHash()(id.GetFilePath());
    }
}

PXR_NAMESPACE_CLOSE_SCOPE
