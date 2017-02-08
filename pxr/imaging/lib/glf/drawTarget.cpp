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
#include "pxr/base/tf/envSetting.h"

#include "pxr/base/tracelite/trace.h"

PXR_NAMESPACE_OPEN_SCOPE


TF_DEFINE_ENV_SETTING(GLF_DRAW_TARGETS_NUM_SAMPLES, 4,
                      "Number of samples greater than 1 forces MSAA.");

static unsigned int 
GetNumSamples()
{
    static int reqNumSamples = TfGetEnvSetting(GLF_DRAW_TARGETS_NUM_SAMPLES);
    unsigned int numSamples = 1;
    if (reqNumSamples > 1) {
        numSamples = (reqNumSamples & (reqNumSamples - 1)) ? 1 : reqNumSamples;    
    }
    return numSamples;
}

GlfDrawTargetRefPtr
GlfDrawTarget::New( GfVec2i const & size, bool requestMSAA )
{
    return TfCreateRefPtr(new This(size, requestMSAA));
}

GlfDrawTarget::GlfDrawTarget( GfVec2i const & size, bool requestMSAA /* =false */) :
    _framebuffer(0),
    _framebufferMS(0),
    _unbindRestoreReadFB(0),
    _unbindRestoreDrawFB(0),
    _bindDepth(0),
    _size(size),
    _numSamples(1)
{
    GlfGlewInit();

    // If MSAA has been requested and it is enabled then we will create
    // msaa buffers
    if (requestMSAA) {
        _numSamples = GetNumSamples();
    }

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
    _framebufferMS(0),
    _unbindRestoreReadFB(0),
    _unbindRestoreDrawFB(0),
    _bindDepth(0),
    _size(drawtarget->_size),
    _numSamples(drawtarget->_numSamples),
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

    if (_framebufferMS) {
        TF_VERIFY(glIsFramebuffer(_framebufferMS),
            "Tried to free invalid multisampled framebuffer");

        glDeleteFramebuffers(1, &_framebufferMS);
        _framebufferMS = 0;
    }
}

