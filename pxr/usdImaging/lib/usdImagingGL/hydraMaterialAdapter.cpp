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
#include "pxr/usdImaging/usdImagingGL/textureUtils.h"

#include "pxr/usdImaging/usdImaging/debugCodes.h"
#include "pxr/usdImaging/usdImaging/delegate.h"
#include "pxr/usdImaging/usdImaging/tokens.h"

#include "pxr/imaging/glf/glslfx.h"
#include "pxr/imaging/glf/ptexTexture.h"

#include "pxr/imaging/hd/material.h"
#include "pxr/imaging/hd/tokens.h"

#include "pxr/usd/usdShade/connectableAPI.h"


PXR_NAMESPACE_OPEN_SCOPE

TF_DEFINE_PRIVATE_TOKENS(
    _tokens,
    (surfaceShader)
    (displacementShader)
    ((filename, "info:filename"))
    (HwPtexTexture_1)
    (HwUvTexture_1)
    (HwPrimvar_1)
    ((varname, "info:varname"))
    (uv)
    (faceIndexPrimvar)
    (faceOffsetPrimvar)
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

bool
UsdImagingGLHydraMaterialAdapter::IsPopulatedIndirectly()
{
    // Materials are populated as a consequence of populating a prim
    // which uses the material.
    return true;
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

    index->InsertSprim(HdPrimTypeTokens->material,
                       cachePath,
                       prim, shared_from_this());
    HD_PERF_COUNTER_INCR(UsdImagingTokens->usdPopulatedPrimCount);

    if (index->IsBprimTypeSupported(HdPrimTypeTokens->texture)) {

        // Extract the textures from the graph of this material.
        SdfPathVector textures;
        TfTokenVector primvars;
        HdMaterialParamVector params;

        UsdPrim surfaceShaderPrim;
        _GatherMaterialData(prim, &surfaceShaderPrim, &textures, &primvars, 
                            &params);

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

/* virtual */
void
UsdImagingGLHydraMaterialAdapter::TrackVariability(UsdPrim const& prim,
                                          SdfPath const& cachePath,
                                          HdDirtyBits* timeVaryingBits,
                                          UsdImagingInstancerContext const*
                                              instancerContext) const
{
    if (IsChildPath(cachePath)) {
        // Textures aren't time-varying.
        return;
    }

    UsdPrim surfaceShaderPrim = _GetSurfaceShaderPrim(UsdShadeMaterial(prim));
    if (!surfaceShaderPrim)
        return;

    // XXX: This is terrifying. Run through all attributes of the prim,
    // and if any are time varying, assume all shader params are time-varying.
    const std::vector<UsdAttribute> &attrs = surfaceShaderPrim.GetAttributes();
    TF_FOR_ALL(attrIter, attrs) {
        const UsdAttribute& attr = *attrIter;
        if (attr.GetNumTimeSamples()>1){
            *timeVaryingBits |= HdMaterial::DirtyParams;
        }
    }
}

static bool
_IsLegacyTextureOrPrimvarInput(const UsdShadeInput &shaderInput)
{
    UsdAttribute attr = shaderInput.GetAttr();

    TfToken baseName = attr.GetBaseName();
    return  attr.SplitName().size() >= 2 && 
            (baseName =="texture" || baseName=="primvar");
}

// XXX : This should use the shader node registry
static TfToken 
GetFilenameInput(TfToken const& id)
{
    return _tokens->filename;
}

// XXX : This should use the shader node registry
static bool
IsPtexTexture(TfToken const& id)
{
    return (id == _tokens->HwPtexTexture_1);
}

// XXX : This should use the shader node registry
static bool
IsTextureFamilyNode(TfToken const& id)
{
    return (id == _tokens->HwUvTexture_1 || 
            id == _tokens->HwPtexTexture_1);
}

// XXX : This should use the shader node registry
static bool
IsPrimvarFamilyNode(TfToken const& id)
{
    return (id == _tokens->HwPrimvar_1);
}

// XXX : This should use the shader node registry
static TfTokenVector
GetPrimvars(TfToken const& id)
{
    TfTokenVector t;
    if (id == _tokens->HwPrimvar_1){
        t.push_back(_tokens->varname);
    } else if(id == _tokens->HwUvTexture_1) {
        t.push_back(_tokens->uv);
    } else if(id == _tokens->HwPtexTexture_1) {
        t.push_back(_tokens->faceIndexPrimvar);
        t.push_back(_tokens->faceOffsetPrimvar);
    }
    return t;
}

static
UsdPrim
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

UsdPrim
UsdImagingGLHydraMaterialAdapter::_GetSurfaceShaderPrim(
    const UsdShadeMaterial &material) const
{
    // Determine the path to the preview shader and return it.
    if (UsdShadeShader glslfxSurface =  material.ComputeSurfaceSource(
            /* purpose */GlfGLSLFXTokens->glslfx)) {
        TF_DEBUG(USDIMAGING_SHADERS).Msg("\t GLSLFX surface: %s\n", 
            glslfxSurface.GetPath().GetText());            
        return glslfxSurface.GetPrim();
    }

    return _GetDeprecatedSurfaceShaderPrim(material);
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
    SdfPathVector textures;
    TfTokenVector primvars;
    HdMaterialParamVector params;

    if (requestedBits & HdMaterial::DirtySurfaceShader ||
        requestedBits & HdMaterial::DirtyParams) 
    {
        _GatherMaterialData(prim, &surfaceShaderPrim, &textures, &primvars, 
                            &params);
    }

    UsdImagingValueCache* valueCache = _GetValueCache();
    if (requestedBits & HdMaterial::DirtySurfaceShader) {
        std::string surfaceSource;
        std::string displacementSource;

        if (surfaceShaderPrim) {
            surfaceSource = _GetShaderSource(surfaceShaderPrim, 
                                             _tokens->surfaceShader);
            displacementSource = _GetShaderSource(surfaceShaderPrim,   
                                                  _tokens->displacementShader);
        }

        // DirtySurfaceShader triggers a refresh of both shader sources.
        valueCache->GetSurfaceShaderSource(cachePath) = surfaceSource;
        valueCache->GetDisplacementShaderSource(cachePath) = displacementSource;

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
    TfToken const& shaderType) const
{
    UsdAttribute srcAttr;
    if (UsdShadeShader shader = UsdShadeShader(shaderPrim)) {
        TfToken filename = GetFilenameInput(shaderType);
        srcAttr = shader.GetInput(filename);
        TF_DEBUG(USDIMAGING_SHADERS).Msg("Loading UsdShade shader: %s\n",
                    srcAttr.GetPath().GetText());
    } else {
        // ------------------------------------------------------------------ //
        // Deprecated
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
UsdImagingGLHydraMaterialAdapter::_GetMaterialParamValue(
    UsdPrim const &shaderPrim, 
    TfToken const &paramName,
    UsdTimeCode time) const
{
    VtValue value;

    if (UsdShadeShader shader = UsdShadeShader(shaderPrim)){
        UsdShadeInput shaderInput = shader.GetInput(paramName);
        if (TF_VERIFY(shaderInput)) {
            shaderInput.Get(&value, time);
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

void
UsdImagingGLHydraMaterialAdapter::_GatherMaterialData(
    UsdPrim const &materialPrim,
    UsdPrim *shaderPrim,
    SdfPathVector *textureIDs,
    TfTokenVector *primvars,
    HdMaterialParamVector *params) const
{
    TF_DEBUG(USDIMAGING_SHADERS).Msg("Material caching : <%s>\n", 
        materialPrim.GetPath().GetText());

    *shaderPrim = _GetSurfaceShaderPrim(UsdShadeMaterial(materialPrim));
    if (*shaderPrim) {
        TF_DEBUG(USDIMAGING_SHADERS).Msg("- found surface shader: <%s>\n",
            shaderPrim->GetPath().GetText());
    } else {
        TF_DEBUG(USDIMAGING_SHADERS).Msg("- No valid surface shader!\n");
        return;
    }

    if (UsdShadeShader s = UsdShadeShader(*shaderPrim)) {
        _WalkShaderNetwork(*shaderPrim, textureIDs, primvars, params);
    } else {
        _WalkShaderNetworkDeprecated(*shaderPrim, textureIDs, primvars, 
                                     params);
    }
}

void
UsdImagingGLHydraMaterialAdapter::_WalkShaderNetworkDeprecated(
    UsdPrim const &shaderPrim,
    SdfPathVector *textureIDs,
    TfTokenVector *primvars,
    HdMaterialParamVector *materialParams) const
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

        VtValue fallbackValue;
        SdfPath connection;
        TfTokenVector samplerCoords;
        bool isPtex = false;
        TfToken t;

        if (!TF_VERIFY(attr.Get(&fallbackValue),
                    "No fallback value for: <%s>\n",
                    attr.GetPath().GetText())) {
            continue;
        }

        if (UsdAttribute texAttr = shaderPrim.GetAttribute(
                                        TfToken(attr.GetPath().GetName() 
                                                + ":texture"))) {
            connection = texAttr.GetPath();
            textureIDs->push_back(connection);

            TF_DEBUG(USDIMAGING_SHADERS).Msg(
                "\t\tFound texture: <%s>\n", connection.GetText());

            SdfAssetPath ap;
            texAttr.Get(&ap, UsdTimeCode::Default());

            isPtex = GlfIsSupportedPtexTexture(TfToken(ap.GetAssetPath()));
            if (isPtex) {
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
            connection = SdfPath("primvar."
                                + pvAttr.GetName().GetString());
            TF_DEBUG(USDIMAGING_SHADERS).Msg(
                "\t\tFound primvar: <%s>\n", connection.GetText());

            if (TF_VERIFY(pvAttr.Get(&t, UsdTimeCode::Default()))) {
                primvars->push_back(t);
                TF_DEBUG(USDIMAGING_SHADERS).Msg(
                    "\t\t\tFound primvar: <%s>\n", t.GetText());
                samplerCoords.push_back(t);
                TF_DEBUG(USDIMAGING_SHADERS).Msg(
                    "\t\t\tFound sampler: <%s>\n", t.GetText());
            }
        }

        shaderInput.Get(&fallbackValue);
        materialParams->push_back(HdMaterialParam(shaderInput.GetBaseName(),
                                  fallbackValue,
                                  connection,
                                  samplerCoords,
                                  isPtex));
    }
}

void
UsdImagingGLHydraMaterialAdapter::_WalkShaderNetwork(
    UsdPrim const &shaderPrim,
    SdfPathVector *textureIDs,
    TfTokenVector *primvars,
    HdMaterialParamVector *materialParams) const
{
    UsdShadeShader shader(shaderPrim);

    // Vector used to walk the graph iteratively.
    SdfPathVector stack(1, shader.GetPath());

    // Internal data structure to keep the parameters organize before 
    // we return them in the actual HdMaterialParamVector.
    struct _MaterialParams {
        TfToken _name;
        VtValue _fallbackValue;
        SdfPath _connection;
        SdfPath _connectionPrimvar;
        TfTokenVector _samplerCoords;
        bool _isPtex;
    };
    std::vector<_MaterialParams> params;
    TfToken id;
    bool hasId;
    bool isRootNode = true;

    // Iteratively walk the graph visiting each node and collecting
    // textures, primvars and material parameters
    while (!stack.empty()) {
        SdfPath shaderPath = stack.back();
        stack.pop_back();
        shader = UsdShadeShader(_GetPrim(shaderPath));

        // Extract the id of the node
        UsdAttribute attr = shader.GetIdAttr();
        hasId = attr.Get(&id);

        TF_DEBUG(USDIMAGING_SHADERS).Msg("\tEvaluating node : <%s>\n",
            shader.GetPath().GetText());

        // For preview materials Hydra Stream material the current
        // assumption is that we have a root material which is typically the
        // first node. This node has a bunch of inputs that can be pointing
        // to a texture or a primvar (or a default value). 
        // The current algorithm is made exclusively to walk this basic
        // materials.

        // XXX : Currently, we identify the root node because it is
        // the first node, in the future this assumption 
        // needs to be revisited.
        if (isRootNode) {
            isRootNode = false;

            for (UsdShadeInput shaderInput: shader.GetInputs()) {
                if (_IsLegacyTextureOrPrimvarInput(shaderInput)) {
                    continue;
                }

                // Extract the fallback value for this input
                VtValue fallbackValue;
                shaderInput.Get(&fallbackValue);

                SdfPath connection;
                UsdShadeConnectableAPI source;
                TfToken outputName;
                UsdShadeAttributeType sourceType;
                if (UsdShadeConnectableAPI::GetConnectedSource(
                    shaderInput, &source, &outputName, &sourceType)) {
                    connection = source.GetPath();
                }

                // Finally, initialize data for this potential input to the 
                // material we are loading.
                _MaterialParams matParam = { shaderInput.GetBaseName(),/*name*/
                                             fallbackValue,/*fallbackValue*/
                                             connection,/*_connection*/
                                             SdfPath(), /*_connectionPrimvar*/
                                             TfTokenVector(), /*_samplerCoords*/
                                             false /*_isPtex*/};
                params.push_back(matParam);

                TF_DEBUG(USDIMAGING_SHADERS).Msg(
                    "\t\tAdding attribute : <%s> at <%s>\n", 
                    shaderInput.GetBaseName().GetText(),
                    connection.GetText());
            }
        } else if (hasId) {
            // For nodes with ids we can actually detect if they are 
            // primvars or textures and add them to the pipeline.
            if (IsTextureFamilyNode(id)) {
                TfToken filename = GetFilenameInput(id);

                // Extract the filename from the shader node
                // and store the paths in the texture array.
                UsdAttribute a = shader.GetInput(filename);
                SdfPath connection = a.GetPath();
                textureIDs->push_back(connection);

                TF_DEBUG(USDIMAGING_SHADERS).Msg(
                    "\t\tFound texture: <%s>\n", connection.GetText());

                bool isPtex = false;
                SdfPath connectionPrimvar;
                if (IsPtexTexture(id)){
                    isPtex = true;
                } else {
                    // For regular textures we need to resolve what node
                    // will be providing the texture coordinates.
                    TfTokenVector primvarsInputsInNode = GetPrimvars(id);
                    for (auto const & input : primvarsInputsInNode ) {
                        if (UsdShadeInput uv = shader.GetInput(input)) {
                            UsdShadeConnectableAPI uvSource;
                                UsdShadeConnectableAPI source;
                            TfToken outputName;
                            UsdShadeAttributeType sourceType;
                            if (UsdShadeConnectableAPI::
                                    GetConnectedSource(uv, &uvSource, 
                                                        &outputName,
                                                        &sourceType)) {
                                connectionPrimvar = uvSource.GetPath();
                            }
                        }
                    }
                }

                for(auto &p : params) {
                    if (p._connection == shader.GetPath()){
                        p._isPtex = isPtex;
                        p._connectionPrimvar = connectionPrimvar;
                        p._connection = connection;
                    }
                }
            } else if (IsPrimvarFamilyNode(id)) {
                SdfPath connection = SdfPath("primvar." 
                                    + shader.GetPrim()
                                            .GetName()
                                            .GetString());

                // Primvars can be providing data to an input to the material
                // or to a texture. We need this distinction in our current
                // design of HdMaterialParam.
                TfTokenVector primvarsInputsInNode = GetPrimvars(id);
                TfToken varname;
                for (auto const& input : primvarsInputsInNode ) {
                    UsdAttribute pv = shader.GetInput(input);
                    if (pv.Get(&varname, UsdTimeCode::Default())) {
                        for(auto &p : params) {
                            if (p._connectionPrimvar == shader.GetPath()){
                                TF_DEBUG(USDIMAGING_SHADERS).Msg(
                                    "\t\tPrimvar connected: <%s>\n", 
                                    varname.GetText());
                                p._samplerCoords.push_back(varname);
                            } else if (p._connection == shader.GetPath()){
                                TF_DEBUG(USDIMAGING_SHADERS).Msg(
                                    "\t\tPrimvar connected: <%s>\n", 
                                    varname.GetText());
                                p._connection = connection;
                                p._samplerCoords.push_back(varname);
                            }
                        }
                    }
                }
            }

            // Extract primvars used by this node. 
            // We need the actual primvar names , so we need 
            // to resolve the inputs.
            TfToken varname;
            TfTokenVector primvarsInputsInNode = GetPrimvars(id);
            for (auto const &input : primvarsInputsInNode ) {
                if (input == UsdImagingTokens->faceIndexPrimvar) {
                    // Special handling for ptex primvar redirection.
                    TfToken faceIndexPrimvarName = 
                        attr.GetMetadata(
                            UsdImagingTokens->faceIndexPrimvar,&varname)
                        ? varname : UsdImagingTokens->ptexFaceIndex;
                    primvars->push_back(faceIndexPrimvarName);

                    TF_DEBUG(USDIMAGING_SHADERS).Msg(
                        "\t\tFound primvar: <%s>\n", 
                        primvars->back().GetText());
                } else if (input == UsdImagingTokens->faceOffsetPrimvar) {
                    // Special handling for ptex primvar redirection.
                    TfToken faceOffsetPrimvarName = 
                        attr.GetMetadata(
                            UsdImagingTokens->faceOffsetPrimvar, &varname)
                        ? varname : UsdImagingTokens->ptexFaceOffset;
                    primvars->push_back(faceOffsetPrimvarName);

                    TF_DEBUG(USDIMAGING_SHADERS).Msg(
                        "\t\tFound primvar: <%s>\n", 
                        primvars->back().GetText());
                } else {
                    UsdAttribute pv = shader.GetInput(input);
                    if (pv.Get(&varname, UsdTimeCode::Default())) {
                        primvars->push_back(varname);

                        TF_DEBUG(USDIMAGING_SHADERS).Msg(
                            "\t\tFound primvar: <%s>\n", 
                            primvars->back().GetText());
                    }
                }
            }
        }

        // Add nodes to the stack to keep walking the graph
        for (UsdShadeInput const & shaderInput: shader.GetInputs()) {
            if (_IsLegacyTextureOrPrimvarInput(shaderInput)) {
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

    // Fill the material parameters structure with all the information
    // we have compiled after walking the material.
    for(_MaterialParams const & param : params) {
        materialParams->emplace_back(param._name, param._fallbackValue, 
                param._connection, param._samplerCoords, param._isPtex);
    }
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
