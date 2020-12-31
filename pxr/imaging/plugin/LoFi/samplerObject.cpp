//
// Copyright 2020 benmalartre
//
// Unlicensed
//
#include "pxr/imaging/garch/glApi.h"

#include "pxr/imaging/plugin/LoFi/samplerObject.h"
#include "pxr/imaging/plugin/LoFi/ptexTextureObject.h"
#include "pxr/imaging/plugin/LoFi/resourceRegistry.h"
#include "pxr/imaging/plugin/LoFi/samplerObjectRegistry.h"
#include "pxr/imaging/plugin/LoFi/textureObject.h"
#include "pxr/imaging/plugin/LoFi/udimTextureObject.h"
#include "pxr/imaging/plugin/LoFi/hgiConversions.h"

#include "pxr/imaging/glf/diagnostic.h"
#include "pxr/imaging/hgi/hgi.h"
#include "pxr/imaging/hgiGL/texture.h"
#include "pxr/imaging/hgiGL/sampler.h"

PXR_NAMESPACE_OPEN_SCOPE

///////////////////////////////////////////////////////////////////////////////
// LoFiTextureObject

LoFiSamplerObject::LoFiSamplerObject(
    LoFiSamplerObjectRegistry * samplerObjectRegistry)
  : _samplerObjectRegistry(samplerObjectRegistry)
{
}

LoFiSamplerObject::~LoFiSamplerObject() = default;

Hgi *
LoFiSamplerObject::_GetHgi() const
{
    if (!TF_VERIFY(_samplerObjectRegistry)) {
        return nullptr;
    }

    LoFiResourceRegistry * const registry =
        _samplerObjectRegistry->GetResourceRegistry();
    if (!TF_VERIFY(registry)) {
        return nullptr;
    }

    Hgi * const hgi = registry->GetHgi();
    TF_VERIFY(hgi);

    return hgi;
}

///////////////////////////////////////////////////////////////////////////////
// Helpers

// Translate to Hgi
static
HgiSamplerDesc
_ToHgiSamplerDesc(HdSamplerParameters const &samplerParameters)
{
    HgiSamplerDesc desc;
    desc.debugName = "LoFiSamplerObject";
    desc.magFilter = LoFiHgiConversions::GetHgiMagFilter(
        samplerParameters.magFilter);
    LoFiHgiConversions::GetHgiMinAndMipFilter(
        samplerParameters.minFilter,
        &desc.minFilter, &desc.mipFilter);
    desc.addressModeU =
        LoFiHgiConversions::GetHgiSamplerAddressMode(samplerParameters.wrapS);
    desc.addressModeV =
        LoFiHgiConversions::GetHgiSamplerAddressMode(samplerParameters.wrapT);
    desc.addressModeW =
        LoFiHgiConversions::GetHgiSamplerAddressMode(samplerParameters.wrapR);

    return desc;
}

// Generate GL sampler
static
HgiSamplerHandle
_GenSampler(LoFiSamplerObjectRegistry * const samplerObjectRegistry,
            HdSamplerParameters const &samplerParameters,
            const bool createSampler)
{
    if (!createSampler) {
        return HgiSamplerHandle();
    }

    LoFiResourceRegistry * const registry =
        samplerObjectRegistry->GetResourceRegistry();
    if (!TF_VERIFY(registry)) {
        return HgiSamplerHandle();
    }

    Hgi * const hgi = registry->GetHgi();
    if (!TF_VERIFY(hgi)) {
        return HgiSamplerHandle();
    }

    return hgi->CreateSampler(_ToHgiSamplerDesc(samplerParameters));
}

// Get texture sampler handle for bindless textures.
static
GLuint64EXT
_GenGLTextureSamplerHandle(HgiTextureHandle const &textureHandle,
                           HgiSamplerHandle const &samplerHandle,
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

    GLuint textureName = glTexture->GetTextureId();
    if (textureName == 0) {
        return 0;
    }

    HgiSampler * const sampler = samplerHandle.Get();
    if (sampler == nullptr) {
        return 0;
    }

    HgiGLSampler * const glSampler = dynamic_cast<HgiGLSampler*>(sampler);
    if (glSampler == nullptr) {
        TF_CODING_ERROR("Only OpenGL samplers supported");
        return 0;
    }

    const GLuint samplerName = glSampler->GetSamplerId();
    if (samplerName == 0) {
        return 0;
    }

    const GLuint64EXT result =
        glGetTextureSamplerHandleARB(textureName, samplerName);

    if (!glIsTextureHandleResidentARB(result)) {
        glMakeTextureHandleResidentARB(result);
    }

    GLF_POST_PENDING_GL_ERRORS();

    return result;
}

// Get texture handle for bindless textures.
static
GLuint64EXT
_GenGlTextureHandle(HgiTextureHandle const &texture,
                    const bool createGLTextureHandle)
{
    if (!createGLTextureHandle) {
        return 0;
    }

    if (!texture) {
        return 0;
    }

    const GLuint textureName = texture->GetRawResource();
    const GLuint64EXT result = glGetTextureHandleARB(textureName);
    if (!glIsTextureHandleResidentARB(result)) {
        glMakeTextureHandleResidentARB(result);
    }

    GLF_POST_PENDING_GL_ERRORS();

    return result;
}

///////////////////////////////////////////////////////////////////////////////
// Uv sampler

// Resolve a wrap parameter using the opinion authored in the metadata of a
// texture file.
static
void
_ResolveSamplerParameter(
    const HdWrap textureOpinion,
    HdWrap * const parameter)
{
    if (*parameter == HdWrapNoOpinion) {
        *parameter = textureOpinion;
    }

    // Legacy behavior for HwUvTexture_1
    if (*parameter == HdWrapLegacyNoOpinionFallbackRepeat) {
        if (textureOpinion == HdWrapNoOpinion) {
            // Use repeat if there is no opinion on either the
            // texture node or in the texture file.
            *parameter = HdWrapRepeat;
        } else {
            *parameter = textureOpinion;
        }
    }
}

// Resolve wrapS or wrapT of the samplerParameters using metadata
// from the texture file.
static
HdSamplerParameters
_ResolveUvSamplerParameters(
    LoFiUvTextureObject const &texture,
    HdSamplerParameters const &samplerParameters)
{
    HdSamplerParameters result = samplerParameters;
    _ResolveSamplerParameter(
        texture.GetWrapParameters().first,
        &result.wrapS);

    _ResolveSamplerParameter(
        texture.GetWrapParameters().second,
        &result.wrapT);

    return result;
}

LoFiUvSamplerObject::LoFiUvSamplerObject(
    LoFiUvTextureObject const &texture,
    HdSamplerParameters const &samplerParameters,
    const bool createBindlessHandle,
    LoFiSamplerObjectRegistry * const samplerObjectRegistry)
  : LoFiSamplerObject(samplerObjectRegistry)
  , _sampler(
      _GenSampler(
          samplerObjectRegistry,
          _ResolveUvSamplerParameters(
              texture, samplerParameters),
          texture.IsValid()))
  , _glTextureSamplerHandle(
      _GenGLTextureSamplerHandle(
          texture.GetTexture(),
          _sampler,
          createBindlessHandle && texture.IsValid()))
{
}

LoFiUvSamplerObject::~LoFiUvSamplerObject()
{
    // Deleting the GL sampler automatically deletes the
    // texture sampler handle.
    // In fact, even destroying the underlying texture (which
    // is out of our control here), deletes the texture sampler
    // handle and the same texture sampler handle might be re-used
    // by the driver, so it is unsafe to call
    // glMakeTextureHandleNonResidentARB(_glTextureSamplerHandle);
    // here: LoFiTextureObject might destroy a GPU texture either
    // because it itself was destroyed or because the file was
    // reloaded or target memory was changed.

    if (Hgi * hgi = _GetHgi()) {
        hgi->DestroySampler(&_sampler);
    }
}

///////////////////////////////////////////////////////////////////////////////
// Field sampler

