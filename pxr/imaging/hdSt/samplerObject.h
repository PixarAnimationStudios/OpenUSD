//
// Copyright 2020 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_IMAGING_HD_ST_SAMPLER_OBJECT_H
#define PXR_IMAGING_HD_ST_SAMPLER_OBJECT_H

#include "pxr/pxr.h"
#include "pxr/imaging/hdSt/api.h"
#include "pxr/imaging/hdSt/enums.h"

#include "pxr/imaging/hgi/handle.h"
#include "pxr/imaging/hd/types.h"

#include <memory>

PXR_NAMESPACE_OPEN_SCOPE

class Hgi;
class HdStUvTextureObject;
class HdStFieldTextureObject;
class HdStPtexTextureObject;
class HdStUdimTextureObject;
class HdSt_SamplerObjectRegistry;
using HgiSamplerHandle = HgiHandle<class HgiSampler>;

using HdStSamplerObjectSharedPtr =
    std::shared_ptr<class HdStSamplerObject>;

/// \class HdStSamplerObject
///
/// A base class encapsulating a GPU sampler object.
///
/// The subclasses of HdStSamplerObject mirror the subclasses of
/// HdStTextureObject with the intention that they will be used in
/// conjunction (e.g., the not yet existing HdStPtexSamplerObject will
/// have two samplers and texture sampler handles for the texels and
/// layout texture in a HdStPtexTextureObject).
///
/// The GPU resources is con-/destructed immediately in the
/// c'tor/d'tor. By going through the HdSt_SamplerObjectRegistry, we
/// can obtain a shared pointer that can safely be dropped in a
/// different thread. The HdSt_SamplerObjectRegistry is also dispatching
/// by texture type to construct the corresponding sampler type.
///
class HdStSamplerObject
{
public:
    virtual ~HdStSamplerObject() = 0;

protected:
    explicit HdStSamplerObject(
        HdSt_SamplerObjectRegistry * samplerObjectRegistry);

    Hgi* _GetHgi() const;
    HdSt_SamplerObjectRegistry * const _samplerObjectRegistry;
};

/// \class HdStUvSamplerObject
///
/// A sampler suitable for HdStUvTextureObject.
///
class HdStUvSamplerObject final : public HdStSamplerObject {
public:
    HDST_API 
    HdStUvSamplerObject(
        HdStUvTextureObject const &uvTexture,
        HdSamplerParameters const &samplerParameters,
        HdSt_SamplerObjectRegistry * samplerObjectRegistry);

    HDST_API 
    ~HdStUvSamplerObject() override;

    /// The sampler.
    ///
    const HgiSamplerHandle &GetSampler() const {
        return _sampler;
    }

private:
    HgiSamplerHandle _sampler;
};

/// \class HdStFieldSamplerObject
///
/// A sampler suitable for HdStFieldTextureObject.
///
class HdStFieldSamplerObject final : public HdStSamplerObject {
public:
    HdStFieldSamplerObject(
        HdStFieldTextureObject const &uvTexture,
        HdSamplerParameters const &samplerParameters,
        HdSt_SamplerObjectRegistry * samplerObjectRegistry);

    ~HdStFieldSamplerObject() override;

    /// The sampler.
    ///
    const HgiSamplerHandle &GetSampler() const {
        return _sampler;
    }

private:
    HgiSamplerHandle _sampler;
};

/// \class HdStPtexSamplerObject
///
/// Ptex doesn't bind samplers, so this class is just holding a
/// sampler to resolve handles for bindless textures.
///
class HdStPtexSamplerObject final : public HdStSamplerObject {
public:
    HdStPtexSamplerObject(
        HdStPtexTextureObject const &ptexTexture,
        // samplerParameters are ignored by ptex
        HdSamplerParameters const &samplerParameters,
        HdSt_SamplerObjectRegistry * samplerObjectRegistry);

    ~HdStPtexSamplerObject() override;

    /// The GPU sampler object for the texels texture.
    ///
    const HgiSamplerHandle &GetTexelsSampler() const {
        return _texelsSampler;
    }

    /// The GPU sampler object for the layout texture.
    ///
    const HgiSamplerHandle &GetLayoutSampler() const {
        return _layoutSampler;
    }

private:
    HgiSamplerHandle _texelsSampler;
    HgiSamplerHandle _layoutSampler;
};

/// \class HdStUdimSamplerObject
///
/// A sampler suitable for Udim textures (wraps one GPU sampler
/// for the texels texture).
///
class HdStUdimSamplerObject final : public HdStSamplerObject {
public:
    HdStUdimSamplerObject(
        HdStUdimTextureObject const &ptexTexture,
        // samplerParameters are ignored by udim (at least for now)
        HdSamplerParameters const &samplerParameters,
        HdSt_SamplerObjectRegistry * samplerObjectRegistry);

    ~HdStUdimSamplerObject() override;

    /// The GPU sampler object for the texels texture.
    ///
    const HgiSamplerHandle &GetTexelsSampler() const {
        return _texelsSampler;
    }

    /// The GPU sampler object for the layout texture.
    ///
    const HgiSamplerHandle &GetLayoutSampler() const {
        return _layoutSampler;
    }

private:
    HgiSamplerHandle _texelsSampler;
    HgiSamplerHandle _layoutSampler;
};

template<HdStTextureType textureType>
struct HdSt_TypedSamplerObjectHelper;

/// \class HdStTypedSamplerObject
///
/// A template alias such that, e.g., HdStUvSamplerObject can be
/// accessed as HdStTypedSamplerObject<HdStTextureType::Uv>.
///
template<HdStTextureType textureType>
using HdStTypedSamplerObject =
    typename HdSt_TypedSamplerObjectHelper<textureType>::type;

template<>
struct HdSt_TypedSamplerObjectHelper<HdStTextureType::Uv> {
    using type = HdStUvSamplerObject;
};

template<>
struct HdSt_TypedSamplerObjectHelper<HdStTextureType::Field> {
    using type = HdStFieldSamplerObject;
};

template<>
struct HdSt_TypedSamplerObjectHelper<HdStTextureType::Ptex> {
    using type = HdStPtexSamplerObject;
};

template<>
struct HdSt_TypedSamplerObjectHelper<HdStTextureType::Udim> {
    using type = HdStUdimSamplerObject;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif
