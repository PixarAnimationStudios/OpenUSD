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
#include "pxr/imaging/glf/glew.h"

#include "pxr/imaging/hdx/drawTargetTextureResource.h"
#include "pxr/imaging/hd/conversions.h"


Hdx_DrawTargetTextureResource::Hdx_DrawTargetTextureResource()
 : _attachment()
 , _sampler(0)
{
    // GL initialization guard for headless unit testing
    if (glGenSamplers) {
        glGenSamplers(1, &_sampler);
    }


}

Hdx_DrawTargetTextureResource::~Hdx_DrawTargetTextureResource()
{
    // GL initialization guard for headless unit test
    if (glDeleteSamplers) {
        glDeleteSamplers(1, &_sampler);
    }
}

void
Hdx_DrawTargetTextureResource::SetAttachment(const GlfDrawTarget::AttachmentRefPtr &attachment)
{
    _attachment = attachment;
}

void
Hdx_DrawTargetTextureResource::SetSampler(HdWrap wrapS,
                                          HdWrap wrapT,
                                          HdMinFilter minFilter,
                                          HdMagFilter magFilter)
{
    static const float borderColor[4] = {0.0f, 0.0f, 0.0f, 0.0f};

    // Convert params to Gl
    GLenum glWrapS = HdConversions::GetWrap(wrapS);
    GLenum glWrapT = HdConversions::GetWrap(wrapT);
    GLenum glMinFilter = HdConversions::GetMinFilter(minFilter);
    GLenum glMagFilter = HdConversions::GetMagFilter(magFilter);

    glSamplerParameteri(_sampler, GL_TEXTURE_WRAP_S, glWrapS);
    glSamplerParameteri(_sampler, GL_TEXTURE_WRAP_T, glWrapT);
    glSamplerParameteri(_sampler, GL_TEXTURE_MIN_FILTER, glMinFilter);
    glSamplerParameteri(_sampler, GL_TEXTURE_MAG_FILTER, glMagFilter);
    glSamplerParameterf(_sampler, GL_TEXTURE_MAX_ANISOTROPY_EXT, 1.0);
    glSamplerParameterfv(_sampler, GL_TEXTURE_BORDER_COLOR, borderColor);
}


bool
Hdx_DrawTargetTextureResource::IsPtex() const
{
    return false;
}

GLuint
Hdx_DrawTargetTextureResource::GetTexelsTextureId()
{
    return _attachment->GetGlTextureName();
}

GLuint
Hdx_DrawTargetTextureResource::GetTexelsSamplerId()
{
    return _sampler;
}

GLuint64EXT
Hdx_DrawTargetTextureResource::GetTexelsTextureHandle()
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
Hdx_DrawTargetTextureResource::GetLayoutTextureId()
{
    TF_CODING_ERROR("Draw targets are not ptex");
    return 0;
}

GLuint64EXT
Hdx_DrawTargetTextureResource::GetLayoutTextureHandle()
{
    TF_CODING_ERROR("Draw targets are not ptex");
    return 0;
}

size_t
Hdx_DrawTargetTextureResource::GetMemoryUsed()
{
    return _attachment->GetMemoryUsed();
}
