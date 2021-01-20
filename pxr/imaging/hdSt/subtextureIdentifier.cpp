//
// Copyright 2020 Pixar
//
// Licensed under the Apache License, Version 2.0 (the "Apache License")
// with the following modification; you may not use this file except in
// compliance with the Apache License and the following modification to it:
// Section 6. Trademarks. is deleted and replaced with:
//
// 6. Trademarks. This License does not grant permission to use the trade
//    names, trademarks, service marks, or product names of the Licensor
//    and its affiliates, except as required to comply with Section 4(c) of
//    the License and to reproduce the content of the NOTICE file.
//
// You may obtain a copy of the Apache License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the Apache License with the above modification is
// distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
// KIND, either express or implied. See the Apache License for the specific
// language governing permissions and limitations under the Apache License.
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