void
GlfDrawTarget::AddAttachment( std::string const & name, 
                              GLenum format, GLenum type,
                              GLenum internalFormat )
{
    if (!IsBound()) {
        TF_CODING_ERROR("Cannot change the size of an unbound GlfDrawTarget");
    }

    AttachmentsMap & attachments = _GetAttachments();
    AttachmentsMap::iterator it = attachments.find( name );

    if (it==attachments.end()) {

        AttachmentRefPtr attachment = Attachment::New((int)attachments.size(),
                                                      format, type,
                                                      internalFormat, _size,
                                                      _numSamples);

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
    if (!drawtarget) {
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

    if (!IsBound()) {
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
    if (!_attachmentsPtr->IsUnique()) {
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

    // Create multisampled framebuffer
    if (HasMSAA()) {
        glGenFramebuffers(1, &_framebufferMS);
        glBindFramebuffer(GL_FRAMEBUFFER, _framebufferMS);
        TF_VERIFY(glIsFramebuffer(_framebufferMS),
            "Failed to allocate multisampled framebuffer");
    }

    // Create non-multisampled framebuffer
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

GLuint 
GlfDrawTarget::GetFramebufferMSId() const
{
    return _framebufferMS;
}

// Attach a texture to one of the attachment points of the framebuffer.
// We assume that the framebuffer is currently bound !
void
GlfDrawTarget::_BindAttachment( GlfDrawTarget::AttachmentRefPtr const & a )
{
    GLuint id = a->GetGlTextureName();
    GLuint idMS = a->GetGlTextureMSName();

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

    // Multisampled framebuffer
    if (HasMSAA()) {
        glBindFramebuffer(GL_FRAMEBUFFER, _framebufferMS);
        glFramebufferTexture2D(GL_FRAMEBUFFER, 
            attachment, GL_TEXTURE_2D_MULTISAMPLE, idMS, /*level*/ 0);
    }

    // Regular framebuffer
    glBindFramebuffer(GL_FRAMEBUFFER, _framebuffer);
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
    if (!TF_VERIFY(_owningContext->IsCurrent())) {
        return;
    }

    if (HasMSAA()) {
        glBindFramebuffer(GL_FRAMEBUFFER, _framebufferMS);
    } else {
        glBindFramebuffer(GL_FRAMEBUFFER, _framebuffer);
    }

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
GlfDrawTarget::Resolve()
{
    if (HasMSAA()) {
        // Resolve MSAA fbo to a regular fbo
        glBindFramebuffer(GL_READ_FRAMEBUFFER, _framebufferMS);
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, _framebuffer);
        glBlitFramebuffer(0, 0, _size[0], _size[1], 
                          0, 0, _size[0], _size[1], 
                          GL_COLOR_BUFFER_BIT | 
                          GL_DEPTH_BUFFER_BIT | 
                          GL_STENCIL_BUFFER_BIT , 
                          GL_NEAREST); 
    }
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
    if (!_framebuffer) {
        return false;
    }

    return GlfCheckGLFrameBufferStatus(GL_FRAMEBUFFER, reason);
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

    if (!_framebuffer) {
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
    if (name == "depth" && ext == "zfile") {
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
    bool writeSuccess = image && image->Write(storage, metadata);

    free(buf);

    if (!writeSuccess) {
        TF_RUNTIME_ERROR("Failed to write image to %s", filename.c_str());
        return false;
    }

    GLF_POST_PENDING_GL_ERRORS();

    return true;
}

//----------------------------------------------------------------------

GlfDrawTarget::AttachmentRefPtr
GlfDrawTarget::Attachment::New(int glIndex, GLenum format, GLenum type,
                                GLenum internalFormat, GfVec2i size,
                                unsigned int numSamples)
{
    return TfCreateRefPtr(new Attachment(glIndex, format, type,
                                         internalFormat, size, 
                                         numSamples));
}

GlfDrawTarget::Attachment::Attachment(int glIndex, GLenum format,
                                       GLenum type, GLenum internalFormat,
                                       GfVec2i size, 
                                       unsigned int numSamples) :
    _textureName(0),
    _textureNameMS(0),
    _format(format),
    _type(type),
    _internalFormat(internalFormat),
    _glIndex(glIndex),
    _size(size),
    _numSamples(numSamples)
{
    _GenTexture();
}

GlfDrawTarget::Attachment::~Attachment()
{
    _DeleteTexture();
}

// Generate a simple GL_TEXTURE_2D to use as an attachment
// we assume that the framebuffer is currently bound !
void
GlfDrawTarget::Attachment::_GenTexture()
{
    GLenum internalFormat = _internalFormat;
    GLenum type = _type;
    size_t memoryUsed = 0;

    if (_format==GL_DEPTH_COMPONENT) {
        internalFormat=GL_DEPTH_COMPONENT32F;
        if (type!=GL_FLOAT) {
            TF_CODING_ERROR("Only GL_FLOAT textures can be used for the"
            " depth attachment point");
            type = GL_FLOAT;
        }
    }

    int bytePerPixel = (_type == GL_FLOAT) ? 4 : 1;
    int numChannel;
    switch (_format)
    {
        case GL_RG:
            numChannel = 2;
            break;

        case GL_RGB:
            numChannel = 3;
            break;

        case GL_RGBA:
            numChannel = 4;
            break;

        default:
            numChannel = 1;
    }

    size_t baseImageSize = (size_t)(bytePerPixel *
                                    numChannel   *
                                    _size[0]     *
                                    _size[1]);

    // Create multisampled texture
    if (_numSamples > 1) {
        glGenTextures( 1, &_textureNameMS ); 
        glBindTexture( GL_TEXTURE_2D_MULTISAMPLE, _textureNameMS );

        // XXX: Hardcoded filtering for now
        glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
        glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
        glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE );
        glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE );

        glTexImage2DMultisample( GL_TEXTURE_2D_MULTISAMPLE,
                                 _numSamples, _internalFormat, 
                                 _size[0], _size[1], GL_TRUE );

        glBindTexture( GL_TEXTURE_2D_MULTISAMPLE, 0);

        memoryUsed = baseImageSize * _numSamples;
    }

    // Create non-multisampled texture
    glGenTextures( 1, &_textureName );
    glBindTexture( GL_TEXTURE_2D, _textureName );

    // XXX: Hardcoded filtering for now
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE );
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE );

    glTexImage2D( GL_TEXTURE_2D, /*level*/ 0, internalFormat,
                  _size[0], _size[1],
                 /*border*/ 0, _format, type, NULL); 

    glBindTexture( GL_TEXTURE_2D, 0 );

    memoryUsed += baseImageSize * _numSamples;

    _SetMemoryUsed(memoryUsed);

    GLF_POST_PENDING_GL_ERRORS();
}

void
GlfDrawTarget::Attachment::_DeleteTexture()
{
    if (_textureName) {
        GlfSharedGLContextScopeHolder sharedGLContextScopeHolder;

        TF_VERIFY(glIsTexture(_textureName), "Tried to delete an invalid texture");
        glDeleteTextures(1, &_textureName);
        _textureName = 0;
    }

    if (_textureNameMS) {
        GlfSharedGLContextScopeHolder sharedGLContextScopeHolder;

        TF_VERIFY(glIsTexture(_textureNameMS), "Tried to delete an invalid texture");
        glDeleteTextures(1, &_textureNameMS);
        _textureNameMS = 0;
    }    

    GLF_POST_PENDING_GL_ERRORS();
}

void
GlfDrawTarget::Attachment::ResizeTexture(const GfVec2i &size)
{
    _size = size;

    _DeleteTexture();
    _GenTexture();
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



    info["width"] = (int)_size[0];
    info["height"] = (int)_size[1];
    info["memoryUsed"] = GetMemoryUsed();
    info["depth"] = 1;
    info["format"] = (int)_internalFormat;
    info["imageFilePath"] = TfToken("DrawTarget");
    info["referenceCount"] = GetRefCount().Get();
    info["numSamples"] = _numSamples;

    return info;
}

void
GlfDrawTarget::Attachment::TouchContents()
{
    _UpdateContentsID();
}

PXR_NAMESPACE_CLOSE_SCOPE

