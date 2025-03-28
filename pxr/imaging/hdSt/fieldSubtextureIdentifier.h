//
// Copyright 2020 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_IMAGING_HD_ST_FIELD_SUBTEXTURE_IDENTIFIER_H
#define PXR_IMAGING_HD_ST_FIELD_SUBTEXTURE_IDENTIFIER_H

#include "pxr/imaging/hdSt/subtextureIdentifier.h"

PXR_NAMESPACE_OPEN_SCOPE

///
/// \class HdStOpenVDBAssetSubtextureIdentifier
///
/// Identifies a grid in an OpenVDB file. Parallels OpenVDBAsset in usdVol.
///
class HdStOpenVDBAssetSubtextureIdentifier final
                : public HdStFieldBaseSubtextureIdentifier
{
public:
    /// C'tor
    ///
    /// fieldName corresponds to the gridName in the OpenVDB file.
    ///
    HDST_API
    explicit HdStOpenVDBAssetSubtextureIdentifier(
        TfToken const &fieldName,
        int fieldIndex);

    HDST_API
    std::unique_ptr<HdStSubtextureIdentifier> Clone() const override;

    HDST_API
    ~HdStOpenVDBAssetSubtextureIdentifier() override;

protected:
    HDST_API
    ID _Hash() const override;
};

///
/// \class HdStField3DAssetSubtextureIdentifier
///
/// Identifies the grid in a Field3DAsset file.
/// Parallels Field3DAsset in usdVol.
///
class HdStField3DAssetSubtextureIdentifier final
                : public HdStFieldBaseSubtextureIdentifier
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
    HDST_API
    explicit HdStField3DAssetSubtextureIdentifier(
        TfToken const &fieldName,
        int fieldIndex,
        TfToken const &fieldPurpose);

    HDST_API
    std::unique_ptr<HdStSubtextureIdentifier> Clone() const override;

    HDST_API
    TfToken const &GetFieldPurpose() const { return _fieldPurpose; }

    HDST_API
    ~HdStField3DAssetSubtextureIdentifier() override;

protected:
    HDST_API
    ID _Hash() const override;

private:
    TfToken _fieldPurpose;
};


PXR_NAMESPACE_CLOSE_SCOPE

#endif
