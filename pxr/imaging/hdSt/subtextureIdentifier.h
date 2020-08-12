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

class HdStDynamicUvTextureImplementation;

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
/// \class HdStAssetUvSubtextureIdentifier
///
/// Specifies whether a UV texture should be loaded flipped vertically, whether 
/// it should be loaded with pre-multiplied alpha values, and the color space 
/// in which the texture is encoded.
///
/// The former functionality allows the texture system to support both the
/// legacy HwUvTexture_1 (flipVertically = true) and UsdUvTexture
/// (flipVertically = false) which have opposite conventions for the
/// vertical orientation.
///
class HdStAssetUvSubtextureIdentifier final
                                : public HdStSubtextureIdentifier
{
public:
    /// C'tor takes bool whether flipping vertically, whether to pre-multiply
    /// by alpha, and the texture's source color space
    HDST_API
    explicit HdStAssetUvSubtextureIdentifier(bool flipVertically, 
                                             bool premultiplyAlpha,
					                         const TfToken& sourceColorSpace);

    HDST_API
    std::unique_ptr<HdStSubtextureIdentifier> Clone() const override;

    HDST_API
    ID Hash() const override;

    HDST_API
    bool GetFlipVertically() const { return _flipVertically; }

    HDST_API
    bool GetPremultiplyAlpha() const { return _premultiplyAlpha; }
    
    HDST_API
    TfToken GetSourceColorSpace() const { return _sourceColorSpace; }
    
    HDST_API
    ~HdStAssetUvSubtextureIdentifier() override;

private:
    bool _flipVertically;
    bool _premultiplyAlpha;
    TfToken _sourceColorSpace;
};

template <class HashState>
void
TfHashAppend(HashState &h, HdStAssetUvSubtextureIdentifier const &subId) {
    static size_t vertFlipFalse = TfToken("notVerticallyFlipped").Hash();
    static size_t vertFlipTrue = TfToken("verticallyFlipped").Hash();

    static size_t premulAlphaFalse = TfToken("noPremultiplyAlpha").Hash();
    static size_t premulAlphaTrue = TfToken("premultiplyAlpha").Hash();

    h.Append(subId.GetFlipVertically() ? vertFlipTrue : vertFlipFalse);
    h.Append(subId.GetPremultiplyAlpha() ? premulAlphaTrue :  premulAlphaFalse);
    h.Append(subId.GetSourceColorSpace().Hash());
}

///
/// \class HdStDynamicUvSubtextureIdentifier
///
/// Used as a tag that the Storm texture system returns a
/// HdStDynamicUvTextureObject that is populated by a client rather
/// than by the Storm texture system.
///
/// Clients can subclass this class and provide their own
/// HdStDynamicUvTextureImplementation to create UV texture with custom
/// load and commit behavior.
///
/// testHdStDynamicUvTexture.cpp is an example of how custom load and
/// commit behavior can be implemented.
///
/// AOV's are another example. In presto, these are baked by
/// HdStDynamicUvTextureObject's. In this case, the
/// HdStDynamicUvTextureObject's do not provide custom load or commit
/// behavior (null-ptr returned by GetTextureImplementation). Instead,
/// GPU memory is allocated by explicitly calling
/// HdStDynamicUvTextureObject::CreateTexture in
/// HdStRenderBuffer::Sync/Allocate and the texture is filled by using
/// it as render target in various render passes.
///
class HdStDynamicUvSubtextureIdentifier : public HdStSubtextureIdentifier
{
public:
    HDST_API
    HdStDynamicUvSubtextureIdentifier();

    HDST_API
    ~HdStDynamicUvSubtextureIdentifier() override;
    
    HDST_API
    std::unique_ptr<HdStSubtextureIdentifier> Clone() const override;

    /// Textures can return their own HdStDynamicUvTextureImplementation
    /// to customize the load and commit behavior.
    HDST_API
    virtual HdStDynamicUvTextureImplementation *GetTextureImplementation() const;

    HDST_API
    ID Hash() const override;
};

///
/// \class HdStPtexSubtextureIdentifier
///
/// Specifies whether a Ptex texture should be loaded with pre-multiplied alpha
/// values.
///
class HdStPtexSubtextureIdentifier final
                                : public HdStSubtextureIdentifier
{
public:
    /// C'tor takes bool whether to pre-multiply by alpha
    HDST_API
    explicit HdStPtexSubtextureIdentifier(bool premultiplyAlpha);

    HDST_API
    std::unique_ptr<HdStSubtextureIdentifier> Clone() const override;

    HDST_API
    ID Hash() const override;

    HDST_API
    bool GetPremultiplyAlpha() const { return _premultiplyAlpha; }
    
    HDST_API
    ~HdStPtexSubtextureIdentifier() override;

private:
    bool _premultiplyAlpha;
};

///
/// \class HdStUdimSubtextureIdentifier
///
/// Specifies whether a Udim texture should be loaded with pre-multiplied alpha
/// values and the color space in which the texture is encoded.
///
class HdStUdimSubtextureIdentifier final
                                : public HdStSubtextureIdentifier
{
public:
    /// C'tor takes bool whether to pre-multiply by alpha and the texture's 
    /// source color space
    HDST_API
    explicit HdStUdimSubtextureIdentifier(bool premultiplyAlpha,
                                          const TfToken& sourceColorSpace);

    HDST_API
    std::unique_ptr<HdStSubtextureIdentifier> Clone() const override;

    HDST_API
    ID Hash() const override;

    HDST_API
    bool GetPremultiplyAlpha() const { return _premultiplyAlpha; }

    HDST_API
    TfToken GetSourceColorSpace() const { return _sourceColorSpace; }
    
    HDST_API
    ~HdStUdimSubtextureIdentifier() override;

private:
    bool _premultiplyAlpha;
    TfToken _sourceColorSpace;
};

template <class HashState>
void
TfHashAppend(HashState &h, HdStUdimSubtextureIdentifier const &subId) {
    static size_t premulAlphaFalse = TfToken("noPremultiplyAlpha").Hash();
    static size_t premulAlphaTrue = TfToken("premultiplyAlpha").Hash();

    h.Append(subId.GetPremultiplyAlpha() ? premulAlphaTrue :  premulAlphaFalse,
             subId.GetSourceColorSpace().Hash());
}


PXR_NAMESPACE_CLOSE_SCOPE

#endif
