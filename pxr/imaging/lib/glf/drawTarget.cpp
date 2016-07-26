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
// glf/drawTarget.cpp
//

#include "pxr/imaging/glf/glew.h"

#include "pxr/imaging/glf/drawTarget.h"
#include "pxr/imaging/glf/glContext.h"
#include "pxr/imaging/glf/diagnostic.h"
#include "pxr/imaging/glf/image.h"
#include "pxr/imaging/glf/utils.h"

#include "pxr/base/tf/iterator.h"
#include "pxr/base/tf/stringUtils.h"

#include "pxr/base/tracelite/trace.h"

GlfDrawTargetRefPtr
GlfDrawTarget::New( GfVec2i const & size )
{
    return TfCreateRefPtr(new This(size));
}

GlfDrawTarget::GlfDrawTarget( GfVec2i const & size ) :
    _framebuffer(0),
    _unbindRestoreReadFB(0),
    _unbindRestoreDrawFB(0),
    _bindDepth(0),
    _size(size)
{
    GlfGlewInit();

    _GenFrameBuffer();

    _attachmentsPtr = TfCreateRefPtr( new AttachmentsContainer );
}

GlfDrawTargetRefPtr
GlfDrawTarget::New( GlfDrawTargetPtr const & drawtarget )
{
    return TfCreateRefPtr(new This(drawtarget));
}

// clone constructor : generates a new GL framebuffer, but share the texture
// attachments.
GlfDrawTarget::GlfDrawTarget( GlfDrawTargetPtr const & drawtarget ) :
    _framebuffer(0),
    _unbindRestoreReadFB(0),
    _unbindRestoreDrawFB(0),
    _bindDepth(0),
    _size(drawtarget->_size),
    _owningContext()

{
    GlfGlewInit();

    _GenFrameBuffer();

    // share the RefPtr to the map of attachments
    _attachmentsPtr = drawtarget->_attachmentsPtr;

    Bind();

    // attach the textures to the correct framebuffer mount points
    TF_FOR_ALL( it, _attachmentsPtr->attachments ) {
        _BindAttachment( it->second );
    }

    Unbind();
}

GlfDrawTarget::~GlfDrawTarget( )
{
    // If the owning context has died, there's nothing to free.
    if (!_owningContext->IsValid()) {
        return;
    }

    // bind the owning context to make sure we delete frame buffer on correct
    // context.
    GlfGLContextScopeHolder contextHolder(_owningContext);

    _DeleteAttachments( );

    if (_framebuffer) {

        TF_VERIFY(glIsFramebuffer(_framebuffer),
            "Tried to free invalid framebuffer");

        glDeleteFramebuffers(1, &_framebuffer);
        _framebuffer = 0;
    }

}

void
GlfDrawTarget::AddAttachment( std::string const & name, 
                              GLenum format, GLenum type,
                              GLenum internalFormat )
{
    if (not IsBound()) {
        TF_CODING_ERROR("Cannot change the size of an unbound GlfDrawTarget");
    }

    AttachmentsMap & attachments = _GetAttachments();
    AttachmentsMap::iterator it = attachments.find( name );

    if (it==attachments.end()) {

        AttachmentRefPtr attachment = Attachment::New((int)attachments.size(),
                                                      format, type,
                                                      internalFormat, _size);

        attachments.insert(AttachmentsMap::value_type(name, attachment));


        TF_VERIFY( attachment->GetGlTextureName() > 0 ,
                   std::string("Attachment \""+name+"\" was not added "
                       "and cannot be bound in MatDisplayMaterial").c_str());

        _BindAttachment( attachment );

    } else {
        TF_CODING_ERROR( "Attachment \""+name+"\" already exists for this "
                         "DrawTarget" );
    }
}

void 
GlfDrawTarget::DeleteAttachment( std::string const & name )
{
    AttachmentsMap & attachments = _GetAttachments();
    AttachmentsMap::iterator it = attachments.find( name );

    if (it!=attachments.end()) {
        attachments.erase( it );
    } else {
        TF_CODING_ERROR( "Attachment \""+name+"\" does not exist for this "
                         "DrawTarget" );        
    }
}

GlfDrawTarget::AttachmentRefPtr 
GlfDrawTarget::GetAttachment(std::string const & name)
{
    AttachmentsMap & attachments = _GetAttachments();
    AttachmentsMap::iterator it = attachments.find( name );

    if (it!=attachments.end()) {
        return it->second;
    } else {
        return TfNullPtr;
    }
}

