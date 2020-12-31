//
// Copyright 2020 benmalartre
//
// Unlicensed
//
#ifndef PXR_IMAGING_LOFI_FIELD_SUBTEXTURE_IDENTIFIER_H
#define PXR_IMAGING_LOFI_FIELD_SUBTEXTURE_IDENTIFIER_H

#include "pxr/imaging/plugin/LoFi/subtextureIdentifier.h"

PXR_NAMESPACE_OPEN_SCOPE

///
/// \class LoFiOpenVDBAssetSubtextureIdentifier
///
/// Identifies a grid in an OpenVDB file. Parallels OpenVDBAsset in usdVol.
///
class LoFiOpenVDBAssetSubtextureIdentifier final
                : public LoFiFieldBaseSubtextureIdentifier
{
public:
    /// C'tor
    ///
    /// fieldName corresponds to the gridName in the OpenVDB file.
    ///
    LOFI_API
    explicit LoFiOpenVDBAssetSubtextureIdentifier(
        TfToken const &fieldName,
        int fieldIndex);

    LOFI_API
    std::unique_ptr<LoFiSubtextureIdentifier> Clone() const override;

    LOFI_API
    ~LoFiOpenVDBAssetSubtextureIdentifier() override;

protected:
    LOFI_API
    ID _Hash() const override;
};

///
/// \class LoFiField3DAssetSubtextureIdentifier
///
/// Identifies the grid in a Field3DAsset file.
/// Parallels Field3DAsset in usdVol.
///
class LoFiField3DAssetSubtextureIdentifier final
                : public LoFiFieldBaseSubtextureIdentifier
{
public:
    /// C'tor
    ///
    /// fieldName corresponds (e.g., density) to the
    ///             layer/attribute name in the Field3D file
    /// fieldIndex corresponds to the
    ///             partition index
    /// fieldPurpose (e.g., BigCloud) corresponds to the
    ///             partition name/grouping
    /// 
    LOFI_API
    explicit LoFiField3DAssetSubtextureIdentifier(
        TfToken const &fieldName,
        int fieldIndex,
        TfToken const &fieldPurpose);

    LOFI_API
    std::unique_ptr<LoFiSubtextureIdentifier> Clone() const override;

    LOFI_API
    TfToken const &GetFieldPurpose() const { return _fieldPurpose; }

    LOFI_API
    ~LoFiField3DAssetSubtextureIdentifier() override;

protected:
    LOFI_API
    ID _Hash() const override;

private:
    TfToken _fieldPurpose;
};


PXR_NAMESPACE_CLOSE_SCOPE

#endif
