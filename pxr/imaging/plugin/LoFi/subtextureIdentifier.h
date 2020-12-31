//
// Copyright 2020 benmalartre
//
// Unlicensed
//
#ifndef PXR_IMAGING_LOFI_SUBTEXTURE_IDENTIFIER_H
#define PXR_IMAGING_LOFI_SUBTEXTURE_IDENTIFIER_H

#include "pxr/pxr.h"
#include "pxr/imaging/plugin/LoFi/api.h"

#include "pxr/base/tf/token.h"

#include <memory>

PXR_NAMESPACE_OPEN_SCOPE

class LoFiDynamicUvTextureImplementation;

///
/// \class LoFiSubtextureIdentifier
///
/// Base class for additional information to identify a texture in a
/// file that can contain several textures (e.g., frames in a movie or
/// grids in an OpenVDB file).
/// 
class LoFiSubtextureIdentifier
{
public:
    using ID = size_t;

    LOFI_API
    virtual std::unique_ptr<LoFiSubtextureIdentifier> Clone() const = 0;

    LOFI_API
    virtual ~LoFiSubtextureIdentifier();

protected:
    LOFI_API
    friend size_t hash_value(const LoFiSubtextureIdentifier &subId);

    virtual ID _Hash() const = 0;
};

LOFI_API
size_t hash_value(const LoFiSubtextureIdentifier &subId);

///
/// \class LoFiFieldBaseSubtextureIdentifier
///
/// Base class for information identifying a grid in a volume field
/// file. Parallels FieldBase in usdVol.
///
class LoFiFieldBaseSubtextureIdentifier : public LoFiSubtextureIdentifier
{
public:
    /// Get field name.
    ///
    LOFI_API
    TfToken const &GetFieldName() const { return _fieldName; }

    /// Get field index.
    ///
    LOFI_API
    int GetFieldIndex() const { return _fieldIndex; }

    LOFI_API
    ~LoFiFieldBaseSubtextureIdentifier() override = 0;
    
protected:
    LOFI_API
    LoFiFieldBaseSubtextureIdentifier(TfToken const &fieldName, int fieldIndex);

    LOFI_API
    ID _Hash() const override;

private:
    TfToken _fieldName;
    int _fieldIndex;
};

///
/// \class LoFiAssetUvSubtextureIdentifier
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
class LoFiAssetUvSubtextureIdentifier final
                                : public LoFiSubtextureIdentifier
{
public:
    /// C'tor takes bool whether flipping vertically, whether to pre-multiply
    /// by alpha, and the texture's source color space
    LOFI_API
    explicit LoFiAssetUvSubtextureIdentifier(bool flipVertically, 
                                             bool premultiplyAlpha,
					                         const TfToken& sourceColorSpace);

    LOFI_API
    std::unique_ptr<LoFiSubtextureIdentifier> Clone() const override;

    LOFI_API
    bool GetFlipVertically() const { return _flipVertically; }

    LOFI_API
    bool GetPremultiplyAlpha() const { return _premultiplyAlpha; }
    
    LOFI_API
    TfToken GetSourceColorSpace() const { return _sourceColorSpace; }
    
    LOFI_API
    ~LoFiAssetUvSubtextureIdentifier() override;

protected:
    LOFI_API
    ID _Hash() const override;

private:
    bool _flipVertically;
    bool _premultiplyAlpha;
    TfToken _sourceColorSpace;
};

///
/// \class LoFiDynamicUvSubtextureIdentifier
///
/// Used as a tag that the Storm texture system returns a
/// LoFiDynamicUvTextureObject that is populated by a client rather
/// than by the Storm texture system.
///
/// Clients can subclass this class and provide their own
/// LoFiDynamicUvTextureImplementation to create UV texture with custom
/// load and commit behavior.
///
/// testLoFiDynamicUvTexture.cpp is an example of how custom load and
/// commit behavior can be implemented.
///
/// AOV's are another example. In presto, these are baked by
/// LoFiDynamicUvTextureObject's. In this case, the
/// LoFiDynamicUvTextureObject's do not provide custom load or commit
/// behavior (null-ptr returned by GetTextureImplementation). Instead,
/// GPU memory is allocated by explicitly calling
/// LoFiDynamicUvTextureObject::CreateTexture in
/// LoFiRenderBuffer::Sync/Allocate and the texture is filled by using
/// it as render target in various render passes.
///
class LoFiDynamicUvSubtextureIdentifier : public LoFiSubtextureIdentifier
{
public:
    LOFI_API
    LoFiDynamicUvSubtextureIdentifier();

    LOFI_API
    ~LoFiDynamicUvSubtextureIdentifier() override;
    
    LOFI_API
    std::unique_ptr<LoFiSubtextureIdentifier> Clone() const override;

    /// Textures can return their own HdStDynamicUvTextureImplementation
    /// to customize the load and commit behavior.
    LOFI_API
    virtual LoFiDynamicUvTextureImplementation *GetTextureImplementation() const;

protected:
    LOFI_API
    ID _Hash() const override;
};

///
/// \class LoFiPtexSubtextureIdentifier
///
/// Specifies whether a Ptex texture should be loaded with pre-multiplied alpha
/// values.
///
class LoFiPtexSubtextureIdentifier final
                                : public LoFiSubtextureIdentifier
{
public:
    /// C'tor takes bool whether to pre-multiply by alpha
    LOFI_API
    explicit LoFiPtexSubtextureIdentifier(bool premultiplyAlpha);

    LOFI_API
    std::unique_ptr<LoFiSubtextureIdentifier> Clone() const override;

    LOFI_API
    bool GetPremultiplyAlpha() const { return _premultiplyAlpha; }
    
    LOFI_API
    ~LoFiPtexSubtextureIdentifier() override;

protected:
    LOFI_API
    ID _Hash() const override;

private:
    bool _premultiplyAlpha;
};

///
/// \class LoFiUdimSubtextureIdentifier
///
/// Specifies whether a Udim texture should be loaded with pre-multiplied alpha
/// values and the color space in which the texture is encoded.
///
class LoFiUdimSubtextureIdentifier final
                                : public LoFiSubtextureIdentifier
{
public:
    /// C'tor takes bool whether to pre-multiply by alpha and the texture's 
    /// source color space
    LOFI_API
    explicit LoFiUdimSubtextureIdentifier(bool premultiplyAlpha,
                                          const TfToken& sourceColorSpace);

    LOFI_API
    std::unique_ptr<LoFiSubtextureIdentifier> Clone() const override;

    LOFI_API
    bool GetPremultiplyAlpha() const { return _premultiplyAlpha; }

    LOFI_API
    TfToken GetSourceColorSpace() const { return _sourceColorSpace; }
    
    LOFI_API
    ~LoFiUdimSubtextureIdentifier() override;

protected:
    LOFI_API
    ID _Hash() const override;

private:
    bool _premultiplyAlpha;
    TfToken _sourceColorSpace;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif
