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
#include "pxr/usdImaging/usdImaging/hydraMaterialAdapter.h"

#include "pxr/usdImaging/usdImaging/debugCodes.h"
#include "pxr/usdImaging/usdImaging/delegate.h"
#include "pxr/usdImaging/usdImaging/tokens.h"

#include "pxr/imaging/glf/glslfx.h"
#include "pxr/imaging/glf/ptexTexture.h"

#include "pxr/imaging/hd/material.h"
#include "pxr/imaging/hd/tokens.h"

#include "pxr/usd/usdShade/connectableAPI.h"

#include "pxr/usd/usdHydra/shader.h"
#include "pxr/usd/usdHydra/uvTexture.h"
#include "pxr/usd/usdHydra/primvar.h"
#include "pxr/usd/usdHydra/tokens.h"

PXR_NAMESPACE_OPEN_SCOPE

TF_DEFINE_PRIVATE_TOKENS(
    _tokens,
    (surfaceShader)
    (displacementShader)
);

TF_REGISTRY_FUNCTION(TfType)
{
    typedef UsdImagingHydraMaterialAdapter Adapter;
    TfType t = TfType::Define<Adapter, TfType::Bases<Adapter::BaseAdapter> >();
    t.SetFactory< UsdImagingPrimAdapterFactory<Adapter> >();
}

UsdImagingHydraMaterialAdapter::~UsdImagingHydraMaterialAdapter()
{
}

bool
UsdImagingHydraMaterialAdapter::IsSupported(UsdImagingIndexProxy const* index) const
{
    return index->IsSprimTypeSupported(HdPrimTypeTokens->material);
}

bool
UsdImagingHydraMaterialAdapter::IsPopulatedIndirectly()
{
    // Materials are populated as a consequence of populating a prim
    // which uses the material.
    return true;
}

SdfPath
UsdImagingHydraMaterialAdapter::Populate(UsdPrim const& prim,
                            UsdImagingIndexProxy* index,
                            UsdImagingInstancerContext const* instancerContext)
{
    // Since shaders are populated by reference, they need to take care not to
    // be populated multiple times.
    SdfPath cachePath = prim.GetPath();
    if (index->IsPopulated(cachePath)) {
        return cachePath;
    }

    index->InsertSprim(HdPrimTypeTokens->material,
                       cachePath,
                       prim, shared_from_this());
    HD_PERF_COUNTER_INCR(UsdImagingTokens->usdPopulatedPrimCount);

    if (index->IsBprimTypeSupported(HdPrimTypeTokens->texture)) {
        SdfPathVector textures = _GetSurfaceShaderTextures(prim);
        TF_FOR_ALL(textureIt, textures)
        {
            // Textures are inserted as property paths, with the property being
            // the texture asset path.  Some textures will have sibling
            // attributes specifying things like filtering modes; that's
            // currently all picked up in UsdImagingDelegate via
            // - GetTextureResourceID
            // - GetTextureResource
            // ... which will get the prim path and explore.
            //
            if (index->IsPopulated(*textureIt)) {
                continue;
            }
            UsdPrim texturePrim = _GetPrim(textureIt->GetPrimPath());
            index->InsertBprim(HdPrimTypeTokens->texture,
                    *textureIt,
                    texturePrim, shared_from_this());
            HD_PERF_COUNTER_INCR(UsdImagingTokens->usdPopulatedPrimCount);
        }
    }

    return prim.GetPath();
}

/* virtual */
void
UsdImagingHydraMaterialAdapter::TrackVariability(UsdPrim const& prim,
                                          SdfPath const& cachePath,
                                          HdDirtyBits* timeVaryingBits,
                                          UsdImagingInstancerContext const*
                                              instancerContext)
{
    if (IsChildPath(cachePath)) {
        // Textures aren't time-varying.
        return;
    }

    // XXX: This is terrifying. Run through all attributes of the prim,
    // and if any are time varying, assume all shader params are time-varying.
    const std::vector<UsdAttribute> &attrs = prim.GetAttributes();
    TF_FOR_ALL(attrIter, attrs) {
        const UsdAttribute& attr = *attrIter;
        if (attr.GetNumTimeSamples()>1){
            *timeVaryingBits |= HdMaterial::DirtyParams;
        }
    }
}

static bool
_IsTextureOrPrimvarInput(const UsdShadeInput &shaderInput)
{
    UsdAttribute attr = shaderInput.GetAttr();

    TfToken baseName = attr.GetBaseName();
    return  attr.SplitName().size() >= 2 && 
            (baseName =="texture" || baseName=="primvar");
}

TfTokenVector
UsdImagingHydraMaterialAdapter::_DiscoverPrimvarsFromShaderNetwork(
    UsdShadeShader const& shader) const
{
    // TODO: It might be convenient to implicitly wire up PtexFaceOffset and
    // PtexFaceIndex primvars.
    TF_DEBUG(USDIMAGING_SHADERS).Msg("\t Looking for primvars at <%s>\n",
                            shader.GetPrim().GetPath().GetText());

    TfTokenVector primvars;

    for (UsdShadeInput const& input : shader.GetInputs()) {

        if (_IsTextureOrPrimvarInput(input))
            continue;

        UsdShadeConnectableAPI source;
        TfToken outputName;
        UsdShadeAttributeType sourceType;
        if (UsdShadeConnectableAPI::GetConnectedSource(input, &source, 
                &outputName, &sourceType)) {
            UsdAttribute attr = UsdShadeShader(source).GetIdAttr();
            TfToken id;
            if (!attr || !attr.Get(&id)) {
                continue;
            }
            
            TF_DEBUG(USDIMAGING_SHADERS).Msg("\t\t Shader input <%s> connected <%s>(%s)\n",
                            input.GetAttr().GetName().GetText(),
                            source.GetPath().GetText(),
                            id.GetText());

            if (id == UsdHydraTokens->HwPrimvar_1) {
                UsdShadeShader sourceShader(source);
                TfToken t;
                if (UsdHydraPrimvar(sourceShader).GetVarnameAttr().Get(&t, 
                                            UsdTimeCode::Default())) {
                    primvars.push_back(t);
                }
            } else {
                // Recursively look for more primvars
                primvars = 
                    _DiscoverPrimvarsFromShaderNetwork(UsdShadeShader(source));
            }
        }
    }

    return primvars;
}

TfTokenVector
UsdImagingHydraMaterialAdapter::_DiscoverPrimvarsDeprecated(
    UsdPrim const& shaderPrim) const
{
    TfTokenVector primvars;
    UsdImagingValueCache::PrimvarInfo primvar;
    UsdShadeShader shader(shaderPrim);

    TF_DEBUG(USDIMAGING_SHADERS).Msg("\t Looking for deprecated primvars at <%s>\n",
                            shader.GetPrim().GetPath().GetText());

    std::vector<UsdShadeInput> const &inputs = shader.GetInputs();
    for (const UsdShadeInput &shaderInput: inputs) {
        if (_IsTextureOrPrimvarInput(shaderInput))
            continue;

        UsdAttribute attr = shaderInput.GetAttr();
        if (!attr) {
            continue;
        }

        // Ok this is a parameter, check source input.
        if (UsdAttribute texAttr = shaderPrim.GetAttribute(
                                TfToken(attr.GetPath().GetName() 
                                + ":texture"))) {
            TfToken t;
            SdfAssetPath ap;
            texAttr.Get(&ap, UsdTimeCode::Default());

            bool isPtex = GlfIsSupportedPtexTexture(TfToken(ap.GetAssetPath()));
            if (isPtex) {

                t = UsdImagingTokens->ptexFaceIndex;
                // Allow the client to override this name
                texAttr.GetMetadata(UsdImagingTokens->faceIndexPrimvar, &t);
                primvars.push_back(t);

                t = UsdImagingTokens->ptexFaceOffset;
                // Allow the client to override this name
                texAttr.GetMetadata(UsdImagingTokens->faceOffsetPrimvar, &t);
                primvars.push_back(t);

            } else {
                texAttr.GetMetadata(UsdImagingTokens->uvPrimvar, &t);
                primvars.push_back(t);
            }
        } else if (UsdAttribute pvAttr = shaderPrim.GetAttribute(
                                        TfToken(attr.GetPath().GetName() 
                                                + ":primvar"))) {
            TfToken t;
            if (TF_VERIFY(pvAttr.Get(&t, UsdTimeCode::Default()))) {
                primvars.push_back(t);
            }
        }
    }

    return primvars;
}

TfTokenVector
UsdImagingHydraMaterialAdapter::_DiscoverPrimvars(SdfPath const& shaderPath) const
{
    TfTokenVector primvars;

    // Check if each parameter/input is bound to a texture or primvar.
    if (UsdPrim const& shaderPrim = _GetPrim(shaderPath)) {
        if (UsdShadeShader s = UsdShadeShader(shaderPrim)) {
            primvars = _DiscoverPrimvarsFromShaderNetwork(s);
        } else {
            primvars = _DiscoverPrimvarsDeprecated(shaderPrim);
        }
    }

    return primvars;
}

/* virtual */
void
UsdImagingHydraMaterialAdapter::UpdateForTime(UsdPrim const& prim,
                                       SdfPath const& cachePath,
                                       UsdTimeCode time,
                                       HdDirtyBits requestedBits,
                                       UsdImagingInstancerContext const*
                                           instancerContext)
{
    if (IsChildPath(cachePath)) {
        // Textures aren't stored in the value cache.
        // XXX: For bonus points, we could move the logic from
        // - GetTextureResourceID and GetTextureResource here.
        return;
    }

    UsdImagingValueCache* valueCache = _GetValueCache();

    if (requestedBits & HdMaterial::DirtySurfaceShader) {
        // DirtySurfaceShader triggers a refresh of both shader sources.
        valueCache->GetSurfaceShaderSource(cachePath) =
            _GetShaderSource(prim, _tokens->surfaceShader);
        valueCache->GetDisplacementShaderSource(cachePath) =
            _GetShaderSource(prim, _tokens->displacementShader);

        // Extract the primvars
        valueCache->GetMaterialPrimvars(cachePath) = 
            _DiscoverPrimvars(cachePath);
    }

    if (requestedBits & HdMaterial::DirtyParams) {
        // XXX: The param list isn't actually time-varying... we should find
        // a way to only do this once.
        HdMaterialParamVector& materialParams =
            valueCache->GetMaterialParams(cachePath);
        materialParams = _GetMaterialParams(prim);

        // Hydra expects values in the value cache for any param that's
        // a "fallback" param (constant, as opposed to texture- or
        // primvar-based).
        TF_FOR_ALL(paramIt, materialParams) {
            if (paramIt->IsFallback()) {
                VtValue& param = valueCache->GetMaterialParam(
                    cachePath, paramIt->GetName());
                param = _GetMaterialParamValue(prim,
                            paramIt->GetName(), time);
            }
        }
    }
}

/* virtual */
HdDirtyBits
UsdImagingHydraMaterialAdapter::ProcessPropertyChange(UsdPrim const& prim,
                                               SdfPath const& cachePath,
                                               TfToken const& propertyName)
{
    // XXX: This doesn't get notifications for dependent nodes.
    return HdChangeTracker::AllDirty;
}

/* virtual */
void
UsdImagingHydraMaterialAdapter::MarkDirty(UsdPrim const& prim,
                                   SdfPath const& cachePath,
                                   HdDirtyBits dirty,
                                   UsdImagingIndexProxy* index)
{
    if (IsChildPath(cachePath)) {
        index->MarkBprimDirty(cachePath, dirty);
    } else {
        index->MarkSprimDirty(cachePath, dirty);
    }
}

/* virtual */
void
UsdImagingHydraMaterialAdapter::_RemovePrim(SdfPath const& cachePath,
                                 UsdImagingIndexProxy* index)
{
    if (IsChildPath(cachePath)) {
        index->RemoveBprim(HdPrimTypeTokens->texture, cachePath);
    } else {
        index->RemoveSprim(HdPrimTypeTokens->material, cachePath);
    }
}

std::string
UsdImagingHydraMaterialAdapter::_GetShaderSource(UsdPrim const& prim, 
                                          TfToken const& shaderType) const
{
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
            return std::string();
        }
        TF_DEBUG(USDIMAGING_SHADERS).Msg("Loading deprecated shader: %s\n",
                    srcAttr.GetPath().GetText());
        // ------------------------------------------------------------------ //
    }

    // PERFORMANCE: We're opening the file on every request currently, but we'd
    // like to share this in some sort of registry in the future.
    SdfAssetPath asset;
    std::string filePath;
    if (!srcAttr.Get(&asset)){
        return std::string();
    }

    filePath = asset.GetResolvedPath();

    // Fallback to the literal path if it couldn't be resolved.
    if (filePath.empty()){
        filePath = asset.GetAssetPath();
    }

    GlfGLSLFX gfx(filePath);
    if (!gfx.IsValid()){
        return std::string();
    }

    if (shaderType == _tokens->surfaceShader){
        return gfx.GetSurfaceSource();
    } else if (shaderType == _tokens->displacementShader){
        return gfx.GetDisplacementSource();
    }

    TF_CODING_ERROR("Unsupported shader type: <%s>\n", shaderType.GetText());
    return std::string();
}

VtValue
UsdImagingHydraMaterialAdapter::_GetMaterialParamValue(
                                                UsdPrim const &prim, 
                                                TfToken const &paramName,
                                                UsdTimeCode time) const
{
    VtValue value;
    UsdAttribute attr = prim.GetAttribute(paramName);
    if (TF_VERIFY(attr)) {
        // XXX: Reading the value may fail, should we warn here when it does?
        attr.Get(&value, time);
    }

    return value;
}

HdMaterialParamVector
UsdImagingHydraMaterialAdapter::_GetMaterialParams(UsdPrim const& prim) const
{
    HdMaterialParamVector params;

    UsdShadeShader shader(prim);
    std::vector<UsdShadeInput> const &inputs = shader.GetInputs();
    for (const UsdShadeInput &shaderInput: inputs) {
        if (_IsTextureOrPrimvarInput(shaderInput)) {
                continue;
        }

        UsdAttribute attr = shaderInput.GetAttr();

        TF_DEBUG(USDIMAGING_SHADERS).Msg("Shader input  found: %s\n",
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

        if (shader) 
        {
            TF_DEBUG(USDIMAGING_SHADERS).Msg("Shader input: %s\n",
                    shaderInput.GetFullName().GetText());

            UsdShadeConnectableAPI source;
            TfToken outputName;
            UsdShadeAttributeType sourceType;

            if (UsdShadeConnectableAPI::GetConnectedSource(
                    shaderInput, &source, &outputName, &sourceType)) {
                UsdShadeShader sourceShader(source);
                if (UsdAttribute attr = sourceShader.GetIdAttr()) {
                    TfToken id;
                    if (attr.Get(&id)) {
                        if (id == UsdHydraTokens->HwUvTexture_1) {
                            connection = UsdHydraTexture(sourceShader).
                                GetFilenameAttr().GetPath();
                            TF_DEBUG(USDIMAGING_SHADERS).Msg(
                                        "\t connected to UV texture\n");
                            UsdHydraUvTexture tex(sourceShader);
                            UsdShadeInput uv(tex.GetUvAttr());
                            UsdShadeConnectableAPI uvSource;
                            if (UsdShadeConnectableAPI::
                                    GetConnectedSource(uv, &uvSource, 
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
                            connection = UsdHydraTexture(sourceShader).
                                GetFilenameAttr().GetPath();
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
        } else {
            // ---------------------------------------------------------- //
            // Deprecated
            // ---------------------------------------------------------- //
            if (UsdAttribute texAttr = prim.GetAttribute(
                                            TfToken(attr.GetPath().GetName() 
                                                    + ":texture"))) {
                // XXX: we should use the connection for both texture and
                // primvars here.
                connection = texAttr.GetPath();
                TfToken t;
                SdfAssetPath ap;
                texAttr.Get(&ap, UsdTimeCode::Default());
                TfToken resolvedPath(ap.GetResolvedPath());
                if (resolvedPath.IsEmpty()) {
                    resolvedPath = TfToken(ap.GetAssetPath());
                }
                isPtex = GlfIsSupportedPtexTexture(resolvedPath);
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
        params.push_back(HdMaterialParam(attr.GetName(), 
                                         fallbackValue,
                                         connection,
                                         samplerCoords,
                                         isPtex));
    }

    return params;
}

SdfPathVector
UsdImagingHydraMaterialAdapter::_GetSurfaceShaderTextures(UsdPrim const &prim) const
{
    SdfPathVector textureIDs;

    UsdShadeShader shader(prim);
    if (shader) {
        SdfPathVector stack(1, shader.GetPath());
        TfToken t;
        while (!stack.empty()) {
            SdfPath shaderPath = stack.back();
            stack.pop_back();
            shader = UsdShadeShader(_GetPrim(shaderPath));
            TF_DEBUG(USDIMAGING_TEXTURES).Msg(
                    " Looking for connected textures at <%s>\n",
                    shader.GetPath().GetText());

            if (shader.GetIdAttr().Get(&t)
                    && (t == UsdHydraTokens->HwUvTexture_1
                        || t == UsdHydraTokens->HwPtexTexture_1)) {
                TF_DEBUG(USDIMAGING_TEXTURES).Msg(
                    "  found texture: <%s>\n",
                    shader.GetPath().GetText());
                SdfPath connection = UsdHydraTexture(shader).GetFilenameAttr()
                    .GetPath();
                textureIDs.push_back(connection);
            }
            for (UsdShadeInput shaderInput: shader.GetInputs()) {
                if (_IsTextureOrPrimvarInput(shaderInput)) {
                    continue;
                }
                UsdShadeConnectableAPI source;
                TfToken outputName;
                UsdShadeAttributeType sourceType;
                if (UsdShadeConnectableAPI::GetConnectedSource(shaderInput, 
                    &source, &outputName, &sourceType)) {
                    stack.push_back(source.GetPath());
                }
            }
        }
    } else {
        for (const UsdShadeInput &shaderInput : shader.GetInputs()) {
            if (_IsTextureOrPrimvarInput(shaderInput)) {
                continue;
            }
            UsdAttribute attr = shaderInput.GetAttr();
            SdfPath connection;
            if (UsdAttribute texAttr = prim.GetAttribute(
                                            TfToken(attr.GetPath().GetName() 
                                                    + ":texture"))) {
                connection = texAttr.GetPath();
                textureIDs.push_back(connection);

                TF_DEBUG(USDIMAGING_TEXTURES).Msg(
                        "Texture connection found: %s\n",
                        texAttr.GetPath().GetText());
            }
        }
    }

    return textureIDs;
}

PXR_NAMESPACE_CLOSE_SCOPE
