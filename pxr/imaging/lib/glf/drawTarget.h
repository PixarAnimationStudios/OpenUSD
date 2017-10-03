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
#ifndef GLF_DRAWTARGET_H
#define GLF_DRAWTARGET_H

/// \file glf/drawTarget.h

#include "pxr/pxr.h"
#include "pxr/imaging/glf/api.h"
#include "pxr/imaging/glf/texture.h"

#include "pxr/base/gf/vec2i.h"
#include "pxr/base/gf/matrix4d.h"
#include "pxr/base/tf/declarePtrs.h"
#include "pxr/base/tf/refBase.h"
#include "pxr/base/tf/weakBase.h"

#include "pxr/imaging/garch/gl.h"

#include <map>
#include <set>
#include <string>

#include <boost/shared_ptr.hpp>

PXR_NAMESPACE_OPEN_SCOPE


TF_DECLARE_WEAK_AND_REF_PTRS(GlfDrawTarget);
typedef boost::shared_ptr<class GlfGLContext> GlfGLContextSharedPtr;

/// \class GlfDrawTarget
///
/// A class representing a GL render target with mutliple image attachments.
///
/// A DrawTarget is essentially a custom render pass into which several
/// arbitrary variables can be output into. These can later be used as
/// texture samplers by GLSL shaders.
///
/// The DrawTarget maintains a map of named attachments that correspond
/// to GL_TEXTURE_2D mages. By default, DrawTargets also create a depth
/// component that is used both as a depth buffer during the draw pass,
/// and can later be accessed as a regular GL_TEXTURE_2D data. Stencils
/// are also available (by setting the format to GL_DEPTH_STENCIL and
/// the internalFormat to GL_DEPTH24_STENCIL8)
///
class GlfDrawTarget : public TfRefBase, public TfWeakBase {
public:
    typedef GlfDrawTarget This;

public:
    
    /// Returns a new instance.
    GLF_API
    static GlfDrawTargetRefPtr New( GfVec2i const & size, 
                                    bool requestMSAA = false );

    /// Returns a new instance.
    /// GL framebuffers cannot be shared across contexts, but texture
    /// attachments can. In order to reflect this, GlfDrawTargets hold
    /// onto their maps of attachments through a RefPtr that can be shared
    /// by multiple GlfDrawTargets, one for each of the active GL contexts
    /// (ex. one for each active QT viewer).
    /// This constructor creates a new framebuffer, but populates its map of
    /// attachments by sharing the RefPtr of the source GlfDrawTarget.
    GLF_API
    static GlfDrawTargetRefPtr New( GlfDrawTargetPtr const & drawtarget );

    class Attachment : public GlfTexture {
    public:
        typedef TfDeclarePtrs<class Attachment>::RefPtr AttachmentRefPtr;

        GLF_API
        static AttachmentRefPtr New(int glIndex, GLenum format, GLenum type,
                                    GLenum internalFormat, GfVec2i size,
                                    unsigned int numSamples);

        GLF_API
        virtual ~Attachment();

        /// Returns the GL texture index (can be used as any regular GL texture)
        GLuint GetGlTextureName() const { return _textureName; }

        /// Returns the GL texture index multisampled of this attachment
        GLuint GetGlTextureMSName() const { return _textureNameMS; }

        /// Returns the GL format of the texture (GL_RGB, GL_DEPTH_COMPONENT...)
        GLenum GetFormat() const { return _format; }

        /// Returns the GL type of the texture (GL_BYTE, GL_INT, GL_FLOAT...)
        GLenum GetType() const { return _type; }

        /// Returns the GL attachment point index in the framebuffer.
        int GetAttach() const { return _glIndex; }

        /// Resize the attachment recreating the texture
        GLF_API
        void ResizeTexture(const GfVec2i &size);

        // GlfTexture overrides
        GLF_API
        virtual BindingVector GetBindings(TfToken const & identifier,
                                          GLuint samplerName) const;
        GLF_API
        virtual VtDictionary GetTextureInfo() const;

        /// Updates the contents signature for the underlying texture
        /// to allow downstream consumers to know that the texture image
        /// data may have changed.
        GLF_API
        void TouchContents();

    private:
        Attachment(int glIndex, GLenum format, GLenum type,
                   GLenum internalFormat, GfVec2i size, 
                   unsigned int numSamples);

        void _GenTexture();
        void _DeleteTexture();

        GLuint       _textureName;
        GLuint       _textureNameMS;

        GLenum       _format,
                     _type,
                     _internalFormat;

        int          _glIndex;

        GfVec2i      _size;

        unsigned int _numSamples;
    };

    typedef TfDeclarePtrs<class Attachment>::RefPtr AttachmentRefPtr;

    typedef std::map<std::string, AttachmentRefPtr> AttachmentsMap;
    
    /// Add an attachment to the DrawTarget.
    GLF_API
    void AddAttachment( std::string const & name, 
                        GLenum format, GLenum type, GLenum internalFormat );

    /// Removes the named attachment from the DrawTarget.
    GLF_API
    void DeleteAttachment( std::string const & name );
    
    /// Clears all the attachments for this DrawTarget.
    GLF_API
    void ClearAttachments();
    
    /// Copies the list of attachments from DrawTarget.
    GLF_API
    void CloneAttachments( GlfDrawTargetPtr const & drawtarget );
    
    /// Returns the list of Attachments for this DrawTarget.
    GLF_API
    AttachmentsMap const & GetAttachments() const;
    
    /// Returns the attachment with a given name or TfNullPtr;
    GLF_API
    AttachmentRefPtr GetAttachment(std::string const & name);
    
    /// Write the Attachment buffer to an image file (debugging).
    GLF_API
    bool WriteToFile(std::string const & name,
                     std::string const & filename,
                     GfMatrix4d const & viewMatrix = GfMatrix4d(1),
                     GfMatrix4d const & projectionMatrix = GfMatrix4d(1));

    /// Resize the DrawTarget.
    GLF_API
    void SetSize( GfVec2i );    

    /// Returns the size of the DrawTarget.
    GfVec2i const & GetSize() const { return _size; }

    /// Returns if the draw target uses msaa
    bool HasMSAA() const { return (_numSamples > 1); }

    /// Returns the framebuffer object Id.
    GLF_API
    GLuint GetFramebufferId() const;
    
    /// Returns the id of the framebuffer object with MSAA buffers.
    GLF_API
    GLuint GetFramebufferMSId() const;

    /// Binds the framebuffer.
    GLF_API
    void Bind();

    /// Unbinds the framebuffer.
    GLF_API
    void Unbind();

    /// Returns whether the framebuffer is currently bound.
    GLF_API
    bool IsBound() const;

    /// Resolve the MSAA framebuffer to a regular framebuffer. If there
    /// is no MSAA enabled, this function does nothing.
    GLF_API
    void Resolve();

    /// Updates the contents signature for attached textures
    /// to allow downstream consumers to know that the texture image
    /// data may have changed.
    GLF_API
    void TouchContents();

    /// Returns whether the enclosed framebuffer object is complete.
    /// If \a reason is non-NULL, and this framebuffer is not valid,
    /// sets \a reason to the reason why not.
    GLF_API
    bool IsValid(std::string * reason = NULL);

protected:

    /// Weak/Ref-based container for the the map of texture attachments.
    /// Multiple GlfDrawTargets can jointly share their attachment textures :
    /// this construction allows the use of a RefPtr on the map of attachments.
    class AttachmentsContainer : public TfRefBase, public TfWeakBase {
    public:
        AttachmentsMap attachments;
    };

    GLF_API
    GlfDrawTarget( GfVec2i const & size, bool requestMSAA );

    GLF_API
    GlfDrawTarget( GlfDrawTargetPtr const & drawtarget );

    GLF_API
    virtual ~GlfDrawTarget();

private:
    void _GenFrameBuffer();

    void _BindAttachment( GlfDrawTarget::AttachmentRefPtr const & a );
    
    GLuint _AllocAttachment( GLenum format, GLenum type );

    AttachmentsMap & _GetAttachments() const;

    void _DeleteAttachments( );
    
    void _AllocDepth( );

    bool _Validate(std::string * reason = NULL);

    void _SaveBindingState();

    void _RestoreBindingState();

    GLuint _framebuffer;
    GLuint _framebufferMS;
    
    GLuint _unbindRestoreReadFB,
           _unbindRestoreDrawFB;

    int _bindDepth;

    GfVec2i _size;
    
    unsigned int _numSamples;

    TfRefPtr<AttachmentsContainer> _attachmentsPtr;
    GlfGLContextSharedPtr _owningContext;
};


PXR_NAMESPACE_CLOSE_SCOPE

#endif  // GLF_DRAW_TARGET_H
