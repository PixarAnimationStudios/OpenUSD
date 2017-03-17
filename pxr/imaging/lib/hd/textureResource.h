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
#ifndef HD_TEXTURE_RESOURCE_H
#define HD_TEXTURE_RESOURCE_H

#include "pxr/pxr.h"
#include "pxr/imaging/hd/api.h"
#include "pxr/imaging/hd/enums.h"

#include "pxr/imaging/glf/texture.h"
#include "pxr/imaging/glf/textureHandle.h"
#include "pxr/imaging/garch/gl.h"

#include "pxr/base/tf/declarePtrs.h"
#include "pxr/base/tf/token.h"
#include "pxr/base/gf/vec4f.h"

#include <boost/noncopyable.hpp>
#include <boost/shared_ptr.hpp>

PXR_NAMESPACE_OPEN_SCOPE


typedef boost::shared_ptr<class HdTextureResource> HdTextureResourceSharedPtr;

class HdTextureResource {
public:
    typedef size_t ID;

    /// Returns the hash value of the texture for \a sourceFile
    HD_API
    static ID ComputeHash(TfToken const & sourceFile);
    HD_API
    static ID ComputeFallbackPtexHash();
    HD_API
    static ID ComputeFallbackUVHash();

    HD_API
    virtual ~HdTextureResource();

    virtual bool IsPtex() const = 0;

    virtual GLuint GetTexelsTextureId() = 0;
    virtual GLuint GetTexelsSamplerId() = 0;
    virtual GLuint64EXT GetTexelsTextureHandle() = 0;

    virtual GLuint GetLayoutTextureId() = 0;
    virtual GLuint64EXT GetLayoutTextureHandle() = 0;

    virtual size_t GetMemoryUsed() = 0;
};

class HdSimpleTextureResource : public HdTextureResource
                              , boost::noncopyable {
public:
    HD_API
    HdSimpleTextureResource(GlfTextureHandleRefPtr const &textureHandle, bool isPtex);
    HD_API
    HdSimpleTextureResource(GlfTextureHandleRefPtr const &textureHandle, bool isPtex, 
        HdWrap wrapS, HdWrap wrapT, HdMinFilter minFilter, HdMagFilter magFilter);
    HD_API
    virtual ~HdSimpleTextureResource();

    HD_API
    virtual bool IsPtex() const;

    HD_API
    virtual GLuint GetTexelsTextureId();
    HD_API
    virtual GLuint GetTexelsSamplerId();
    HD_API
    virtual GLuint64EXT GetTexelsTextureHandle();

    HD_API
    virtual GLuint GetLayoutTextureId();
    HD_API
    virtual GLuint64EXT GetLayoutTextureHandle();

    HD_API
    virtual size_t GetMemoryUsed();

private:
    GlfTextureHandleRefPtr _textureHandle;
    GlfTextureRefPtr _texture;
    GfVec4f _borderColor;
    float _maxAnisotropy;
    GLuint _sampler;
    bool _isPtex;
};


PXR_NAMESPACE_CLOSE_SCOPE

#endif //HD_TEXTURE_RESOURCE_H
