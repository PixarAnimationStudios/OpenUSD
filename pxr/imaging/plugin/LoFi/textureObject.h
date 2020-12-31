//
// Copyright 2020 benmalartre
//
// Unlicensed
//
#ifndef PXR_IMAGING_LOFI_TEXTURE_OBJECT_H
#define PXR_IMAGING_LOFI_TEXTURE_OBJECT_H

#include "pxr/pxr.h"
#include "pxr/imaging/plugin/LoFi/api.h"

#include "pxr/imaging/plugin/LoFi/textureIdentifier.h"
#include "pxr/imaging/hd/enums.h"
#include "pxr/imaging/hd/types.h"

#include "pxr/imaging/hgi/handle.h"
#include "pxr/imaging/hio/image.h"

#include "pxr/base/gf/bbox3d.h"
#include "pxr/base/tf/declarePtrs.h"

#include <memory>

PXR_NAMESPACE_OPEN_SCOPE

class Hgi;
using HgiTextureHandle = HgiHandle<class HgiTexture>;
class LoFiTextureObjectRegistry;
struct HgiTextureDesc;
class LoFiTextureCpuData;
class LoFiResourceRegistry;

using LoFiTextureObjectSharedPtr = std::shared_ptr<class LoFiTextureObject>;

/// \class LoFiTextureObject
/// 
/// Base class for a texture object. The actual GPU resources will be
/// allocated during the commit phase.
///
class LoFiTextureObject :
            public std::enable_shared_from_this<LoFiTextureObject>
{
public:
    /// Get texture identifier
    ///
    const LoFiTextureIdentifier &
    GetTextureIdentifier() const { return _textureId; }

    /// Get the target memory for the texture.
    ///
    size_t GetTargetMemory() const { return _targetMemory; }

    /// Set the target memory (in bytes).
    ///
    /// When uploading the texture to the GPU, it will be downsampled
    /// to meet this target memory.
    ///
    LOFI_API
    void SetTargetMemory(size_t);

    /// Is texture valid? Only correct after commit phase.
    ///
    /// E.g., no file at given file path. Consulted by clients to
    /// determine whether to use the fallback value.
    ///
    LOFI_API
    virtual bool IsValid() const = 0;

    /// Get texture type
    ///
    LOFI_API
    virtual HdTextureType GetTextureType() const = 0;

    LOFI_API
    virtual ~LoFiTextureObject();

protected:
    LoFiTextureObject(
        const LoFiTextureIdentifier &textureId,
        LoFiTextureObjectRegistry * textureObjectRegistry);

    LOFI_API
    LoFiResourceRegistry* _GetResourceRegistry() const;

    LOFI_API
    Hgi* _GetHgi() const;
    
    LOFI_API
    std::string _GetDebugName(const LoFiTextureIdentifier &textureId) const;

    LOFI_API
    bool
    _GetPremultiplyAlpha(const LoFiSubtextureIdentifier * const subId) const;

    LOFI_API
    HioImage::SourceColorSpace
    _GetSourceColorSpace(const LoFiSubtextureIdentifier * const subId) const;

    /// Load texture to CPU (thread-safe)
    ///
    LOFI_API
    virtual void _Load() = 0;

    /// Commit texture to GPU (not thread-safe)
    ///
    LOFI_API
    virtual void _Commit() = 0;

    /// Add signed number to total texture memory amount maintained by
    /// registry.
    ///
    LOFI_API
    void _AdjustTotalTextureMemory(int64_t memDiff);

    /// Compute memory of texture and add to total texture memory
    /// amount maintained by registry.
    LOFI_API
    void _AddToTotalTextureMemory(const HgiTextureHandle &texture);

    /// Compute memory of texture and subtract to total texture memory
    /// amount maintained by registry.
    LOFI_API
    void _SubtractFromTotalTextureMemory(const HgiTextureHandle &texture);

private:
    friend class LoFiTextureObjectRegistry;

    LoFiTextureObjectRegistry * const _textureObjectRegistry;
    const LoFiTextureIdentifier _textureId;
    size_t _targetMemory;
};

/// \class LoFiUvTextureObject
///
/// A base class for uv textures.
///
class LoFiUvTextureObject : public LoFiTextureObject
{
public:
    ~LoFiUvTextureObject() override;

    /// Get the handle to the actual GPU resource.
    ///
    /// Only valid after commit phase.
    ///
    HgiTextureHandle const &GetTexture() const {
        return _gpuTexture;
    }

    /// Opinion about wrapS and wrapT parameters from the texture file.
    ///
    /// Only valid after commit phase. Can be HdWrapNoOpinion.
    const std::pair<HdWrap, HdWrap> &GetWrapParameters() const {
        return _wrapParameters;
    }

    LOFI_API
    HdTextureType GetTextureType() const override final;

protected:
    LoFiUvTextureObject(
        const LoFiTextureIdentifier &textureId,
        LoFiTextureObjectRegistry * textureObjectRegistry);

    void _SetWrapParameters(
        const std::pair<HdWrap, HdWrap> &wrapParameters);

    void _SetCpuData(std::unique_ptr<LoFiTextureCpuData> &&);
    LoFiTextureCpuData * _GetCpuData() const;

    void _CreateTexture(const HgiTextureDesc &desc);
    void _GenerateMipmaps();
    void _DestroyTexture();

private:
    std::pair<HdWrap, HdWrap> _wrapParameters;
    std::unique_ptr<LoFiTextureCpuData> _cpuData;
    HgiTextureHandle _gpuTexture;
};

/// \class HdAssetStUvTextureObject
///
/// A uv texture loading the asset identified by the texture identifier.
///
class LoFiAssetUvTextureObject final : public LoFiUvTextureObject
{
public:
    LOFI_API
    LoFiAssetUvTextureObject(
        const LoFiTextureIdentifier &textureId,
        LoFiTextureObjectRegistry *textureObjectRegistry);

    LOFI_API
    ~LoFiAssetUvTextureObject() override;

    LOFI_API
    bool IsValid() const override;

protected:
    LOFI_API
    void _Load() override;

    LOFI_API
    void _Commit() override;
};

/// \class LoFiFieldTextureObject
///
/// A uvw texture with a bounding box describing how to transform it.
///
class LoFiFieldTextureObject final : public LoFiTextureObject
{
public:
    LOFI_API
    LoFiFieldTextureObject(
        const LoFiTextureIdentifier &textureId,
        LoFiTextureObjectRegistry *textureObjectRegistry);

    LOFI_API
    ~LoFiFieldTextureObject() override;

    /// Get the handle to the actual GPU resource.
    ///
    /// Only valid after commit phase.
    ///
    HgiTextureHandle const &GetTexture() const {
        return _gpuTexture;
    }

    /// The box the texture fills out.
    ///
    /// Only valid after the commit phase.
    ///
    const GfBBox3d &GetBoundingBox() const { return _bbox; }

    /// The sampling transform.
    ///
    /// Only valid after the commit phase.
    ///
    const GfMatrix4d &GetSamplingTransform() const {
        return _samplingTransform;
    }

    LOFI_API
    bool IsValid() const override;

    LOFI_API
    HdTextureType GetTextureType() const override;

protected:
    LOFI_API
    void _Load() override;

    LOFI_API
    void _Commit() override;

private:
    std::unique_ptr<LoFiTextureCpuData> _cpuData;
    GfBBox3d _bbox;
    GfMatrix4d _samplingTransform;
    HgiTextureHandle _gpuTexture;
};

template<HdTextureType textureType>
struct LoFiTypedTextureObjectHelper;

/// \class LoFiTypedTextureObject
///
/// A template alias such that, e.g., LoFiUvTextureObject can be
/// accessed as LoFiTypedTextureObject<HdTextureType::Uv>.
///
template<HdTextureType textureType>
using LoFiTypedTextureObject =
    typename LoFiTypedTextureObjectHelper<textureType>::type;

template<>
struct LoFiTypedTextureObjectHelper<HdTextureType::Uv> {
    using type = LoFiUvTextureObject;
};

template<>
struct LoFiTypedTextureObjectHelper<HdTextureType::Field> {
    using type = LoFiFieldTextureObject;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif
