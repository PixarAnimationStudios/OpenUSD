//
// Copyright 2020 benmalartre
//
// Unlicensed
// 
#include "pxr/imaging/glf/glew.h"

#include "pxr/imaging/plugin/LoFi/drawTargetTextureResource.h"
#include "pxr/imaging/plugin/LoFi/glConversions.h"

PXR_NAMESPACE_OPEN_SCOPE


LoFiDrawTargetTextureResource::LoFiDrawTargetTextureResource()
 : LoFiTextureResource()
 , _attachment()
 , _sampler(0)
 , _borderColor(0.0,0.0,0.0,0.0)
 , _maxAnisotropy(16.0)
{
    // GL initialization guard for headless unit testing
    if (glGenSamplers) {
        glGenSamplers(1, &_sampler);
    }
}

LoFiDrawTargetTextureResource::~LoFiDrawTargetTextureResource()
{
    // GL initialization guard for headless unit test
    if (glDeleteSamplers) {
        glDeleteSamplers(1, &_sampler);
    }
}

void
LoFiDrawTargetTextureResource::SetAttachment(
                              const GlfDrawTarget::AttachmentRefPtr &attachment)
{
    _attachment = attachment;
}

void
LoFiDrawTargetTextureResource::SetSampler(HdWrap wrapS,
                                          HdWrap wrapT,
                                          HdMinFilter minFilter,
                                          HdMagFilter magFilter)
{
    // Convert params to Gl
    GLenum glWrapS = LoFiGLConversions::GetWrap(wrapS);
    GLenum glWrapT = LoFiGLConversions::GetWrap(wrapT);
    GLenum glMinFilter = LoFiGLConversions::GetMinFilter(minFilter);
    GLenum glMagFilter = LoFiGLConversions::GetMagFilter(magFilter);

    glSamplerParameteri(_sampler, GL_TEXTURE_WRAP_S, glWrapS);
    glSamplerParameteri(_sampler, GL_TEXTURE_WRAP_T, glWrapT);
    glSamplerParameteri(_sampler, GL_TEXTURE_MIN_FILTER, glMinFilter);
    glSamplerParameteri(_sampler, GL_TEXTURE_MAG_FILTER, glMagFilter);
    glSamplerParameterf(_sampler, GL_TEXTURE_MAX_ANISOTROPY_EXT, 
                        _maxAnisotropy);
    glSamplerParameterfv(_sampler, GL_TEXTURE_BORDER_COLOR, 
                         _borderColor.GetArray());
}

HdTextureType
LoFiDrawTargetTextureResource::GetTextureType() const
{
    return HdTextureType::Uv;
}

GLuint
LoFiDrawTargetTextureResource::GetTexelsTextureId()
{
    return _attachment->GetGlTextureName();
}

GLuint
LoFiDrawTargetTextureResource::GetTexelsSamplerId()
{
    return _sampler;
}

GLuint64EXT
LoFiDrawTargetTextureResource::GetTexelsTextureHandle()
{
    GLuint textureId = GetTexelsTextureId();

    if (textureId == 0) {
        return 0;
    }

    if (!TF_VERIFY(glGetTextureHandleARB) ||
        !TF_VERIFY(glGetTextureSamplerHandleARB)) {
        return 0;
    }

    GLuint samplerId = GetTexelsSamplerId();

    return glGetTextureSamplerHandleARB(textureId, samplerId);
}

GLuint
LoFiDrawTargetTextureResource::GetLayoutTextureId()
{
    TF_CODING_ERROR("Draw targets are not ptex");
    return 0;
}

GLuint64EXT
LoFiDrawTargetTextureResource::GetLayoutTextureHandle()
{
    TF_CODING_ERROR("Draw targets are not ptex");
    return 0;
}

size_t
LoFiDrawTargetTextureResource::GetMemoryUsed()
{
    return _attachment->GetMemoryUsed();
}

PXR_NAMESPACE_CLOSE_SCOPE

