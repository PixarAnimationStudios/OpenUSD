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
#ifndef PXR_IMAGING_HD_ST_SUBTEXTURE_IDENTIFIER_H
#define PXR_IMAGING_HD_ST_SUBTEXTURE_IDENTIFIER_H

#include "pxr/pxr.h"
#include "pxr/imaging/hdSt/api.h"

#include "pxr/base/tf/token.h"

#include <memory>

PXR_NAMESPACE_OPEN_SCOPE

///
/// \class HdStSubtextureIdentifier
///
/// Base class for additional information to identify a texture in a
/// file that can contain several textures (e.g., frames in a movie or
/// grids in an OpenVDB file).
/// 
class HdStSubtextureIdentifier
{
public:
    using ID = size_t;

    HDST_API
    virtual std::unique_ptr<HdStSubtextureIdentifier> Clone() const = 0;

    HDST_API
    virtual ID Hash() const;

    HDST_API
    virtual ~HdStSubtextureIdentifier();
};

///
/// \class HdStVdbSubtextureIdentifier
///
/// Identifies the grid in an OpenVDB file by its name.
///
class HdStVdbSubtextureIdentifier final : public HdStSubtextureIdentifier
{
public:
    /// C'tor using name of grid in OpenVDB file
    HDST_API
    explicit HdStVdbSubtextureIdentifier(TfToken const &gridName);

    HDST_API
    std::unique_ptr<HdStSubtextureIdentifier> Clone() const override;

    HDST_API
    ID Hash() const override;

    /// Name of grid in OpenVDB file
    HDST_API
    TfToken const &GetGridName() const { return _gridName; }

    HDST_API
    ~HdStVdbSubtextureIdentifier() override;

private:
    TfToken _gridName;
};

///
/// \class HdStUvOrientationSubtextureIdentifier
///
/// Specifies whether a UV texture should be loaded flipped vertically.
///
/// This class is here for the texture system to support both the
/// legacy HwUvTexture_1 (flipVertically = true) and UsdUvTexture
/// (flipVertically = false) which have opposite conventions for the
/// vertical orientation.
///
class HdStUvOrientationSubtextureIdentifier final
                                : public HdStSubtextureIdentifier
{
public:
    /// C'tor takes bool whether flipping vertically
    HDST_API
    explicit HdStUvOrientationSubtextureIdentifier(bool flipVertically);

    HDST_API
    std::unique_ptr<HdStSubtextureIdentifier> Clone() const override;

    HDST_API
    ID Hash() const override;

    HDST_API
    bool GetFlipVertically() const { return _flipVertically; }
    
    HDST_API
    ~HdStUvOrientationSubtextureIdentifier() override;

private:
    bool _flipVertically;
};

///
/// \class HdStDynamicUvSubtextureIdentifier
///
/// Used as a tag that the Storm texture system returns a
/// HdStDynamicUvTextureObject that is populated by a client rather
/// than by the Storm texture system.
///
class HdStDynamicUvSubtextureIdentifier final
                                : public HdStSubtextureIdentifier
{
public:
    HDST_API
    HdStDynamicUvSubtextureIdentifier();

    HDST_API
    ~HdStDynamicUvSubtextureIdentifier() override;
    
    HDST_API
    std::unique_ptr<HdStSubtextureIdentifier> Clone() const override;

    HDST_API
    ID Hash() const override;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif
