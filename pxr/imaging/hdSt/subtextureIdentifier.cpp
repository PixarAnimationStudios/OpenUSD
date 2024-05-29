//
// Copyright 2020 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/imaging/hdSt/subtextureIdentifier.h"

#include "pxr/base/tf/hash.h"

PXR_NAMESPACE_OPEN_SCOPE

////////////////////////////////////////////////////////////////////////////
// HdStSubtextureIdentifier

HdStSubtextureIdentifier::~HdStSubtextureIdentifier() = default;

size_t
hash_value(const HdStSubtextureIdentifier &subId)
{
    return subId._Hash();
}

////////////////////////////////////////////////////////////////////////////
// HdStFieldBaseSubtextureIdentifier
HdStFieldBaseSubtextureIdentifier::HdStFieldBaseSubtextureIdentifier(
    TfToken const &fieldName, const int fieldIndex)
  : _fieldName(fieldName), _fieldIndex(fieldIndex)
{
}

HdStFieldBaseSubtextureIdentifier::~HdStFieldBaseSubtextureIdentifier()
    = default;

HdStSubtextureIdentifier::ID
HdStFieldBaseSubtextureIdentifier::_Hash() const {
    return TfHash::Combine(_fieldName, _fieldIndex);
}

////////////////////////////////////////////////////////////////////////////
// HdStAssetUvSubtextureIdentifier

HdStAssetUvSubtextureIdentifier::HdStAssetUvSubtextureIdentifier(
    const bool flipVertically, 
    const bool premultiplyAlpha, 
    const TfToken& sourceColorSpace)
 : _flipVertically(flipVertically), 
   _premultiplyAlpha(premultiplyAlpha), 
   _sourceColorSpace(sourceColorSpace)
{
}

HdStAssetUvSubtextureIdentifier::~HdStAssetUvSubtextureIdentifier()
    = default;

std::unique_ptr<HdStSubtextureIdentifier>
HdStAssetUvSubtextureIdentifier::Clone() const
{
    return std::make_unique<HdStAssetUvSubtextureIdentifier>(
        GetFlipVertically(), GetPremultiplyAlpha(), GetSourceColorSpace());
}

HdStSubtextureIdentifier::ID
HdStAssetUvSubtextureIdentifier::_Hash() const
{
    static ID typeHash =
        TfHash()(std::string("HdStAssetUvSubtextureIdentifier"));

    return TfHash::Combine(
        typeHash,
        GetFlipVertically(),
        GetPremultiplyAlpha(),
        GetSourceColorSpace());
}

////////////////////////////////////////////////////////////////////////////
// HdStDynamicUvSubtextureIdentifier

HdStDynamicUvSubtextureIdentifier::HdStDynamicUvSubtextureIdentifier()
    = default;

HdStDynamicUvSubtextureIdentifier::~HdStDynamicUvSubtextureIdentifier()
    = default;

std::unique_ptr<HdStSubtextureIdentifier>
HdStDynamicUvSubtextureIdentifier::Clone() const
{
    return std::make_unique<HdStDynamicUvSubtextureIdentifier>();
}

HdStSubtextureIdentifier::ID
HdStDynamicUvSubtextureIdentifier::_Hash() const
{
    static ID typeHash =
        TfHash()(std::string("HdStDynamicUvSubtextureIdentifier"));
    return typeHash;
}

HdStDynamicUvTextureImplementation *
HdStDynamicUvSubtextureIdentifier::GetTextureImplementation() const
{
    return nullptr;
}

////////////////////////////////////////////////////////////////////////////
// HdStUdimSubtextureIdentifier

HdStUdimSubtextureIdentifier::HdStUdimSubtextureIdentifier(
    const bool premultiplyAlpha, const TfToken &sourceColorSpace)
 : _premultiplyAlpha(premultiplyAlpha), _sourceColorSpace(sourceColorSpace)
{
}

HdStUdimSubtextureIdentifier::~HdStUdimSubtextureIdentifier()
    = default;

std::unique_ptr<HdStSubtextureIdentifier>
HdStUdimSubtextureIdentifier::Clone() const
{
    return std::make_unique<HdStUdimSubtextureIdentifier>(
        GetPremultiplyAlpha(), GetSourceColorSpace());
}

HdStSubtextureIdentifier::ID
HdStUdimSubtextureIdentifier::_Hash() const
{
    static ID typeHash =
        TfHash()(std::string("HdStUdimSubtextureIdentifier"));

    return TfHash::Combine(
        typeHash,
        GetPremultiplyAlpha(),
        GetSourceColorSpace());
}

////////////////////////////////////////////////////////////////////////////
// HdStPtexSubtextureIdentifier

HdStPtexSubtextureIdentifier::HdStPtexSubtextureIdentifier(
    const bool premultiplyAlpha)
 : _premultiplyAlpha(premultiplyAlpha)
{
}

HdStPtexSubtextureIdentifier::~HdStPtexSubtextureIdentifier()
    = default;

std::unique_ptr<HdStSubtextureIdentifier>
HdStPtexSubtextureIdentifier::Clone() const
{
    return std::make_unique<HdStPtexSubtextureIdentifier>(
        GetPremultiplyAlpha());
}

HdStSubtextureIdentifier::ID
HdStPtexSubtextureIdentifier::_Hash() const
{
    static ID typeHash =
        TfHash()(std::string("HdStPtexSubtextureIdentifier"));

    return TfHash::Combine(
        typeHash,
        GetPremultiplyAlpha());
}

PXR_NAMESPACE_CLOSE_SCOPE
