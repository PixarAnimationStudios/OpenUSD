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
#include "pxr/usdImaging/usdImagingGL/drawModeAdapter.h"
#include "pxr/usdImaging/usdImagingGL/package.h"

#include "pxr/usdImaging/usdImaging/gprimAdapter.h"
#include "pxr/usdImaging/usdImaging/indexProxy.h"
#include "pxr/usdImaging/usdImaging/instancerContext.h"
#include "pxr/usdImaging/usdImaging/tokens.h"

#include "pxr/imaging/hd/material.h"

#include "pxr/imaging/hio/glslfx.h"
#include "pxr/imaging/hio/image.h"
#include "pxr/imaging/pxOsd/tokens.h"

#include "pxr/usd/usdGeom/modelAPI.h"
#include "pxr/usd/sdr/registry.h"
#include "pxr/usd/sdr/shaderNode.h"

#include "pxr/base/gf/matrix4f.h"
#include "pxr/base/tf/type.h"

PXR_NAMESPACE_OPEN_SCOPE

TF_DEFINE_PRIVATE_TOKENS(
    _tokens,

    (material)

    (cardsUv)
    (cardsTexAssign)

    (textureXPosColor)
    (textureYPosColor)
    (textureZPosColor)
    (textureXNegColor)
    (textureYNegColor)
    (textureZNegColor)
    (textureXPosOpacity)
    (textureYPosOpacity)
    (textureZPosOpacity)
    (textureXNegOpacity)
    (textureYNegOpacity)
    (textureZNegOpacity)

    (worldtoscreen)

    (displayRoughness)

    (file)
    (st)
    (rgb)
    (a)
    (fallback)
    (minFilter)
    (magFilter)
    (linear)
    (linearMipmapLinear)

    (varname)
    (result)
    (activeTexCard)
);

namespace {
    enum AxesMask : uint8_t {
        xPos = (1 << 0),
        yPos = (1 << 1),
        zPos = (1 << 2),
        xNeg = (1 << 3),
        yNeg = (1 << 4),
        zNeg = (1 << 5),
        xAxis = (xPos | xNeg),
        yAxis = (yPos | yNeg),
        zAxis = (zPos | zNeg),
    };
}

TF_REGISTRY_FUNCTION(TfType)
{
    typedef UsdImagingGLDrawModeAdapter Adapter;
    TfType t = TfType::Define<Adapter, TfType::Bases<Adapter::BaseAdapter> >();
    t.SetFactory< UsdImagingPrimAdapterFactory<Adapter> >();
}

static SdfPath
_GetMaterialPath(UsdPrim const& prim)
{
    const SdfPath matPath = SdfPath(_tokens->material.GetString());
    return prim.GetPath().AppendPath(matPath);
}

UsdImagingGLDrawModeAdapter::UsdImagingGLDrawModeAdapter()
    : UsdImagingPrimAdapter()
    , _schemaColor(0)
{
    // Look up the default color in the schema registry.
    const UsdPrimDefinition *primDef =
        UsdSchemaRegistry::GetInstance()
        .FindAppliedAPIPrimDefinition(TfToken("GeomModelAPI"));
    if (primDef) {
        primDef->GetAttributeFallbackValue(
            UsdGeomTokens->modelDrawModeColor, &_schemaColor);
    }
}

UsdImagingGLDrawModeAdapter::~UsdImagingGLDrawModeAdapter()
{
}

bool
UsdImagingGLDrawModeAdapter::ShouldCullChildren() const
{
    return true;
}

bool
UsdImagingGLDrawModeAdapter::CanPopulateUsdInstance() const
{
    return true;
}

bool
UsdImagingGLDrawModeAdapter::IsSupported(
    UsdImagingIndexProxy const* index) const
{
    return true;
}

SdfPath
UsdImagingGLDrawModeAdapter::Populate(UsdPrim const& prim,
                                    UsdImagingIndexProxy* index,
                                    UsdImagingInstancerContext const*
                                       instancerContext)
{
    SdfPath cachePath = UsdImagingGprimAdapter::_ResolveCachePath(
        prim.GetPath(), instancerContext);

    // The draw mode adapter only supports models or unloaded prims.
    // This is enforced in UsdImagingDelegate::_IsDrawModeApplied.
    if (!TF_VERIFY(prim.IsModel() || !prim.IsLoaded(), "<%s>",
                   prim.GetPath().GetText())) {
        return SdfPath();
    }

    // There should have been a non-default draw mode applied for this
    // adapter to be called; this is enforced in
    // UsdImagingDelegate::_IsDrawModeApplied.
    TfToken drawMode = GetModelDrawMode(prim);
    if (drawMode == UsdGeomTokens->default_ && instancerContext) {
        drawMode = instancerContext->instanceDrawMode;
    }
    if (!TF_VERIFY(drawMode != UsdGeomTokens->default_, "<%s>",
        prim.GetPath().GetText())) {
        return SdfPath();
    }

    // If this object is instanced, we need to use the instancer adapter for the
    // rprim, which will forward to the draw mode adapter but additionally
    // handle instancer attributes like instance index.
    UsdImagingPrimAdapterSharedPtr rprimAdapter =
        (instancerContext && instancerContext->instancerAdapter) ?
        instancerContext->instancerAdapter :
        shared_from_this();

    // If this prim isn't instanced, cachePrim will be the same as "prim", but
    // if it is instanced the instancer adapters expect us to pass in this
    // prim, which should point to the instancer.
    UsdPrim cachePrim = _GetPrim(cachePath.GetAbsoluteRootOrPrimPath());

    if (drawMode == UsdGeomTokens->origin ||
        drawMode == UsdGeomTokens->bounds) {
        // Origin and bounds both draw as basis curves
        if (!index->IsRprimTypeSupported(HdPrimTypeTokens->basisCurves)) {
            TF_WARN("Unable to display origin or bounds draw mode for model "
                    "%s, basis curves not supported", cachePath.GetText());
            return SdfPath();
        }
        index->InsertRprim(HdPrimTypeTokens->basisCurves,
            cachePath, cachePrim, rprimAdapter);
        HD_PERF_COUNTER_INCR(UsdImagingTokens->usdPopulatedPrimCount);
    } else if (drawMode == UsdGeomTokens->cards) {
        // Cards draw as a mesh
        if (!index->IsRprimTypeSupported(HdPrimTypeTokens->mesh)) {
            TF_WARN("Unable to display cards draw mode for model %s, "
                    "meshes not supported", cachePath.GetText());
            return SdfPath();
        }
        index->InsertRprim(HdPrimTypeTokens->mesh,
            cachePath, cachePrim, rprimAdapter);
        HD_PERF_COUNTER_INCR(UsdImagingTokens->usdPopulatedPrimCount);
    } else {
        TF_CODING_ERROR("Model <%s> has unsupported drawMode '%s'",
            prim.GetPath().GetText(), drawMode.GetText());
        return SdfPath();
    }

    // As long as we're passing cachePrim to InsertRprim, we need to fix up
    // the dependency map ourselves. For USD edit purposes, we depend on the
    // prototype prim ("prim"), rather than the instancer prim.
    // See similar code in GprimAdapter::_AddRprim.
    if (instancerContext != nullptr) {
        index->RemovePrimInfoDependency(cachePath);
        index->AddDependency(cachePath, prim);
    }

    // Additionally, insert the material.
    SdfPath materialPath = _GetMaterialPath(prim);
    if (index->IsSprimTypeSupported(HdPrimTypeTokens->material) &&
        !index->IsPopulated(materialPath)) {
        index->InsertSprim(HdPrimTypeTokens->material,
            materialPath, prim, shared_from_this());
        HD_PERF_COUNTER_INCR(UsdImagingTokens->usdPopulatedPrimCount);
    }

    // Record the drawmode for use in UpdateForTime().
    _drawModeMap.insert(std::make_pair(cachePath, drawMode));

    // Record the material for use in remove/resync.
    _materialMap.insert(std::make_pair(cachePath, materialPath));

    return cachePath;
}

bool
UsdImagingGLDrawModeAdapter::_IsMaterialPath(SdfPath const& path) const
{
    return path.GetNameToken() == _tokens->material;
}

void
UsdImagingGLDrawModeAdapter::ProcessPrimResync(SdfPath const& cachePath,
        UsdImagingIndexProxy* index)

{
    if (cachePath.GetNameToken() == _tokens->material) {
        // Ignore a resync of the material on the theory that the rprim resync
        // will take care of it.
        return;
    }

    ProcessPrimRemoval(cachePath, index);

    // XXX(UsdImagingPaths): We use the cachePath directly here,
    // same as PrimAdapter::ProcessPrimResync.  Its use is questionable.
    // Instanced cards prims should be removed, never resynced, since they are
    // repopulated by instancer population loops, so this is probably ok?
    index->Repopulate(cachePath);
}

void
UsdImagingGLDrawModeAdapter::ProcessPrimRemoval(SdfPath const& cachePath,
        UsdImagingIndexProxy* index)
{
    if (cachePath.GetNameToken() == _tokens->material) {
        // Ignore a removal of the material on the theory that the rprim removal
        // will take care of it.
        return;
    }

    // Remove the material
    _MaterialMap::const_iterator it = _materialMap.find(cachePath);
    if (it != _materialMap.end()) {
        index->RemoveSprim(HdPrimTypeTokens->material, it->second);
        _materialMap.erase(it);
    }

    // Remove the rprim
    _drawModeMap.erase(cachePath);
    index->RemoveRprim(cachePath);
}

void
UsdImagingGLDrawModeAdapter::_RemovePrim(SdfPath const& cachePath,
                                       UsdImagingIndexProxy* index)
{
    TF_CODING_ERROR("_RemovePrim called on draw mode adapter!");
}

void
UsdImagingGLDrawModeAdapter::MarkDirty(UsdPrim const& prim,
                                     SdfPath const& cachePath,
                                     HdDirtyBits dirty,
                                     UsdImagingIndexProxy* index)
{
    if (_IsMaterialPath(cachePath)) {
        index->MarkSprimDirty(cachePath, dirty);
    } else {
        index->MarkRprimDirty(cachePath, dirty);
    }
}

void
UsdImagingGLDrawModeAdapter::MarkTransformDirty(UsdPrim const& prim,
                                              SdfPath const& cachePath,
                                              UsdImagingIndexProxy* index)
{
    if (!_IsMaterialPath(cachePath)) {
        index->MarkRprimDirty(cachePath, HdChangeTracker::DirtyTransform);
    }
}

void
UsdImagingGLDrawModeAdapter::MarkVisibilityDirty(UsdPrim const& prim,
                                               SdfPath const& cachePath,
                                               UsdImagingIndexProxy* index)
{
    if (!_IsMaterialPath(cachePath)) {
        index->MarkRprimDirty(cachePath, HdChangeTracker::DirtyVisibility);
    }
}

void
UsdImagingGLDrawModeAdapter::MarkMaterialDirty(UsdPrim const& prim,
                                               SdfPath const& cachePath,
                                               UsdImagingIndexProxy* index)
{
    if (_IsMaterialPath(cachePath)) {
        index->MarkSprimDirty(cachePath, HdMaterial::DirtyResource);
    } else {
        // If the Usd material changed, it could mean the primvar set also
        // changed Hydra doesn't currently manage detection and propagation of
        // these changes, so we must mark the rprim dirty.
        index->MarkRprimDirty(cachePath, HdChangeTracker::DirtyMaterialId);
    }
}

bool
UsdImagingGLDrawModeAdapter::_HasVaryingExtent(UsdPrim const& prim) const
{
    UsdAttribute attr;

    attr = prim.GetAttribute(UsdGeomTokens->extent);
    if (attr && attr.ValueMightBeTimeVarying())
        return true;

    attr = prim.GetAttribute(UsdGeomTokens->extentsHint);
    if (attr && attr.ValueMightBeTimeVarying())
        return true;

    return false;
}

void
UsdImagingGLDrawModeAdapter::_ComputeGeometryData(
    UsdPrim const& prim,
    SdfPath const& cachePath,
    UsdTimeCode time,
    TfToken const& drawMode,
    VtValue* topology, 
    VtValue* points, 
    GfRange3d* extent,
    VtValue* uv,
    VtValue* assign) const
{
    if (drawMode == UsdGeomTokens->origin) {
        *extent = _ComputeExtent(prim,
            _HasVaryingExtent(prim) ? time : UsdTimeCode::EarliestTime());
        _GenerateOriginGeometry(topology, points, *extent);

    } else if (drawMode == UsdGeomTokens->bounds) {
        *extent = _ComputeExtent(prim,
            _HasVaryingExtent(prim) ? time : UsdTimeCode::EarliestTime());
        _GenerateBoundsGeometry(topology, points, *extent);

    } else if (drawMode == UsdGeomTokens->cards) {
        UsdGeomModelAPI model(prim);
        TfToken cardGeometry = UsdGeomTokens->cross;
        if (model) {
            model.GetModelCardGeometryAttr().Get(&cardGeometry);
        }

        if (cardGeometry == UsdGeomTokens->fromTexture) {
            // In "fromTexture" mode, read all the geometry data in from
            // the textures.
            _GenerateCardsFromTextureGeometry(topology, points,
                    uv, assign, extent, prim);

        } else {
            // First compute the extents.
            *extent = _ComputeExtent(prim,
                _HasVaryingExtent(prim) ? time : UsdTimeCode::EarliestTime());

            // Generate mask for suppressing axes with no textures
            uint8_t axes_mask = 0;

            if (model) {
                const TfToken textureAttrs[6] = {
                    UsdGeomTokens->modelCardTextureXPos,
                    UsdGeomTokens->modelCardTextureYPos,
                    UsdGeomTokens->modelCardTextureZPos,
                    UsdGeomTokens->modelCardTextureXNeg,
                    UsdGeomTokens->modelCardTextureYNeg,
                    UsdGeomTokens->modelCardTextureZNeg,
                };
                const uint8_t mask[6] = {
                    xPos, yPos, zPos, xNeg, yNeg, zNeg,
                };
                for (int i = 0; i < 6; ++i) {
                    SdfAssetPath asset;
                    prim.GetAttribute(textureAttrs[i]).Get(&asset, time);
                    if (!asset.GetAssetPath().empty()) {
                        axes_mask |= mask[i];
                    }
                }
            }

            // If no textures are bound, generate the full geometry.
            if (axes_mask == 0) { axes_mask = xAxis | yAxis | zAxis; }
    
            // Generate UVs.
            _GenerateTextureCoordinates(uv, assign, axes_mask);

            // Generate geometry based on card type.
            if (cardGeometry == UsdGeomTokens->cross) {
                _GenerateCardsCrossGeometry(topology, points, *extent,
                    axes_mask);
            } else if (cardGeometry == UsdGeomTokens->box) {
                _GenerateCardsBoxGeometry(topology, points, *extent,
                    axes_mask);
            } else {
                TF_CODING_ERROR("<%s> Unexpected card geometry mode %s",
                    cachePath.GetText(), cardGeometry.GetText());
            }

            // Issue warnings for zero-area faces that we're supposedly
            // drawing.
            _SanityCheckFaceSizes(cachePath, *extent, axes_mask);
        }
    } else {
        TF_CODING_ERROR("<%s> Unexpected draw mode %s",
            cachePath.GetText(), drawMode.GetText());
    }
}

/*virtual*/ 
VtValue
UsdImagingGLDrawModeAdapter::GetTopology(UsdPrim const& prim,
                                         SdfPath const& cachePath,
                                         UsdTimeCode time) const
{
    TRACE_FUNCTION();
    HF_MALLOC_TAG_FUNCTION();

    TfToken drawMode = UsdGeomTokens->default_;
    _DrawModeMap::const_iterator it = _drawModeMap.find(cachePath);
    if (TF_VERIFY(it != _drawModeMap.end())) {
        drawMode = it->second;
    }

    VtValue topology;
    VtValue points;
    VtValue uv;
    VtValue assign;
    GfRange3d extent;
    _ComputeGeometryData(prim, cachePath, time, drawMode, &topology, 
        &points, &extent, &uv, &assign);
    return topology;
}

/*virtual*/
GfRange3d 
UsdImagingGLDrawModeAdapter::GetExtent(UsdPrim const& prim, 
                                       SdfPath const& cachePath, 
                                       UsdTimeCode time) const
{
    TRACE_FUNCTION();
    HF_MALLOC_TAG_FUNCTION();

    TfToken drawMode = UsdGeomTokens->default_;
    _DrawModeMap::const_iterator it = _drawModeMap.find(cachePath);
    if (TF_VERIFY(it != _drawModeMap.end())) {
        drawMode = it->second;
    }

    VtValue topology;
    VtValue points;
    VtValue uv;
    VtValue assign;
    GfRange3d extent;
    _ComputeGeometryData(prim, cachePath, time, drawMode, &topology, 
        &points, &extent, &uv, &assign);
    return extent;
}


/*virtual*/
bool
UsdImagingGLDrawModeAdapter::GetDoubleSided(UsdPrim const& prim, 
                                            SdfPath const& cachePath, 
                                            UsdTimeCode time) const
{
    return false;
}

/*virtual*/
VtValue 
UsdImagingGLDrawModeAdapter::Get(UsdPrim const& prim, 
                                 SdfPath const& cachePath,
                                 TfToken const& key,
                                 UsdTimeCode time,
                                 VtIntArray *outIndices) const
{
    TRACE_FUNCTION();

    VtValue value;
    UsdGeomModelAPI model(prim);

    if (key == HdTokens->displayColor) {
        VtVec3fArray color = VtVec3fArray(1);
        GfVec3f drawModeColor;
        if (model) {
            model.GetModelDrawModeColorAttr().Get(&drawModeColor);
        } else {
            drawModeColor = _schemaColor;
        }

        color[0] = drawModeColor;
        value = color;

    } else if (key == HdTokens->displayOpacity) {
        VtFloatArray opacity = VtFloatArray(1);

        // Full opacity.
        opacity[0] = 1.0f;
        value = opacity;

    } else if (key == HdTokens->widths) {
        VtFloatArray widths = VtFloatArray(1);
        widths[0] = 1.0f;
        value = widths;

    } else if (key == HdTokens->points) {
        TRACE_FUNCTION_SCOPE("points");
        TfToken drawMode = UsdGeomTokens->default_;
        _DrawModeMap::const_iterator it = _drawModeMap.find(cachePath);
        if (TF_VERIFY(it != _drawModeMap.end())) {
            drawMode = it->second;
        }

        VtValue topology;
        VtValue points;
        VtValue uv;
        VtValue assign;
        GfRange3d extent;
        _ComputeGeometryData(prim, cachePath, time, drawMode, &topology, 
            &points, &extent, &uv, &assign);
        return points;

    } else if (key == _tokens->cardsUv) {
        TRACE_FUNCTION_SCOPE("cardsUV");
        TfToken drawMode = UsdGeomTokens->default_;
        _DrawModeMap::const_iterator it = _drawModeMap.find(cachePath);
        if (TF_VERIFY(it != _drawModeMap.end())) {
            drawMode = it->second;
        }

        VtValue topology;
        VtValue points;
        VtValue uv;
        VtValue assign;
        GfRange3d extent;
        _ComputeGeometryData(prim, cachePath, time, drawMode, &topology, 
            &points, &extent, &uv, &assign);
        return uv;

    } else if (key == _tokens->cardsTexAssign) {
        TRACE_FUNCTION_SCOPE("cardsTexAssign");
        TfToken drawMode = UsdGeomTokens->default_;
        _DrawModeMap::const_iterator it = _drawModeMap.find(cachePath);
        if (TF_VERIFY(it != _drawModeMap.end())) {
            drawMode = it->second;
        }

        VtValue topology;
        VtValue points;
        VtValue uv;
        VtValue assign;
        GfRange3d extent;
        _ComputeGeometryData(prim, cachePath, time, drawMode, &topology, 
            &points, &extent, &uv, &assign);
        return assign;

    } else if (key == _tokens->displayRoughness) {
        return VtValue(1.0f);
    }

    return value;
}

/*virtual*/
SdfPath
UsdImagingGLDrawModeAdapter::GetMaterialId(UsdPrim const& prim, 
                                           SdfPath const& cachePath, 
                                           UsdTimeCode time) const
{
    _MaterialMap::const_iterator it = _materialMap.find(cachePath);
    if (it != _materialMap.end()) {
        return it->second;
    }
    return SdfPath();
}

/*virtual*/
VtValue
UsdImagingGLDrawModeAdapter::GetMaterialResource(UsdPrim const& prim, 
                            SdfPath const& cachePath, 
                            UsdTimeCode time) const
{
    if (!_IsMaterialPath(cachePath)) {
        return BaseAdapter::GetMaterialResource(prim, cachePath, time);
    }

    UsdGeomModelAPI model(prim);

    SdfAssetPath path(UsdImagingGLPackageDrawModeShader());

    SdrRegistry &shaderReg = SdrRegistry::GetInstance();
    SdrShaderNodeConstPtr sdrNode = 
        shaderReg.GetShaderNodeFromAsset(
            path, 
            NdrTokenMap(), 
            TfToken(), 
            HioGlslfxTokens->glslfx);

    // An sdr node representing the drawCards.glslfx should be added
    // to the registry, so we don't expect this to fail.
    if (!TF_VERIFY(sdrNode)) {
        return VtValue();
    }

    // Generate material network with a terminal that points to
    // the DrawMode glslfx shader.
    TfToken const& terminalType = HdMaterialTerminalTokens->surface;
    HdMaterialNetworkMap networkMap;
    HdMaterialNetwork& network = networkMap.map[terminalType];
    HdMaterialNode terminal;
    terminal.path = cachePath;
    terminal.identifier = sdrNode->GetIdentifier();

    const TfToken textureNames[12] = {
        _tokens->textureXPosColor,
        _tokens->textureYPosColor,
        _tokens->textureZPosColor,
        _tokens->textureXNegColor,
        _tokens->textureYNegColor,
        _tokens->textureZNegColor,
        _tokens->textureXPosOpacity,
        _tokens->textureYPosOpacity,
        _tokens->textureZPosOpacity,
        _tokens->textureXNegOpacity,
        _tokens->textureYNegOpacity,
        _tokens->textureZNegOpacity
    };

    if (model) {
        const TfToken textureAttrs[6] = {
            UsdGeomTokens->modelCardTextureXPos,
            UsdGeomTokens->modelCardTextureYPos,
            UsdGeomTokens->modelCardTextureZPos,
            UsdGeomTokens->modelCardTextureXNeg,
            UsdGeomTokens->modelCardTextureYNeg,
            UsdGeomTokens->modelCardTextureZNeg,
        };

        GfVec3f drawModeColor;
        model.GetModelDrawModeColorAttr().Get(&drawModeColor);
        VtValue fallback = VtValue(GfVec4f(
            drawModeColor[0], drawModeColor[1], drawModeColor[2],
            1.0f));

        for (int i = 0; i < 6; ++i) {
            SdfAssetPath textureFile;
            prim.GetAttribute(textureAttrs[i]).Get(&textureFile, time);
            if (!textureFile.GetAssetPath().empty()) {
                SdfPath textureNodePath = _GetMaterialPath(prim)
                    .AppendProperty(textureAttrs[i]);

                // Make texture node
                HdMaterialNode textureNode;
                textureNode.path = textureNodePath;
                textureNode.identifier = UsdImagingTokens->UsdUVTexture;
                textureNode.parameters[_tokens->st] = _tokens->cardsUv;
                textureNode.parameters[_tokens->fallback] = fallback;
                textureNode.parameters[_tokens->file] = textureFile;
                textureNode.parameters[_tokens->minFilter] =
                    _tokens->linearMipmapLinear;
                textureNode.parameters[_tokens->magFilter] =
                    _tokens->linear;

                // Insert connection between texture node and terminal color
                // input
                HdMaterialRelationship colorRel;
                colorRel.inputId = textureNode.path;
                colorRel.inputName = _tokens->rgb;
                colorRel.outputId = terminal.path;
                colorRel.outputName = textureNames[i];
                network.relationships.emplace_back(std::move(colorRel));

                // Insert connection between texture node and terminal 
                // opacity input
                HdMaterialRelationship opacityRel;
                opacityRel.inputId = textureNode.path;
                opacityRel.inputName = _tokens->a;
                opacityRel.outputId = terminal.path;
                opacityRel.outputName = textureNames[i + 6];
                network.relationships.emplace_back(std::move(opacityRel));

                // Insert texture node
                network.nodes.emplace_back(std::move(textureNode));
            } else {
                terminal.parameters[textureNames[i]] = drawModeColor;
                terminal.parameters[textureNames[i + 6]] = VtValue(1.f);
            }
        }
    } else {
        for (int i = 0; i < 6; ++i) {
            terminal.parameters[textureNames[i]] = _schemaColor;
            terminal.parameters[textureNames[i + 6]] = VtValue(1.f);
        }
    }

    // Adding a primvar reader for the card assignment
    // Make primvar reader node
    SdfPath primvarNodePath = _GetMaterialPath(prim)
        .AppendProperty(_tokens->cardsTexAssign);
    HdMaterialNode primvarNode;
    primvarNode.path = primvarNodePath;
    primvarNode.identifier = UsdImagingTokens->UsdPrimvarReader_int;
    primvarNode.parameters[_tokens->varname] = _tokens->cardsTexAssign;
    primvarNode.parameters[_tokens->fallback] = VtValue(0);

    // Insert connection between primvar reader node and terminal
    HdMaterialRelationship relPrimvar;
    relPrimvar.inputId = primvarNode.path;
    relPrimvar.inputName = _tokens->result;
    relPrimvar.outputId = terminal.path;
    relPrimvar.outputName = _tokens->activeTexCard;
    network.relationships.emplace_back(std::move(relPrimvar));

    // Insert primvar reader node
    network.nodes.emplace_back(std::move(primvarNode));

    // Insert terminal and update material network
    networkMap.terminals.push_back(terminal.path);
    network.nodes.emplace_back(std::move(terminal));

    return VtValue(networkMap);
}

void
UsdImagingGLDrawModeAdapter::_CheckForTextureVariability(
    UsdPrim const& prim, HdDirtyBits dirtyBits,
    HdDirtyBits *timeVaryingBits) const
{
    const std::array<TfToken, 6> textureAttrs = {
        UsdGeomTokens->modelCardTextureXPos,
        UsdGeomTokens->modelCardTextureYPos,
        UsdGeomTokens->modelCardTextureZPos,
        UsdGeomTokens->modelCardTextureXNeg,
        UsdGeomTokens->modelCardTextureYNeg,
        UsdGeomTokens->modelCardTextureZNeg,
    };

    for (const TfToken& attr: textureAttrs) {
        if (_IsVarying(prim, attr, dirtyBits,
                       UsdImagingTokens->usdVaryingTexture,
                       timeVaryingBits, false)) {
            break;
        }
    }
}

void
UsdImagingGLDrawModeAdapter::TrackVariability(UsdPrim const& prim,
                                            SdfPath const& cachePath,
                                            HdDirtyBits* timeVaryingBits,
                                            UsdImagingInstancerContext const*
                                               instancerContext) const
{
    if (_IsMaterialPath(cachePath)) {
        _CheckForTextureVariability(prim, HdMaterial::DirtyResource,
                                    timeVaryingBits);
        return;
    }

    // Discover time-varying transforms. If this card is instantiated on an
    // instance, skip since the instance adapter will handle transforms
    // and master roots always have identity transform.
    if (!prim.IsInstance()) {
        _IsTransformVarying(prim,
            HdChangeTracker::DirtyTransform,
            UsdImagingTokens->usdVaryingXform,
            timeVaryingBits);
    }

    // Discover time-varying visibility.
    _IsVarying(prim,
            UsdGeomTokens->visibility,
            HdChangeTracker::DirtyVisibility,
            UsdImagingTokens->usdVaryingVisibility,
            timeVaryingBits,
            true);

    // Discover time-varying extents. Look for time samples on either the
    // extent or extentsHint attribute.
    if (!_IsVarying(prim,
            UsdGeomTokens->extent,
            HdChangeTracker::DirtyPoints | HdChangeTracker::DirtyExtent,
            UsdImagingTokens->usdVaryingExtent,
            timeVaryingBits,
            false)) {
        _IsVarying(prim,
            UsdGeomTokens->extentsHint,
            HdChangeTracker::DirtyPoints | HdChangeTracker::DirtyExtent,
            UsdImagingTokens->usdVaryingExtent,
            timeVaryingBits,
            false);
    }
}

void
UsdImagingGLDrawModeAdapter::UpdateForTime(UsdPrim const& prim,
                                         SdfPath const& cachePath,
                                         UsdTimeCode time,
                                         HdDirtyBits requestedBits,
                                         UsdImagingInstancerContext const*
                                            instancerContext) const
{
    if (_IsMaterialPath(cachePath)) {
        // The draw mode material doesn't make use of UpdateForTime.
        return;
    }

    UsdImagingPrimvarDescCache* primvarDescCache = _GetPrimvarDescCache();

    // Geometry aspect
    HdPrimvarDescriptorVector& primvars = 
        primvarDescCache->GetPrimvars(cachePath);

    if (requestedBits & HdChangeTracker::DirtyWidths) {
        _MergePrimvar(&primvars, UsdGeomTokens->widths,
                      HdInterpolationConstant);
    }

    if (requestedBits & HdChangeTracker::DirtyPrimvar) {
        _MergePrimvar(&primvars, HdTokens->displayColor,
                      HdInterpolationConstant, HdPrimvarRoleTokens->color);
        _MergePrimvar(&primvars, HdTokens->displayOpacity,
                      HdInterpolationConstant);
    }

    // We compute all of the below items together, since their derivations
    // aren't easily separable.
    HdDirtyBits geometryBits = HdChangeTracker::DirtyTopology |
                               HdChangeTracker::DirtyPoints |
                               HdChangeTracker::DirtyPrimvar |
                               HdChangeTracker::DirtyExtent;

    if (requestedBits & geometryBits) {
        TfToken drawMode = UsdGeomTokens->default_;
        _DrawModeMap::const_iterator it = _drawModeMap.find(cachePath);
        if (TF_VERIFY(it != _drawModeMap.end())) {
            drawMode = it->second;
        }

        VtValue topology;
        GfRange3d extent;
        VtValue points;
        VtValue uv;
        VtValue assign;
        _ComputeGeometryData(prim, cachePath, time, drawMode, &topology, 
            &points, &extent, &uv, &assign);

        if (drawMode == UsdGeomTokens->cards) {
            // Merge "cardsUv" and "cardsTexAssign" primvars
            _MergePrimvar(&primvars, _tokens->cardsUv,
                HdInterpolationVertex);
            _MergePrimvar(&primvars, _tokens->cardsTexAssign,
                HdInterpolationUniform);

            // XXX: backdoor into the material system.
            _MergePrimvar(&primvars, _tokens->displayRoughness, 
                HdInterpolationConstant);
        }

        // Merge "points" primvar
        _MergePrimvar(&primvars, 
            HdTokens->points,
            HdInterpolationVertex,
            HdPrimvarRoleTokens->point);
    }
}

HdDirtyBits
UsdImagingGLDrawModeAdapter::ProcessPropertyChange(UsdPrim const& prim,
                                                   SdfPath const& cachePath,
                                                   TfToken const& propertyName)
{
    const std::array<TfToken, 6> textureAttrs = {
        UsdGeomTokens->modelCardTextureXPos,
        UsdGeomTokens->modelCardTextureYPos,
        UsdGeomTokens->modelCardTextureZPos,
        UsdGeomTokens->modelCardTextureXNeg,
        UsdGeomTokens->modelCardTextureYNeg,
        UsdGeomTokens->modelCardTextureZNeg,
    };

    if (_IsMaterialPath(cachePath)) {
        // Check if a texture has been changed.
        for (const TfToken& attr : textureAttrs) {
            if (propertyName == attr) {
                return HdMaterial::DirtyResource;
            }
        }
        return HdChangeTracker::Clean;
    }

    HdDirtyBits dirtyGeo =
        HdChangeTracker::DirtyTopology | HdChangeTracker::DirtyPoints |
        HdChangeTracker::DirtyPrimvar | HdChangeTracker::DirtyExtent;

    if (propertyName == UsdGeomTokens->modelDrawModeColor)
        return HdChangeTracker::DirtyPrimvar;
    else if (propertyName == UsdGeomTokens->modelCardGeometry ||
             propertyName == UsdGeomTokens->extent ||
             propertyName == UsdGeomTokens->extentsHint)
        return dirtyGeo;
    else if (propertyName == UsdGeomTokens->visibility ||
             propertyName == UsdGeomTokens->purpose)
        return HdChangeTracker::DirtyVisibility;
    else if (UsdGeomXformable::IsTransformationAffectedByAttrNamed(
                propertyName))
        return HdChangeTracker::DirtyTransform;

    // In "cards" mode the texture assignments change what geometry
    // is generated.
    for (const TfToken& attr : textureAttrs) {
        if (propertyName == attr) {
            return dirtyGeo;
        }
    }

    return HdChangeTracker::Clean;
}

void
UsdImagingGLDrawModeAdapter::_GenerateOriginGeometry(
    VtValue *topo, VtValue *points, GfRange3d const& extents) const
{
    // Origin: vertices are (0,0,0); (1,0,0); (0,1,0); (0,0,1)
    VtVec3fArray pt = VtVec3fArray(4);
    pt[0] = GfVec3f(0,0,0);
    pt[1] = GfVec3f(1,0,0);
    pt[2] = GfVec3f(0,1,0);
    pt[3] = GfVec3f(0,0,1);
    *points = VtValue(pt);

    // segments are +X, +Y, +Z.
    VtIntArray curveVertexCounts = VtIntArray(1);
    curveVertexCounts[0] = 6;
    VtIntArray curveIndices = VtIntArray(6);
    const int indices[] = { 0, 1, 0, 2, 0, 3 };
    for (int i = 0; i < 6; ++i) { curveIndices[i] = indices[i]; }

    HdBasisCurvesTopology topology(
        HdTokens->linear, HdTokens->bezier, HdTokens->segmented,
        curveVertexCounts, curveIndices);
    *topo = VtValue(topology);
}

void
UsdImagingGLDrawModeAdapter::_GenerateBoundsGeometry(
        VtValue *topo, VtValue *points, GfRange3d const& extents) const
{
    // Bounding box: vertices are for(i: 0 -> 7) {
    //   ((i & 1) ? z : -z) +
    //   ((i & 2) ? y : -y) +
    //   ((i & 4) ? x : -x)
    // } ... where x is extents[1].x, -x is extents[0].x
    GfVec3f min = GfVec3f(extents.GetMin()),
            max = GfVec3f(extents.GetMax());
    VtVec3fArray pt = VtVec3fArray(8);
    for(int i = 0; i < 8; ++i) {
        pt[i] = GfVec3f((i & 4) ? max[0] : min[0],
                        (i & 2) ? max[1] : min[1],
                        (i & 1) ? max[2] : min[2]);
    }
    *points = VtValue(pt);

    // Segments: CCW bottom face starting at (-x, -y, -z)
    //           CCW top face starting at (-x, -y, z)
    //           CCW vertical edges, starting at (-x, -y)
    VtIntArray curveVertexCounts = VtIntArray(1);
    curveVertexCounts[0] = 24;
    VtIntArray curveIndices = VtIntArray(24);
    const int indices[] = { /* bottom face */ 0, 4, 4, 6, 6, 2, 2, 0,
                            /* top face */    1, 5, 5, 7, 7, 3, 3, 1,
                            /* edge pairs */  0, 1, 4, 5, 6, 7, 2, 3 };
    for (int i = 0; i < 24; ++i) { curveIndices[i] = indices[i]; }

    HdBasisCurvesTopology topology(
        HdTokens->linear, HdTokens->bezier, HdTokens->segmented,
        curveVertexCounts, curveIndices);
    *topo = VtValue(topology);
}

