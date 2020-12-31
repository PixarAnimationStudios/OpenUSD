//
// Copyright 2020 benmalartre
//
// Unlicensed
//
#include "pxr/imaging/plugin/LoFi/subtextureIdentifier.h"

#include "pxr/base/tf/hash.h"

PXR_NAMESPACE_OPEN_SCOPE

////////////////////////////////////////////////////////////////////////////
// LoFiSubtextureIdentifier

LoFiSubtextureIdentifier::~LoFiSubtextureIdentifier() = default;

size_t
hash_value(const LoFiSubtextureIdentifier &subId)
{
    return subId._Hash();
}

////////////////////////////////////////////////////////////////////////////
// LoFiFieldBaseSubtextureIdentifier
LoFiFieldBaseSubtextureIdentifier::LoFiFieldBaseSubtextureIdentifier(
    TfToken const &fieldName, const int fieldIndex)
  : _fieldName(fieldName), _fieldIndex(fieldIndex)
{
}

LoFiFieldBaseSubtextureIdentifier::~LoFiFieldBaseSubtextureIdentifier()
    = default;

LoFiSubtextureIdentifier::ID
LoFiFieldBaseSubtextureIdentifier::_Hash() const {
    return TfHash::Combine(_fieldName, _fieldIndex);
}

////////////////////////////////////////////////////////////////////////////
// LoFiAssetUvSubtextureIdentifier

LoFiAssetUvSubtextureIdentifier::LoFiAssetUvSubtextureIdentifier(
    const bool flipVertically, 
    const bool premultiplyAlpha, 
    const TfToken& sourceColorSpace)
 : _flipVertically(flipVertically), 
   _premultiplyAlpha(premultiplyAlpha), 
   _sourceColorSpace(sourceColorSpace)
{
}

LoFiAssetUvSubtextureIdentifier::~LoFiAssetUvSubtextureIdentifier()
    = default;

std::unique_ptr<LoFiSubtextureIdentifier>
LoFiAssetUvSubtextureIdentifier::Clone() const
{
    return std::make_unique<LoFiAssetUvSubtextureIdentifier>(
        GetFlipVertically(), GetPremultiplyAlpha(), GetSourceColorSpace());
}

LoFiSubtextureIdentifier::ID
LoFiAssetUvSubtextureIdentifier::_Hash() const
{
    static ID typeHash =
        TfHash()(std::string("LoFiAssetUvSubtextureIdentifier"));

    return TfHash::Combine(
        typeHash,
        GetFlipVertically(),
        GetPremultiplyAlpha(),
        GetSourceColorSpace());
}

////////////////////////////////////////////////////////////////////////////
// LoFiDynamicUvSubtextureIdentifier

LoFiDynamicUvSubtextureIdentifier::LoFiDynamicUvSubtextureIdentifier()
    = default;

LoFiDynamicUvSubtextureIdentifier::~LoFiDynamicUvSubtextureIdentifier()
    = default;

std::unique_ptr<LoFiSubtextureIdentifier>
LoFiDynamicUvSubtextureIdentifier::Clone() const
{
    return std::make_unique<LoFiDynamicUvSubtextureIdentifier>();
}

LoFiSubtextureIdentifier::ID
LoFiDynamicUvSubtextureIdentifier::_Hash() const
{
    static ID typeHash =
        TfHash()(std::string("LoFiDynamicUvSubtextureIdentifier"));
    return typeHash;
}

LoFiDynamicUvTextureImplementation *
LoFiDynamicUvSubtextureIdentifier::GetTextureImplementation() const
{
    return nullptr;
}

////////////////////////////////////////////////////////////////////////////
// LoFiUdimSubtextureIdentifier

LoFiUdimSubtextureIdentifier::LoFiUdimSubtextureIdentifier(
    const bool premultiplyAlpha, const TfToken &sourceColorSpace)
 : _premultiplyAlpha(premultiplyAlpha), _sourceColorSpace(sourceColorSpace)
{
}

LoFiUdimSubtextureIdentifier::~LoFiUdimSubtextureIdentifier()
    = default;

std::unique_ptr<LoFiSubtextureIdentifier>
LoFiUdimSubtextureIdentifier::Clone() const
{
    return std::make_unique<LoFiUdimSubtextureIdentifier>(
        GetPremultiplyAlpha(), GetSourceColorSpace());
}

LoFiSubtextureIdentifier::ID
LoFiUdimSubtextureIdentifier::_Hash() const
{
    static ID typeHash =
        TfHash()(std::string("LoFiUdimSubtextureIdentifier"));

    return TfHash::Combine(
        typeHash,
        GetPremultiplyAlpha(),
        GetSourceColorSpace());
}

////////////////////////////////////////////////////////////////////////////
// LoFiPtexSubtextureIdentifier

LoFiPtexSubtextureIdentifier::LoFiPtexSubtextureIdentifier(
    const bool premultiplyAlpha)
 : _premultiplyAlpha(premultiplyAlpha)
{
}

LoFiPtexSubtextureIdentifier::~LoFiPtexSubtextureIdentifier()
    = default;

std::unique_ptr<LoFiSubtextureIdentifier>
LoFiPtexSubtextureIdentifier::Clone() const
{
    return std::make_unique<LoFiPtexSubtextureIdentifier>(
        GetPremultiplyAlpha());
}

LoFiSubtextureIdentifier::ID
LoFiPtexSubtextureIdentifier::_Hash() const
{
    static ID typeHash =
        TfHash()(std::string("LoFiPtexSubtextureIdentifier"));

    return TfHash::Combine(
        typeHash,
        GetPremultiplyAlpha());
}

PXR_NAMESPACE_CLOSE_SCOPE
