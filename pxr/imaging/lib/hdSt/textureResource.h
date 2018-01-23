//
// Copyright 2016 Pixar
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
#ifndef HDST_TEXTURE_RESOURCE_H
#define HDST_TEXTURE_RESOURCE_H

#include "pxr/pxr.h"
#include "pxr/imaging/hd/textureResource.h"
#include "pxr/imaging/hd/enums.h"
#include "pxr/imaging/hdSt/api.h"

#include "pxr/imaging/glf/texture.h"
#include "pxr/imaging/glf/textureHandle.h"
#include "pxr/imaging/garch/gl.h"

#include "pxr/base/tf/declarePtrs.h"
#include "pxr/base/tf/token.h"
#include "pxr/base/gf/vec4f.h"

#include <boost/shared_ptr.hpp>

#include <cstdint>

PXR_NAMESPACE_OPEN_SCOPE


typedef boost::shared_ptr<class HdStTextureResource> HdStTextureResourceSharedPtr;
typedef boost::shared_ptr<class HdStSimpleTextureResource> HdStSimpleTextureResourceSharedPtr;

/// HdStTextureResource is an interface to a GL-backed texture.
class HdStTextureResource : public HdTextureResource {
public:
    HDST_API
    virtual ~HdStTextureResource();

    HDST_API
    HdStTextureResource() = default; 

    // Disallow copies
    HdStTextureResource(const HdStTextureResource&) = delete;
    HdStTextureResource& operator=(const HdStTextureResource&) = delete;

    // Access to underlying GL storage.
    HDST_API virtual GLuint GetTexelsTextureId() = 0;
    HDST_API virtual GLuint GetTexelsSamplerId() = 0;
    HDST_API virtual uint64_t GetTexelsTextureHandle() = 0;
    HDST_API virtual GLuint GetLayoutTextureId() = 0;
    HDST_API virtual uint64_t GetLayoutTextureHandle() = 0;
};

/// HdStSimpleTextureResource is a simple (non-drawtarget) texture.
class HdStSimpleTextureResource : public HdStTextureResource {
public:
    /// Create a texture resource around a Glf handle.
    /// While the texture handle maybe shared between many references to a
    /// texture.
    /// The texture resource represents a single texture binding.
    ///
    /// The memory request can be used to limit, the amount of texture memory
    /// this reference requires of the texture.  Set to 0 for unrestricted.
    HDST_API
    HdStSimpleTextureResource(GlfTextureHandleRefPtr const &textureHandle,
                              bool isPtex,
                              HdWrap wrapS,
                              HdWrap wrapT,
                              HdMinFilter minFilter,
                              HdMagFilter magFilter,
                              size_t memoryRequest = 0);

    HDST_API
    virtual ~HdStSimpleTextureResource();

    virtual bool IsPtex() const override;
    virtual size_t GetMemoryUsed() override;

    HDST_API virtual GLuint GetTexelsTextureId() override;
    HDST_API virtual GLuint GetTexelsSamplerId() override;
    HDST_API virtual uint64_t GetTexelsTextureHandle() override;
    HDST_API virtual GLuint GetLayoutTextureId() override;
    HDST_API virtual uint64_t GetLayoutTextureHandle() override;

private:
    GlfTextureHandleRefPtr _textureHandle;
    GlfTextureRefPtr _texture;
    GfVec4f _borderColor;
    float _maxAnisotropy;
    GLuint _sampler;
    bool _isPtex;
    size_t _memoryRequest;

    HdWrap _wrapS;
    HdWrap _wrapT;
    HdMinFilter _minFilter;
    HdMagFilter _magFilter;
};


PXR_NAMESPACE_CLOSE_SCOPE

#endif //HDST_TEXTURE_RESOURCE_H
