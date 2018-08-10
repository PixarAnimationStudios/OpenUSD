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
#ifndef GLF_TEXTURE_H
#define GLF_TEXTURE_H

/// \file glf/texture.h

#include "pxr/pxr.h"
#include "pxr/imaging/glf/api.h"
#include "pxr/imaging/glf/image.h"
#include "pxr/base/tf/declarePtrs.h"
#include "pxr/base/tf/refPtr.h"
#include "pxr/base/tf/staticTokens.h"
#include "pxr/base/tf/weakPtr.h"
#include "pxr/base/vt/dictionary.h"

#include "pxr/imaging/garch/gl.h"

#include <map>
#include <string>
#include <vector>
#include <boost/noncopyable.hpp>

PXR_NAMESPACE_OPEN_SCOPE


#define GLF_TEXTURE_TOKENS                      \
    (texels)                                    \
    (layout)

TF_DECLARE_PUBLIC_TOKENS(GlfTextureTokens, GLF_API, GLF_TEXTURE_TOKENS);

TF_DECLARE_WEAK_AND_REF_PTRS(GlfTexture);

/// \class GlfTexture
///
/// Represents a texture object in Glf.
///
/// A texture is typically defined by reading texture image data from an image
/// file but a texture might also represent an attachment of a draw target.
///
class GlfTexture : public TfRefBase, public TfWeakBase, boost::noncopyable {
public:
    /// \class Binding
    ///
    /// A texture has one or more bindings which describe how the different
    /// aspects of the texture should be bound in order to allow shader
    /// access. Most textures will have a single binding for the role
    /// "texels", but some textures might need multiple bindings, e.g. a
    /// ptexTexture will have an additional binding for the role "layout".
    ///
    struct Binding {
        Binding(TfToken name, TfToken role, GLenum target,
                GLuint textureId, GLuint samplerId)
            : name(name)
            , role(role)
            , target(target)
            , textureId(textureId)
            , samplerId(samplerId) { }

            TfToken name;
            TfToken role;
            GLenum target;
            GLuint textureId;
            GLuint samplerId;
    };
    typedef std::vector<Binding> BindingVector;

    GLF_API
    virtual ~GlfTexture() = 0;

    /// Returns the bindings to use this texture for the shader resource
    /// named \a identifier. If \a samplerId is specified, the bindings
    /// returned will use this samplerId for resources which can be sampled.
    virtual BindingVector GetBindings(TfToken const & identifier,
                                      GLuint samplerId = 0) = 0;

    /// Amount of memory used to store the texture
    GLF_API
    size_t GetMemoryUsed() const;
    
    /// Amount of memory the user wishes to allocate to the texture
    GLF_API
    size_t GetMemoryRequested() const;

    /// Specify the amount of memory the user wishes to allocate to the texture
    GLF_API
    void SetMemoryRequested(size_t targetMemory);

    virtual VtDictionary GetTextureInfo(bool forceLoad) = 0;

    GLF_API
    virtual bool IsMinFilterSupported(GLenum filter);

    GLF_API
    virtual bool IsMagFilterSupported(GLenum filter);

    /// static reporting function
    GLF_API
    static size_t GetTextureMemoryAllocated();

    /// Returns an identifier that can be used to determine when the
    /// contents of this texture (i.e. its image data) has changed.
    ///
    /// The contents of most textures will be immutable for the lifetime
    /// of the texture. However, the contents of the texture attachments
    /// of a draw target change when the draw target is updated.
    GLF_API
    size_t GetContentsID() const;

    GLF_API
    GlfImage::ImageOriginLocation GetOriginLocation() const;

    GLF_API
    bool IsOriginLowerLeft() const;

protected:
    GLF_API
    GlfTexture();

    GLF_API
    GlfTexture(GlfImage::ImageOriginLocation originLocation);

    GLF_API
    void _SetMemoryUsed(size_t size);
    
    GLF_API
    virtual void _OnMemoryRequestedDirty();

    GLF_API
    void _UpdateContentsID();
    
private:
    size_t _memoryUsed;
    size_t _memoryRequested;
    size_t _contentsID;
    GlfImage::ImageOriginLocation _originLocation;
};

class GlfTextureFactoryBase : public TfType::FactoryBase {
public:
    virtual GlfTextureRefPtr New(const TfToken& texturePath,
                        GlfImage::ImageOriginLocation originLocation) const = 0;
    virtual GlfTextureRefPtr New(const TfTokenVector& texturePaths,
                        GlfImage::ImageOriginLocation originLocation) const = 0;
};

template <class T>
class GlfTextureFactory : public GlfTextureFactoryBase {
public:
    virtual GlfTextureRefPtr New(const TfToken& texturePath, 
                                 GlfImage::ImageOriginLocation originLocation = 
                                                GlfImage::OriginUpperLeft) const
    {
        return T::New(texturePath);
    }

    virtual GlfTextureRefPtr New(const TfTokenVector& texturePaths,
                                 GlfImage::ImageOriginLocation originLocation = 
                                                GlfImage::OriginUpperLeft) const
    {
        return TfNullPtr;
    }
};


PXR_NAMESPACE_CLOSE_SCOPE

#endif  // GLF_TEXTURE_H
