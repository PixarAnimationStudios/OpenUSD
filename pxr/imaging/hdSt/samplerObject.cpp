//
// Copyright 2020 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/imaging/hdSt/samplerObject.h"
#include "pxr/imaging/hdSt/ptexTextureObject.h"
#include "pxr/imaging/hdSt/resourceRegistry.h"
#include "pxr/imaging/hdSt/samplerObjectRegistry.h"
#include "pxr/imaging/hdSt/textureObject.h"
#include "pxr/imaging/hdSt/udimTextureObject.h"
#include "pxr/imaging/hdSt/hgiConversions.h"

#include "pxr/imaging/hgi/hgi.h"

PXR_NAMESPACE_OPEN_SCOPE

///////////////////////////////////////////////////////////////////////////////
// HdStTextureObject

HdStSamplerObject::HdStSamplerObject(
    HdSt_SamplerObjectRegistry * samplerObjectRegistry)
  : _samplerObjectRegistry(samplerObjectRegistry)
{
}

HdStSamplerObject::~HdStSamplerObject() = default;

Hgi *
HdStSamplerObject::_GetHgi() const
{
    if (!TF_VERIFY(_samplerObjectRegistry)) {
        return nullptr;
    }

    HdStResourceRegistry * const registry =
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
    desc.debugName = "HdStSamplerObject";
    desc.magFilter = HdStHgiConversions::GetHgiMagFilter(
        samplerParameters.magFilter);
    HdStHgiConversions::GetHgiMinAndMipFilter(
        samplerParameters.minFilter,
        &desc.minFilter, &desc.mipFilter);
    desc.addressModeU =
        HdStHgiConversions::GetHgiSamplerAddressMode(samplerParameters.wrapS);
    desc.addressModeV =
        HdStHgiConversions::GetHgiSamplerAddressMode(samplerParameters.wrapT);
    desc.addressModeW =
        HdStHgiConversions::GetHgiSamplerAddressMode(samplerParameters.wrapR);
    desc.borderColor =
        HdStHgiConversions::GetHgiBorderColor(samplerParameters.borderColor);
    desc.enableCompare = samplerParameters.enableCompare;
    desc.compareFunction = HdStHgiConversions::GetHgiCompareFunction(
        samplerParameters.compareFunction);
    
    return desc;
}

// Generate sampler
static
HgiSamplerHandle
_GenSampler(HdSt_SamplerObjectRegistry * const samplerObjectRegistry,
            HdSamplerParameters const &samplerParameters)
{
    HdStResourceRegistry * const registry =
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
    HdStUvTextureObject const &texture,
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

HdStUvSamplerObject::HdStUvSamplerObject(
    HdStUvTextureObject const &texture,
    HdSamplerParameters const &samplerParameters,
    HdSt_SamplerObjectRegistry * const samplerObjectRegistry)
  : HdStSamplerObject(samplerObjectRegistry)
  , _sampler(
      _GenSampler(
          samplerObjectRegistry,
          _ResolveUvSamplerParameters(
              texture, samplerParameters)))
{
}

HdStUvSamplerObject::~HdStUvSamplerObject()
{
    if (Hgi * hgi = _GetHgi()) {
        hgi->DestroySampler(&_sampler);
    }
}

///////////////////////////////////////////////////////////////////////////////
// Field sampler

HdStFieldSamplerObject::HdStFieldSamplerObject(
    HdStFieldTextureObject const &texture,
    HdSamplerParameters const &samplerParameters,
    HdSt_SamplerObjectRegistry * const samplerObjectRegistry)
  : HdStSamplerObject(samplerObjectRegistry)
  , _sampler(
      _GenSampler(
          samplerObjectRegistry,
          samplerParameters))
{
}

HdStFieldSamplerObject::~HdStFieldSamplerObject()
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
HdSamplerParameters PTEX_SAMPLER_PARAMETERS(
    HdWrapClamp,
    HdWrapClamp,
    HdWrapClamp,
    HdMinFilterLinear,
    HdMagFilterLinear,
    HdBorderColorTransparentBlack, 
    /*enableCompare*/false, 
    HdCmpFuncNever);

static
HdSamplerParameters LAYOUT_SAMPLER_PARAMETERS(
    HdWrapRepeat,
    HdWrapRepeat,
    HdWrapRepeat,
    HdMinFilterNearest,
    HdMagFilterNearest,
    HdBorderColorTransparentBlack, 
    /*enableCompare*/false, 
    HdCmpFuncNever);

HdStPtexSamplerObject::HdStPtexSamplerObject(
    HdStPtexTextureObject const &ptexTexture,
    // samplerParameters are ignored are ptex
    HdSamplerParameters const &samplerParameters,
    HdSt_SamplerObjectRegistry * const samplerObjectRegistry)
  : HdStSamplerObject(samplerObjectRegistry)
  , _texelsSampler(
      _GenSampler(
          samplerObjectRegistry,
          PTEX_SAMPLER_PARAMETERS))
  , _layoutSampler(
      _GenSampler(
          samplerObjectRegistry,
          LAYOUT_SAMPLER_PARAMETERS))
{
}

HdStPtexSamplerObject::~HdStPtexSamplerObject()
{
    if (Hgi * hgi = _GetHgi()) {
        hgi->DestroySampler(&_texelsSampler);
        hgi->DestroySampler(&_layoutSampler);
    }
}

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
HdSamplerParameters UDIM_SAMPLER_PARAMETERS(
    HdWrapClamp,
    HdWrapClamp,
    HdWrapClamp,
    HdMinFilterLinearMipmapLinear,
    HdMagFilterLinear,
    HdBorderColorTransparentBlack, 
    /*enableCompare*/false, 
    HdCmpFuncNever);

HdStUdimSamplerObject::HdStUdimSamplerObject(
    HdStUdimTextureObject const &udimTexture,
    HdSamplerParameters const &samplerParameters,
    HdSt_SamplerObjectRegistry * const samplerObjectRegistry)
  : HdStSamplerObject(samplerObjectRegistry)
  , _texelsSampler(
      _GenSampler(
          samplerObjectRegistry,
          UDIM_SAMPLER_PARAMETERS))
  , _layoutSampler(
      _GenSampler(
          samplerObjectRegistry,
          LAYOUT_SAMPLER_PARAMETERS))
{
}

HdStUdimSamplerObject::~HdStUdimSamplerObject()
{
    if (Hgi * hgi = _GetHgi()) {
        hgi->DestroySampler(&_texelsSampler);
        hgi->DestroySampler(&_layoutSampler);
    }
}

PXR_NAMESPACE_CLOSE_SCOPE