void 
GlfDrawTarget::ClearAttachments()
{
    _DeleteAttachments();
}

void
GlfDrawTarget::CloneAttachments( GlfDrawTargetPtr const & drawtarget )
{
    if (not drawtarget) {
        TF_CODING_ERROR( "Cannot clone TfNullPtr attachments." );
    }

    // garbage collection will take care of the existing instance pointed to
    // by the RefPtr
    _attachmentsPtr = drawtarget->_attachmentsPtr;

    TF_FOR_ALL( it, _attachmentsPtr->attachments ) {
        _BindAttachment( it->second );
    }
}

GlfDrawTarget::AttachmentsMap const &
GlfDrawTarget::GetAttachments() const
{
    return _GetAttachments();
}

GlfDrawTarget::AttachmentsMap &
GlfDrawTarget::_GetAttachments() const
{
    TF_VERIFY( _attachmentsPtr,
        "DrawTarget has uninitialized attachments map.");

    return _attachmentsPtr->attachments;
}

void
GlfDrawTarget::SetSize( GfVec2i size )
{
    if (size==_size) {
        return;
    }

    if (not IsBound()) {
        TF_CODING_ERROR( "Cannot change the size of an unbound DrawTarget" );
    }

    _size = size;

    AttachmentsMap & attachments = _GetAttachments();

    TF_FOR_ALL ( it, attachments ) {
        AttachmentRefPtr var = it->second;

        var->ResizeTexture(_size);

        _BindAttachment(var);
    }
}

void
GlfDrawTarget::_DeleteAttachments()
{
    // Can't delete the attachment textures while someone else is still holding
    // onto them.
    // XXX This code needs refactoring so that Attachment & AttachmentsContainer
    // own the methods over their data (with casccading calls coming from the
    // DrawTarget API). Checking for the RefPtr uniqueness is somewhat working
    // against the nature of RefPtr..
    if (not _attachmentsPtr->IsUnique()) {
        return;
    }

    AttachmentsMap & attachments = _GetAttachments();

    attachments.clear();
}

static int _GetMaxAttachments( )
{
    int maxAttach = 0;
    glGetIntegerv(GL_MAX_COLOR_ATTACHMENTS, &maxAttach);
    return maxAttach;
}

void 
GlfDrawTarget::_GenFrameBuffer()
{
    _SaveBindingState();

    _owningContext = GlfGLContext::GetCurrentGLContext();

    glGenFramebuffers(1, &_framebuffer);

    glBindFramebuffer(GL_FRAMEBUFFER, _framebuffer);

    TF_VERIFY(glIsFramebuffer(_framebuffer),
        "Failed to allocate framebuffer");

    _RestoreBindingState();
}

GLuint
GlfDrawTarget::GetFramebufferId() const
{
    return _framebuffer;
}

// Attach a texture to one of the attachment points of the framebuffer.
// We assume that the framebuffer is currently bound !
void
GlfDrawTarget::_BindAttachment( GlfDrawTarget::AttachmentRefPtr const & a )
{
    GLuint id = a->GetGlTextureName();

    int attach = a->GetAttach();

    GLenum attachment = GL_COLOR_ATTACHMENT0;
    if (a->GetFormat()==GL_DEPTH_COMPONENT) {
        attachment = GL_DEPTH_ATTACHMENT;
    } else if (a->GetFormat()==GL_DEPTH_STENCIL) {
        attachment = GL_DEPTH_STENCIL_ATTACHMENT;
    } else {
        if (attach < 0) {
            TF_CODING_ERROR("Attachment index cannot be negative");
            return;
        }

        TF_VERIFY( attach < _GetMaxAttachments(),
            "Exceeding number of Attachments available ");

        attachment += attach;
    }

    glFramebufferTexture2D(GL_FRAMEBUFFER,
        attachment, GL_TEXTURE_2D, id, /*level*/ 0);

    GLF_POST_PENDING_GL_ERRORS();
}

void
GlfDrawTarget::_SaveBindingState()
{
    glGetIntegerv(GL_READ_FRAMEBUFFER_BINDING,
                        (GLint*)&_unbindRestoreReadFB);
    glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING,
                        (GLint*)&_unbindRestoreDrawFB);

}

