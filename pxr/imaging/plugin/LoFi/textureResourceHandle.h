//
// Copyright 2020 benmalartre
//
// Unlicensed 
// 
#ifndef PXR_IMAGING_PLUGIN_LOFI_TEXTURE_RESOURCE_HANDLE_H
#define PXR_IMAGING_PLUGIN_LOFI_TEXTURE_RESOURCE_HANDLE_H

#include "pxr/pxr.h"
#include "pxr/imaging/plugin/LoFi/api.h"

#include "pxr/imaging/hd/textureResource.h"

#include <memory>

PXR_NAMESPACE_OPEN_SCOPE

class HdRenderIndex;
class SdfPath;

using LoFiTextureResourceHandleSharedPtr = 
    std::shared_ptr<class LoFiTextureResourceHandle>;

using LoFiTextureResourceSharedPtr = std::shared_ptr<class LoFiTextureResource>;

/// LoFiTextureResourceHandle is an indirect handle to a GL texture resource
///
/// This class provides a way for an LoFiTextureResource to be registered
/// for a specific scene path. This allows clients (e.g. shader code) to
/// access the most recently registered texture resource without having
/// to directly observe changes to upstream material, texture, or drawtarget
/// texture resource changes.
///
/// The underlying LoFiTextureResource instances are typically created
/// outside of LoFi. They are obtained from the scene delegate and are
/// registered with the resource registry via an HdTextureResource::ID
/// which is also provided by the scene delegate.
///
/// Clients which use LoFiTexture resources (e.g. shader code) get the
/// underlying GL texture object and GL sampler object IDs from the
/// texture resource.
///
/// So:
///  LoFiShaderCode has an array of texture descriptors holding
///    LoFiTextureResourceHandles. LoFiShaderCode binds GL texture
///    and sampler objects for the current set of underlying
///    LoFiTextureResource instances.
///
///  LoFiTexture Bprim and LoFiDrawTarget Sprim and LoFiMaterial Sprim
///    register LoFiTextureResource instances with their scene delegate
///    provided HdTextureResource::ID and also register their current
///    LoFiTextureResource instances with scene path locations using
///    LoFiTextureResourceHandles.
///
///  LoFiMaterial Sprim can assign LoFiTextureResourceHandles to
///    LoFiShaderCode to satisfy input texture scene path connections
///    and allow LoFiShaderCode to resolve GL texture and sampler objects
///    during resource bind/unbind.
///
/// We then need to propagate dirtiness from HdTexture to HdMaterial
/// only when an incompatible change is made to the texture resource
/// binding, avoiding an expensive invalidation, e.g. for animated
/// texture sources.
///
/// This class provides static helper methods to compute registry keys and
/// to identify incompatible texture resource changes.
///
class LoFiTextureResourceHandle {
public:
    LOFI_API
    LoFiTextureResourceHandle(
        LoFiTextureResourceSharedPtr const & textureResource = nullptr);

    LOFI_API
    virtual ~LoFiTextureResourceHandle();

    LOFI_API
    LoFiTextureResourceSharedPtr const & GetTextureResource() const;

    LOFI_API
    void SetTextureResource(
                LoFiTextureResourceSharedPtr const & textureResource);

    LOFI_API
    static HdTextureResource::ID GetHandleKey(
                HdRenderIndex const * renderIndex,
                SdfPath const &textureHandleId);

    LOFI_API
    static bool IsIncompatibleTextureResource(
                        LoFiTextureResourceSharedPtr const & a,
                        LoFiTextureResourceSharedPtr const & b);

private:
    LoFiTextureResourceSharedPtr _textureResource;

    LoFiTextureResourceHandle(const LoFiTextureResourceHandle &)
        = delete;
    LoFiTextureResourceHandle &operator =(const LoFiTextureResourceHandle &)
        = delete;
};


PXR_NAMESPACE_CLOSE_SCOPE

#endif //PXR_IMAGING_PLUGIN_LOFI_TEXTURE_RESOURCE_HANDLE_H
