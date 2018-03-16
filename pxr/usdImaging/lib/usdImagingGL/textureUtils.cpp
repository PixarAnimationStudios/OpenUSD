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
#include "pxr/usdImaging/usdImagingGL/textureUtils.h"

#include "pxr/usdImaging/usdImaging/debugCodes.h"
#include "pxr/usdImaging/usdImaging/delegate.h"
#include "pxr/usdImaging/usdImaging/tokens.h"

#include "pxr/imaging/glf/glslfx.h"
#include "pxr/imaging/glf/ptexTexture.h"
#include "pxr/imaging/glf/textureHandle.h"
#include "pxr/imaging/glf/textureRegistry.h"

#include "pxr/imaging/hd/material.h"
#include "pxr/imaging/hd/tokens.h"
#include "pxr/imaging/hdSt/textureResource.h"

#include "pxr/base/tf/fileUtils.h"

#include "pxr/usd/usdShade/shader.h"

PXR_NAMESPACE_OPEN_SCOPE

TF_DEFINE_PRIVATE_TOKENS(
    _tokens,
    (wrapS)
    (wrapT)
    (minFilter)
    (magFilter)
    (textureMemory)
    (clamp)
    (repeat)
    (nearest)
    (nearestMipmapNearest)
    (nearestMipmapLinear)
    (linearMipmapNearest)
    (linearMipmapLinear)
);

HdTextureResource::ID
UsdImagingGL_GetTextureResourceID(UsdPrim const& usdPrim,
                                  SdfPath const& usdPath,
                                  UsdTimeCode time,
                                  size_t salt)
{
    // Compute the hash, but we need to validate that the texture exists
    // (in case we need to return a fallback texture).
    size_t hash = usdPath.GetHash();

    // Salt the result to prevent collisions in non-shared imaging.
    // Note that this salt is ignored if we end up using a fallback
    // texture below.
    boost::hash_combine(hash, salt);

    if (!usdPrim || usdPath == SdfPath())
        return HdTextureResource::ID(hash);

    UsdAttribute attr = usdPrim.GetAttribute(usdPath.GetNameToken());
    SdfAssetPath asset;
    if (!attr || !attr.Get(&asset, time))
        return HdTextureResource::ID(hash);

    TfToken filePath = TfToken(asset.GetResolvedPath());
    // Fallback to the literal path if it couldn't be resolved.
    if (filePath.IsEmpty())
        filePath = TfToken(asset.GetAssetPath());

    const bool isPtex = GlfIsSupportedPtexTexture(filePath);

    if (!TfPathExists(filePath)) {
        if (isPtex) {
            TF_WARN("Unable to find Texture '%s' with path '%s'. Fallback " 
                    "textures are not supported for ptex", 
                    filePath.GetText(), usdPath.GetText());
            return HdTextureResource::ComputeFallbackPtexHash(); 
        } else {
            TF_WARN("Unable to find Texture '%s' with path '%s'. A black " 
                    "texture will be substituted in its place.", 
                    filePath.GetText(), usdPath.GetText());
            return HdTextureResource::ComputeFallbackUVHash();
        }
    }

    return HdTextureResource::ID(hash);
}

HdTextureResourceSharedPtr
UsdImagingGL_GetTextureResource(UsdPrim const& usdPrim,
                                SdfPath const& usdPath,
                                UsdTimeCode time)
{
    if (!TF_VERIFY(usdPrim))
        return HdTextureResourceSharedPtr();
    if (!TF_VERIFY(usdPath != SdfPath()))
        return HdTextureResourceSharedPtr();

    UsdAttribute attr = usdPrim.GetAttribute(usdPath.GetNameToken());
    SdfAssetPath asset;
    if (!TF_VERIFY(attr) || !TF_VERIFY(attr.Get(&asset, time))) {
        return HdTextureResourceSharedPtr();
    }

    TfToken filePath = TfToken(asset.GetResolvedPath());

    // Fallback to the literal path if it couldn't be resolved.
    if (filePath.IsEmpty()) {
        filePath = TfToken(asset.GetAssetPath());
    }

    const bool isPtex = GlfIsSupportedPtexTexture(filePath);

    // XXX: This default values should come from the registry
    TfToken outWrapS("repeat");
    TfToken outWrapT("repeat");
    TfToken outMinFilter("linear");
    TfToken outMagFilter("linear");
    float outMemoryLimit = 0.0f;

    // Extract metadata abot the texture node
    UsdShadeShader shader(usdPrim);
    if (shader) {
        UsdAttribute attr = shader.GetInput(_tokens->textureMemory);
        if (attr) attr.Get(&outMemoryLimit);

        if (!isPtex) {
            attr = shader.GetInput(_tokens->wrapS);
            if (attr) attr.Get(&outWrapS);

            attr = shader.GetInput(_tokens->wrapT);
            if (attr) attr.Get(&outWrapT);

            attr = shader.GetInput(_tokens->minFilter);
            if (attr) attr.Get(&outMinFilter);

            attr = shader.GetInput(_tokens->magFilter);
            if (attr) attr.Get(&outMagFilter);
        }
    }

    TF_DEBUG(USDIMAGING_TEXTURES).Msg(
            "Loading texture: id(%s), isPtex(%s)\n",
            usdPath.GetText(),
            isPtex ? "true" : "false");
 
    if (!TfPathExists(filePath)) {
        TF_DEBUG(USDIMAGING_TEXTURES).Msg(
                "File does not exist, returning nullptr");
        TF_WARN("Unable to find Texture '%s' with path '%s'.", 
            filePath.GetText(), usdPath.GetText());
        return HdTextureResourceSharedPtr();
    }

    HdTextureResourceSharedPtr texResource;
    TfStopwatch timer;
    timer.Start();
    GlfTextureHandleRefPtr texture =
        GlfTextureRegistry::GetInstance().GetTextureHandle(filePath);
    texture->AddMemoryRequest(outMemoryLimit);

    HdWrap wrapShd = (outWrapS == _tokens->clamp) ? HdWrapClamp
                 : (outWrapS == _tokens->repeat) ? HdWrapRepeat
                 : HdWrapBlack; 
    HdWrap wrapThd = (outWrapT == _tokens->clamp) ? HdWrapClamp
                 : (outWrapT == _tokens->repeat) ? HdWrapRepeat
                 : HdWrapBlack; 
    HdMagFilter magFilterHd = 
                 (outMagFilter == _tokens->nearest) ? HdMagFilterNearest
                 : HdMagFilterLinear; 
    HdMinFilter minFilterHd = 
                 (outMinFilter == _tokens->nearest) ? HdMinFilterNearest
                 : (outMinFilter == _tokens->nearestMipmapNearest) 
                                ? HdMinFilterNearestMipmapNearest
                 : (outMinFilter == _tokens->nearestMipmapLinear) 
                                ? HdMinFilterNearestMipmapLinear
                 : (outMinFilter == _tokens->linearMipmapNearest) 
                                ? HdMinFilterLinearMipmapNearest
                 : (outMinFilter == _tokens->linearMipmapLinear) 
                                ? HdMinFilterLinearMipmapLinear
                 : HdMinFilterLinear; 

    texResource = HdTextureResourceSharedPtr(
        new HdStSimpleTextureResource(texture, isPtex, wrapShd, wrapThd,
                                      minFilterHd, magFilterHd));
    timer.Stop();

    TF_DEBUG(USDIMAGING_TEXTURES).Msg("    Load time: %.3f s\n", 
                                     timer.GetSeconds());

    return texResource;
}

PXR_NAMESPACE_CLOSE_SCOPE