void
GlfDrawTarget::_RestoreBindingState()
{
    glBindFramebuffer(GL_READ_FRAMEBUFFER,
                         _unbindRestoreReadFB);
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER,
                         _unbindRestoreDrawFB);
}

void
GlfDrawTarget::Bind()
{
    if (++_bindDepth != 1) {
        return;
    }

    _SaveBindingState();

    // GL Frame buffer objects are not shared between
    // contexts,  So make sure we are on our owning context before we try to
    // bind.  The reason to test rather than switch is because the user's
    // code may have setup other gl state and not expect a context switch here.
    // Also the switch may be expensive, so we want to be explict about when
    // they can occur.
    if (not TF_VERIFY(_owningContext->IsCurrent())) {
        return;
    }



    glBindFramebuffer(GL_FRAMEBUFFER, _framebuffer);

    GLF_POST_PENDING_GL_ERRORS();
}

bool
GlfDrawTarget::IsBound() const
{
    return (_bindDepth > 0);
}

void
GlfDrawTarget::Unbind()
{
    if (--_bindDepth != 0) {
        return;
    }

    _RestoreBindingState();

    TouchContents();

    GLF_POST_PENDING_GL_ERRORS();
}

void
GlfDrawTarget::TouchContents()
{
    AttachmentsMap const & attachments = GetAttachments();

    TF_FOR_ALL ( it, attachments ) {
        it->second->TouchContents();
    }
}

bool
GlfDrawTarget::IsValid(std::string * reason)
{
    return _Validate(reason);
}


bool
GlfDrawTarget::_Validate(std::string * reason)
{
    if (not _framebuffer) {
        return false;
    }

    bool status = false;

    status = GlfCheckGLFrameBufferStatus(GL_FRAMEBUFFER, reason);

    return status;
}

bool
GlfDrawTarget::WriteToFile(std::string const & name,
                            std::string const & filename,
                            GfMatrix4d const & viewMatrix,
                            GfMatrix4d const & projectionMatrix)
{
    AttachmentsMap const & attachments = GetAttachments();
    AttachmentsMap::const_iterator it = attachments.find( name );

    if (it==attachments.end()) {
        TF_CODING_ERROR( "\""+name+"\" is not a valid variable name for this"
                         " DrawTarget" );
        return false;
    }

    AttachmentRefPtr const & a = it->second;

    if (not _framebuffer) {
        TF_CODING_ERROR( "DrawTarget has no framebuffer" );
        return false;
    }

    int nelems = GlfGetNumElements(a->GetFormat()),
        elemsize = GlfGetElementSize(a->GetType()),
        stride = _size[0] * nelems * elemsize,
        bufsize = _size[1] * stride;

    void * buf = malloc( bufsize );

    {
        glPushClientAttrib(GL_CLIENT_PIXEL_STORE_BIT);

        glPixelStorei(GL_PACK_ROW_LENGTH, 0);
        glPixelStorei(GL_PACK_ALIGNMENT, 1);
        glPixelStorei(GL_PACK_SKIP_PIXELS, 0);
        glPixelStorei(GL_PACK_SKIP_ROWS, 0);

        GLint restoreBinding, restoreActiveTexture;
        glGetIntegerv( GL_TEXTURE_BINDING_2D, &restoreBinding );
        glGetIntegerv( GL_ACTIVE_TEXTURE, & restoreActiveTexture);

        glActiveTexture( GL_TEXTURE0 );
        glBindTexture( GL_TEXTURE_2D, a->GetGlTextureName() );

        glGetTexImage(GL_TEXTURE_2D, 0, a->GetFormat(), a->GetType(), buf);

        glActiveTexture( restoreActiveTexture );
        glBindTexture( GL_TEXTURE_2D, restoreBinding );

        glPopClientAttrib();

        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }

    VtDictionary metadata;

    std::string ext = TfStringGetSuffix(filename);
    if (name == "depth" and ext == "zfile") {
        // transform depth value from normalized to camera space length
        float *p = (float*)buf;
        for (size_t i = 0; i < bufsize/sizeof(float); ++i){
            p[i] = (float)(-2*p[i] / projectionMatrix[2][2]);
        }

        // embed matrices into metadata
        GfMatrix4d worldToCameraTransform = viewMatrix;
        GfMatrix4d worldToScreenTransform = viewMatrix * projectionMatrix;

        GfMatrix4d invZ = GfMatrix4d().SetScale(GfVec3d(1, 1, -1));
        worldToCameraTransform *= invZ;

        metadata["Nl"] = worldToCameraTransform;
        metadata["NP"] = worldToScreenTransform;
    }

    GlfImage::StorageSpec storage;
    storage.width = _size[0];
    storage.height = _size[1];
    storage.format = a->GetFormat();
    storage.type = a->GetType();
    storage.flipped = true;
    storage.data = buf;

    GlfImageSharedPtr image = GlfImage::OpenForWriting(filename);
    bool writeSuccess = image and image->Write(storage, metadata);

    free(buf);

    if (not writeSuccess) {
        TF_RUNTIME_ERROR("Failed to write image to %s", filename.c_str());
        return false;
    }

    GLF_POST_PENDING_GL_ERRORS();

    return true;
}

//----------------------------------------------------------------------

GlfDrawTarget::AttachmentRefPtr
GlfDrawTarget::Attachment::New(int glIndex, GLenum format, GLenum type,
                                GLenum internalFormat, GfVec2i size)
{
    return TfCreateRefPtr(new Attachment(glIndex, format, type,
                                         internalFormat, size));
}

GlfDrawTarget::Attachment::Attachment(int glIndex, GLenum format,
                                       GLenum type, GLenum internalFormat,
                                       GfVec2i size) :
    _format(format),
    _type(type),
    _internalFormat(internalFormat),
    _glIndex(glIndex),
    _size(size)
{
    _textureName = _GenTexture();
}

GlfDrawTarget::Attachment::~Attachment()
{
    _DeleteTexture(_textureName);
}

// Generate a simple GL_TEXTURE_2D to use as an attachment
// we assume that the framebuffer is currently bound !
GLuint
GlfDrawTarget::Attachment::_GenTexture()
{
    GLuint id=0;

    glGenTextures(1, &id);

    glBindTexture( GL_TEXTURE_2D, id );

    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE );
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE );

    GLenum internalFormat = _internalFormat;
    GLenum type = _type;

    if (_format==GL_DEPTH_COMPONENT) {
        internalFormat=GL_DEPTH_COMPONENT32F;
        if (type!=GL_FLOAT) {
            TF_CODING_ERROR("Only GL_FLOAT textures can be used for the"
            " depth attachment point");
            type = GL_FLOAT;
        }
    }

    glTexImage2D( GL_TEXTURE_2D, /*level*/ 0, internalFormat,
                  _size[0], _size[1],
                 /*border*/ 0, _format, type, NULL); 

    glBindTexture( GL_TEXTURE_2D, 0 );

    GLF_POST_PENDING_GL_ERRORS();

    return id;
}

void
GlfDrawTarget::Attachment::_DeleteTexture(GLuint & id)
{
    if (id) {
        GlfSharedGLContextScopeHolder sharedGLContextScopeHolder;

        TF_VERIFY(glIsTexture(id), "Tried to delete an invalid texture");

        glDeleteTextures(1, &id);
        id=0;
    }

    GLF_POST_PENDING_GL_ERRORS();
}

void
GlfDrawTarget::Attachment::ResizeTexture(const GfVec2i &size)
{
    _size = size;

    _DeleteTexture(_textureName);
    _textureName = _GenTexture();
}

/* virtual */
GlfTexture::BindingVector
GlfDrawTarget::Attachment::GetBindings(TfToken const & identifier,
                                       GLuint samplerName) const
{
    return BindingVector(1,
                Binding(identifier, GlfTextureTokens->texels,
                        GL_TEXTURE_2D, GetGlTextureName(), samplerName));
}

/* virtual */
VtDictionary
GlfDrawTarget::Attachment::GetTextureInfo() const
{
    VtDictionary info;

    int bytePerPixel = (_type == GL_FLOAT) ? 4 : 1;
    int numChannel = 1;
    if (_format == GL_RG) numChannel = 2;
    else if (_format == GL_RGB) numChannel = 3;
    else if (_format == GL_RGBA) numChannel = 4;

    info["width"] = (int)_size[0];
    info["height"] = (int)_size[1];
    info["memoryUsed"] = (size_t)(bytePerPixel * numChannel * _size[0] * _size[1]);
    info["depth"] = 1;
    info["format"] = (int)_internalFormat;
    info["imageFilePath"] = TfToken("DrawTarget");
    info["referenceCount"] = GetRefCount().Get();

    return info;
}

void
GlfDrawTarget::Attachment::TouchContents()
{
    _UpdateContentsID();
}
