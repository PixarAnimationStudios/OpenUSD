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

#include "pxr/imaging/hgi/handle.h"
#include "pxr/base/tf/declarePtrs.h"
#include "pxr/base/gf/bbox3d.h"

#include <memory>

PXR_NAMESPACE_OPEN_SCOPE

TF_DECLARE_WEAK_AND_REF_PTRS(GlfBaseTextureData);

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

using HdStUvTextureObjectSharedPtr =
    std::shared_ptr<class HdStUvTextureObject>;

/// \class HdStUvTextureObject
///
/// A uv texture.
///
class HdStUvTextureObject final : public HdStTextureObject
{
public:
    HDST_API
    HdStUvTextureObject(
        const HdStTextureIdentifier &textureId,
        HdSt_TextureObjectRegistry *textureObjectRegistry);

    HDST_API
    ~HdStUvTextureObject() override;

    /// Get the handle to the actual GPU resource.
    ///
    /// Only valid after commit phase.
    ///
    HDST_API
    HgiTextureHandle const &GetTexture() const {
        return _gpuTexture;
    }

    HDST_API
    HdTextureType GetTextureType() const override;

protected:
    HDST_API
    void _Load() override;

    HDST_API
    void _Commit() override;

private:
    GlfBaseTextureDataRefPtr _cpuData;
    HgiTextureHandle _gpuTexture;
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

PXR_NAMESPACE_CLOSE_SCOPE

#endif
