//
// Copyright 2020 benmalartre
//
// Unlicensed
// 
#ifndef PXR_IMAGING_PLUGIN_LOFI_TEXTURE_RESOURCE_H
#define PXR_IMAGING_PLUGIN_LOFI_TEXTURE_RESOURCE_H

#include "pxr/pxr.h"
#include "pxr/imaging/hd/textureResource.h"
#include "pxr/imaging/hd/enums.h"
#include "pxr/imaging/LoFi/api.h"

#include "pxr/imaging/glf/texture.h"
#include "pxr/imaging/glf/textureHandle.h"
#include "pxr/imaging/garch/gl.h"

#include "pxr/base/tf/declarePtrs.h"
#include "pxr/base/tf/token.h"
#include "pxr/base/gf/vec4f.h"

#include <boost/noncopyable.hpp>

#include <memory>
#include <cstdint>

PXR_NAMESPACE_OPEN_SCOPE


using LoFiTextureResourceSharedPtr = std::shared_ptr<class LoFiTextureResource>;

/// LoFiTextureResource is an interface to a GL-backed texture.
class LoFiTextureResource : public HdTextureResource, boost::noncopyable {
public:
    LOFI_API
    virtual ~LoFiTextureResource();

    // Access to underlying GL storage.
    LOFI_API virtual GLuint GetTexelsTextureId() = 0;
    LOFI_API virtual GLuint GetTexelsSamplerId() = 0;
    LOFI_API virtual uint64_t GetTexelsTextureHandle() = 0;
    LOFI_API virtual GLuint GetLayoutTextureId() = 0;
    LOFI_API virtual uint64_t GetLayoutTextureHandle() = 0;
};

/// LoFiSimpleTextureResource is a simple (non-drawtarget) texture.
class LoFiSimpleTextureResource : public LoFiTextureResource {
public:
    /// Create a texture resource around a Glf handle.
    /// While the texture handle maybe shared between many references to a
    /// texture.
    /// The texture resource represents a single texture binding.
    ///
    /// The memory request can be used to limit, the amount of texture memory
    /// this reference requires of the texture.  Set to 0 for unrestricted.
    LOFI_API
    LoFiSimpleTextureResource(GlfTextureHandleRefPtr const &textureHandle,
                              HdTextureType textureType,
                              HdWrap wrapS,
                              HdWrap wrapT,
                              HdWrap wrapR,
                              HdMinFilter minFilter,
                              HdMagFilter magFilter,
                              size_t memoryRequest = 0);

    LOFI_API
    virtual ~LoFiSimpleTextureResource();

    virtual HdTextureType GetTextureType() const override;
    virtual size_t GetMemoryUsed() override;

    LOFI_API virtual GLuint GetTexelsTextureId() override;
    LOFI_API virtual GLuint GetTexelsSamplerId() override;
    LOFI_API virtual uint64_t GetTexelsTextureHandle() override;
    LOFI_API virtual GLuint GetLayoutTextureId() override;
    LOFI_API virtual uint64_t GetLayoutTextureHandle() override;

private:
    GlfTextureHandleRefPtr _textureHandle;
    GlfTextureRefPtr _texture;
    GfVec4f _borderColor;
    float _maxAnisotropy;
    GLuint _sampler;
    HdTextureType _textureType;
    size_t _memoryRequest;

    HdWrap _wrapS;
    HdWrap _wrapT;
    HdWrap _wrapR;
    HdMinFilter _minFilter;
    HdMagFilter _magFilter;
};


PXR_NAMESPACE_CLOSE_SCOPE

#endif //PXR_IMAGING_PLUGIN_LOFI_TEXTURE_RESOURCE_H
