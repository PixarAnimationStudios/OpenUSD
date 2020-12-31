//
// Copyright 2020 benmalartre
//
// Unlicensed
//
#ifndef PXR_IMAGING_LOFI_DYNAMIC_UV_TEXTURE_OBJECT_H
#define PXR_IMAGING_LOFI_DYNAMIC_UV_TEXTURE_OBJECT_H

#include "pxr/pxr.h"
#include "pxr/imaging/plugin/LoFi/api.h"

#include "pxr/imaging/plugin/LoFi/textureObject.h"

PXR_NAMESPACE_OPEN_SCOPE

class LoFiDynamicUvTextureImplementation;

/// \class LoFiDynamicUvTextureObject
///
/// A uv texture that is managed but not populated by the Storm texture system.
///
/// Clients can allocate an instance of this class through
/// LoFiResourceRegistry::AllocateTextureHandle or AllocateTextureObject
/// by passing an LoFiTextureIdentifier with a
/// LoFiDynamicUvSubtextureIdentifier.
///
/// The client can allocate the GPU resource with CreateTexture and populate it
/// by either giving data in the HgiTextureDesc or binding the texture as target
/// for a computation or render.
///
/// Bindless texture sampler handles can only be created correctly if
/// a client has created the texture before the texture commit phase
/// is finished.
///
class LoFiDynamicUvTextureObject final : public LoFiUvTextureObject
{
public:
    LOFI_API
    LoFiDynamicUvTextureObject(
        const LoFiTextureIdentifier &textureId,
        LoFiTextureObjectRegistry *textureObjectRegistry);

    LOFI_API
    ~LoFiDynamicUvTextureObject() override;

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
    /// This is typically called from LoFiDynamicUvTextureImplementation::Load
    /// when the texture file has wrap mode hints.
    void SetWrapParameters(const std::pair<HdWrap, HdWrap> &wrapParameters) {
        _SetWrapParameters(wrapParameters);
    }

    /// Save CPU data for this texture (transfering ownership).
    ///
    /// This is typically called from LoFiDynamicUvTextureImplementation::Load
    /// so that the CPU data can be uploaded during commit.
    ///
    /// To free the CPU data, call with nullptr.
    ///
    void SetCpuData(std::unique_ptr<LoFiTextureCpuData> &&cpuData) {
        _SetCpuData(std::move(cpuData));
    }

    /// Get the CPU data stored for this texture.
    ///
    /// Typically used in LoFiDynamicUvTextureImplementation::Commit to
    /// commit CPU data to GPU.
    LoFiTextureCpuData * GetCpuData() const {
        return _GetCpuData();
    }

    /// Always returns true - so that samplers for this texture are
    /// created.
    ///
    LOFI_API
    bool IsValid() const override;

protected:
    LOFI_API
    void _Load() override;

    LOFI_API
    void _Commit() override;

private:
    LoFiDynamicUvTextureImplementation * _GetImpl() const;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif
