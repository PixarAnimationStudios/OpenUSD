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
#ifndef PXR_IMAGING_HD_ST_SAMPLER_OBJECT_H
#define PXR_IMAGING_HD_ST_SAMPLER_OBJECT_H

#include "pxr/pxr.h"
#include "pxr/imaging/hdSt/api.h"

#include "pxr/imaging/hd/enums.h"
#include "pxr/imaging/hd/types.h"

#include <memory>

PXR_NAMESPACE_OPEN_SCOPE

class HdStUvTextureObject;
class HdStFieldTextureObject;
class HdStPtexTextureObject;

using HdStSamplerObjectSharedPtr =
    std::shared_ptr<class HdStSamplerObject>;

/// \class HdStSamplerObject
///
/// A base class encapsulating a GPU sampler object and, optionally, a
/// texture sampler handle (for bindless textures).
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
        bool createBindlessHandle);

    HDST_API 
    ~HdStUvSamplerObject() override;

    /// The GL sampler (as understood by glBindSampler)
    ///
    uint32_t GetGLSamplerName() const {
        return _glSamplerName;
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
    const uint32_t _glSamplerName;
    const uint64_t _glTextureSamplerHandle;
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
        bool createBindlessHandle);

    ~HdStFieldSamplerObject() override;

    /// The GL sampler (as understood by glBindSampler)
    ///
    uint32_t GetGLSamplerName() const {
        return _glSamplerName;
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
    const uint32_t _glSamplerName;
    const uint64_t _glTextureSamplerHandle;
};

/// \class HdStPtexSamplerObject
///
/// Ptex doesn't bind samplers, so this class is just holding the
/// texture handles for bindless textures.
///
class HdStPtexSamplerObject final : public HdStSamplerObject {
public:
    HdStPtexSamplerObject(
        HdStPtexTextureObject const &ptexTexture,
        // samplerParameters are ignored by ptex
        HdSamplerParameters const &samplerParameters,
        bool createBindlessHandle);

    ~HdStPtexSamplerObject() override;

    /// The GL texture handle for bindless textures (as returned by
    /// glGetTextureHandleARB). This is for texels.
    ///
    /// Only available when requested.
    ///
    uint64_t GetTexelsGLTextureHandle() const {
        return _texelsGLTextureHandle;
    }

    /// Similar to GetGLTexelsTextureHandle but for layout.
    uint64_t GetLayoutGLTextureHandle() const {
        return _layoutGLTextureHandle;
    }

private:
    const uint64_t _texelsGLTextureHandle;
    const uint64_t _layoutGLTextureHandle;
};

template<HdTextureType textureType>
struct HdSt_TypedSamplerObjectHelper;

/// \class HdStTypedSamplerObject
///
/// A template alias such that, e.g., HdStUvSamplerObject can be
/// accessed as HdStTypedSamplerObject<HdTextureType::Uv>.
///
template<HdTextureType textureType>
using HdStTypedSamplerObject =
    typename HdSt_TypedSamplerObjectHelper<textureType>::type;

template<>
struct HdSt_TypedSamplerObjectHelper<HdTextureType::Uv> {
    using type = HdStUvSamplerObject;
};

template<>
struct HdSt_TypedSamplerObjectHelper<HdTextureType::Field> {
    using type = HdStFieldSamplerObject;
};

template<>
struct HdSt_TypedSamplerObjectHelper<HdTextureType::Ptex> {
    using type = HdStPtexSamplerObject;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif
