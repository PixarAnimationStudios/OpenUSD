//
// Copyright 2020 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
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
    virtual ~HdStSubtextureIdentifier();

protected:
    HDST_API
    friend size_t hash_value(const HdStSubtextureIdentifier &subId);

    virtual ID _Hash() const = 0;
};

HDST_API
size_t hash_value(const HdStSubtextureIdentifier &subId);

///
/// \class HdStFieldBaseSubtextureIdentifier
///
/// Base class for information identifying a grid in a volume field
/// file. Parallels FieldBase in usdVol.
///
class HdStFieldBaseSubtextureIdentifier : public HdStSubtextureIdentifier
{
public:
    /// Get field name.
    ///
    HDST_API
    TfToken const &GetFieldName() const { return _fieldName; }

    /// Get field index.
    ///
    HDST_API
    int GetFieldIndex() const { return _fieldIndex; }

    HDST_API
    ~HdStFieldBaseSubtextureIdentifier() override = 0;
    
protected:
    HDST_API
    HdStFieldBaseSubtextureIdentifier(TfToken const &fieldName, int fieldIndex);

    HDST_API
    ID _Hash() const override;

private:
    TfToken _fieldName;
    int _fieldIndex;
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
    bool GetFlipVertically() const { return _flipVertically; }

    HDST_API
    bool GetPremultiplyAlpha() const { return _premultiplyAlpha; }
    
    HDST_API
    TfToken GetSourceColorSpace() const { return _sourceColorSpace; }
    
    HDST_API
    ~HdStAssetUvSubtextureIdentifier() override;

protected:
    HDST_API
    ID _Hash() const override;

private:
    bool _flipVertically;
    bool _premultiplyAlpha;
    TfToken _sourceColorSpace;
};

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

protected:
    HDST_API
    ID _Hash() const override;
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
    bool GetPremultiplyAlpha() const { return _premultiplyAlpha; }
    
    HDST_API
    ~HdStPtexSubtextureIdentifier() override;

protected:
    HDST_API
    ID _Hash() const override;

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
    bool GetPremultiplyAlpha() const { return _premultiplyAlpha; }

    HDST_API
    TfToken GetSourceColorSpace() const { return _sourceColorSpace; }
    
    HDST_API
    ~HdStUdimSubtextureIdentifier() override;

protected:
    HDST_API
    ID _Hash() const override;

private:
    bool _premultiplyAlpha;
    TfToken _sourceColorSpace;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif
