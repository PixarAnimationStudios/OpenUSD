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
#include "pxr/usdImaging/usdImagingGL/hydraMaterialAdapter.h"
#include "pxr/usdImaging/usdImagingGL/package.h"
#include "pxr/usdImaging/usdImagingGL/textureUtils.h"

#include "pxr/usdImaging/usdImaging/debugCodes.h"
#include "pxr/usdImaging/usdImaging/delegate.h"
#include "pxr/usdImaging/usdImaging/indexProxy.h"
#include "pxr/usdImaging/usdImaging/tokens.h"

#include "pxr/imaging/glf/ptexTexture.h"
#include "pxr/imaging/glf/udimTexture.h"

#include "pxr/imaging/hd/enums.h"
#include "pxr/imaging/hd/material.h"
#include "pxr/imaging/hd/tokens.h"
#include "pxr/imaging/hio/glslfx.h"
#include "pxr/imaging/hdx/tokens.h"

#include "pxr/usd/ar/resolver.h"
#include "pxr/usd/sdr/registry.h"
#include "pxr/usd/sdr/shaderNode.h"
#include "pxr/usd/sdr/shaderProperty.h"
#include "pxr/usd/usdHydra/tokens.h"
#include "pxr/usd/usdShade/connectableAPI.h"

PXR_NAMESPACE_OPEN_SCOPE

TF_DEFINE_PRIVATE_TOKENS(
    _tokens,
    (surfaceShader)
    (displacementShader)
    (texture)
    (primvar)
    (isPtex)
    (opacity)
);

TF_REGISTRY_FUNCTION(TfType)
{
    typedef UsdImagingGLHydraMaterialAdapter Adapter;
    TfType t = TfType::Define<Adapter, TfType::Bases<Adapter::BaseAdapter> >();
    t.SetFactory< UsdImagingPrimAdapterFactory<Adapter> >();
}

UsdImagingGLHydraMaterialAdapter::~UsdImagingGLHydraMaterialAdapter()
{
}

bool
UsdImagingGLHydraMaterialAdapter::IsSupported(
    UsdImagingIndexProxy const* index) const
{
    return index->IsSprimTypeSupported(HdPrimTypeTokens->material);
}

SdfPath
UsdImagingGLHydraMaterialAdapter::Populate(UsdPrim const& prim,
                            UsdImagingIndexProxy* index,
                            UsdImagingInstancerContext const* instancerContext)
{
    // Since shaders are populated by reference, they need to take care not to
    // be populated multiple times.
    SdfPath cachePath = prim.GetPath();
    if (index->IsPopulated(cachePath)) {
        return cachePath;
    }

    // Extract the textures from the graph of this material.
    SdfPathVector textures;
    TfTokenVector primvars;
    HdMaterialParamVector params;
    UsdPrim surfaceShaderPrim;
    UsdPrim displacementShaderPrim;
    TfToken materialTag;
    if (!_GatherMaterialData(prim, &surfaceShaderPrim, 
                            &displacementShaderPrim,
                            &textures, &primvars, 
                            &params, &materialTag)) {
        return prim.GetPath();
    }

    index->InsertSprim(HdPrimTypeTokens->material,
                       cachePath,
                       prim, shared_from_this());
    HD_PERF_COUNTER_INCR(UsdImagingTokens->usdPopulatedPrimCount);

    if (index->IsBprimTypeSupported(HdPrimTypeTokens->texture)) {
        TF_FOR_ALL(textureIt, textures) {
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
            TF_DEBUG(USDIMAGING_TEXTURES).Msg("Populating texture found: %s\n",
                    texturePrim.GetPath().GetText());
            index->InsertBprim(HdPrimTypeTokens->texture,
                    *textureIt,
                    texturePrim, shared_from_this());
            HD_PERF_COUNTER_INCR(UsdImagingTokens->usdPopulatedPrimCount);
        }
    }

    return prim.GetPath();
}

static bool
_MightBeTimeVarying(UsdPrim const& prim)
{
    // Iterate the attributes to figure out if there is a time 
    // varying attribute in this node.
    for(UsdAttribute const& attr: prim.GetAttributes()) {
        if (attr.ValueMightBeTimeVarying()) {
            return true;
        }
    }
    return false;
}

static bool
_IsLegacyTextureOrPrimvarInput(UsdShadeInput const& shaderInput)
{
    UsdAttribute attr = shaderInput.GetAttr();

    TfToken baseName = attr.GetBaseName();
    return  attr.SplitName().size() >= 2 && 
            (baseName == _tokens->texture || baseName == _tokens->primvar);
}

static bool
_IsSupportedShaderInputType(SdfValueTypeName const& input)
{
    // This simple material adapter does not support tokens in the shader.
    return input != SdfValueTypeNames->Token;
}

static void
_GetMaterialTag(const TfToken& inputName, const UsdAttribute& attr, 
                TfToken* materialTag)
{
    // Another input has already determined the materialTag for this prim
    if (!materialTag->IsEmpty()) {
        return;
    }

    bool hasOpacity = inputName == _tokens->opacity;
    if (hasOpacity) {
        // Avoid prims from changing between opaque and translucent collections
        // by not just checking the authored value, but also checking if the
        // value changes over time.
        bool isTranslucent = attr.ValueMightBeTimeVarying() || 
                             attr.HasAuthoredConnections();

        // Avoid putting prims with an authored value of 1.0 in the translucent
        // collection.
        if (!isTranslucent) {
            VtValue vtOpacity;
            attr.Get(&vtOpacity);
            isTranslucent = vtOpacity.IsHolding<float>() && 
                            vtOpacity.UncheckedGet<float>() < 1.0f;
        }

        if (isTranslucent) {
            *materialTag = HdxMaterialTagTokens->translucent;
        }
    }
}

static UsdPrim
_GetDeprecatedSurfaceShaderPrim(const UsdShadeMaterial &material)
{
    // ---------------------------------------------------------------------- //
    // Hydra-only shader style - displayLook:bxdf
    // ---------------------------------------------------------------------- //
    static const TfToken displayLookBxdf("displayLook:bxdf");

    // ---------------------------------------------------------------------- //
    // Deprecated shader style - hydraLook:Surface
    // ---------------------------------------------------------------------- //
    static const TfToken hdSurf("hydraLook:surface");
    static const TfToken surfType("HydraPbsSurface");

    UsdRelationship displayShaderRel = material.GetPrim().GetRelationship(
        displayLookBxdf);

    if (!displayShaderRel) {
        displayShaderRel = material.GetPrim().GetRelationship(hdSurf);
    }

    // Return if neither deprecated relationship can be found.
    if (!displayShaderRel)
        return UsdPrim();

    SdfPathVector targets;
    if (!displayShaderRel.GetForwardedTargets(&targets))
        return UsdPrim();

    if (targets.size() != 1) {
        // XXX: This should really be a validation error once USD gets that
        // feature.
        TF_WARN("We expect only one target on relationship %s of prim <%s>, "
                "but got %zu.",
                displayShaderRel.GetName().GetText(),
                material.GetPath().GetText(),
                targets.size());
        return UsdPrim();
    }

    if (!targets[0].IsPrimPath()) {
        // XXX: This should really be a validation error once USD gets that
        // feature.
        TF_WARN("We expect the target of the relationship %s of prim <%s> "
                "to be a prim, instead it is <%s>.",
                displayShaderRel.GetName().GetText(),
                material.GetPath().GetText(),
                targets[0].GetText());
        return UsdPrim();
    }

    UsdPrim shaderPrim = displayShaderRel.GetStage()->GetPrimAtPath(targets[0]);
    if (displayShaderRel.GetName() == hdSurf) {
        if (TF_VERIFY(shaderPrim.GetTypeName() == surfType)) {
            TF_DEBUG(USDIMAGING_SHADERS).Msg(
                     "\t Deprecated hydraLook:surface binding found: %s\n", 
                     shaderPrim.GetPath().GetText());
                return shaderPrim;
        }
    } else {
        TF_DEBUG(USDIMAGING_SHADERS).Msg("\t Deprecated displayLook:bxdf "
                "binding found: %s\n", shaderPrim.GetPath().GetText());
        return shaderPrim;
    }

    return UsdPrim();
}

/* virtual */
void
UsdImagingGLHydraMaterialAdapter::TrackVariability(UsdPrim const& prim,
                                          SdfPath const& cachePath,
                                          HdDirtyBits* timeVaryingBits,
                                          UsdImagingInstancerContext const*
                                              instancerContext) const
{
    // If it is a child path, this adapter is dealing with a texture.
    // Otherwise, we are tracking variability of the material.
    if (IsChildPath(cachePath)) {
        if (_MightBeTimeVarying(prim)) {
            *timeVaryingBits |= HdTexture::DirtyTexture;
        }
        return;
    }

    UsdPrim surfaceShaderPrim = _GetSurfaceShaderPrim(UsdShadeMaterial(prim));
    if (!surfaceShaderPrim) {
        return;
    }

    // Checking if any of the connected shade nodes have time samples.
    UsdShadeConnectableAPI source;
    TfToken sourceName;
    UsdShadeAttributeType sourceType;
    UsdShadeConnectableAPI connectableAPI(surfaceShaderPrim);
    for (const UsdShadeInput & input: connectableAPI.GetInputs()) {
        if (input.GetConnectedSource(&source, &sourceName, &sourceType)) {
            if (_MightBeTimeVarying(source.GetPrim())) {
                *timeVaryingBits |= HdMaterial::DirtyParams;
                return;
            }
        } else {
            if (input.GetAttr().ValueMightBeTimeVarying()) {
                *timeVaryingBits |= HdMaterial::DirtyParams;
                return;
            }
        }
    }
}

UsdPrim
UsdImagingGLHydraMaterialAdapter::_GetSurfaceShaderPrim(
    const UsdShadeMaterial &material) const
{
    // Determine the path to the preview shader and return it.
    const TfToken context = _GetMaterialNetworkSelector();
    if (UsdShadeShader surface =  
        material.ComputeSurfaceSource(context)) {
        TF_DEBUG(USDIMAGING_SHADERS).Msg("\t GLSLFX surface: %s\n", 
            surface.GetPath().GetText());
        return surface.GetPrim();
    }

    return _GetDeprecatedSurfaceShaderPrim(material);
}

UsdPrim
UsdImagingGLHydraMaterialAdapter::_GetDisplacementShaderPrim(
    const UsdShadeMaterial &material) const
{
    // Determine the path to the preview displacement shader and return it.
    const TfToken context = _GetMaterialNetworkSelector();
    if (UsdShadeShader displacement = 
        material.ComputeDisplacementSource(context)) {
        TF_DEBUG(USDIMAGING_SHADERS).Msg("\t GLSLFX displacement: %s\n", 
            displacement.GetPath().GetText());
        return displacement.GetPrim();
    }

    return UsdPrim();
}

/* virtual */
void
UsdImagingGLHydraMaterialAdapter::UpdateForTime(
    UsdPrim const& prim,
    SdfPath const& cachePath,
    UsdTimeCode time,
    HdDirtyBits requestedBits,
    UsdImagingInstancerContext const* instancerContext) const
{
    if (IsChildPath(cachePath)) {
        // Textures aren't stored in the value cache.
        // XXX: For bonus points, we could move the logic from
        // - GetTextureResourceID and GetTextureResource here.
        return;
    }

    UsdPrim surfaceShaderPrim;
    UsdPrim displacementShaderPrim;
    SdfPathVector textures;
    TfTokenVector primvars;
    HdMaterialParamVector params;
    TfToken materialTag;

    if (requestedBits & HdMaterial::DirtySurfaceShader ||
        requestedBits & HdMaterial::DirtyParams) 
    {
        if (!_GatherMaterialData(prim, &surfaceShaderPrim, 
                                &displacementShaderPrim,
                                &textures, &primvars, 
                                &params, &materialTag)) {
            TF_CODING_ERROR("Failed to gather material data for already "
                "populated material prim <%s>.", prim.GetPath().GetText());
            return;
        }
    }

    UsdImagingValueCache* valueCache = _GetValueCache();
    if (requestedBits & HdMaterial::DirtySurfaceShader) {
        std::string surfaceSource;
        std::string displacementSource;

        VtDictionary surfaceMetadata;

        if (surfaceShaderPrim) {
            surfaceSource = _GetShaderSource(surfaceShaderPrim,
                                             _tokens->surfaceShader,
                                             &surfaceMetadata);

            // A hardcoded materialTag in the glslfx is a stronger opinion than
            // any materialTag we auto-determines by looking at authored inputs
            // of the material. If the glslfx file had a materialTag then it
            // will already be present in the metadata.
            if (!materialTag.IsEmpty()) {
                VtValue vtMaterialTag = TfMapLookupByValue(surfaceMetadata,
                        HdShaderTokens->materialTag,
                        VtValue());
                
                if (vtMaterialTag.IsEmpty()) {
                    surfaceMetadata[HdShaderTokens->materialTag] = materialTag;
                }
            }
        }

        if (displacementShaderPrim) {
            displacementSource = _GetShaderSource(displacementShaderPrim,
                                                  _tokens->displacementShader);
        }

        // DirtySurfaceShader triggers a refresh of both shader sources.
        valueCache->GetSurfaceShaderSource(cachePath) = surfaceSource;
        valueCache->GetDisplacementShaderSource(cachePath) =
                                                        displacementSource;
        valueCache->GetMaterialMetadata(cachePath) =
                                            VtValue(surfaceMetadata);

        // Extract the primvars
        valueCache->GetMaterialPrimvars(cachePath) = primvars;
    }

    if (requestedBits & HdMaterial::DirtyParams) {
        // XXX: The param list isn't actually time-varying... we should find
        // a way to only do this once.
        HdMaterialParamVector& materialParams =
            valueCache->GetMaterialParams(cachePath);
        materialParams = params;

        // Hydra expects values in the value cache for any param that's
        // a "fallback" param (constant, as opposed to texture- or
        // primvar-based).
        TF_FOR_ALL(paramIt, materialParams) {
            if (paramIt->IsFallback()) {
                VtValue& param = valueCache->GetMaterialParam(
                    cachePath, paramIt->GetName());
                param = _GetMaterialParamValue(surfaceShaderPrim,
                            paramIt->GetName(), time);
            }
        }
    }
}

/* virtual */
HdDirtyBits
UsdImagingGLHydraMaterialAdapter::ProcessPropertyChange(UsdPrim const& prim,
                                               SdfPath const& cachePath,
                                               TfToken const& propertyName)
{
    if (propertyName == UsdGeomTokens->visibility) {
        // Materials aren't affected by visibility
        return HdChangeTracker::Clean;
    }

    // XXX: This doesn't get notifications for dependent nodes.
    return HdChangeTracker::AllDirty;
}

/* virtual */
void
UsdImagingGLHydraMaterialAdapter::MarkDirty(UsdPrim const& prim,
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
UsdImagingGLHydraMaterialAdapter::MarkMaterialDirty(UsdPrim const& prim,
                                                    SdfPath const& cachePath,
                                                    UsdImagingIndexProxy* index)
{
    if (!IsChildPath(cachePath)) {
        index->MarkSprimDirty(cachePath, HdMaterial::DirtySurfaceShader |
                                         HdMaterial::DirtyParams);
    }
}

/* virtual */
void
UsdImagingGLHydraMaterialAdapter::_RemovePrim(SdfPath const& cachePath,
                                 UsdImagingIndexProxy* index)
{
    if (IsChildPath(cachePath)) {
        index->RemoveBprim(HdPrimTypeTokens->texture, cachePath);
    } else {
        index->RemoveSprim(HdPrimTypeTokens->material, cachePath);
    }
}

std::string
UsdImagingGLHydraMaterialAdapter::_GetShaderSource(
    UsdPrim const& shaderPrim, 
    TfToken const& shaderType,
    VtDictionary * metadataOut) const
{
    auto getGLSLFXSource = [&shaderType, &metadataOut](const HioGlslfx &gfx) {
        if (!gfx.IsValid()){
            return std::string();
        }
        if (metadataOut) {
            *metadataOut = gfx.GetMetadata();
        }
        if (shaderType == _tokens->surfaceShader){
            return gfx.GetSurfaceSource();
        } else if (shaderType == _tokens->displacementShader){
            return gfx.GetDisplacementSource();
        } else {
            TF_CODING_ERROR("Unsupported shader type: <%s>\n", 
                            shaderType.GetText());
            return std::string();
        }
    };

    UsdShadeShader shader = UsdShadeShader(shaderPrim);
    TfToken shaderId;
    if (shader) {
        // XXX: This doesn't use UsdShadeShader::GetShaderNodeForSourceType() 
        // yet, since we don't have a glslfx parser plugin.

        TfToken implSource = shader.GetImplementationSource();
        if (implSource == UsdShadeTokens->id) {
            if (shader.GetShaderId(&shaderId)) {
                // XXX: Process other shaderIds here using a shader 
                // registry.
                if (shaderId == UsdImagingTokens->UsdPreviewSurface) {
                    auto &shaderReg = SdrRegistry::GetInstance();
                    if (SdrShaderNodeConstPtr sdrNode = 
                            shaderReg.GetShaderNodeByIdentifierAndType(shaderId, 
                                HioGlslfxTokens->glslfx)) {
                        const std::string &glslfxPath = sdrNode->GetSourceURI();
                        TF_DEBUG(USDIMAGING_SHADERS).Msg(
                            "Loading UsdShade preview surface %s\n", 
                            glslfxPath.c_str());
                        return getGLSLFXSource(HioGlslfx(glslfxPath));
                    }
                }
            }
        } else if (implSource == UsdShadeTokens->sourceAsset) {
            SdfAssetPath sourceAsset;
            if (shader.GetSourceAsset(&sourceAsset, 
                                      /*sourceType*/ HioGlslfxTokens->glslfx)) {
                std::string resolvedSrcAsset = 
                    ArGetResolver().Resolve(sourceAsset.GetAssetPath());
                if (!resolvedSrcAsset.empty()) {
                    return getGLSLFXSource(HioGlslfx(resolvedSrcAsset));
                }
            }
        } else if (implSource == UsdShadeTokens->sourceCode) {
            std::string sourceCode; 
            if (shader.GetSourceCode(&sourceCode, 
                                     /*sourceType*/ HioGlslfxTokens->glslfx)) {
                std::istringstream sourceCodeStream(sourceCode);
                return getGLSLFXSource(HioGlslfx(sourceCodeStream));
            }
        }
    }

    // ------------------------------------------------------------------ //
    // Deprecated
    // ------------------------------------------------------------------ //
    UsdAttribute srcAttr = shaderPrim.GetAttribute(
            UsdHydraTokens->infoFilename);
    if (srcAttr) {
        TF_DEBUG(USDIMAGING_SHADERS).Msg("Loading UsdShade shader: %s\n",
            srcAttr.GetPath().GetText());
    } else {
        // ------------------------------------------------------------------ //
        // Long-Deprecated
        // ------------------------------------------------------------------ //
        srcAttr = shaderPrim.GetAttribute(UsdImagingTokens->infoSource);
        if (!srcAttr) {
            TF_DEBUG(USDIMAGING_SHADERS).Msg("No shader source attribute: %s\n",
                    shaderPrim.GetPath().GetText());
            return std::string();
        }
        TF_DEBUG(USDIMAGING_SHADERS).Msg("Loading deprecated shader: %s\n",
                    srcAttr.GetPath().GetText());
        // ------------------------------------------------------------------ //
    }

    // PERFORMANCE: We're opening the file on every request currently, but we'd
    // like to share this in some sort of registry in the future.
    SdfAssetPath asset;
    if (!srcAttr.Get(&asset)){
        return std::string();
    }

    std::string filePath = asset.GetResolvedPath();

    // Fallback to the literal path if it couldn't be resolved.
    if (filePath.empty()){
        filePath = asset.GetAssetPath();
    }

    HioGlslfx gfx(filePath);
    return getGLSLFXSource(gfx);
}

VtValue
UsdImagingGLHydraMaterialAdapter::_GetMaterialParamValue(
    UsdPrim const &shaderPrim, 
    TfToken const &paramName,
    UsdTimeCode time) const
{
    VtValue value;
    UsdShadeConnectableAPI source;
    TfToken sourceName;
    UsdShadeAttributeType sourceType;

    if (UsdShadeShader shader = UsdShadeShader(shaderPrim)) {
        if(UsdShadeInput shaderInput = shader.GetInput(paramName)) {
            // Check if it is connected to an input on the public interface.
            // If so, pull the information from the public interface.
            if (shaderInput.GetConnectedSource(
                &source, &sourceName, &sourceType)) {
                if (sourceType == UsdShadeAttributeType::Input) {
                    if (UsdShadeInput connectedInput = 
                        source.GetInput(sourceName)) {
                        connectedInput.Get(&value, time);
                    }
                }
            } else {
                shaderInput.Get(&value, time);
            }
        } 

        if (value.IsEmpty()) {
            TfToken shaderId; 
            if (shader.GetShaderId(&shaderId) && !shaderId.IsEmpty()) {
                auto &shaderReg = SdrRegistry::GetInstance();
                if (SdrShaderNodeConstPtr sdrNode = 
                    shaderReg.GetShaderNodeByIdentifierAndType(shaderId, 
                        HioGlslfxTokens->glslfx)) {
                    if (const auto &sdrInput = 
                                sdrNode->GetShaderInput(paramName)) {
                        value = sdrInput->GetDefaultValue();
                    }
                }
            }
        }
    } else {
        // ------------------------------------------------------------------ //
        // Deprecated
        // ------------------------------------------------------------------ //

        // First we try to read the attribute prefixed by "inputs:", if
        // that fails then we try the legacy name without "inputs:".
        TfToken inputAttr = 
            UsdShadeUtils::GetFullName(paramName, UsdShadeAttributeType::Input);
        UsdAttribute attr = shaderPrim.GetAttribute(inputAttr);
        if (!attr) {
            attr = shaderPrim.GetAttribute(paramName);
        }

        if (TF_VERIFY(attr)) {
            attr.Get(&value, time);
        }
        // ------------------------------------------------------------------ //
    }

    return value;
}

bool
UsdImagingGLHydraMaterialAdapter::_GatherMaterialData(
    UsdPrim const &materialPrim,
    UsdPrim *shaderPrim,
    UsdPrim *displacementShaderPrim,
    SdfPathVector *textureIDs,
    TfTokenVector *primvars,
    HdMaterialParamVector *params,
    TfToken *materialTag) const
{
    TF_DEBUG(USDIMAGING_SHADERS).Msg("Material caching : <%s>\n", 
        materialPrim.GetPath().GetText());

    *shaderPrim = _GetSurfaceShaderPrim(UsdShadeMaterial(materialPrim));
    if (*shaderPrim) {
        TF_DEBUG(USDIMAGING_SHADERS).Msg("- found surface shader: <%s>\n",
            shaderPrim->GetPath().GetText());
    } else {
        TF_DEBUG(USDIMAGING_SHADERS).Msg("- No valid surface shader!\n");
        return false;
    }

    *displacementShaderPrim = 
        _GetDisplacementShaderPrim(UsdShadeMaterial(materialPrim));
    if (*displacementShaderPrim) {
        TF_DEBUG(USDIMAGING_SHADERS).Msg("- found displacement shader: <%s>\n",
            displacementShaderPrim->GetPath().GetText());
    } else {
        TF_DEBUG(USDIMAGING_SHADERS).Msg("- No valid displacement shader!\n");
    }

    if (UsdShadeShader s = UsdShadeShader(*shaderPrim)) {
        _WalkShaderNetwork(*shaderPrim, textureIDs, primvars, params, 
                           materialTag);
    } else {
        _WalkShaderNetworkDeprecated(*shaderPrim, textureIDs, primvars, 
                                     params, materialTag);
    }

    return true;
}

void
UsdImagingGLHydraMaterialAdapter::_WalkShaderNetworkDeprecated(
    UsdPrim const &shaderPrim,
    SdfPathVector *textureIDs,
    TfTokenVector *primvars,
    HdMaterialParamVector *materialParams,
    TfToken *materialTag) const
{
    UsdShadeShader shader(shaderPrim);
    
    for (const UsdShadeInput &shaderInput : shader.GetInputs()) {
        if (_IsLegacyTextureOrPrimvarInput(shaderInput)) {
            continue;
        }

        UsdAttribute attr = shaderInput.GetAttr();
        if (!attr) {
            continue;
        }

        TF_DEBUG(USDIMAGING_SHADERS).Msg("\tShader input  found: %s\n",
                attr.GetPath().GetText());

        HdMaterialParam::ParamType paramType =
                HdMaterialParam::ParamTypeFallback;
        VtValue fallbackValue;
        SdfPath connection;
        TfTokenVector samplerCoords;
        HdTextureType textureType = HdTextureType::Uv;
        TfToken t;

        if (!TF_VERIFY(attr.Get(&fallbackValue),
                    "No fallback value for: <%s>\n",
                    attr.GetPath().GetText())) {
            continue;
        }

        if (UsdAttribute texAttr = shaderPrim.GetAttribute(
                                        TfToken(attr.GetPath().GetName() 
                                                + ":texture"))) {
            paramType = HdMaterialParam::ParamTypeTexture;
            connection = texAttr.GetPath();
            textureIDs->push_back(connection);

            TF_DEBUG(USDIMAGING_SHADERS).Msg(
                "\t\tFound texture: <%s>\n", connection.GetText());

            SdfAssetPath ap;
            texAttr.Get(&ap, UsdTimeCode::EarliestTime());

            if (GlfIsSupportedPtexTexture(TfToken(ap.GetAssetPath()))) {
                textureType = HdTextureType::Ptex;
                t = UsdImagingTokens->ptexFaceIndex;
                // Allow the client to override this name
                texAttr.GetMetadata(UsdImagingTokens->faceIndexPrimvar, &t);
                primvars->push_back(t);

                TF_DEBUG(USDIMAGING_SHADERS).Msg(
                    "\t\t\tFound primvar: <%s>\n", t.GetText());

                t = UsdImagingTokens->ptexFaceOffset;
                // Allow the client to override this name
                texAttr.GetMetadata(UsdImagingTokens->faceOffsetPrimvar, &t);
                primvars->push_back(t);
                TF_DEBUG(USDIMAGING_SHADERS).Msg(
                    "\t\t\tFound primvar: <%s>\n", t.GetText());

            } else {
                if (GlfIsSupportedUdimTexture(TfToken(ap.GetAssetPath()))) {
                    textureType = HdTextureType::Udim;
                }
                texAttr.GetMetadata(UsdImagingTokens->uvPrimvar, &t);
                primvars->push_back(t);
                TF_DEBUG(USDIMAGING_SHADERS).Msg(
                    "\t\t\tFound primvar: <%s>\n", t.GetText());
                samplerCoords.push_back(t);
                TF_DEBUG(USDIMAGING_SHADERS).Msg(
                    "\t\t\tFound sampler: <%s>\n", t.GetText());
            }

        } else if (UsdAttribute pvAttr = shaderPrim.GetAttribute(
                                        TfToken(attr.GetPath().GetName() 
                                                + ":primvar"))) {
            paramType = HdMaterialParam::ParamTypePrimvar;
            connection = SdfPath("primvar."
                                + pvAttr.GetName().GetString());
            TF_DEBUG(USDIMAGING_SHADERS).Msg(
                "\t\tFound primvar: <%s>\n", connection.GetText());

            if (TF_VERIFY(pvAttr.Get(&t, UsdTimeCode::EarliestTime()))) {
                primvars->push_back(t);
                TF_DEBUG(USDIMAGING_SHADERS).Msg(
                    "\t\t\tFound primvar: <%s>\n", t.GetText());
                samplerCoords.push_back(t);
                TF_DEBUG(USDIMAGING_SHADERS).Msg(
                    "\t\t\tFound sampler: <%s>\n", t.GetText());
            }
        }

        TfToken inputName = shaderInput.GetBaseName();

        // Check if input affects what collection the prim should go into.
        _GetMaterialTag(inputName, shaderInput.GetAttr(), materialTag);

        shaderInput.Get(&fallbackValue);
        materialParams->push_back(HdMaterialParam(paramType,
                                                  inputName,
                                                  fallbackValue,
                                                  connection,
                                                  samplerCoords,
                                                  textureType));
    }
}

class _ShaderNetworkWalker
{
public:
    _ShaderNetworkWalker(UsdPrim const &shaderPrim, 
                         SdfPathVector *textureIDs,
                         TfTokenVector *primvars,
                         HdMaterialParamVector *materialParams,
                         TfToken *materialTag,
                         const std::function<UsdPrim (const SdfPath &)> &
                            getPrimFunc);

private:
    // Internal data structure to keep the parameters organized before 
    // we return them in the actual HdMaterialParamVector.
    struct _MaterialParam {
        HdMaterialParam::ParamType _paramType;
        TfToken _name;
        VtValue _fallbackValue;
        SdfPath _connection;
        SdfPath _connectionPrimvar;
        TfTokenVector _samplerCoords;
        HdTextureType _textureType;
    };
    std::vector<_MaterialParam> _params;

    // These are declared here and reused to avoid repeated allocation 
    // everytime a connection is queried.
    UsdShadeConnectableAPI _source;
    TfToken _sourceName;
    UsdShadeAttributeType _sourceType;

    // Boolean that records whether the root node of the shading network 
    // has been processed.
    bool _processedRootNode=false;

    // Helper methods.
    void _ProcessRootNode(const UsdShadeShader &shader,
                          const SdrShaderNodeConstPtr &sdrNode,
                          TfToken *materialTag);

    void _ProcessTextureNode(const UsdShadeShader &shader,
                             const SdrShaderNodeConstPtr &sdrNode,
                             SdfPathVector *textureIDs,
                             TfTokenVector *primvars);

    void _ProcessPrimvarNode(const UsdShadeShader &shader,
                             const SdrShaderNodeConstPtr &sdrNode,
                             TfTokenVector *primvars);

    std::string _GetShaderRole(const UsdShadeShader &shader);

    std::pair<VtValue, SdfPath> _GetFallbackValueAndConnection(
                              const UsdShadeInput &shaderInput);

    VtValue _GetFallbackValue(const UsdShadeShader &shader, 
                              const SdrShaderNodeConstPtr &sdrNode);

    void _ProcessPrimvarInput(const TfToken &primvarInputName, 
                              const UsdShadeShader &shader, 
                              const SdrShaderNodeConstPtr &sdrNode,
                              TfTokenVector *primvars,
                              TfTokenVector *varNames=nullptr);
};

_ShaderNetworkWalker::_ShaderNetworkWalker(
    UsdPrim const &shaderPrim, 
    SdfPathVector *textureIDs,
    TfTokenVector *primvars,
    HdMaterialParamVector *materialParams,
    TfToken *materialTag,
    const std::function<UsdPrim (const SdfPath &)> &getPrimFunc)
{
    auto &shaderReg = SdrRegistry::GetInstance();

    // Iteratively walk the graph visiting each node and collecting
    // textures, primvars and material parameters

    // Vector used to walk the graph iteratively.
    SdfPathVector stack(1, shaderPrim.GetPath());
    while (!stack.empty()) {
        SdfPath shaderPath = stack.back();
        stack.pop_back();

        UsdShadeShader shader(getPrimFunc(shaderPath));

        // XXX: Ideally, we would use the API 
        // UsdShadeShader::GetShaderNodeForSourceType() here, but it will 
        // only work right now for implementationSource="id", since we don't 
        // have a glslfx parser plugin.

        // Extract the id of the node
        TfToken id;
        shader.GetShaderId(&id);

        SdrShaderNodeConstPtr sdrNode = 
                shaderReg.GetShaderNodeByIdentifierAndType(id, 
                    HioGlslfxTokens->glslfx);

        TfToken sdrFamily(sdrNode ? sdrNode->GetFamily() : TfToken());
        TfToken sdrRole(sdrNode ? sdrNode->GetRole() : "");

        TF_DEBUG(USDIMAGING_SHADERS).Msg("\tEvaluating %s node : <%s> with "
            "id='%s', family='%s', role='%s'\n", 
            (_processedRootNode ? "" : "root"),
            shader.GetPath().GetText(), id.GetText(), sdrFamily.GetText(),
            sdrRole.GetText());
        
        // For preview materials Hydra Stream material the current
        // assumption is that we have a root material which is typically the
        // first node. This node has a bunch of inputs that can be pointing
        // to a texture or a primvar (or a default value). 
        // The current algorithm is made exclusively to walk this basic
        // materials.

        // For non-id based nodes, sdrRole will be empty. Hence, we assume that 
        // the surface node will be the first (root) node in the network. 
        // We may want to relax this restriction in the future. 
        if (!_processedRootNode) {
            _ProcessRootNode(shader, sdrNode, materialTag);
        } else if (sdrNode) {
            // For nodes with valid sdrNodes we can actually detect if they 
            // are primvars or textures based on their role and add them to 
            // the pipeline.
            if (sdrRole == SdrNodeRole->Texture) {
                _ProcessTextureNode(shader, sdrNode, textureIDs, primvars);
            } else if (sdrRole == SdrNodeRole->Primvar) {
                _ProcessPrimvarNode(shader, sdrNode, primvars);
            } else {
                TF_DEBUG(USDIMAGING_SHADERS).Msg("Warning: found shader node "
                    "<%s> with invalid role '%s'!\n", shader.GetPath().GetText(),
                    sdrRole.GetText());
            }
        }

        // Add nodes to the stack to keep walking the graph
        for (UsdShadeInput const & shaderInput: shader.GetInputs()) {
            if (_IsLegacyTextureOrPrimvarInput(shaderInput)) {
                continue;
            }

            if (UsdShadeConnectableAPI::GetConnectedSource(shaderInput, 
                &_source, &_sourceName, &_sourceType)) {
                // When we find a connection to a shading node output,
                // walk the upstream shading node.  Do not do this for
                // other sources (ex: a connection to a material
                // public interface parameter), since they are not
                // part of the shading node graph.
                if (_sourceType == UsdShadeAttributeType::Output) {
                    // XXX: What if there's a cyclic dependency and source 
                    // has already been processed? We should at least guard 
                    // against an infinite loop.
                    stack.push_back(_source.GetPath());
                }
            }
        }
    }

    // Fill the material parameters structure with all the information
    // we have compiled after walking the material.
    for(_MaterialParam const & param : _params) {
        materialParams->emplace_back(param._paramType,
                param._name, param._fallbackValue,
                param._connection, param._samplerCoords, param._textureType);
    }
}

void 
_ShaderNetworkWalker::_ProcessRootNode(
    const UsdShadeShader &shader,
    const SdrShaderNodeConstPtr &sdrNode,
    TfToken *materialTag) 
{
    // We won't have a valid sdrNode for shaders using custom 
    // glslfx.
    if (sdrNode) {
        const auto &inputNames = sdrNode->GetInputNames();
        for (auto &inputName : inputNames) {
            const auto &usdShadeInput = shader.GetInput(inputName);
            const auto &sdrInput = sdrNode->GetShaderInput(inputName);

            SdfPath inputConn;
            VtValue fallbackValue;

            if (!usdShadeInput && sdrInput) {
                fallbackValue = sdrInput->GetDefaultValue();
            } else {
                std::tie(fallbackValue, inputConn) = 
                    _GetFallbackValueAndConnection(usdShadeInput);

                // Check if input affects the collection the prim should go into
                _GetMaterialTag(inputName, usdShadeInput.GetAttr(),materialTag);
            }

            // Finally, initialize data for this potential input to the 
            // material we are loading.
            _MaterialParam matParam = {
                    HdMaterialParam::ParamTypeFallback,/*paramType*/
                    inputName,/*name*/
                    fallbackValue,/*fallbackValue*/
                    inputConn,/*_connection*/
                    SdfPath(), /*_connectionPrimvar*/
                    TfTokenVector(), /*_samplerCoords*/
                    HdTextureType::Uv /*_textureType*/};
            _params.push_back(matParam);
            TF_DEBUG(USDIMAGING_SHADERS).Msg(
                "\t\tAdding attribute <%s> with connection <%s>%s.\n", 
                inputName.GetText(), inputConn.GetText(), 
                (sdrInput && !usdShadeInput) ? " from registry" : 
                " from UsdShadeInput");
        }
    } else {
        for (const UsdShadeInput &shaderInput: shader.GetInputs()) {
            // Early out for any legacy texture/primvar inputs.
            if (_IsLegacyTextureOrPrimvarInput(shaderInput)) {
                continue;
            }

            // The current simplified shading system does not support
            // tokens as inputs, so we will drop them at this point.
            if (!_IsSupportedShaderInputType(shaderInput.GetTypeName())) {
                continue;
            }

            // Extract the fallback value for this input
            VtValue fallbackValue;
            SdfPath inputConn;
            
            std::tie(fallbackValue, inputConn) = 
                _GetFallbackValueAndConnection(shaderInput);

            TfToken inputName = shaderInput.GetBaseName();

            // Check if input affects what collection the prim should go into.
            _GetMaterialTag(inputName, shaderInput.GetAttr(), materialTag);

            // Finally, initialize data for this potential input to the 
            // material we are loading.
            _MaterialParam matParam = {
                    HdMaterialParam::ParamTypeFallback,/*paramType*/
                    inputName, /*name*/
                    fallbackValue,/*fallbackValue*/
                    inputConn,/*_connection*/
                    SdfPath(), /*_connectionPrimvar*/
                    TfTokenVector(), /*_samplerCoords*/
                    HdTextureType::Uv /*_textureType*/};
            _params.push_back(matParam);

            TF_DEBUG(USDIMAGING_SHADERS).Msg(
                "\t\tAdding attribute : <%s> at <%s>\n", 
                shaderInput.GetBaseName().GetText(),
                inputConn.GetText());
        }
    }

    _processedRootNode = true;
}

void 
_ShaderNetworkWalker::_ProcessTextureNode(
    const UsdShadeShader &shader,
    const SdrShaderNodeConstPtr &sdrNode,
    SdfPathVector *textureIDs,
    TfTokenVector *primvars) 
{
    HdTextureType textureType = HdTextureType::Uv;

    if (sdrNode && sdrNode->GetMetadata().count(_tokens->isPtex)) {
        textureType = HdTextureType::Ptex;
    }

    // Extract the filename property from the shader node and store 
    // the path in the textureIDs array.
    SdfPath connection;
    const auto assetIdentifierPropertyNames = 
            sdrNode->GetAssetIdentifierInputNames();
    if (assetIdentifierPropertyNames.size() > 0) {
        if (assetIdentifierPropertyNames.size() > 1) {
            TF_WARN("Found texture node <%s> with more than "
                "one (%zu) asset-identifier properties. "
                "Considering only the first one.", 
                shader.GetPath().GetText(), 
                assetIdentifierPropertyNames.size());
        }
        const auto &input = shader.GetInput(assetIdentifierPropertyNames[0]);
        if (input) {
            connection = input.GetAttr().GetPath();
            if (textureType != HdTextureType::Ptex) {
                SdfAssetPath ap;
                if (input.GetAttr().Get(&ap, UsdTimeCode::EarliestTime())) {
                    if (GlfIsSupportedUdimTexture(TfToken(ap.GetAssetPath()))) {
                        textureType = HdTextureType::Udim;
                    }
                }
            }
        }
    } else {
        if (assetIdentifierPropertyNames.size() > 1) {
            TF_WARN("Found texture node <%s> with no "
                "asset-identifier properties.", 
                shader.GetPath().GetText());
        }
    }

    // It is possible that there is no path available, in that
    // case we won't try to load the texture and we will just
    // use the fallback value
    if (!connection.IsEmpty()) {
        textureIDs->push_back(connection);

        TF_DEBUG(USDIMAGING_SHADERS).Msg(
            "\t\tFound texture: <%s>\n", connection.GetText());
    }

    SdfPath connectionPrimvar;
    VtValue fallback = _GetFallbackValue(shader, sdrNode);
    if (textureType == HdTextureType::Ptex) {
        for (auto const & primvarInputName : 
                sdrNode->GetAdditionalPrimvarProperties()) {
            _ProcessPrimvarInput(primvarInputName, shader, 
                                sdrNode, primvars);
        }
    } else {
        // For regular textures we need to resolve what node
        // will be providing the texture coordinates.
        for (const UsdShadeInput &primvarInput : shader.GetInputs()) {
            // If the input is connected to a primvar node's output, then record
            // the path to the shader in connectionPrimvar.
            // XXX: In the future, we want to allow for connections for 
            // "texcoord" to any node that can produce a surface-varying output.
            if (primvarInput.GetConnectedSource(&_source, &_sourceName, 
                        &_sourceType) && 
                    _sourceType == UsdShadeAttributeType::Output &&
                    _GetShaderRole(_source) == SdrNodeRole->Primvar) {
                connectionPrimvar = _source.GetPath();
            }
        }
    }

    for(auto &p : _params) {
        if (p._connection == shader.GetPath()) {
            p._paramType = HdMaterialParam::ParamTypeTexture;
            p._textureType = textureType;
            p._connectionPrimvar = connectionPrimvar;
            p._connection = connection;
            if (!fallback.IsEmpty()) {
                TF_DEBUG(USDIMAGING_SHADERS).Msg(
                    "\t\t Fallback value: %s\n", 
                    TfStringify(fallback).c_str());

                p._fallbackValue = fallback;
            }
        }
    }
}

void 
_ShaderNetworkWalker::_ProcessPrimvarNode(
    const UsdShadeShader &shader,
    const SdrShaderNodeConstPtr &sdrNode,
    TfTokenVector *primvars)
{
    SdfPath connection = SdfPath("primvar." 
                        + shader.GetPrim()
                                .GetName()
                                .GetString());

    // Primvars can be providing data to an input to the material
    // or to a texture. We need this distinction in our current
    // design of HdMaterialParam.
    TfTokenVector varNames;
    VtValue fallback = _GetFallbackValue(shader, sdrNode);
    for (auto const& primvarInputName: 
            sdrNode->GetAdditionalPrimvarProperties()) {
        _ProcessPrimvarInput(primvarInputName, shader, sdrNode, 
                            primvars, &varNames);
    }

    for(auto &p : _params) {
        if (p._connectionPrimvar == shader.GetPath()){
            for (auto &varname : varNames) {
                TF_DEBUG(USDIMAGING_SHADERS).Msg(
                    "\t\tPrimvar connected : <%s>\n", 
                    varname.GetText());

                // No need to change the paramType here.
                p._samplerCoords.push_back(varname);
            }
        } else if (p._connection == shader.GetPath()){

            for (auto &varname : varNames) {
                TF_DEBUG(USDIMAGING_SHADERS).Msg(
                    "\t\tPrimvar connected : <%s>\n", 
                    varname.GetText());
                p._paramType = HdMaterialParam::ParamTypePrimvar;
                p._connection = connection;
                p._samplerCoords.push_back(varname);

                if (!fallback.IsEmpty()) {
                    TF_DEBUG(USDIMAGING_SHADERS).Msg(
                        "\t\t Fallback value: %s\n", 
                        TfStringify(fallback).c_str());

                    p._fallbackValue = fallback;
                }
            }
        }
    }
}

std::string
_ShaderNetworkWalker::_GetShaderRole(const UsdShadeShader &shader)
{
    TfToken id;
    if (shader && shader.GetShaderId(&id) && !id.IsEmpty()) {
        auto &shaderReg = SdrRegistry::GetInstance();
        SdrShaderNodeConstPtr sdrNode = 
                shaderReg.GetShaderNodeByIdentifierAndType(id, 
                    HioGlslfxTokens->glslfx);
        return sdrNode ? sdrNode->GetRole() : std::string();
    }
    return std::string();
}

std::pair<VtValue, SdfPath>
_ShaderNetworkWalker::_GetFallbackValueAndConnection(
    const UsdShadeInput &shaderInput)
{
    VtValue fallbackValue; 
    SdfPath connection;

    const bool hasFallbackValue = shaderInput.Get(&fallbackValue);
    if (UsdShadeConnectableAPI::GetConnectedSource(
        shaderInput, &_source, &_sourceName, &_sourceType)) {

        if (_sourceType == UsdShadeAttributeType::Output) {
            connection = _source.GetPath();
            // We need to have a valid fallback value based on the
            // input's type, otherwise codeGen won't know the 
            // correct function signature and will generate faulty 
            // shader code.
            if (!hasFallbackValue) {
                fallbackValue = 
                    shaderInput.GetTypeName().GetDefaultValue();
            }
        } else if (_sourceType == UsdShadeAttributeType::Input) {
            if (UsdShadeInput connectedInput = 
                    _source.GetInput(_sourceName)) {
                connectedInput.Get(&fallbackValue);
            }
        }
    }
    return std::make_pair(fallbackValue, connection);
}

VtValue
_ShaderNetworkWalker::_GetFallbackValue(
    const UsdShadeShader &shader, 
    const SdrShaderNodeConstPtr &sdrNode)
{
    VtValue fallback;
    if (sdrNode) {
        if (const auto &defaultInput = sdrNode->GetDefaultInput()) {
            auto usdShadeInput = shader.GetInput(defaultInput->GetName());
            if (usdShadeInput) {
                // "fallback" input should have interfaceOnly connectability. 
                if (usdShadeInput.GetConnectedSource(&_source, &_sourceName, 
                        &_sourceType)) {
                    // XXX: Fallback should be connectable to an output, but 
                    // HdSt does not support this!
                    if (UsdShadeInput connectedInput = _source.GetInput(
                            _sourceName)) {
                        connectedInput.Get(&fallback);
                    }
                } else {
                    usdShadeInput.Get(&fallback);
                }
            }

            // If the fallback input doesn't exist on the UsdShader, get the 
            // fallback value from the corresponding shader input in the 
            // registry. 
            if (fallback.IsEmpty()) {
                fallback = defaultInput->GetDefaultValue();
            }

            // If the default input has no default value, get a fallback value 
            // from the sdf typename?
            if (fallback.IsEmpty() && usdShadeInput) {
                fallback = 
                    defaultInput->GetTypeAsSdfType().first.GetDefaultValue();
            }
        }
    }
    return fallback;
}

void
_ShaderNetworkWalker::_ProcessPrimvarInput(
    const TfToken &primvarInputName, 
    const UsdShadeShader &shader, 
    const SdrShaderNodeConstPtr &sdrNode,
    TfTokenVector *primvars,
    TfTokenVector *varNames)
{
    UsdShadeInput usdPrimvarInput = 
            shader.GetInput(primvarInputName);
    SdrShaderPropertyConstPtr sdrPrimvarInput = 
            sdrNode ? sdrNode->GetShaderInput(primvarInputName)
                    : nullptr;
    TfToken varname;
    if (usdPrimvarInput) {
        if (usdPrimvarInput.GetConnectedSource(&_source,
                    &_sourceName, &_sourceType)) {
            if (UsdShadeInput connectedInput = _source.GetInput(
                    _sourceName)) {
                connectedInput.Get(&varname);
            }
        } else {
            usdPrimvarInput.Get(&varname);
        }
    }

    if (sdrPrimvarInput && varname.IsEmpty()) {
        VtValue defValue = sdrPrimvarInput->GetDefaultValue();
        if (defValue.IsHolding<TfToken>()) {
            varname = defValue.UncheckedGet<TfToken>();
        } else if (defValue.IsHolding<std::string>()) {
            varname = TfToken(defValue.UncheckedGet<std::string>());
        }
    }

    // Track this primvar as this shader accesses mesh data.
    if (!varname.IsEmpty()) {
        if (varNames) {
            varNames->push_back(varname);
        }

        // If the primvar accesses mesh data, we store it in the 
        // array of primvars that the material will return to inform
        // the meshes of the information it needs.
        if (std::find(primvars->begin(), primvars->end(), varname) == 
                primvars->end()) {
            primvars->push_back(varname);
            TF_DEBUG(USDIMAGING_SHADERS).Msg(
                "\t\tFound primvar: <%s>\n", varname.GetText());
        }
    }
}

void
UsdImagingGLHydraMaterialAdapter::_WalkShaderNetwork(
    UsdPrim const &shaderPrim,
    SdfPathVector *textureIDs,
    TfTokenVector *primvars,
    HdMaterialParamVector *materialParams,
    TfToken *materialTag) const
{
    _ShaderNetworkWalker(
        shaderPrim, 
        textureIDs, 
        primvars, 
        materialParams,
        materialTag,
        [this](const SdfPath &path) { return this->_GetPrim(path); });
}

HdTextureResource::ID
UsdImagingGLHydraMaterialAdapter::GetTextureResourceID(UsdPrim const& usdPrim,
                                                       SdfPath const &id,
                                                       UsdTimeCode time,
                                                       size_t salt) const
{
    return UsdImagingGL_GetTextureResourceID(usdPrim, id, time, salt);
}

HdTextureResourceSharedPtr
UsdImagingGLHydraMaterialAdapter::GetTextureResource(UsdPrim const& usdPrim,
                                                     SdfPath const &id,
                                                     UsdTimeCode time) const
{
    return UsdImagingGL_GetTextureResource(usdPrim, id, time);
}

PXR_NAMESPACE_CLOSE_SCOPE
