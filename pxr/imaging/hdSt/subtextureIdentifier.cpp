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

#include <boost/functional/hash.hpp>

PXR_NAMESPACE_OPEN_SCOPE

////////////////////////////////////////////////////////////////////////////
// HdStSubtextureIdentifier

HdStSubtextureIdentifier::~HdStSubtextureIdentifier() = default;

HdStSubtextureIdentifier::ID
HdStSubtextureIdentifier::Hash() const {
    static ID result = TfToken().Hash();
    return result;
}

////////////////////////////////////////////////////////////////////////////
// HdStVdbSubtextureIdentifier

HdStVdbSubtextureIdentifier::HdStVdbSubtextureIdentifier(
    TfToken const &gridName)
 : _gridName(gridName)
{
}

HdStVdbSubtextureIdentifier::~HdStVdbSubtextureIdentifier() = default;

std::unique_ptr<HdStSubtextureIdentifier>
HdStVdbSubtextureIdentifier::Clone() const
{
    return std::make_unique<HdStVdbSubtextureIdentifier>(GetGridName());
}

HdStSubtextureIdentifier::ID
HdStVdbSubtextureIdentifier::Hash() const
{
    static ID typeHash = TfToken("vdb").Hash();

    ID hash = typeHash;
    boost::hash_combine(hash, _gridName.Hash());

    return hash;
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
HdStAssetUvSubtextureIdentifier::Hash() const
{
    return TfHash()(*this);
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
HdStDynamicUvSubtextureIdentifier::Hash() const
{
    static ID result = TfToken("dynamicTexture").Hash();
    return result;
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
HdStUdimSubtextureIdentifier::Hash() const
{
    return TfHash()(*this);
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
HdStPtexSubtextureIdentifier::Hash() const
{
    static ID premultiplyAlphaTrue = TfToken("premultiplyAlpha").Hash();
    static ID premultiplyAlphaFalse = TfToken("noPremultiplyAlpha").Hash();

    if (GetPremultiplyAlpha()) {
        return premultiplyAlphaTrue;
    } else {
        return premultiplyAlphaFalse;
    }
}

PXR_NAMESPACE_CLOSE_SCOPE
