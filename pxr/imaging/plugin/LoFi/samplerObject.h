//
// Copyright 2020 benmalartre
//
// Unlicensed
//
#ifndef PXR_IMAGING_LOFI_SAMPLER_OBJECT_H
#define PXR_IMAGING_LOFI_SAMPLER_OBJECT_H

#include "pxr/pxr.h"
#include "pxr/imaging/plugin/LoFi/api.h"

#include "pxr/imaging/hgi/handle.h"
#include "pxr/imaging/hd/enums.h"
#include "pxr/imaging/hd/types.h"

#include <memory>

PXR_NAMESPACE_OPEN_SCOPE

class Hgi;
class LoFiUvTextureObject;
class LoFiFieldTextureObject;
class LoFiPtexTextureObject;
class LoFiUdimTextureObject;
class LoFiSamplerObjectRegistry;
using HgiSamplerHandle = HgiHandle<class HgiSampler>;

using LoFiSamplerObjectSharedPtr =
    std::shared_ptr<class LoFiSamplerObject>;

/// \class LoFiSamplerObject
///
/// A base class encapsulating a GPU sampler object and, optionally, a
/// texture sampler handle (for bindless textures).
///
/// The subclasses of LoFiSamplerObject mirror the subclasses of
/// LoFiTextureObject with the intention that they will be used in
/// conjunction (e.g., the not yet existing LoFiPtexSamplerObject will
/// have two samplers and texture sampler handles for the texels and
/// layout texture in a LoFiPtexTextureObject).
///
/// The GPU resources is con-/destructed immediately in the
/// c'tor/d'tor. By going through the LoFiSamplerObjectRegistry, we
/// can obtain a shared pointer that can safely be dropped in a
/// different thread. The LoFiSamplerObjectRegistry is also dispatching
/// by texture type to construct the corresponding sampler type.
///
class LoFiSamplerObject
{
public:
    virtual ~LoFiSamplerObject() = 0;

protected:
    explicit LoFiSamplerObject(
        LoFiSamplerObjectRegistry * samplerObjectRegistry);

    Hgi* _GetHgi() const;
    LoFiSamplerObjectRegistry * const _samplerObjectRegistry;
};

/// \class LoFiUvSamplerObject
///
/// A sampler suitable for LoFiUvTextureObject.
///
class LoFiUvSamplerObject final : public LoFiSamplerObject {
public:
    LOFI_API 
    LoFiUvSamplerObject(
        LoFiUvTextureObject const &uvTexture,
        HdSamplerParameters const &samplerParameters,
        bool createBindlessHandle,
        LoFiSamplerObjectRegistry * samplerObjectRegistry);

    LOFI_API 
    ~LoFiUvSamplerObject() override;

    /// The sampler.
    ///
    const HgiSamplerHandle &GetSampler() const {
        return _sampler;
    }

    /// The GL sampler texture handle for bindless textures (as returned by
    /// glGetTextureSamplerHandleARB).
    ///
    /// Only available when requested.
    ///
    uint64_t GetGLTextureSamplerHandle() const {
        return _glTextureSamplerHandle;
    }

private:
    HgiSamplerHandle _sampler;
    const uint64_t _glTextureSamplerHandle;
};

/// \class LoFiFieldSamplerObject
///
/// A sampler suitable for LoFiFieldTextureObject.
///
class LoFiFieldSamplerObject final : public LoFiSamplerObject {
public:
    LoFiFieldSamplerObject(
        LoFiFieldTextureObject const &uvTexture,
        HdSamplerParameters const &samplerParameters,
        bool createBindlessHandle,
        LoFiSamplerObjectRegistry * samplerObjectRegistry);

    ~LoFiFieldSamplerObject() override;

    /// The sampler.
    ///
    const HgiSamplerHandle &GetSampler() const {
        return _sampler;
    }

