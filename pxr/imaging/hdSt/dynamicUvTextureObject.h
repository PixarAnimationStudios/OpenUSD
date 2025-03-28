//
// Copyright 2020 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_IMAGING_HD_ST_DYNAMIC_UV_TEXTURE_OBJECT_H
#define PXR_IMAGING_HD_ST_DYNAMIC_UV_TEXTURE_OBJECT_H

#include "pxr/pxr.h"
#include "pxr/imaging/hdSt/api.h"

#include "pxr/imaging/hdSt/textureObject.h"

PXR_NAMESPACE_OPEN_SCOPE

class HdStDynamicUvTextureImplementation;

/// \class HdStDynamicUvTextureObject
///
/// A uv texture that is managed but not populated by the Storm texture system.
///
/// Clients can allocate an instance of this class through
/// HdStResourceRegistry::AllocateTextureHandle or AllocateTextureObject
/// by passing an HdStTextureIdentifier with a
/// HdStDynamicUvSubtextureIdentifier.
///
/// The client can allocate the GPU resource with CreateTexture and populate it
/// by either giving data in the HgiTextureDesc or binding the texture as target
/// for a computation or render.
///
/// Bindless texture sampler handles can only be created correctly if
/// a client has created the texture before the texture commit phase
/// is finished.
///
class HdStDynamicUvTextureObject final : public HdStUvTextureObject
{
public:
    HDST_API
    HdStDynamicUvTextureObject(
        const HdStTextureIdentifier &textureId,
        HdSt_TextureObjectRegistry *textureObjectRegistry);

    HDST_API
    ~HdStDynamicUvTextureObject() override;

    /// Allocate GPU resource using the texture descriptor. Populate
    /// if data are given in the descriptor.
    /// 
    void CreateTexture(const HgiTextureDesc &desc) {
        _CreateTexture(desc);
    }

    /// Make GPU generate mipmaps.
    ///
    void GenerateMipmaps()
    {
        _GenerateMipmaps();
    }

    /// Release GPU resource.
    void DestroyTexture() {
        _DestroyTexture();
    }

    /// Set wrap mode hints used when a texture node's wrap mode
    /// is use metadata.
    ///
    /// This is typically called from HdStDynamicUvTextureImplementation::Load
    /// when the texture file has wrap mode hints.
    void SetWrapParameters(const std::pair<HdWrap, HdWrap> &wrapParameters) {
        _SetWrapParameters(wrapParameters);
    }

    /// Save CPU data for this texture (transfering ownership).
    ///
    /// This is typically called from HdStDynamicUvTextureImplementation::Load
    /// so that the CPU data can be uploaded during commit.
    ///
    /// To free the CPU data, call with nullptr.
    ///
    void SetCpuData(std::unique_ptr<HdStTextureCpuData> &&cpuData) {
        _SetCpuData(std::move(cpuData));
    }

    /// Get the CPU data stored for this texture.
    ///
    /// Typically used in HdStDynamicUvTextureImplementation::Commit to
    /// commit CPU data to GPU.
    HdStTextureCpuData * GetCpuData() const {
        return _GetCpuData();
    }

    /// Always returns true - so that samplers for this texture are
    /// created.
    ///
    HDST_API
    bool IsValid() const override;

protected:
    HDST_API
    void _Load() override;

    HDST_API
    void _Commit() override;

private:
    HdStDynamicUvTextureImplementation * _GetImpl() const;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif
