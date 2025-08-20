//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_IMAGING_HD_ST_DYNAMIC_CUBEMAP_TEXTURE_OBJECT_H
#define PXR_IMAGING_HD_ST_DYNAMIC_CUBEMAP_TEXTURE_OBJECT_H

#include "pxr/pxr.h"
#include "pxr/imaging/hdSt/api.h"

#include "pxr/imaging/hdSt/textureObject.h"

PXR_NAMESPACE_OPEN_SCOPE

class HdStDynamicCubemapTextureImplementation;

/// \class HdStDynamicCubemapeTextureObject
///
/// A cubemap texture that is managed but not populated by the Storm texture
/// system.
///
/// Clients can allocate an instance of this class through
/// HdStResourceRegistry::AllocateTextureHandle or AllocateTextureObject
/// by passing an HdStTextureIdentifier with an
/// HdStDynamicCubemapSubtextureIdentifier.
///
/// The client can allocate the GPU resource with CreateTexture and populate it
/// by either giving data in the HgiTextureDesc or binding the texture as target
/// for a computation or render.
///
/// Bindless texture sampler handles can only be created correctly if
/// a client has created the texture before the texture commit phase
/// is finished.
///
class HdStDynamicCubemapTextureObject final : public HdStCubemapTextureObject
{
public:
    HDST_API
    HdStDynamicCubemapTextureObject(
        const HdStTextureIdentifier &textureId,
        HdSt_TextureObjectRegistry *textureObjectRegistry);

    HDST_API
    ~HdStDynamicCubemapTextureObject() override;

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
    HdStDynamicCubemapTextureImplementation * _GetImpl() const;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif
