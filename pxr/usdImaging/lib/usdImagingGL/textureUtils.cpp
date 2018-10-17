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
#include "pxr/usdImaging/usdImaging/textureUtils.h"
#include "pxr/usdImaging/usdImaging/tokens.h"

#include "pxr/imaging/glf/glslfx.h"
#include "pxr/imaging/glf/ptexTexture.h"
#include "pxr/imaging/glf/udimTexture.h"
#include "pxr/imaging/glf/textureHandle.h"
#include "pxr/imaging/glf/textureRegistry.h"
#include "pxr/imaging/glf/contextCaps.h"

#include "pxr/imaging/hd/material.h"
#include "pxr/imaging/hd/tokens.h"
#include "pxr/imaging/hdSt/textureResource.h"

#include "pxr/base/tf/fileUtils.h"
#include "pxr/base/vt/value.h"

#include "pxr/usd/sdr/registry.h"
#include "pxr/usd/sdr/shaderNode.h"
#include "pxr/usd/sdr/shaderProperty.h"
#include "pxr/usd/usdHydra/tokens.h"
#include "pxr/usd/usdShade/shader.h"

PXR_NAMESPACE_OPEN_SCOPE

namespace {

HdWrap 
_GetWrap(UsdPrim const &usdPrim, 
    HdTextureType textureType, 
    const TfToken &wrapAttr)
{
    // A Udim always uses black wrap
    if (textureType == HdTextureType::Udim) {
        return HdWrapBlack;
    }

    // The fallback, when the prim has no opinion is to use the metadata on
    // the texture.
    TfToken usdWrap = UsdHydraTokens->useMetadata;
    UsdShadeShader shader(usdPrim);

    if (shader) {
        if (auto wrapInput = shader.GetInput(wrapAttr)) {
            wrapInput.Get(&usdWrap);
        } else {
            // Get the default value from the shader registry if the input is
            // not authored on the shader prim.
            TfToken shaderId;
            shader.GetShaderId(&shaderId);
            if (!shaderId.IsEmpty()) {
                auto &shaderReg = SdrRegistry::GetInstance();
                if (SdrShaderNodeConstPtr sdrNode = 
                    shaderReg.GetShaderNodeByIdentifierAndType(shaderId, 
                                GlfGLSLFXTokens->glslfx)) {
                    if (const auto &sdrInput = 
                            sdrNode->GetShaderInput(wrapAttr)) {
                        VtValue wrapVal = sdrInput->GetDefaultValue();
                        if (wrapVal.IsHolding<TfToken>()) {
                            usdWrap = wrapVal.UncheckedGet<TfToken>();
                        }
                    }
                }
            }
        }
    }

    HdWrap hdWrap;
    if (usdWrap == UsdHydraTokens->clamp) {
        hdWrap = HdWrapClamp;
    } else if (usdWrap == UsdHydraTokens->repeat) {
        hdWrap = HdWrapRepeat;
    } else if (usdWrap == UsdHydraTokens->mirror) {
        hdWrap = HdWrapMirror;
    } else if (usdWrap == UsdHydraTokens->black) {
        hdWrap = HdWrapBlack;
    } else {
        if (usdWrap != UsdHydraTokens->useMetadata) {
            TF_WARN("Unknown wrap mode on prim %s: %s",
                    usdPrim.GetPath().GetText(),
                    usdWrap.GetText());
        }

        hdWrap = HdWrapUseMetadata;

        // For legacy reasons, there is two different behaviors for
        // useMetadata.  The deprecated HwUvTexture_1 shader nodes
        // use the legacy behavior, while new nodes should use the new
        // behavior.
        TfToken id;
        UsdAttribute attr = shader.GetIdAttr();
        if (attr.Get(&id)) {
            if (id == UsdHydraTokens->HwUvTexture_1) {
                hdWrap = HdWrapLegacy;
            }
        }
    }

    return hdWrap;
}

HdWrap
_GetWrapS(UsdPrim const &usdPrim, HdTextureType textureType)
{
    return _GetWrap(usdPrim, textureType, UsdHydraTokens->wrapS);
}

HdWrap
_GetWrapT(UsdPrim const &usdPrim, HdTextureType textureType)
{
    return _GetWrap(usdPrim, textureType, UsdHydraTokens->wrapT);
}

HdMinFilter
_GetMinFilter(UsdPrim const &usdPrim)
{
    // XXX: This default value should come from the registry
    TfToken minFilter("linear");
    UsdShadeShader shader(usdPrim);
    if (shader) {
        UsdAttribute attr = shader.GetInput(UsdHydraTokens->minFilter);
        if (attr) attr.Get(&minFilter);
    }
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
    return minFilterHd;
}

HdMagFilter
_GetMagFilter(UsdPrim const &usdPrim)
{
    // XXX: This default value should come from the registry
    TfToken magFilter("linear");
    UsdShadeShader shader(usdPrim);
    if (shader) {
        UsdAttribute attr = shader.GetInput(UsdHydraTokens->magFilter);
        if (attr) attr.Get(&magFilter);
    }
    HdMagFilter magFilterHd = 
                 (magFilter == UsdHydraTokens->nearest) ? HdMagFilterNearest
                 : HdMagFilterLinear; 
    return magFilterHd;
}

float
_GetMemoryLimit(UsdPrim const &usdPrim)
{
    // XXX: This default value should come from the registry
    float memoryLimit = 0.0f;
    UsdShadeShader shader(usdPrim);
    if (shader) {
        UsdAttribute attr = shader.GetInput(UsdHydraTokens->textureMemory);
        if (attr) attr.Get(&memoryLimit);
    }
    return memoryLimit;
}

GlfImage::ImageOriginLocation
UsdImagingGL_ComputeTextureOrigin(UsdPrim const& usdPrim)
{
    // XXX : This is transitional code. Currently, only textures read
    //       via UsdUVTexture have the origin at the lower left.
    // Extract the id of the node and if it is a UsdUVTexture
    // then we need to use the new coordinate system with (0,0)
    // in the bottom left.
    GlfImage::ImageOriginLocation origin =
        GlfImage::ImageOriginLocation::OriginUpperLeft;
    TfToken id;
    UsdAttribute attr1 = UsdShadeShader(usdPrim).GetIdAttr();
    attr1.Get(&id);
    if (id == UsdImagingTokens->UsdUVTexture) {
        origin = GlfImage::ImageOriginLocation::OriginLowerLeft;
    }

    return origin;
}

class UdimTextureFactory : public GlfTextureFactoryBase {
public:
    UdimTextureFactory(
        const SdfLayerHandle& layerHandle)
        : _layerHandle(layerHandle) { }

    virtual GlfTextureRefPtr New(
        TfToken const& texturePath,
        GlfImage::ImageOriginLocation originLocation =
        GlfImage::OriginUpperLeft) const override {
        const GlfContextCaps& caps = GlfContextCaps::GetInstance();
        return GlfUdimTexture::New(
            texturePath, originLocation, UsdImaging_GetUdimTiles(
                texturePath, caps.maxArrayTextureLayers, _layerHandle));
    }

    virtual GlfTextureRefPtr New(
        TfTokenVector const& texturePaths,
        GlfImage::ImageOriginLocation originLocation =
        GlfImage::OriginUpperLeft) const override {
        return nullptr;
    }
private:
    const SdfLayerHandle& _layerHandle;
};

// We need to find the first layer that changes the value
// of the parameter and anchor relative paths to that.
SdfLayerHandle 
_FindLayerHandle(const UsdAttribute& attr, const UsdTimeCode& time) {
    for (const auto& spec: attr.GetPropertyStack(time)) {
        if (spec->HasDefaultValue() ||
            spec->GetLayer()->GetNumTimeSamplesForPath(
                spec->GetPath()) > 0) {
            return spec->GetLayer();
        }
    }
    return {};
}

UsdAttribute 
_GetTextureResourceAttr(UsdPrim const &shaderPrim, 
                        SdfPath const &fileInputPath)
{
    UsdAttribute attr = shaderPrim.GetAttribute(fileInputPath.GetNameToken());
    if (!attr) {
        return attr;
    }

    UsdShadeInput attrInput(attr);
    if (!attrInput) {
        return attr;
    }

    // If the texture 'file' input is connected to an interface input on a 
    // node-graph, then read from the connection source instead.
    UsdShadeConnectableAPI source;
    TfToken sourceName;
    UsdShadeAttributeType sourceType;
    if (attrInput.GetConnectedSource(&source, &sourceName, &sourceType) && 
        sourceType == UsdShadeAttributeType::Input && 
        source.IsNodeGraph()) {
        if (UsdShadeInput sourceInput = source.GetInput(sourceName)) {
            return sourceInput.GetAttr();
        }
    }

    return attr;
}

}

HdTextureResource::ID
UsdImagingGL_GetTextureResourceID(UsdPrim const& usdPrim,
                                  SdfPath const& usdPath,
                                  UsdTimeCode time,
                                  size_t salt)
{
    if (!TF_VERIFY(usdPrim)) {
        return HdTextureResource::ID(-1);
    }
    if (!TF_VERIFY(usdPath != SdfPath())) {
        return HdTextureResource::ID(-1);
    }

    // If the texture name attribute doesn't exist, it might be badly specified
    // in scene data.
    UsdAttribute attr = _GetTextureResourceAttr(usdPrim, usdPath);

    SdfAssetPath asset;
    if (!attr || !attr.Get(&asset, time)) {
        TF_WARN("Unable to find texture attribute <%s> in scene data",
                usdPath.GetText());
        return HdTextureResource::ID(-1);
    }

    HdTextureType textureType = HdTextureType::Uv;
    TfToken filePath = TfToken(asset.GetResolvedPath());

    if (!filePath.IsEmpty()) {
        // If the resolved path contains a correct path, then we are 
        // dealing with a ptex or uv textures.
        if (GlfIsSupportedPtexTexture(filePath)) {
            textureType = HdTextureType::Ptex;
        } else {
            textureType = HdTextureType::Uv;
        }
    } else {
        // If the path couldn't be resolved, then it might be a Udim as they 
        // contain special characters in the path to identify them <Udim>.
        // Another option is that the path is just wrong and it can not be
        // resolved.
        filePath = TfToken(asset.GetAssetPath());
        if (GlfIsSupportedUdimTexture(filePath)) {
            const GlfContextCaps& caps = GlfContextCaps::GetInstance();
            if (!UsdImaging_UdimTilesExist(filePath, caps.maxArrayTextureLayers,
                _FindLayerHandle(attr, time))) {
                TF_WARN("Unable to find Texture '%s' with path '%s'. Fallback "
                        "textures are not supported for udim",
                        filePath.GetText(), usdPath.GetText());
                return HdTextureResource::ID(-1);
            }
            if (!caps.arrayTexturesEnabled) {
                TF_WARN("OpenGL context does not support array textures, "
                        "skipping UDIM Texture %s with path %s.",
                        filePath.GetText(), usdPath.GetText());
                return HdTextureResource::ID(-1);
            }
            textureType = HdTextureType::Udim;
        } else if (GlfIsSupportedPtexTexture(filePath)) {
            TF_WARN("Unable to find Texture '%s' with path '%s'. Fallback "
                    "textures are not supported for ptex",
                    filePath.GetText(), usdPath.GetText());
            return HdTextureResource::ID(-1);
        } else {
            TF_WARN("Unable to find Texture '%s' with path '%s'. A black "
                    "texture will be substituted in its place.",
                    filePath.GetText(), usdPath.GetText());
            return HdTextureResource::ID(-1);
        }
    }

    GlfImage::ImageOriginLocation origin =
            UsdImagingGL_ComputeTextureOrigin(usdPrim);

    // Hash on the texture filename.
    size_t hash = asset.GetHash();

    // Hash in wrapping and filtering metadata.
    HdWrap wrapS = _GetWrapS(usdPrim, textureType);
    HdWrap wrapT = _GetWrapT(usdPrim, textureType);
    HdMinFilter minFilter = _GetMinFilter(usdPrim);
    HdMagFilter magFilter = _GetMagFilter(usdPrim);
    float memoryLimit = _GetMemoryLimit(usdPrim);

    boost::hash_combine(hash, origin);
    boost::hash_combine(hash, wrapS);
    boost::hash_combine(hash, wrapT);
    boost::hash_combine(hash, minFilter);
    boost::hash_combine(hash, magFilter);
    boost::hash_combine(hash, memoryLimit);

    // Salt the result to prevent collisions in non-shared imaging.
    // Note that the salt is ignored for fallback texture hashes above.
    boost::hash_combine(hash, salt);

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

    UsdAttribute attr = _GetTextureResourceAttr(usdPrim, usdPath);
    SdfAssetPath asset;
    if (!TF_VERIFY(attr) || !TF_VERIFY(attr.Get(&asset, time))) {
        return HdTextureResourceSharedPtr();
    }

    HdTextureType textureType = HdTextureType::Uv;

    TfToken filePath = TfToken(asset.GetResolvedPath());
    // If the path can't be resolved, it's either an UDIM texture
    // or the texture doesn't exists and we can to exit early.
    if (filePath.IsEmpty()) {
        filePath = TfToken(asset.GetAssetPath());
        if (GlfIsSupportedUdimTexture(filePath)) {
            textureType = HdTextureType::Udim;
        } else {
            TF_DEBUG(USDIMAGING_TEXTURES).Msg(
                "File does not exist, returning nullptr");
            TF_WARN("Unable to find Texture '%s' with path '%s'.",
                    filePath.GetText(), usdPath.GetText());
            return {};
        }
    } else {
        if (GlfIsSupportedPtexTexture(filePath)) {
            textureType = HdTextureType::Ptex;
        }
    }

    GlfImage::ImageOriginLocation origin =
            UsdImagingGL_ComputeTextureOrigin(usdPrim);

    HdWrap wrapS = _GetWrapS(usdPrim, textureType);
    HdWrap wrapT = _GetWrapT(usdPrim, textureType);
    HdMinFilter minFilter = _GetMinFilter(usdPrim);
    HdMagFilter magFilter = _GetMagFilter(usdPrim);
    float memoryLimit = _GetMemoryLimit(usdPrim);

    TF_DEBUG(USDIMAGING_TEXTURES).Msg(
            "Loading texture: id(%s), type(%s)\n",
            usdPath.GetText(),
            textureType == HdTextureType::Uv ? "Uv" :
            textureType == HdTextureType::Ptex ? "Ptex" : "Udim");
 
    HdTextureResourceSharedPtr texResource;
    TfStopwatch timer;
    timer.Start();
    // Udim's can't be loaded through like other textures, because
    // we can't select the right factory based on the file type.
    // We also need to pass the layer context to the factory,
    // so each file gets resolved properly.
    GlfTextureHandleRefPtr texture;
    if (textureType == HdTextureType::Udim) {
        UdimTextureFactory factory(_FindLayerHandle(attr, time));
        texture = GlfTextureRegistry::GetInstance().GetTextureHandle(
            filePath, origin, &factory);
    } else {
        texture = GlfTextureRegistry::GetInstance().GetTextureHandle(
            filePath, origin);
    }

    texResource = HdTextureResourceSharedPtr(
        new HdStSimpleTextureResource(texture, textureType, wrapS, wrapT,
                                      minFilter, magFilter, memoryLimit));
    timer.Stop();

    TF_DEBUG(USDIMAGING_TEXTURES).Msg("    Load time: %.3f s\n", 
                                     timer.GetSeconds());

    return texResource;
}

PXR_NAMESPACE_CLOSE_SCOPE
