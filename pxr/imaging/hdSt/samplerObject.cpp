//
// Copyright 2020 Pixar
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

#include "pxr/imaging/hdSt/samplerObject.h"

#include "pxr/imaging/hdSt/textureObject.h"
#include "pxr/imaging/hdSt/glConversions.h"

#include "pxr/imaging/glf/diagnostic.h"
#include "pxr/imaging/hgiGL/texture.h"

PXR_NAMESPACE_OPEN_SCOPE

bool
HdStSamplerParameters::operator==(const HdStSamplerParameters &other) const
{
    return
        (wrapS == other.wrapS) &&
        (wrapT == other.wrapT) &&
        (wrapR == other.wrapR) &&
        (minFilter == other.minFilter) &&
        (magFilter == other.magFilter);
}

bool
HdStSamplerParameters::operator!=(const HdStSamplerParameters &other) const
{
    return !(*this == other);
}

HdStSamplerObject::~HdStSamplerObject() = default;

// Generate GL sampler
static
GLuint
_GenGLSampler(HdStSamplerParameters const &samplerParameters)
{
    GLuint result = 0;
    glGenSamplers(1, &result);

    if (samplerParameters.wrapS == HdWrapUseMetadata ||
        samplerParameters.wrapS == HdWrapLegacy) {
        TF_WARN(
            "Using texture metadata to get wrapS is not supported yet.");
    }

    if (samplerParameters.wrapT == HdWrapUseMetadata ||
        samplerParameters.wrapT == HdWrapLegacy) {
        TF_WARN(
            "Using texture metadata to get wrapT is not supported yet.");
    }

    if (samplerParameters.wrapR == HdWrapUseMetadata ||
        samplerParameters.wrapR == HdWrapLegacy) {
        TF_WARN(
            "Using texture metadata to get wrapR is not supported yet.");
    }

    glSamplerParameteri(
        result,
        GL_TEXTURE_WRAP_S,
        HdStGLConversions::GetWrap(samplerParameters.wrapS));

    glSamplerParameteri(
        result,
        GL_TEXTURE_WRAP_T,
        HdStGLConversions::GetWrap(samplerParameters.wrapT));

    glSamplerParameteri(
        result,
        GL_TEXTURE_WRAP_R,
        HdStGLConversions::GetWrap(samplerParameters.wrapR));

    glSamplerParameteri(
        result,
        GL_TEXTURE_MIN_FILTER,
        HdStGLConversions::GetMinFilter(samplerParameters.minFilter));

    glSamplerParameteri(
        result,
        GL_TEXTURE_MAG_FILTER,
        HdStGLConversions::GetMagFilter(samplerParameters.magFilter));

    static const GfVec4f borderColor(0.0);
    glSamplerParameterfv(
        result,
        GL_TEXTURE_BORDER_COLOR,
        borderColor.GetArray());

    static const float _maxAnisotropy = 16.0;

    glSamplerParameterf(
        result,
        GL_TEXTURE_MAX_ANISOTROPY_EXT,
        _maxAnisotropy);

    GLF_POST_PENDING_GL_ERRORS();

    return result;
}

// Get texture sampler handle for bindless textures.
static
GLuint64EXT 
_GenGLTextureSamplerHandle(HgiTextureHandle const &textureHandle,
                           const GLuint samplerName,
                           const bool createBindlessHandle)
{
    if (!createBindlessHandle) {
        return 0;
    }

    HgiTexture * const texture = textureHandle.Get();
    if (texture == nullptr) {
        return 0;
    }

    HgiGLTexture * const glTexture = dynamic_cast<HgiGLTexture*>(texture);
    if (glTexture == nullptr) {
        TF_CODING_ERROR("Only OpenGL textures supported");
        return 0;
    }

    const GLuint textureName = glTexture->GetTextureId();

    if (textureName == 0) {
        return 0;
    }

    if (samplerName == 0) {
        return 0;
    }

    const GLuint64EXT result =
        glGetTextureSamplerHandleARB(textureName, samplerName);

    glMakeTextureHandleResidentARB(result);

    GLF_POST_PENDING_GL_ERRORS();

    return result;
}

HdStUvSamplerObject::HdStUvSamplerObject(
    HdStUvTextureObject const &texture,
    HdStSamplerParameters const &samplerParameters,
    const bool createBindlessHandle)
  : _glSamplerName(
      _GenGLSampler(
          samplerParameters))
  , _glTextureSamplerHandle(
      _GenGLTextureSamplerHandle(
          texture.GetTexture(),
          _glSamplerName,
          createBindlessHandle))
{
}

HdStUvSamplerObject::~HdStUvSamplerObject()
{
    // Delete GL objects in order opposite of creation.

    if (_glTextureSamplerHandle) {
        glMakeTextureHandleNonResidentARB(_glTextureSamplerHandle);
    }

    if (_glSamplerName) {
        glDeleteSamplers(1, &_glSamplerName);
    }
}
    
PXR_NAMESPACE_CLOSE_SCOPE