    /// The GL sampler texture handle for bindless textures (as returned by
    /// glGetTextureSamplerHandleARB).
    ///
    /// Only available when requested.
    ///
    uint64_t GetGLTextureSamplerHandle() const {
        return _glTextureSamplerHandle;
    }

private:
    HgiSamplerHandle _sampler;
    const uint64_t _glTextureSamplerHandle;
};

/// \class LoFiPtexSamplerObject
///
/// Ptex doesn't bind samplers, so this class is just holding the
/// texture handles for bindless textures.
///
class LoFiPtexSamplerObject final : public LoFiSamplerObject {
public:
    LoFiPtexSamplerObject(
        LoFiPtexTextureObject const &ptexTexture,
        // samplerParameters are ignored by ptex
        HdSamplerParameters const &samplerParameters,
        bool createBindlessHandle,
        LoFiSamplerObjectRegistry * samplerObjectRegistry);

    ~LoFiPtexSamplerObject() override;

    /// The GPU sampler object for the texels texture.
    ///
    const HgiSamplerHandle &GetTexelsSampler() const {
        return _texelsSampler;
    }

    /// The GL texture handle for bindless textures (as returned by
    /// glGetTextureHandleARB). This is for texels.
    ///
    /// Only available when requested.
    ///
    uint64_t GetTexelsGLTextureHandle() const {
        return _texelsGLTextureHandle;
    }

    /// Similar to GetGLTexelsTextureHandle but for layout.
    ///
    uint64_t GetLayoutGLTextureHandle() const {
        return _layoutGLTextureHandle;
    }

private:
    HgiSamplerHandle _texelsSampler;

    const uint64_t _texelsGLTextureHandle;
    const uint64_t _layoutGLTextureHandle;
};

/// \class LoFiUdimSamplerObject
///
/// A sampler suitable for Udim textures (wraps one GPU sampler
/// for the texels texture).
///
class LoFiUdimSamplerObject final : public LoFiSamplerObject {
public:
    LoFiUdimSamplerObject(
        LoFiUdimTextureObject const &ptexTexture,
        // samplerParameters are ignored by udim (at least for now)
        HdSamplerParameters const &samplerParameters,
        bool createBindlessHandle,
        LoFiSamplerObjectRegistry * samplerObjectRegistry);

    ~LoFiUdimSamplerObject() override;

    /// The GPU sampler object for the texels texture.
    ///
    const HgiSamplerHandle &GetTexelsSampler() const {
        return _texelsSampler;
    }

    /// The GL texture handle for bindless textures (as returned by
    /// glGetTextureHandleARB). This is for texels.
    ///
    /// Only available when requested.
    ///
    uint64_t GetTexelsGLTextureHandle() const {
        return _texelsGLTextureHandle;
    }

    /// Similar to GetGLTexelsTextureHandle but for layout.
    ///
    uint64_t GetLayoutGLTextureHandle() const {
        return _layoutGLTextureHandle;
    }

private:
    HgiSamplerHandle _texelsSampler;

    const uint64_t _texelsGLTextureHandle;
    const uint64_t _layoutGLTextureHandle;
};

template<HdTextureType textureType>
struct LoFiTypedSamplerObjectHelper;

/// \class LoFiTypedSamplerObject
///
/// A template alias such that, e.g., LoFiUvSamplerObject can be
/// accessed as LoFiTypedSamplerObject<HdTextureType::Uv>.
///
template<HdTextureType textureType>
using LoFiTypedSamplerObject =
    typename LoFiTypedSamplerObjectHelper<textureType>::type;

template<>
struct LoFiTypedSamplerObjectHelper<HdTextureType::Uv> {
    using type = LoFiUvSamplerObject;
};

template<>
struct LoFiTypedSamplerObjectHelper<HdTextureType::Field> {
    using type = LoFiFieldSamplerObject;
};

template<>
struct LoFiTypedSamplerObjectHelper<HdTextureType::Ptex> {
    using type = LoFiPtexSamplerObject;
};

template<>
struct LoFiTypedSamplerObjectHelper<HdTextureType::Udim> {
    using type = LoFiUdimSamplerObject;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif
