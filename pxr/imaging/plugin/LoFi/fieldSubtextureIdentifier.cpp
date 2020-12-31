//
// Copyright 2020 benmalartre
//
// Unlicensed
//

#include "pxr/imaging/plugin/LoFi/fieldSubtextureIdentifier.h"

PXR_NAMESPACE_OPEN_SCOPE

////////////////////////////////////////////////////////////////////////////
// LoFiOpenVDBAssetSubtextureIdentifier

LoFiOpenVDBAssetSubtextureIdentifier::LoFiOpenVDBAssetSubtextureIdentifier(
    TfToken const &fieldName, const int fieldIndex)
  : LoFiFieldBaseSubtextureIdentifier(fieldName, fieldIndex)
{
}

LoFiOpenVDBAssetSubtextureIdentifier::~LoFiOpenVDBAssetSubtextureIdentifier()
 = default;

std::unique_ptr<LoFiSubtextureIdentifier>
LoFiOpenVDBAssetSubtextureIdentifier::Clone() const
{
    return std::make_unique<LoFiOpenVDBAssetSubtextureIdentifier>(
        GetFieldName(), GetFieldIndex());
}

LoFiSubtextureIdentifier::ID
LoFiOpenVDBAssetSubtextureIdentifier::_Hash() const
{
    static ID typeHash =
        TfHash()(std::string("vdb"));

    return TfHash::Combine(
        typeHash,
        LoFiFieldBaseSubtextureIdentifier::_Hash());
}

////////////////////////////////////////////////////////////////////////////
// LoFiField3DAssetSubtextureIdentifier

LoFiField3DAssetSubtextureIdentifier::LoFiField3DAssetSubtextureIdentifier(
    TfToken const &fieldName,
    const int fieldIndex,
    TfToken const &fieldPurpose)
  : LoFiFieldBaseSubtextureIdentifier(fieldName, fieldIndex)
  , _fieldPurpose(fieldPurpose)
{
}

LoFiField3DAssetSubtextureIdentifier::~LoFiField3DAssetSubtextureIdentifier()
 = default;

std::unique_ptr<LoFiSubtextureIdentifier>
LoFiField3DAssetSubtextureIdentifier::Clone() const
{
    return std::make_unique<LoFiField3DAssetSubtextureIdentifier>(
        GetFieldName(), GetFieldIndex(), GetFieldPurpose());
}

LoFiSubtextureIdentifier::ID
LoFiField3DAssetSubtextureIdentifier::_Hash() const
{
    static ID typeHash =
        TfHash()(std::string("Field3D"));

    return TfHash::Combine(
        typeHash,
        LoFiFieldBaseSubtextureIdentifier::_Hash(),
        _fieldPurpose);
}

PXR_NAMESPACE_CLOSE_SCOPE