void
UsdImagingGLDrawModeAdapter::_GenerateCardsCrossGeometry(
        VtValue *topo, VtValue *points, GfRange3d const& extents,
        uint8_t axes_mask) const
{
    // Generate one face per axis direction, for included axes.
    const int numFaces = ((axes_mask & xAxis) ? 2 : 0) +
                         ((axes_mask & yAxis) ? 2 : 0) +
                         ((axes_mask & zAxis) ? 2 : 0);

    // Cards (Cross) vertices:
    // - +/-X vertices (CCW wrt +X)
    // - +/-Y vertices (CCW wrt +Y)
    // - +/-Z vertices (CCW wrt +Z)
    GfVec3f min = GfVec3f(extents.GetMin()),
            max = GfVec3f(extents.GetMax()),
            mid = (min+max)/2.0f;

    VtVec3fArray pt = VtVec3fArray(numFaces * 4);
    int ptIdx = 0;

    if (axes_mask & xAxis) {
        // +X
        pt[ptIdx++] = GfVec3f(mid[0], max[1], max[2]);
        pt[ptIdx++] = GfVec3f(mid[0], min[1], max[2]);
        pt[ptIdx++] = GfVec3f(mid[0], min[1], min[2]);
        pt[ptIdx++] = GfVec3f(mid[0], max[1], min[2]);

        // -X
        pt[ptIdx++] = GfVec3f(mid[0], min[1], max[2]);
        pt[ptIdx++] = GfVec3f(mid[0], max[1], max[2]);
        pt[ptIdx++] = GfVec3f(mid[0], max[1], min[2]);
        pt[ptIdx++] = GfVec3f(mid[0], min[1], min[2]);
    }

    if (axes_mask & yAxis) {
        // +Y
        pt[ptIdx++] = GfVec3f(min[0], mid[1], max[2]);
        pt[ptIdx++] = GfVec3f(max[0], mid[1], max[2]);
        pt[ptIdx++] = GfVec3f(max[0], mid[1], min[2]);
        pt[ptIdx++] = GfVec3f(min[0], mid[1], min[2]);

        // -Y
        pt[ptIdx++] = GfVec3f(max[0], mid[1], max[2]);
        pt[ptIdx++] = GfVec3f(min[0], mid[1], max[2]);
        pt[ptIdx++] = GfVec3f(min[0], mid[1], min[2]);
        pt[ptIdx++] = GfVec3f(max[0], mid[1], min[2]);
    }

    if (axes_mask & zAxis) {
        // +Z
        pt[ptIdx++] = GfVec3f(max[0], max[1], mid[2]);
        pt[ptIdx++] = GfVec3f(min[0], max[1], mid[2]);
        pt[ptIdx++] = GfVec3f(min[0], min[1], mid[2]);
        pt[ptIdx++] = GfVec3f(max[0], min[1], mid[2]);

        // -Z
        pt[ptIdx++] = GfVec3f(min[0], max[1], mid[2]);
        pt[ptIdx++] = GfVec3f(max[0], max[1], mid[2]);
        pt[ptIdx++] = GfVec3f(max[0], min[1], mid[2]);
        pt[ptIdx++] = GfVec3f(min[0], min[1], mid[2]);
    }

    VtIntArray faceCounts = VtIntArray(numFaces);
    VtIntArray faceIndices = VtIntArray(numFaces * 4);
    for (int i = 0; i < numFaces; ++i) {
        faceCounts[i] = 4;
        faceIndices[i*4+0] = i*4+0;
        faceIndices[i*4+1] = i*4+1;
        faceIndices[i*4+2] = i*4+2;
        faceIndices[i*4+3] = i*4+3;
    }

    VtIntArray holeIndices(0);

    HdMeshTopology topology(
        PxOsdOpenSubdivTokens->none, PxOsdOpenSubdivTokens->rightHanded,
        faceCounts, faceIndices, holeIndices);

    *points = VtValue(pt);
    *topo = VtValue(topology);
}

void
UsdImagingGLDrawModeAdapter::_SanityCheckFaceSizes(SdfPath const& cachePath,
        GfRange3d const& extents, uint8_t axes_mask) const
{
    GfVec3d min = extents.GetMin(),
            max = extents.GetMax();
    bool zeroX = (min[0] == max[0]);
    bool zeroY = (min[1] == max[1]);
    bool zeroZ = (min[2] == max[2]);

    if ((axes_mask & xAxis) && (zeroY || zeroZ)) {
        // XXX: validation
        TF_WARN("Cards rendering for <%s>: X+/X- faces have zero area.",
                cachePath.GetText());
    }
    if ((axes_mask & yAxis) && (zeroX || zeroZ)) {
        // XXX: validation
        TF_WARN("Cards rendering for <%s>: Y+/Y- faces have zero area.",
                cachePath.GetText());
    }
    if ((axes_mask & zAxis) && (zeroX || zeroY)) {
        // XXX: validation
        TF_WARN("Cards rendering for <%s>: Z+/Z- faces have zero area.",
                cachePath.GetText());
    }
}

void
UsdImagingGLDrawModeAdapter::_GenerateCardsBoxGeometry(
        VtValue *topo, VtValue *points, GfRange3d const& extents,
        uint8_t axes_mask) const
{
    // Generate one face per axis direction, for included axes.
    const int numFaces = ((axes_mask & xAxis) ? 2 : 0) +
                         ((axes_mask & yAxis) ? 2 : 0) +
                         ((axes_mask & zAxis) ? 2 : 0);

    // Bounding box: vertices are for(i: 0 -> 7) {
    //   ((i & 1) ? z : -z) +
    //   ((i & 2) ? y : -y) +
    //   ((i & 4) ? x : -x)
    // } ... where x is extents[1].x, -x is extents[0].x
    GfVec3f min = GfVec3f(extents.GetMin()),
            max = GfVec3f(extents.GetMax());

    VtVec3fArray pt = VtVec3fArray(numFaces * 4);
    int ptIdx = 0;

    VtVec3fArray corners = VtVec3fArray(8);
    for(int i = 0; i < 8; ++i) {
        corners[i] = GfVec3f((i & 4) ? max[0] : min[0],
                             (i & 2) ? max[1] : min[1],
                             (i & 1) ? max[2] : min[2]);
    }

    if (axes_mask & xAxis) {
        // +X
        pt[ptIdx++] = corners[7];
        pt[ptIdx++] = corners[5];
        pt[ptIdx++] = corners[4];
        pt[ptIdx++] = corners[6];

        // -X
        pt[ptIdx++] = corners[1];
        pt[ptIdx++] = corners[3];
        pt[ptIdx++] = corners[2];
        pt[ptIdx++] = corners[0];
    }

    if (axes_mask & yAxis) {
        // +Y
        pt[ptIdx++] = corners[3];
        pt[ptIdx++] = corners[7];
        pt[ptIdx++] = corners[6];
        pt[ptIdx++] = corners[2];

        // -Y
        pt[ptIdx++] = corners[5];
        pt[ptIdx++] = corners[1];
        pt[ptIdx++] = corners[0];
        pt[ptIdx++] = corners[4];
    }

    if (axes_mask & zAxis) {
        // +Z
        pt[ptIdx++] = corners[7];
        pt[ptIdx++] = corners[3];
        pt[ptIdx++] = corners[1];
        pt[ptIdx++] = corners[5];

        // -Z
        pt[ptIdx++] = corners[2];
        pt[ptIdx++] = corners[6];
        pt[ptIdx++] = corners[4];
        pt[ptIdx++] = corners[0];
    }

    *points = VtValue(pt);

    VtIntArray faceCounts = VtIntArray(numFaces);
    VtIntArray faceIndices = VtIntArray(numFaces * 4);
    for (int i = 0; i < numFaces; ++i) {
        faceCounts[i] = 4;
        faceIndices[i*4+0] = i*4+0;
        faceIndices[i*4+1] = i*4+1;
        faceIndices[i*4+2] = i*4+2;
        faceIndices[i*4+3] = i*4+3;
    }

    VtIntArray holeIndices(0);

    HdMeshTopology topology(
        UsdGeomTokens->none, UsdGeomTokens->rightHanded,
        faceCounts, faceIndices, holeIndices);

    *points = VtValue(pt);
    *topo = VtValue(topology);
}

void
UsdImagingGLDrawModeAdapter::_GenerateCardsFromTextureGeometry(
        VtValue *topo, VtValue *points, VtValue *uv, VtValue *assign,
        GfRange3d *extents, UsdPrim const& prim) const
{
    UsdGeomModelAPI model(prim);
    if (!model) {
        TF_CODING_ERROR("Prim <%s> has model:cardGeometry = fromTexture,"
                " but GeomModelAPI is not applied!", prim.GetPath().GetText());
        return;
    }

    std::vector<std::pair<GfMatrix4d, int>> faces;

    // Compute the face matrix/texture assignment pairs.
    GfMatrix4d mat;
    if (_GetMatrixFromImageMetadata(
            model.GetModelCardTextureXPosAttr(), &mat))
        faces.push_back(std::make_pair(mat, xPos));
    if (_GetMatrixFromImageMetadata(
            model.GetModelCardTextureYPosAttr(), &mat))
        faces.push_back(std::make_pair(mat, yPos));
    if (_GetMatrixFromImageMetadata(
            model.GetModelCardTextureZPosAttr(), &mat))
        faces.push_back(std::make_pair(mat, zPos));
    if (_GetMatrixFromImageMetadata(
            model.GetModelCardTextureXNegAttr(), &mat))
        faces.push_back(std::make_pair(mat, xNeg));
    if (_GetMatrixFromImageMetadata(
            model.GetModelCardTextureYNegAttr(), &mat))
        faces.push_back(std::make_pair(mat, yNeg));
    if (_GetMatrixFromImageMetadata(
            model.GetModelCardTextureZNegAttr(), &mat))
        faces.push_back(std::make_pair(mat, zNeg));

    // Generate points, UV, and assignment primvars, plus index data.
    VtVec3fArray arr_pt = VtVec3fArray(faces.size() * 4);
    VtVec2fArray arr_uv = VtVec2fArray(faces.size() * 4);
    VtIntArray arr_assign = VtIntArray(faces.size());
    VtIntArray faceCounts = VtIntArray(faces.size());
    VtIntArray faceIndices = VtIntArray(faces.size() * 4);

    static const std::array<GfVec3f, 4> corners = {
        GfVec3f(-1, -1,  0), GfVec3f(-1,  1,  0),
        GfVec3f( 1,  1,  0), GfVec3f( 1, -1,  0) };
    static const std::array<GfVec2f, 4> std_uvs = 
        std::array<GfVec2f, 4>(
            {GfVec2f(0,1), GfVec2f(0,0), GfVec2f(1,0), GfVec2f(1,1) });

    for(size_t i = 0; i < faces.size(); ++i) {
        GfMatrix4d screenToWorld = faces[i].first.GetInverse();
        faceCounts[i] = 4;
        arr_assign[i] = faces[i].second;
        for (size_t j = 0; j < 4; ++j) {
            faceIndices[i*4+j] = i*4+j;
            arr_pt[i*4+j] = screenToWorld.Transform(corners[j]);
            arr_uv[i*4+j] = std_uvs[j];
        }    
    }

    // Create the topology object, and put our buffers in the out-values.
    VtIntArray holeIndices(0);
    HdMeshTopology topology(
        UsdGeomTokens->none, UsdGeomTokens->rightHanded,
        faceCounts, faceIndices, holeIndices);

    *topo = VtValue(topology);
    *points = VtValue(arr_pt);
    *uv = VtValue(arr_uv);
    *assign = VtValue(arr_assign);

    // Compute extents from points.
    extents->SetEmpty();
    for (size_t i = 0; i < faces.size()*4; ++i) {
        extents->UnionWith(arr_pt[i]);
    }
}

namespace
{

template <class Vec>
bool
_ConvertToMatrix(const Vec &mvec, GfMatrix4d *mat)
{
    if (mvec.size() == 16) {
        mat->Set(mvec[ 0], mvec[ 1], mvec[ 2], mvec[ 3],
                 mvec[ 4], mvec[ 5], mvec[ 6], mvec[ 7],
                 mvec[ 8], mvec[ 9], mvec[10], mvec[11],
                 mvec[12], mvec[13], mvec[14], mvec[15]);
        return true;
    }

    TF_WARN(
        "worldtoscreen metadata expected 16 values, got %zu",
        mvec.size());
    return false;
};

}

bool
UsdImagingGLDrawModeAdapter::_GetMatrixFromImageMetadata(
    UsdAttribute const& attr, GfMatrix4d *mat) const
{
    // This function expects the input attribute to be an image asset path.
    SdfAssetPath asset;
    attr.Get(&asset);

    // If the literal path is empty, ignore this attribute.
    if (asset.GetAssetPath().empty()) {
        return false;
    }

    std::string file = asset.GetResolvedPath();
    // Fallback to the literal path if it couldn't be resolved.
    if (file.empty()) {
        file = asset.GetAssetPath();
    }

    HioImageSharedPtr img = HioImage::OpenForReading(file);
    if (!img) {
        return false;
    }

    // Read the "worldtoscreen" metadata. This metadata specifies a 4x4
    // matrix but may be given as any the following data types, since
    // some image formats may support certain metadata types but not others.
    //
    // - std::vector<float> or std::vector<double> with 16 elements
    //   in row major order.
    // - GfMatrix4f or GfMatrix4d
    VtValue worldtoscreen;
    if (img->GetMetadata(_tokens->worldtoscreen, &worldtoscreen)) {
        if (worldtoscreen.IsHolding<std::vector<float>>()) {
            return _ConvertToMatrix(
                worldtoscreen.UncheckedGet<std::vector<float>>(), mat);
        }
        else if (worldtoscreen.IsHolding<std::vector<double>>()) {
            return _ConvertToMatrix(
                worldtoscreen.UncheckedGet<std::vector<double>>(), mat);
        }
        else if (worldtoscreen.IsHolding<GfMatrix4f>()) {
            *mat = GfMatrix4d(worldtoscreen.UncheckedGet<GfMatrix4f>());
            return true;
        }
        else if (worldtoscreen.IsHolding<GfMatrix4d>()) {
            *mat = worldtoscreen.UncheckedGet<GfMatrix4d>();
            return true;
        }
        else {
            TF_WARN(
                "worldtoscreen metadata holding unexpected type '%s'",
                worldtoscreen.GetTypeName().c_str());
        }
    }

    return false;
}

static
std::array<GfVec2f, 4>
_GetUVsForQuad(const bool flipU, bool flipV)
{
    return {
        GfVec2f(flipU ? 0.0 : 1.0, flipV ? 0.0 : 1.0),
        GfVec2f(flipU ? 1.0 : 0.0, flipV ? 0.0 : 1.0),
        GfVec2f(flipU ? 1.0 : 0.0, flipV ? 1.0 : 0.0),
        GfVec2f(flipU ? 0.0 : 1.0, flipV ? 1.0 : 0.0) };
}

void
UsdImagingGLDrawModeAdapter::_GenerateTextureCoordinates(
        VtValue *uv, VtValue *assign, uint8_t axes_mask) const
{
    // This function generates a UV quad per face, with the correct orientation,
    // and also uniform indices for each face specifying which texture to
    // sample.  The order is [X+, X-, Y+, Y-, Z+, Z-], possibly with some of
    // the axes omitted.

    static const std::array<GfVec2f, 4> uv_normal =
        _GetUVsForQuad(false, false);
    static const std::array<GfVec2f, 4> uv_flipped_s =
        _GetUVsForQuad(true, false);
    static const std::array<GfVec2f, 4> uv_flipped_t =
        _GetUVsForQuad(false, true);
    static const std::array<GfVec2f, 4> uv_flipped_st =
        _GetUVsForQuad(true, true);

    std::vector<const GfVec2f *> uv_faces;
    std::vector<int> face_assign;
    if (axes_mask & xAxis) {
        uv_faces.push_back(
            (axes_mask & xPos) ? uv_normal.data() : uv_flipped_s.data());
        face_assign.push_back((axes_mask & xPos) ? xPos : xNeg);
        uv_faces.push_back(
            (axes_mask & xNeg) ? uv_normal.data() : uv_flipped_s.data());
        face_assign.push_back((axes_mask & xNeg) ? xNeg : xPos);
    }
    if (axes_mask & yAxis) {
        uv_faces.push_back(
            (axes_mask & yPos) ? uv_normal.data() : uv_flipped_s.data());
        face_assign.push_back((axes_mask & yPos) ? yPos : yNeg);
        uv_faces.push_back(
            (axes_mask & yNeg) ? uv_normal.data() : uv_flipped_s.data());
        face_assign.push_back((axes_mask & yNeg) ? yNeg : yPos);
    }
    if (axes_mask & zAxis) {
        // (Z+) and (Z-) need to be flipped on the (t) axis instead of the (s)
        // axis when we're borrowing a texture from the other side of the axis.
        uv_faces.push_back(
            (axes_mask & zPos) ? uv_normal.data() : uv_flipped_t.data());
        face_assign.push_back((axes_mask & zPos) ? zPos : zNeg);
        uv_faces.push_back(
            (axes_mask & zNeg) ? uv_flipped_st.data() : uv_flipped_s.data());
        face_assign.push_back((axes_mask & zNeg) ? zNeg : zPos);
    }

    VtVec2fArray faceUV = VtVec2fArray(uv_faces.size() * 4);
    for (size_t i = 0; i < uv_faces.size(); ++i) {
        memcpy(&faceUV[i*4], uv_faces[i], 4 * sizeof(GfVec2f));
    }
    *uv = VtValue(faceUV);

    VtIntArray faceAssign = VtIntArray(face_assign.size());
    for (size_t i = 0; i < face_assign.size(); ++i) {
        faceAssign[i] = face_assign[i];
    }
    *assign = VtValue(faceAssign);
}

GfRange3d
UsdImagingGLDrawModeAdapter::_ComputeExtent(UsdPrim const& prim,
        const UsdTimeCode& timecode) const
{
    HD_TRACE_FUNCTION();
    HF_MALLOC_TAG_FUNCTION();

    TfTokenVector purposes = { UsdGeomTokens->default_, UsdGeomTokens->proxy,
                               UsdGeomTokens->render };

    if (prim.IsLoaded()) {
        UsdGeomBBoxCache bboxCache(timecode, purposes, true);
        return bboxCache.ComputeUntransformedBound(prim).ComputeAlignedBox();
    } else {
        GfRange3d extent;
        UsdAttribute attr;
        VtVec3fArray extentsHint;
        // Get the extent either from the authored extent attribute of a
        // UsdGeomBoundable prim, or get the extentsHint attribute from the
        // prim.
        if (prim.IsA<UsdGeomBoundable>() &&
            (attr = UsdGeomBoundable(prim).GetExtentAttr()) &&
            attr.Get(&extentsHint, timecode) &&
            extentsHint.size() == 2) {
            extent = GfRange3d(extentsHint[0], extentsHint[1]);
        }
        else if ((attr = UsdGeomModelAPI(prim).GetExtentsHintAttr()) &&
            attr.Get(&extentsHint, timecode) &&
            extentsHint.size() >= 2) {
            // XXX: This code to merge the extentsHint values over a set of
            // purposes probably belongs in UsdGeomBBoxCache.
            const TfTokenVector &purposeTokens =
                UsdGeomImageable::GetOrderedPurposeTokens();
            for (size_t i = 0; i < purposeTokens.size(); ++i) {
                size_t idx = i*2;
                // If extents are not available for the value of purpose,
                // it implies that the rest of the bounds are empty.
                if ((idx + 2) > extentsHint.size())
                    break;
                // If this purpose isn't one we are interested in, skip it.
                if (std::find(purposes.begin(), purposes.end(),
                              purposeTokens[i]) == purposes.end())
                    continue;

                GfRange3d purposeExtent =
                    GfRange3d(extentsHint[idx], extentsHint[idx+1]);
                // Extents for an unauthored geometry purpose may be empty,
                // even though the extent for a later purpose may exist.
                if (!purposeExtent.IsEmpty()) {
                    extent.ExtendBy(purposeExtent);
                }
            }
        }
        return extent;
    }
}

/*virtual*/
HdCullStyle 
UsdImagingGLDrawModeAdapter::GetCullStyle(UsdPrim const& prim,
                                          SdfPath const& cachePath,
                                          UsdTimeCode time) const
{
    return HdCullStyleBack;
}

/*virtual*/
GfMatrix4d 
UsdImagingGLDrawModeAdapter::GetTransform(UsdPrim const& prim, 
                                          SdfPath const& cachePath,
                                          UsdTimeCode time,
                                          bool ignoreRootTransform) const
{
    // If the draw mode is instantiated on an instance, prim will be
    // the instance prim, but we want to ignore transforms on that
    // prim since the instance adapter will incorporate it into the per-instance
    // transform and we don't want to double-transform the prim.
    //
    // Note: if the prim is unloaded (because unloaded prims are drawing as
    // bounds), we skip the normal instancing machinery and need to handle
    // the transform ourselves.
    if (prim.IsInstance() && prim.IsLoaded()) {
        return GfMatrix4d(1.0);
    } else {
        return BaseAdapter::GetTransform(
            prim, prim.GetPath(), time, ignoreRootTransform);
    }
}

PXR_NAMESPACE_CLOSE_SCOPE
