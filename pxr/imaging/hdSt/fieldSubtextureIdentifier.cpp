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

#include "pxr/imaging/hdSt/fieldSubtextureIdentifier.h"

PXR_NAMESPACE_OPEN_SCOPE

////////////////////////////////////////////////////////////////////////////
// HdStOpenVDBAssetSubtextureIdentifier

HdStOpenVDBAssetSubtextureIdentifier::HdStOpenVDBAssetSubtextureIdentifier(
    TfToken const &fieldName, const int fieldIndex)
  : HdStFieldBaseSubtextureIdentifier(fieldName, fieldIndex)
{
}

HdStOpenVDBAssetSubtextureIdentifier::~HdStOpenVDBAssetSubtextureIdentifier()
 = default;

std::unique_ptr<HdStSubtextureIdentifier>
HdStOpenVDBAssetSubtextureIdentifier::Clone() const
{
    return std::make_unique<HdStOpenVDBAssetSubtextureIdentifier>(
        GetFieldName(), GetFieldIndex());
}

HdStSubtextureIdentifier::ID
HdStOpenVDBAssetSubtextureIdentifier::_Hash() const
{
    static ID typeHash =
        TfHash()(std::string("vdb"));

    return TfHash::Combine(
        typeHash,
        HdStFieldBaseSubtextureIdentifier::_Hash());
}

////////////////////////////////////////////////////////////////////////////
// HdStField3DAssetSubtextureIdentifier

HdStField3DAssetSubtextureIdentifier::HdStField3DAssetSubtextureIdentifier(
    TfToken const &fieldName,
    const int fieldIndex,
    TfToken const &fieldPurpose)
  : HdStFieldBaseSubtextureIdentifier(fieldName, fieldIndex)
  , _fieldPurpose(fieldPurpose)
{
}

HdStField3DAssetSubtextureIdentifier::~HdStField3DAssetSubtextureIdentifier()
 = default;

std::unique_ptr<HdStSubtextureIdentifier>
HdStField3DAssetSubtextureIdentifier::Clone() const
{
    return std::make_unique<HdStField3DAssetSubtextureIdentifier>(
        GetFieldName(), GetFieldIndex(), GetFieldPurpose());
}

HdStSubtextureIdentifier::ID
HdStField3DAssetSubtextureIdentifier::_Hash() const
{
    static ID typeHash =
        TfHash()(std::string("Field3D"));

    return TfHash::Combine(
        typeHash,
        HdStFieldBaseSubtextureIdentifier::_Hash(),
        _fieldPurpose);
}

PXR_NAMESPACE_CLOSE_SCOPE
