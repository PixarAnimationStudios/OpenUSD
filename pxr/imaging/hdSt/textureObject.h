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
#ifndef PXR_IMAGING_HD_ST_TEXTURE_OBJECT_H
#define PXR_IMAGING_HD_ST_TEXTURE_OBJECT_H

#include "pxr/pxr.h"
#include "pxr/imaging/hdSt/api.h"

#include "pxr/imaging/hdSt/textureIdentifier.h"
#include "pxr/imaging/hd/enums.h"
#include "pxr/imaging/hd/types.h"

#include "pxr/imaging/hgi/handle.h"
#include "pxr/base/gf/bbox3d.h"
#include "pxr/base/tf/declarePtrs.h"

#include <memory>

PXR_NAMESPACE_OPEN_SCOPE

#ifdef PXR_PTEX_SUPPORT_ENABLED
TF_DECLARE_WEAK_AND_REF_PTRS(GlfPtexTexture);
#endif
TF_DECLARE_WEAK_AND_REF_PTRS(GlfUdimTexture);

class Hgi;
using HgiTextureHandle = HgiHandle<class HgiTexture>;
class HdSt_TextureObjectRegistry;

using HdStTextureObjectSharedPtr = std::shared_ptr<class HdStTextureObject>;

/// \class HdStTextureObject
/// 
/// Base class for a texture object. The actual GPU resources will be
/// allocated during the commit phase.
///
class HdStTextureObject :
            public std::enable_shared_from_this<HdStTextureObject>
{
public:
    /// Get texture identifier
    ///
    HDST_API
    const HdStTextureIdentifier &
    GetTextureIdentifier() const { return _textureId; }

    /// Get the target memory for the texture.
    ///
    HDST_API
    size_t GetTargetMemory() const { return _targetMemory; }

    /// Set the target memory (in bytes).
    ///
    /// When uploading the texture to the GPU, it will be downsampled
    /// to meet this target memory.
    ///
    HDST_API
    void SetTargetMemory(size_t);

    /// Is texture valid? Only correct after commit phase.
    ///
    /// E.g., no file at given file path. Consulted by clients to
    /// determine whether to use the fallback value.
    ///
    HDST_API
    virtual bool IsValid() const = 0;

    /// Get texture type
    ///
    HDST_API
    virtual HdTextureType GetTextureType() const = 0;

    HDST_API
    virtual ~HdStTextureObject();

protected:
    HdStTextureObject(
        const HdStTextureIdentifier &textureId,
        HdSt_TextureObjectRegistry * textureObjectRegistry);

    Hgi* _GetHgi() const;

    /// Load texture to CPU (thread-safe)
    ///
    HDST_API
    virtual void _Load() = 0;

    /// Commit texture to GPU (not thread-safe)
    ///
    HDST_API
    virtual void _Commit() = 0;

private:
    friend class HdSt_TextureObjectRegistry;

    HdSt_TextureObjectRegistry * const _textureObjectRegistry;
    const HdStTextureIdentifier _textureId;
    size_t _targetMemory;
};

/// \class HdStUvTextureObject
///
/// A base class for uv textures.
///
class HdStUvTextureObject : public HdStTextureObject
{
public:
    /// Get the handle to the actual GPU resource.
    ///
    /// Only valid after commit phase.
    ///
    HDST_API
    virtual HgiTextureHandle const &GetTexture() const = 0;

    /// Opinion about wrapS and wrapT parameters from the texture file.
    ///
    /// Only valid after commit phase. Can be HdWrapNoOpinion.
    HDST_API
    virtual const std::pair<HdWrap, HdWrap> &GetWrapParameters() const = 0;

    HDST_API
    HdTextureType GetTextureType() const override final;

protected:
    HdStUvTextureObject(
        const HdStTextureIdentifier &textureId,
        HdSt_TextureObjectRegistry * textureObjectRegistry);
};

/// \class HdAssetStUvTextureObject
///
/// A uv texture loading the asset identified by the texture identifier.
///
class HdStAssetUvTextureObject final : public HdStUvTextureObject
{
public:
    HDST_API
    HdStAssetUvTextureObject(
        const HdStTextureIdentifier &textureId,
        HdSt_TextureObjectRegistry *textureObjectRegistry);

    HDST_API
    ~HdStAssetUvTextureObject() override;

    /// Get the handle to the actual GPU resource.
    ///
    /// Only valid after commit phase.
    ///
    HDST_API
    HgiTextureHandle const &GetTexture() const override {
        return _gpuTexture;
    }

    /// Opinion about wrapS and wrapT parameters from the texture file.
    ///
    /// Only valid after commit phase. Can be HdWrapNoOpinion.
    HDST_API
    const std::pair<HdWrap, HdWrap> &GetWrapParameters() const override {
        return _wrapParameters;
    }

    HDST_API
    bool IsValid() const override;

protected:
    HDST_API
    void _Load() override;

    HDST_API
    void _Commit() override;

private:
    std::unique_ptr<class HdSt_TextureObjectCpuData> _cpuData;
    HgiTextureHandle _gpuTexture;
    std::pair<HdWrap, HdWrap> _wrapParameters;
};

/// \class HdStFieldTextureObject
///
/// A uvw texture with a bounding box describing how to transform it.
///
class HdStFieldTextureObject final : public HdStTextureObject
{
public:
    HDST_API
    HdStFieldTextureObject(
        const HdStTextureIdentifier &textureId,
        HdSt_TextureObjectRegistry *textureObjectRegistry);

    HDST_API
    ~HdStFieldTextureObject() override;

    /// Get the handle to the actual GPU resource.
    ///
    /// Only valid after commit phase.
    ///
    HDST_API
    HgiTextureHandle const &GetTexture() const {
        return _gpuTexture;
    }

    /// The box the texture fills out.
    ///
    /// Only valid after the commit phase.
    ///
    HDST_API
    const GfBBox3d &GetBoundingBox() const { return _bbox; }

    /// The sampling transform.
    ///
    /// Only valid after the commit phase.
    ///
    HDST_API
    const GfMatrix4d &GetSamplingTransform() const {
        return _samplingTransform;
    }

    HDST_API
    bool IsValid() const override;

    HDST_API
    HdTextureType GetTextureType() const override;

protected:
    HDST_API
    void _Load() override;

    HDST_API
    void _Commit() override;

private:
    std::unique_ptr<class HdSt_TextureObjectCpuData> _cpuData;
    GfBBox3d _bbox;
    GfMatrix4d _samplingTransform;
    HgiTextureHandle _gpuTexture;
};

/// \class HdStPtexTextureObject
///
/// A ptex texture - it is using Glf to both load the texture
/// and allocate the GPU resources (unlike the other texture
/// types).
///
class HdStPtexTextureObject final : public HdStTextureObject
{
public:
    HDST_API
    HdStPtexTextureObject(
        const HdStTextureIdentifier &textureId,
        HdSt_TextureObjectRegistry *textureObjectRegistry);

    HDST_API
    ~HdStPtexTextureObject() override;

    /// Get the GL texture name for the texels
    ///
    /// Only valid after commit phase.
    ///
    HDST_API
    uint32_t GetTexelGLTextureName() const { return _texelGLTextureName; }

    /// Get the GL texture name for the layout
    ///
    /// Only valid after commit phase.
    ///
    HDST_API
    uint32_t GetLayoutGLTextureName() const { return _layoutGLTextureName; }

    HDST_API
    bool IsValid() const override;

    HDST_API
    HdTextureType GetTextureType() const override;

protected:
    HDST_API
    void _Load() override;

    HDST_API
    void _Commit() override;

private:
#ifdef PXR_PTEX_SUPPORT_ENABLED
    GlfPtexTextureRefPtr _gpuTexture;
#endif

    uint32_t _texelGLTextureName;
    uint32_t _layoutGLTextureName;
};

/// \class HdStUdimTextureObject
///
/// A udim texture - it is using Glf to both load the texture
/// and allocate the GPU resources (unlike the other texture
/// types).
///
class HdStUdimTextureObject final : public HdStTextureObject
{
public:
    HDST_API
    HdStUdimTextureObject(
        const HdStTextureIdentifier &textureId,
        HdSt_TextureObjectRegistry *textureObjectRegistry);

    HDST_API
    ~HdStUdimTextureObject() override;

    /// Get the GL texture name for the texels
    ///
    /// Only valid after commit phase.
    ///
    HDST_API
    uint32_t GetTexelGLTextureName() const { return _texelGLTextureName; }

    /// Get the GL texture name for the layout
    ///
    /// Only valid after commit phase.
    ///
    HDST_API
    uint32_t GetLayoutGLTextureName() const { return _layoutGLTextureName; }

    HDST_API
    bool IsValid() const override;

    HDST_API
    HdTextureType GetTextureType() const override;

protected:
    HDST_API
    void _Load() override;

    HDST_API
    void _Commit() override;

private:
    std::vector<std::tuple<int, TfToken>> _tiles;

    GlfUdimTextureRefPtr _gpuTexture;

    uint32_t _texelGLTextureName;
    uint32_t _layoutGLTextureName;
};

template<HdTextureType textureType>
struct HdSt_TypedTextureObjectHelper;

/// \class HdStTypedTextureObject
///
/// A template alias such that, e.g., HdStUvTextureObject can be
/// accessed as HdStTypedTextureObject<HdTextureType::Uv>.
///
template<HdTextureType textureType>
using HdStTypedTextureObject =
    typename HdSt_TypedTextureObjectHelper<textureType>::type;

template<>
struct HdSt_TypedTextureObjectHelper<HdTextureType::Uv> {
    using type = HdStUvTextureObject;
};

template<>
struct HdSt_TypedTextureObjectHelper<HdTextureType::Field> {
    using type = HdStFieldTextureObject;
};

template<>
struct HdSt_TypedTextureObjectHelper<HdTextureType::Ptex> {
    using type = HdStPtexTextureObject;
};

template<>
struct HdSt_TypedTextureObjectHelper<HdTextureType::Udim> {
    using type = HdStUdimTextureObject;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif
