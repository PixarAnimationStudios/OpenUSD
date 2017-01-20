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
#include "pxr/usdImaging/usdImaging/shaderAdapter.h"

#include "pxr/usdImaging/usdImaging/delegate.h"
#include "pxr/usdImaging/usdImaging/tokens.h"

#include "pxr/imaging/glf/glslfx.h"
#include "pxr/imaging/glf/ptexTexture.h"

#include "pxr/imaging/hd/tokens.h"

#include "pxr/usd/usdShade/connectableAPI.h"

#include "pxr/usd/usdHydra/shader.h"
#include "pxr/usd/usdHydra/uvTexture.h"
#include "pxr/usd/usdHydra/primvar.h"
#include "pxr/usd/usdHydra/tokens.h"

UsdImagingShaderAdapter::UsdImagingShaderAdapter(UsdImagingDelegate* delegate)
    : _delegate(delegate)
{
}

bool
UsdImagingShaderAdapter::GetSurfaceShaderIsTimeVarying(SdfPath const& usdPath) const
{
    if (UsdPrim p = _delegate->_GetPrim(usdPath)) {
        const std::vector<UsdAttribute> &attrs = p.GetAttributes();
        TF_FOR_ALL(attrIter, attrs) {
            const UsdAttribute& attr = *attrIter;
            if (attr.GetNumTimeSamples()>1){
                return true;
            }
        }
    }
    return false;
}

std::string
UsdImagingShaderAdapter::GetSurfaceShaderSource(SdfPath const &usdPath) const
{
    std::string const EMPTY;
    if (!TF_VERIFY(usdPath != SdfPath()))
        return EMPTY;

    UsdPrim prim = _delegate->_GetPrim(usdPath);

    if (!prim) {
        return EMPTY;
    }

    UsdAttribute srcAttr;
    if (UsdShadeShader shader = UsdShadeShader(prim)) {
        srcAttr = UsdHydraShader(shader).GetFilenameAttr();
        TF_DEBUG(USDIMAGING_SHADERS).Msg("Loading UsdShade shader: %s\n",
                    srcAttr.GetPath().GetText());
    } else {
        // ------------------------------------------------------------------ //
        // Deprecated
        // ------------------------------------------------------------------ //
        srcAttr = prim.GetAttribute(UsdImagingTokens->infoSource);
        if (!srcAttr) {
            TF_DEBUG(USDIMAGING_SHADERS).Msg("No shader source attribute: %s\n",
                    prim.GetPath().GetText());
            return EMPTY;
        }
        TF_DEBUG(USDIMAGING_SHADERS).Msg("Loading deprecated shader: %s\n",
                    srcAttr.GetPath().GetText());
        // ------------------------------------------------------------------ //
    }

    // PERFORMANCE: We're opening the file on every request currently, but we'd
    // like to share this in some sort of registry in the future.
    SdfAssetPath asset;
    std::string filePath;
    if (!srcAttr.Get(&asset))
        return EMPTY;

    filePath = asset.GetResolvedPath();

    // Fallback to the literal path if it couldn't be resolved.
    if (filePath.empty())
        filePath = asset.GetAssetPath();

    GlfGLSLFX gfx(filePath);

    if (!gfx.IsValid())
        return EMPTY;

    return gfx.GetSurfaceSource();
}

TfTokenVector
UsdImagingShaderAdapter::GetSurfaceShaderParamNames(SdfPath const &usdPath) const
{
    TfTokenVector names;
    if (!TF_VERIFY(usdPath != SdfPath()))
        return names;

    UsdPrim prim = _delegate->_GetPrim(usdPath);
    if (!prim)
        return names;

    if (UsdShadeShader shader = UsdShadeShader(prim)) {
        TF_DEBUG(USDIMAGING_SHADERS).Msg("Parameters found:\n");
        std::vector<UsdShadeParameter> params = shader.GetParameters();
        names.reserve(params.size());
        for (UsdShadeParameter const& param : params) {
            TF_DEBUG(USDIMAGING_SHADERS).Msg("\t - %s\n",
                    param.GetAttr().GetName().GetText());
            names.push_back(param.GetAttr().GetName());
        }
    } else {
        // ------------------------------------------------------------------ //
        // Deprecated
        // ------------------------------------------------------------------ //
        
        std::vector<UsdProperty> const& props = prim.GetProperties();
        TF_DEBUG(USDIMAGING_SHADERS).Msg("Parameters found:\n");
        TF_FOR_ALL(propIt, props) {
            if (UsdAttribute attr = propIt->As<UsdAttribute>()) {
                if (!attr.GetPath().IsNamespacedPropertyPath()) {
                    TF_DEBUG(USDIMAGING_SHADERS).Msg("\t - %s\n",
                        attr.GetName().GetText());
                    names.push_back(attr.GetName());
                }
            }
        }
        // ------------------------------------------------------------------ //
    }

    return names;
}

VtValue
UsdImagingShaderAdapter::GetSurfaceShaderParamValue(SdfPath const &usdPath, 
                                               TfToken const &paramName) const
{
    if (!TF_VERIFY(usdPath != SdfPath()))
        return VtValue();

    UsdPrim prim = _delegate->_GetPrim(usdPath);
    if (!TF_VERIFY(prim)) {
        // XXX: hydra crashes with empty vt values, should fix
        VtFloatArray dummy;
        dummy.resize(1);
        return VtValue(dummy);
    }

    VtValue value;
    UsdAttribute attr = prim.GetAttribute(paramName);
    if (!TF_VERIFY(attr)) {
        // XXX: hydra crashes with empty vt values, should fix
        VtFloatArray dummy;
        dummy.resize(1);
        return VtValue(dummy);
    }

    // Reading the value may fail, should we warn here when it does?
    attr.Get(&value, _delegate->GetTime());
    return value;
}

HdShaderParamVector
UsdImagingShaderAdapter::GetSurfaceShaderParams(SdfPath const &usdPath) const
{
    HdShaderParamVector params;

    if (!TF_VERIFY(usdPath != SdfPath()))
        return params;

    UsdPrim prim = _delegate->_GetPrim(usdPath);
    if (!prim)
        return params;

    std::vector<UsdProperty> const& props = prim.GetProperties();
    TF_FOR_ALL(propIt, props) {
        if (UsdAttribute attr = propIt->As<UsdAttribute>()) {
            if (attr.GetPath().IsNamespacedPropertyPath())
                continue;

            TF_DEBUG(USDIMAGING_SHADERS).Msg("Parameter found: %s\n",
                    attr.GetPath().GetText());
            
            VtValue fallbackValue;
            SdfPath connection;
            TfTokenVector samplerCoords;
            bool isPtex = false;

            if (!TF_VERIFY(attr.Get(&fallbackValue),
                            "No fallback value for: <%s>\n",
                            attr.GetPath().GetText())) {
                continue;
            }

            if (UsdShadeShader shader = UsdShadeShader(prim)) {
                if (auto usdParam = UsdShadeParameter(attr)) {
                    TF_DEBUG(USDIMAGING_SHADERS).Msg("Parameter: %s\n",
                            usdParam.GetAttr().GetName().GetText());
                    UsdShadeConnectableAPI source;
                    TfToken outputName;
                    UsdShadeAttributeType sourceType;
                    if (usdParam.GetConnectedSource(&source, &outputName,
                                                    &sourceType)) {
                        UsdShadeShader sourceShader(source);
                        if (UsdAttribute attr = sourceShader.GetIdAttr()) {
                            TfToken id;
                            if (attr.Get(&id)) {
                                if (id == UsdHydraTokens->HwUvTexture_1) {
                                    connection = _delegate->GetPathForIndex(
                                        sourceShader.GetPath());
                                    TF_DEBUG(USDIMAGING_SHADERS).Msg(
                                                "\t connected to UV texture\n");
                                    UsdHydraUvTexture tex(sourceShader);
                                    UsdShadeParameter uv(tex.GetUvAttr());
                                    UsdShadeConnectableAPI uvSource;
                                    if (uv.GetConnectedSource(&uvSource, 
                                                              &outputName,
                                                              &sourceType)) {
                                        TfToken map;
                                        UsdShadeShader uvSourceShader(uvSource);
                                        UsdHydraPrimvar pv(uvSourceShader);
                                        if (pv.GetVarnameAttr().Get(&map)) {
                                            samplerCoords.push_back(map);
                                            TF_DEBUG(USDIMAGING_SHADERS).Msg(
                                                    "\t\t sampler: %s\n",
                                                    map.GetText());
                                        }
                                    }
                                } else if (id == UsdHydraTokens->HwPtexTexture_1) {
                                    isPtex = true;
                                    TF_DEBUG(USDIMAGING_SHADERS).Msg(
                                              "\t connected to Ptex texture\n");
                                    connection = _delegate->GetPathForIndex(source.GetPath());
                                    // Ptex doesn't need explicit sampler params
                                } else if (id == UsdHydraTokens->HwPrimvar_1) {
                                    connection = SdfPath("primvar." 
                                                         + source.GetPrim()
                                                                 .GetName()
                                                                 .GetString());
                                    TF_DEBUG(USDIMAGING_SHADERS).Msg(
                                                   "\t connected to Primvar\n");
                                    UsdHydraPrimvar pv(sourceShader);
                                    TfToken name;
                                    if (TF_VERIFY(pv.GetVarnameAttr().Get(&name))) {
                                        samplerCoords.push_back(name);
                                        TF_DEBUG(USDIMAGING_SHADERS).Msg(
                                                "\t - %s\n", name.GetText());
                                    }
                                }
                            }
                        }
                    }
                }
            } else {
                // ---------------------------------------------------------- //
                // Deprecated
                // ---------------------------------------------------------- //
                if (UsdAttribute texAttr = prim.GetAttribute(
                                                TfToken(attr.GetPath().GetName() 
                                                        + ":texture"))) {
                    // XXX: we should use the connection for both texture and
                    // primvars here.
                    connection = _delegate->GetPathForIndex(texAttr.GetPath());
                    TfToken t;
                    SdfAssetPath ap;
                    texAttr.Get(&ap, UsdTimeCode::Default());
                    TfToken resolvedPath(ap.GetResolvedPath());
                    if (resolvedPath.IsEmpty()) {
                        resolvedPath = TfToken(ap.GetAssetPath());
                    }
                    isPtex = GlfPtexTexture::IsPtexTexture(resolvedPath);
                    if (!isPtex) {
                        TF_VERIFY(texAttr.GetMetadata(
                                              UsdImagingTokens->uvPrimvar, &t),
                                "<%s>", texAttr.GetPath().GetText());
                        samplerCoords.push_back(t);
                    }
                } else if (UsdAttribute pvAttr = prim.GetAttribute(
                                                TfToken(attr.GetPath().GetName() 
                                                        + ":primvar"))) {
                    connection = SdfPath("primvar."
                                         + pvAttr.GetName().GetString());
                    TfToken t;
                    pvAttr.Get(&t, UsdTimeCode::Default());
                    TF_DEBUG(USDIMAGING_SHADERS).Msg(
                            "Primvar connection found: %s = %s\n",
                            pvAttr.GetPath().GetText(),
                            t.GetText());
                    samplerCoords.push_back(t);
                }
                // ---------------------------------------------------------- //
            }

            attr.Get(&fallbackValue);
            params.push_back(HdShaderParam(attr.GetName(), 
                                          fallbackValue,
                                          connection,
                                          samplerCoords,
                                          isPtex));
        }
    }

    return params;
}

SdfPathVector
UsdImagingShaderAdapter::GetSurfaceShaderTextures(SdfPath const &usdPath) const
{
    SdfPathVector textureIDs;

    if (!TF_VERIFY(usdPath != SdfPath())) {
        return textureIDs;
    }

    UsdPrim prim = _delegate->_GetPrim(usdPath);
    if (!prim) {
        return textureIDs;
    }

    if (UsdShadeShader shader = UsdShadeShader(prim)) {
        SdfPathVector stack(1, shader.GetPath());
        TfToken t;
        while (!stack.empty()) {
            SdfPath shaderPath = stack.back();
            stack.pop_back();
            shader = UsdShadeShader(prim.GetStage()->GetPrimAtPath(shaderPath));
            TF_DEBUG(USDIMAGING_TEXTURES).Msg(
                    " Looking for connected textures at <%s>\n",
                    shader.GetPath().GetText());

            if (shader.GetIdAttr().Get(&t)
                    && (t == UsdHydraTokens->HwUvTexture_1
                        || t == UsdHydraTokens->HwPtexTexture_1)) {
                TF_DEBUG(USDIMAGING_TEXTURES).Msg(
                    "  found texture: <%s>\n",
                    shader.GetPath().GetText());
                textureIDs.push_back(_delegate->GetPathForIndex(shader.GetPath()));
            }
            for (UsdShadeParameter param : shader.GetParameters()) {
                UsdShadeConnectableAPI source;
                TfToken outputName;
                UsdShadeAttributeType sourceType;
                if (param.GetConnectedSource(&source, &outputName, &sourceType))
                    stack.push_back(source.GetPath());
            }
        }
    } else {
        std::vector<UsdProperty> const& props = prim.GetProperties();
        TF_FOR_ALL(propIt, props) {
            if (UsdAttribute attr = propIt->As<UsdAttribute>()) {
                if (attr.GetPath().IsNamespacedPropertyPath())
                    continue;

                SdfPath connection;
                if (UsdAttribute texAttr = prim.GetAttribute(
                                                TfToken(attr.GetPath().GetName() 
                                                        + ":texture"))) {
                    connection = texAttr.GetPath();
                    textureIDs.push_back(_delegate->GetPathForIndex(connection));

                    TF_DEBUG(USDIMAGING_TEXTURES).Msg(
                            "Texture connection found: %s\n",
                            texAttr.GetPath().GetText());
                }
            }
        }
    }
    
    return textureIDs;
}

