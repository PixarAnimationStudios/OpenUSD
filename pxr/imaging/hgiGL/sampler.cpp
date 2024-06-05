//
// Copyright 2020 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/imaging/garch/glApi.h"

#include "pxr/imaging/hgiGL/conversions.h"
#include "pxr/imaging/hgiGL/diagnostic.h"
#include "pxr/imaging/hgiGL/sampler.h"
#include "pxr/imaging/hgiGL/texture.h"

#include "pxr/base/gf/vec4f.h"
#include "pxr/base/tf/diagnostic.h"

PXR_NAMESPACE_OPEN_SCOPE


HgiGLSampler::HgiGLSampler(HgiSamplerDesc const& desc)
    : HgiSampler(desc)
    , _samplerId(0)
    , _bindlessTextureId(0)
    , _bindlessHandle(0)
{
    glCreateSamplers(1, &_samplerId);

    if (!_descriptor.debugName.empty()) {
        HgiGLObjectLabel(GL_SAMPLER, _samplerId, _descriptor.debugName);
    }

    glSamplerParameteri(
        _samplerId,
        GL_TEXTURE_WRAP_S,
        HgiGLConversions::GetSamplerAddressMode(desc.addressModeU));

    glSamplerParameteri(
        _samplerId,
        GL_TEXTURE_WRAP_T,
        HgiGLConversions::GetSamplerAddressMode(desc.addressModeV));

    glSamplerParameteri(
        _samplerId,
        GL_TEXTURE_WRAP_R,
        HgiGLConversions::GetSamplerAddressMode(desc.addressModeW));

    const GLenum minFilter =
        HgiGLConversions::GetMinFilter(desc.minFilter, desc.mipFilter);
    glSamplerParameteri(
        _samplerId,
        GL_TEXTURE_MIN_FILTER,
        minFilter);

    const GLenum magFilter =
        HgiGLConversions::GetMagFilter(desc.magFilter);
    glSamplerParameteri(
        _samplerId,
        GL_TEXTURE_MAG_FILTER,
        magFilter);

    glSamplerParameterfv(
        _samplerId,
        GL_TEXTURE_BORDER_COLOR,
        HgiGLConversions::GetBorderColor(desc.borderColor).GetArray());

    // Certain platforms will ignore minFilter and magFilter when 
    // GL_TEXTURE_MAX_ANISOTROPY_EXT is > 1. We choose not to enable anisotropy
    // when the filters are "nearest" to ensure those filters are used.
    if (minFilter != GL_NEAREST && minFilter != GL_NEAREST_MIPMAP_NEAREST &&
        magFilter != GL_NEAREST) {
        static const float maxAnisotropy = 16.0;
        glSamplerParameterf(
            _samplerId,
            GL_TEXTURE_MAX_ANISOTROPY_EXT,
            maxAnisotropy);
    }

    glSamplerParameteri(
        _samplerId, 
        GL_TEXTURE_COMPARE_MODE, 
        desc.enableCompare ? GL_COMPARE_REF_TO_TEXTURE : GL_NONE);
        
    glSamplerParameteri(
        _samplerId, 
        GL_TEXTURE_COMPARE_FUNC, 
        HgiGLConversions::GetCompareFunction(desc.compareFunction));

    HGIGL_POST_PENDING_GL_ERRORS();
}

HgiGLSampler::~HgiGLSampler()
{
    // Deleting the GL sampler automatically deletes the bindless
    // sampler handle. In fact, even destroying the underlying
    // texture (which is out of our control here), deletes the
    // bindless sampler handle and the same bindless sampler handle
    // value might be re-used by the driver. So it is unsafe to
    // call glMakeTextureHandleNonResidentARB(_bindlessHandle) here.
    glDeleteSamplers(1, &_samplerId);
    _samplerId = 0;
    _bindlessTextureId = 0;
    _bindlessHandle = 0;
    HGIGL_POST_PENDING_GL_ERRORS();
}

uint64_t
HgiGLSampler::GetRawResource() const
{
    return (uint64_t) _samplerId;
}

uint32_t
HgiGLSampler::GetSamplerId() const
{
    return _samplerId;
}

uint64_t
HgiGLSampler::GetBindlessHandle(HgiTextureHandle const &textureHandle)
{
    GLuint textureId = textureHandle->GetRawResource();
    if (textureId == 0) {
        return 0;
    }

    if (!_bindlessHandle || _bindlessTextureId != textureId) {
        const GLuint64EXT result =
            glGetTextureSamplerHandleARB(textureId, _samplerId);

        if (!glIsTextureHandleResidentARB(result)) {
            glMakeTextureHandleResidentARB(result);
        }

        _bindlessTextureId = textureId;
        _bindlessHandle = result;

        HGIGL_POST_PENDING_GL_ERRORS();
    }

    return _bindlessHandle;
}


PXR_NAMESPACE_CLOSE_SCOPE