LoFiFieldSamplerObject::LoFiFieldSamplerObject(
    LoFiFieldTextureObject const &texture,
    HdSamplerParameters const &samplerParameters,
    const bool createBindlessHandle,
    LoFiSamplerObjectRegistry * const samplerObjectRegistry)
  : LoFiSamplerObject(samplerObjectRegistry)
  , _sampler(
      _GenSampler(
          samplerObjectRegistry,
          samplerParameters,
          texture.IsValid()))
  , _glTextureSamplerHandle(
      _GenGLTextureSamplerHandle(
          texture.GetTexture(),
          _sampler,
          createBindlessHandle && texture.IsValid()))
{
}

LoFiFieldSamplerObject::~LoFiFieldSamplerObject()
{
    // See above comment about destroying _glTextureSamplerHandle
    if (Hgi * hgi = _GetHgi()) {
        hgi->DestroySampler(&_sampler);
    }
}

///////////////////////////////////////////////////////////////////////////////
// Ptex sampler

// Wrap modes such as repeat or mirror do not make sense for ptex, so set them
// to clamp.
static
HdSamplerParameters PTEX_SAMPLER_PARAMETERS{
    HdWrapClamp,
    HdWrapClamp,
    HdWrapClamp,
    HdMinFilterLinear,
    HdMagFilterLinear};

LoFiPtexSamplerObject::LoFiPtexSamplerObject(
    LoFiPtexTextureObject const &ptexTexture,
    // samplerParameters are ignored are ptex
    HdSamplerParameters const &samplerParameters,
    const bool createBindlessHandle,
    LoFiSamplerObjectRegistry * const samplerObjectRegistry)
  : LoFiSamplerObject(samplerObjectRegistry)
  , _texelsSampler(
      _GenSampler(
          samplerObjectRegistry,
          PTEX_SAMPLER_PARAMETERS,
          ptexTexture.IsValid()))
  , _texelsGLTextureHandle(
      _GenGLTextureSamplerHandle(
          ptexTexture.GetTexelTexture(),
          _texelsSampler,
          createBindlessHandle && ptexTexture.IsValid()))
  , _layoutGLTextureHandle(
      _GenGlTextureHandle(
          ptexTexture.GetLayoutTexture(),
          createBindlessHandle && ptexTexture.IsValid()))
{
}

// See above comment about destroying bindless texture handles
LoFiPtexSamplerObject::~LoFiPtexSamplerObject() = default;

///////////////////////////////////////////////////////////////////////////////
// Udim sampler

// Wrap modes such as repeat or mirror do not make sense for udim, so set them
// to clamp.
//
// Mipmaps would make sense for udim up to a certain level, but
// GlfUdimTexture produces broken mipmaps, so forcing HdMinFilterLinear.
// The old texture system apparently never exercised the case of using
// mipmaps for a udim.
static
HdSamplerParameters UDIM_SAMPLER_PARAMETERS{
    HdWrapClamp,
    HdWrapClamp,
    HdWrapClamp,
    HdMinFilterLinearMipmapLinear,
    HdMagFilterLinear};

LoFiUdimSamplerObject::LoFiUdimSamplerObject(
    LoFiUdimTextureObject const &udimTexture,
    HdSamplerParameters const &samplerParameters,
    const bool createBindlessHandle,
    LoFiSamplerObjectRegistry * const samplerObjectRegistry)
  : LoFiSamplerObject(samplerObjectRegistry)
  , _texelsSampler(
      _GenSampler(
          samplerObjectRegistry,
          UDIM_SAMPLER_PARAMETERS,
          udimTexture.IsValid()))
  , _texelsGLTextureHandle(
      _GenGLTextureSamplerHandle(
          udimTexture.GetTexelTexture(),
          _texelsSampler,
          createBindlessHandle && udimTexture.IsValid()))
  , _layoutGLTextureHandle(
      _GenGlTextureHandle(
          udimTexture.GetLayoutTexture(),
          createBindlessHandle && udimTexture.IsValid()))
{
}

LoFiUdimSamplerObject::~LoFiUdimSamplerObject()
{
    // See above comment about destroying bindless texture handles

    if (Hgi * hgi = _GetHgi()) {
        hgi->DestroySampler(&_texelsSampler);
    }
}

PXR_NAMESPACE_CLOSE_SCOPE
