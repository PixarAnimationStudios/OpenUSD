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

#include "pxr/usd/usdHydra/shader.h"
#include "pxr/usd/usdHydra/uvTexture.h"
#include "pxr/usd/usdHydra/primvar.h"
#include "pxr/usd/usdHydra/tokens.h"

PXR_NAMESPACE_OPEN_SCOPE

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
    if (filePath.IsEmpty())
        filePath = TfToken(asset.GetAssetPath());

    const bool isPtex = GlfIsSupportedPtexTexture(filePath);

    TfToken wrapS = UsdHydraTokens->repeat;
    TfToken wrapT = UsdHydraTokens->repeat;
    TfToken minFilter = UsdHydraTokens->linear;
    TfToken magFilter = UsdHydraTokens->linear;
    float memoryLimit = 0.0f;

    // Mode overrides for UsdHydraTexture
    UsdShadeShader shader(usdPrim);
    if (shader) {
        UsdAttribute attr = UsdHydraTexture(shader).GetTextureMemoryAttr();
        if (attr) attr.Get(&memoryLimit);
        if (!isPtex) {
            UsdHydraUvTexture uvt(shader);
            attr = uvt.GetWrapSAttr();
            if (attr) attr.Get(&wrapS);
            attr = uvt.GetWrapTAttr();
            if (attr) attr.Get(&wrapT);
            attr = uvt.GetMinFilterAttr();
            if (attr) attr.Get(&minFilter);
            attr = uvt.GetMagFilterAttr();
            if (attr) attr.Get(&magFilter);
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
    texture->AddMemoryRequest(memoryLimit);
    HdWrap wrapShd = (wrapS == UsdHydraTokens->clamp) ? HdWrapClamp
                 : (wrapS == UsdHydraTokens->repeat) ? HdWrapRepeat
                 : HdWrapBlack; 
    HdWrap wrapThd = (wrapT == UsdHydraTokens->clamp) ? HdWrapClamp
                 : (wrapT == UsdHydraTokens->repeat) ? HdWrapRepeat
                 : HdWrapBlack; 
    HdMagFilter magFilterHd = 
                 (magFilter == UsdHydraTokens->nearest) ? HdMagFilterNearest
                 : HdMagFilterLinear; 
    HdMinFilter minFilterHd = 
                 (minFilter == UsdHydraTokens->nearest) ? HdMinFilterNearest
                 : (minFilter == UsdHydraTokens->nearestMipmapNearest) 
                                ? HdMinFilterNearestMipmapNearest
                 : (minFilter == UsdHydraTokens->nearestMipmapLinear) 
                                ? HdMinFilterNearestMipmapLinear
                 : (minFilter == UsdHydraTokens->linearMipmapNearest) 
                                ? HdMinFilterLinearMipmapNearest
                 : (minFilter == UsdHydraTokens->linearMipmapLinear) 
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
