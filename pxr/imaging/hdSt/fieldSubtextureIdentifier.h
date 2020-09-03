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
