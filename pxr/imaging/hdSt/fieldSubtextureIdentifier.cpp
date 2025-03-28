//
// Copyright 2020 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
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
