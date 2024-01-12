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
#include "pxr/usdImaging/usdImaging/drawModeAdapter.h"

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

    (cardSurface)
    (cardTexture)
    (cardUvCoords)

    (cardsUv)

    (subsetXPos)
    (subsetYPos)
    (subsetZPos)
    (subsetXNeg)
    (subsetYNeg)
    (subsetZNeg)

    (subsetMaterialXPos)
    (subsetMaterialYPos)
    (subsetMaterialZPos)
    (subsetMaterialXNeg)
    (subsetMaterialYNeg)
    (subsetMaterialZNeg)

    (worldtoscreen)
    (worldToNDC)

    (displayRoughness)
    (diffuseColor)
    (opacity)
    (opacityThreshold)

    (file)
    (st)
    (rgb)
    (a)
    (fallback)
    (wrapS)
    (wrapT)
    (clamp)

    (varname)
    (result)
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
    typedef UsdImagingDrawModeAdapter Adapter;
    TfType t = TfType::Define<Adapter, TfType::Bases<Adapter::BaseAdapter> >();
    t.SetFactory< UsdImagingPrimAdapterFactory<Adapter> >();
}

static AxesMask
_GetOppositeFace(AxesMask face) {
    switch (face) {
        case xPos: return xNeg;
        case xNeg: return xPos;
        case yPos: return yNeg;
        case yNeg: return yPos;
        case zPos: return zNeg;
        case zNeg: return zPos;
        default: return face;
    }
}

static TfToken
_GetSubsetTokenForFace(AxesMask axis) {
    switch (axis) {
        case xPos: return _tokens->subsetXPos;
        case yPos: return _tokens->subsetYPos;
        case zPos: return _tokens->subsetZPos;
        case xNeg: return _tokens->subsetXNeg;
        case yNeg: return _tokens->subsetYNeg;
        case zNeg: return _tokens->subsetZNeg;
        default: return TfToken(); // multiple axes not supported
    }
}

static TfToken
_GetSubsetMaterialTokenForFace(AxesMask axis) {
    switch (axis) {
        case xPos: return _tokens->subsetMaterialXPos;
        case yPos: return _tokens->subsetMaterialYPos;
        case zPos: return _tokens->subsetMaterialZPos;
        case xNeg: return _tokens->subsetMaterialXNeg;
        case yNeg: return _tokens->subsetMaterialYNeg;
        case zNeg: return _tokens->subsetMaterialZNeg;
        default: return TfToken(); // multiple axes not supported
    }
}

static uint8_t
_GetAxesMask(UsdPrim const& prim, UsdTimeCode time) {
    // Generate mask for suppressing axes with no textures
    uint8_t axes_mask = 0;
    UsdGeomModelAPI model(prim);
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

    return axes_mask;
}

UsdImagingDrawModeAdapter::UsdImagingDrawModeAdapter()
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

UsdImagingDrawModeAdapter::~UsdImagingDrawModeAdapter()
{
}

bool
UsdImagingDrawModeAdapter::ShouldCullChildren() const
{
    return true;
}

bool
UsdImagingDrawModeAdapter::CanPopulateUsdInstance() const
{
    return true;
}

bool
UsdImagingDrawModeAdapter::IsSupported(
    UsdImagingIndexProxy const* index) const
{
    return true;
}

SdfPath
UsdImagingDrawModeAdapter::Populate(UsdPrim const& prim,
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

    // When instancing, cachePath may have a proto prop part on the end.
    // This will strip the prop part, leaving primPath as the instancer's path.
    SdfPath primPath = cachePath.GetAbsoluteRootOrPrimPath();

    // Additionally, insert the material.
    if (drawMode == UsdGeomTokens->cards) {

        // Note that because population happens only once, any faces that need
        // time-varying textures should begin with some texture applied, and
        // no face should ever transition between textured and untextured 
        // states. The addition or subtraction of an entire face texture over 
        // time is not supported.
        uint8_t mask = _GetAxesMask(prim, UsdTimeCode::EarliestTime());

        // If no face on any axis has a texture assigned to it, no face-
        // specific materials will be inserted. Only the prim-level fallback
        // material needs to be added. It is always added, just in case.

        // If neither face of a given axis has a texture assigned to it,
        // no geometry for that axis will be generated, and no materials
        // created for either face.

        // If only one face of a given axis has a texture assigned to it,
        // both faces of that axis will use the same material and the UVs
        // on the untextured face will be mirrored.

        // If both faces of a given axis have textures assigned to them,
        // each face will receive its own material and no adjustments will
        // be made to the UVs of either face.

        const AxesMask faces[6] = { xPos, xNeg, yPos, yNeg, zPos, zNeg };
        for (const AxesMask face : faces) {
            if (mask & face) {
                SdfPath materialPath = prim.GetPath()
                    .AppendChild(_GetSubsetMaterialTokenForFace(face));
                if (index->IsSprimTypeSupported(HdPrimTypeTokens->material) &&
                    !index->IsPopulated(materialPath)) {
                    index->InsertSprim(
                        HdPrimTypeTokens->material,
                        materialPath, 
                        prim, 
                        shared_from_this()
                    );
                    HD_PERF_COUNTER_INCR(UsdImagingTokens->usdPopulatedPrimCount);
                }
                // Record the material(s) for use in remove/resync.
                _materialMap[cachePath].insert(materialPath);
            }
        }
    }

    // Record the drawmode for use in UpdateForTime().
    _drawModeMap.insert(std::make_pair(cachePath, drawMode));

    return cachePath;
}

bool
UsdImagingDrawModeAdapter::_IsMaterialPath(SdfPath const& path) const
{
    TfToken nameToken = path.GetNameToken();
    return nameToken == _tokens->subsetMaterialXPos ||
        nameToken == _tokens->subsetMaterialYPos ||
        nameToken == _tokens->subsetMaterialZPos ||
        nameToken == _tokens->subsetMaterialXNeg ||
        nameToken == _tokens->subsetMaterialYNeg ||
        nameToken == _tokens->subsetMaterialZNeg;
}

void
UsdImagingDrawModeAdapter::ProcessPrimResync(SdfPath const& cachePath,
        UsdImagingIndexProxy* index)

{
    if (_IsMaterialPath(cachePath)) {
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
UsdImagingDrawModeAdapter::ProcessPrimRemoval(SdfPath const& cachePath,
        UsdImagingIndexProxy* index)
{
    if (_IsMaterialPath(cachePath)) {
        // Ignore a removal of the material on the theory that the rprim removal
        // will take care of it.
        return;
    }

    // Remove the materials for this rprim
    _MaterialMap::const_iterator it = _materialMap.find(cachePath);
    if (it != _materialMap.end()) {
        for (SdfPath path : it->second) {
            index->RemoveSprim(HdPrimTypeTokens->material, path);
        }
        _materialMap.erase(it);
    }

    // Remove the rprim
    _drawModeMap.erase(cachePath);
    index->RemoveRprim(cachePath);
}

void
UsdImagingDrawModeAdapter::_RemovePrim(SdfPath const& cachePath,
                                       UsdImagingIndexProxy* index)
{
    TF_CODING_ERROR("_RemovePrim called on draw mode adapter!");
}

void
UsdImagingDrawModeAdapter::MarkDirty(UsdPrim const& prim,
                                     SdfPath const& cachePath,
                                     HdDirtyBits dirty,
                                     UsdImagingIndexProxy* index)
{
    if (_IsMaterialPath(cachePath)) {
        index->MarkSprimDirty(cachePath, dirty);
    } else {
        index->MarkRprimDirty(cachePath, dirty);
        // Note: certain bits mean we need to recompute the primvar set.
        HdDirtyBits bitsMask = HdChangeTracker::DirtyTopology |
            HdChangeTracker::DirtyPoints |
            HdChangeTracker::DirtyPrimvar |
            HdChangeTracker::DirtyExtent |
            HdChangeTracker::DirtyWidths;
        if (dirty & bitsMask) {
            index->RequestUpdateForTime(cachePath);
        }
    }
}

void
UsdImagingDrawModeAdapter::MarkTransformDirty(UsdPrim const& prim,
                                              SdfPath const& cachePath,
                                              UsdImagingIndexProxy* index)
{
    if (!_IsMaterialPath(cachePath)) {
        index->MarkRprimDirty(cachePath, HdChangeTracker::DirtyTransform);
    }
}

void
UsdImagingDrawModeAdapter::MarkVisibilityDirty(UsdPrim const& prim,
                                               SdfPath const& cachePath,
                                               UsdImagingIndexProxy* index)
{
    if (!_IsMaterialPath(cachePath)) {
        index->MarkRprimDirty(cachePath, HdChangeTracker::DirtyVisibility);
    }
}

void
UsdImagingDrawModeAdapter::MarkMaterialDirty(UsdPrim const& prim,
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
        index->RequestUpdateForTime(cachePath);
    }
}

bool
UsdImagingDrawModeAdapter::_HasVaryingExtent(UsdPrim const& prim) const
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
UsdImagingDrawModeAdapter::_ComputeGeometryData(
    UsdPrim const& prim,
    SdfPath const& cachePath,
    UsdTimeCode time,
    TfToken const& drawMode,
    VtValue* topology, 
    VtValue* points, 
    GfRange3d* extent,
    VtValue* uv) const
{
    if (drawMode == UsdGeomTokens->origin) {
        *extent = _ComputeExtent(
            prim,
            _HasVaryingExtent(prim) ? time : UsdTimeCode::EarliestTime()
        );
        _GenerateOriginGeometry(topology, points, *extent);

    } else if (drawMode == UsdGeomTokens->bounds) {
        *extent = _ComputeExtent(
            prim,
            _HasVaryingExtent(prim) ? time : UsdTimeCode::EarliestTime()
        );
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
            _GenerateCardsFromTextureGeometry(topology, points, uv, extent, prim);

        } else {
            // First compute the extents.
            *extent = _ComputeExtent(
                prim,
                _HasVaryingExtent(prim) ? time : UsdTimeCode::EarliestTime()
            );

            // Generate mask for suppressing axes with no textures
            uint8_t axes_mask = _GetAxesMask(prim, time);
            bool generateSubsets = true;
            if (axes_mask == 0) {
                // If no face of any axis has a texture, build full geometry.
                // In this case, no face-specific materials were populated.
                // All faces will use the prim-level fallback. No subsets
                // need be generated.
                axes_mask = xAxis | yAxis | zAxis;
                // generateSubsets = false;
            }
    
            // Generate UVs.
            _GenerateTextureCoordinates(uv, axes_mask);

            // Generate geometry based on card type.
            if (cardGeometry == UsdGeomTokens->cross ||
                cardGeometry == UsdGeomTokens->box) {
                _GenerateCardsGeometry(
                    topology, points, *extent,
                    axes_mask, cardGeometry, generateSubsets, prim
                );
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
UsdImagingDrawModeAdapter::GetTopology(UsdPrim const& prim,
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
    GfRange3d extent;
    _ComputeGeometryData(
        prim, cachePath, time, drawMode, &topology, 
        &points, &extent, &uv
    );
    return topology;
}

/*virtual*/
GfRange3d 
UsdImagingDrawModeAdapter::GetExtent(UsdPrim const& prim, 
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
    GfRange3d extent;
    _ComputeGeometryData(
        prim, cachePath, time, drawMode, &topology, 
        &points, &extent, &uv
    );
    return extent;
}

/*virtual*/
bool
UsdImagingDrawModeAdapter::GetDoubleSided(UsdPrim const& prim, 
                                            SdfPath const& cachePath, 
                                            UsdTimeCode time) const
{
    return false;
}

/*virtual*/
VtValue 
UsdImagingDrawModeAdapter::Get(UsdPrim const& prim, 
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
        GfRange3d extent;
        _ComputeGeometryData(
            prim, cachePath, time, drawMode, &topology, 
            &points, &extent, &uv
        );
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
        GfRange3d extent;
        _ComputeGeometryData(
            prim, cachePath, time, drawMode, &topology, 
            &points, &extent, &uv
        );
        return uv;

    } else if (key == _tokens->displayRoughness) {
        return VtValue(1.0f);
    }

    return value;
}

/*virtual*/
SdfPath
UsdImagingDrawModeAdapter::GetMaterialId(UsdPrim const& prim, 
                                           SdfPath const& cachePath, 
                                           UsdTimeCode time) const
{
    // Because there may be many materials associated with a single prim,
    // this method will return an empty path. Consumers interested in material
    // ids for individual subsets must get those from the topology themselves.
    return SdfPath();
}

/*virtual*/
VtValue
UsdImagingDrawModeAdapter::GetMaterialResource(UsdPrim const& prim, 
                            SdfPath const& cachePath, 
                            UsdTimeCode time) const
{
    if (!_IsMaterialPath(cachePath)) {
        return BaseAdapter::GetMaterialResource(prim, cachePath, time);
    }

    UsdGeomModelAPI model(prim);

    // Generate material network with a UsdPreviewSurface terminal.
    TfToken const& terminalType = HdMaterialTerminalTokens->surface;
    HdMaterialNetworkMap networkMap;
    HdMaterialNetwork& network = networkMap.map[terminalType];
    HdMaterialNode terminal;
    terminal.path = SdfPath(_tokens->cardSurface);
    terminal.identifier = UsdImagingTokens->UsdPreviewSurface;

    if (model) {
        GfVec3f drawModeColor;
        model.GetModelDrawModeColorAttr().Get(&drawModeColor);
        VtValue fallback = VtValue(GfVec4f(
            drawModeColor[0], drawModeColor[1], drawModeColor[2], 1.0f
        ));

        TfToken textureAttr;
        TfToken materialName = cachePath.GetNameToken();

        if (materialName == _tokens->subsetMaterialXPos) {
            textureAttr = UsdGeomTokens->modelCardTextureXPos;
        } else if (materialName == _tokens->subsetMaterialYPos) {
            textureAttr = UsdGeomTokens->modelCardTextureYPos;
        } else if (materialName == _tokens->subsetMaterialZPos) {
            textureAttr = UsdGeomTokens->modelCardTextureZPos;
        } else if (materialName == _tokens->subsetMaterialXNeg) {
            textureAttr = UsdGeomTokens->modelCardTextureXNeg;
        } else if (materialName == _tokens->subsetMaterialYNeg) {
            textureAttr = UsdGeomTokens->modelCardTextureYNeg;
        } else if (materialName == _tokens->subsetMaterialZNeg) {
            textureAttr = UsdGeomTokens->modelCardTextureZNeg;
        }

        SdfAssetPath textureFile;
        prim.GetAttribute(textureAttr).Get(&textureFile, time);
        if (!textureFile.GetAssetPath().empty()) {
            SdfPath textureNodePath = SdfPath(_tokens->cardTexture);

            // Create the texture node
            HdMaterialNode textureNode;
            textureNode.path = textureNodePath;
            textureNode.identifier = UsdImagingTokens->UsdUVTexture;
            textureNode.parameters[_tokens->st] = _tokens->cardsUv;
            textureNode.parameters[_tokens->fallback] = fallback;
            textureNode.parameters[_tokens->wrapS] = _tokens->clamp;
            textureNode.parameters[_tokens->wrapT] = _tokens->clamp;
            textureNode.parameters[_tokens->file] = textureFile;
            network.nodes.emplace_back(std::move(textureNode));

            // Insert connection between texture node and terminal color input
            HdMaterialRelationship colorRel;
            colorRel.inputId = textureNodePath;
            colorRel.inputName = _tokens->rgb;
            colorRel.outputId = terminal.path;
            colorRel.outputName = _tokens->diffuseColor;
            network.relationships.emplace_back(std::move(colorRel));

            // Insert connection between texture node and terminal opacity input
            HdMaterialRelationship opacityRel;
            opacityRel.inputId = textureNodePath;
            opacityRel.inputName = _tokens->a;
            opacityRel.outputId = terminal.path;
            opacityRel.outputName = _tokens->opacity;
            network.relationships.emplace_back(std::move(opacityRel));

            // Create the UV primvar reader node
            SdfPath uvPrimvarNodePath = SdfPath(_tokens->cardUvCoords);
            HdMaterialNode uvPrimvarNode;
            uvPrimvarNode.path = uvPrimvarNodePath;
            uvPrimvarNode.identifier = UsdImagingTokens->UsdPrimvarReader_float2;
            uvPrimvarNode.parameters[_tokens->varname] = _tokens->cardsUv;
            network.nodes.emplace_back(std::move(uvPrimvarNode));

            // Insert connection between UV primvar reader node 
            // and texture st input
            HdMaterialRelationship uvPrimvarRel;
            uvPrimvarRel.inputId = uvPrimvarNodePath;
            uvPrimvarRel.inputName = _tokens->result;
            uvPrimvarRel.outputId = textureNodePath;
            uvPrimvarRel.outputName = _tokens->st;
            network.relationships.emplace_back(std::move(uvPrimvarRel));

            // opacityThreshold must be > 0 to achieve desired performance for
            // cutouts in storm, but will produce artifacts around the edges of
            // cutouts in both storm and prman. Per the preview surface spec,
            // cutouts are not combinable with translucency/partial presence.
            terminal.parameters[_tokens->opacityThreshold] = VtValue(0.1f);
        } else {
            terminal.parameters[_tokens->diffuseColor] = drawModeColor;
            terminal.parameters[_tokens->opacity] = VtValue(1.f);
        }
    } else {
        terminal.parameters[_tokens->diffuseColor] = _schemaColor;
        terminal.parameters[_tokens->opacity] = VtValue(1.f);
    }

    // Insert terminal and update material network
    networkMap.terminals.push_back(terminal.path);
    network.nodes.emplace_back(std::move(terminal));

    return VtValue(networkMap);
}

void
UsdImagingDrawModeAdapter::_CheckForTextureVariability(
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
UsdImagingDrawModeAdapter::TrackVariability(UsdPrim const& prim,
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
UsdImagingDrawModeAdapter::UpdateForTime(UsdPrim const& prim,
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
        _ComputeGeometryData(prim, cachePath, time, drawMode, &topology, 
            &points, &extent, &uv);

        if (drawMode == UsdGeomTokens->cards) {
            // Merge "cardsUv" primvar
            _MergePrimvar(&primvars, _tokens->cardsUv,
                HdInterpolationVertex);

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
UsdImagingDrawModeAdapter::ProcessPropertyChange(UsdPrim const& prim,
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
UsdImagingDrawModeAdapter::_GenerateOriginGeometry(
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
UsdImagingDrawModeAdapter::_GenerateBoundsGeometry(
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
UsdImagingDrawModeAdapter::_GenerateCardsGeometry(
        VtValue *topo, VtValue *points, GfRange3d const& extents,
        uint8_t axes_mask, TfToken cardGeometry, bool generateSubsets, 
        UsdPrim const& prim) const
{
    // Generate one face per axis direction, for included axes.
    const int numFaces = ((axes_mask & xAxis) ? 2 : 0) +
                         ((axes_mask & yAxis) ? 2 : 0) +
                         ((axes_mask & zAxis) ? 2 : 0);

    // cardGeometry is either "cross" or "box", enforced in _ComputeGeometryData()
    bool cross = cardGeometry == UsdGeomTokens->cross;

    GfVec3f min = GfVec3f(extents.GetMin()),
            max = GfVec3f(extents.GetMax()),
            mid = (min+max)/2.0f;

    VtVec3fArray pt = VtVec3fArray(numFaces * 4);
    int ptIdx = 0;
    int faceIndex = 0;

    HdGeomSubsets geomSubsets;

    SdfPath primPath = prim.GetPath();

    auto generateSubset = [&](AxesMask face) {
        TfToken subset = _GetSubsetTokenForFace(face);
        TfToken material = _GetSubsetMaterialTokenForFace(
            axes_mask & face ? face : _GetOppositeFace(face)
        );
        if (!subset.IsEmpty() && !material.IsEmpty()) {
            geomSubsets.emplace_back(HdGeomSubset {
                HdGeomSubset::TypeFaceSet,
                SdfPath(subset),
                // materialBinding path must be absolute!
                primPath
                    .AppendChild(material),
                VtIntArray { faceIndex++ }
            });
        }
    };

    if (axes_mask & xAxis) {
        // +X
        float x = cross ? mid[0] : max[0];
        pt[ptIdx++] = GfVec3f(x, max[1], max[2]);
        pt[ptIdx++] = GfVec3f(x, min[1], max[2]);
        pt[ptIdx++] = GfVec3f(x, min[1], min[2]);
        pt[ptIdx++] = GfVec3f(x, max[1], min[2]);
        if (generateSubsets) {
            generateSubset(xPos);
        }

        // -X
        x = cross ? mid[0] : min[0];
        pt[ptIdx++] = GfVec3f(x, min[1], max[2]);
        pt[ptIdx++] = GfVec3f(x, max[1], max[2]);
        pt[ptIdx++] = GfVec3f(x, max[1], min[2]);
        pt[ptIdx++] = GfVec3f(x, min[1], min[2]);
        if (generateSubsets) {
            generateSubset(xNeg);
        }
    }

    if (axes_mask & yAxis) {
        // +Y
        float y = cross ? mid[1] : max[1];
        pt[ptIdx++] = GfVec3f(min[0], y, max[2]);
        pt[ptIdx++] = GfVec3f(max[0], y, max[2]);
        pt[ptIdx++] = GfVec3f(max[0], y, min[2]);
        pt[ptIdx++] = GfVec3f(min[0], y, min[2]);
        if (generateSubsets) {
            generateSubset(yPos);
        }

        // -Y
        y = cross ? mid[1] : min[1];
        pt[ptIdx++] = GfVec3f(max[0], y, max[2]);
        pt[ptIdx++] = GfVec3f(min[0], y, max[2]);
        pt[ptIdx++] = GfVec3f(min[0], y, min[2]);
        pt[ptIdx++] = GfVec3f(max[0], y, min[2]);
        if (generateSubsets) {
            generateSubset(yNeg);
        }
    }

    if (axes_mask & zAxis) {
        // +Z
        float z = cross ? mid[2] : max[2];
        pt[ptIdx++] = GfVec3f(max[0], max[1], z);
        pt[ptIdx++] = GfVec3f(min[0], max[1], z);
        pt[ptIdx++] = GfVec3f(min[0], min[1], z);
        pt[ptIdx++] = GfVec3f(max[0], min[1], z);
        if (generateSubsets) {
            generateSubset(zPos);
        }

        // -Z
        z = cross ? mid[2] : min[2];
        pt[ptIdx++] = GfVec3f(min[0], max[1], z);
        pt[ptIdx++] = GfVec3f(max[0], max[1], z);
        pt[ptIdx++] = GfVec3f(max[0], min[1], z);
        pt[ptIdx++] = GfVec3f(min[0], min[1], z);
        if (generateSubsets) {
            generateSubset(zNeg);
        }
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
        UsdGeomTokens->none, UsdGeomTokens->rightHanded,
        faceCounts, faceIndices, holeIndices
    );

    if (!geomSubsets.empty()) {
        topology.SetGeomSubsets(geomSubsets);
    }

    *points = VtValue(pt);
    *topo = VtValue(topology);
}

void
UsdImagingDrawModeAdapter::_SanityCheckFaceSizes(SdfPath const& cachePath,
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
UsdImagingDrawModeAdapter::_GenerateCardsFromTextureGeometry(
        VtValue *topo, VtValue *points, VtValue *uv,
        GfRange3d *extents, UsdPrim const& prim) const
{
    UsdGeomModelAPI model(prim);
    SdfPath primPath = prim.GetPath();
    if (!model) {
        TF_CODING_ERROR("Prim <%s> has model:cardGeometry = fromTexture,"
                " but GeomModelAPI is not applied!", primPath.GetText());
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

    HdGeomSubsets geomSubsets;
    for(size_t i = 0; i < faces.size(); ++i) {
        GfMatrix4d screenToWorld = faces[i].first.GetInverse();
        faceCounts[i] = 4;
        arr_assign[i] = faces[i].second;
        for (size_t j = 0; j < 4; ++j) {
            faceIndices[i*4+j] = i*4+j;
            arr_pt[i*4+j] = screenToWorld.Transform(corners[j]);
            arr_uv[i*4+j] = std_uvs[j];
        }

        // generate the subset
        TfToken subset = _GetSubsetTokenForFace(AxesMask(faces[i].second));
        TfToken material = _GetSubsetMaterialTokenForFace(
            AxesMask(faces[i].second));
        if (!subset.IsEmpty() && !material.IsEmpty()) {
            geomSubsets.emplace_back(HdGeomSubset {
                HdGeomSubset::TypeFaceSet,
                SdfPath(subset),
                // materialBinding path must be absolute!
                primPath
                    .AppendChild(material),
                VtIntArray { int(i) }
            });
        }
    }

    // Create the topology object, and put our buffers in the out-values.
    VtIntArray holeIndices(0);
    HdMeshTopology topology(
        UsdGeomTokens->none, UsdGeomTokens->rightHanded,
        faceCounts, faceIndices, holeIndices
    );

    if (!geomSubsets.empty()) {
        topology.SetGeomSubsets(geomSubsets);
    }

    *topo = VtValue(topology);
    *points = VtValue(arr_pt);
    *uv = VtValue(arr_uv);

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
UsdImagingDrawModeAdapter::_GetMatrixFromImageMetadata(
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

    // XXX: OpenImageIO >= 2.2 no longer flips 'worldtoscreen' with 'worldToNDC'
    // on read and write, so assets where 'worldtoscreen' was written with > 2.2
    // have 'worldToNDC' actually in the metadata, and OIIO < 2.2 would read
    // and return 'worldToNDC' from the file in response to a request for 
    // 'worldtoscreen'. OIIO >= 2.2 no longer does either, so 'worldtoscreen'
    // gets written as 'worldtoscreen' and returned when asked for
    // 'worldtoscreen'. Issues only arise when trying to read 'worldtoscreen'
    // from an asset written with OIIO < 2.2, when the authoring program told
    // OIIO to write it as 'worldtoscreen'. Old OIIO flipped it to 'worldToNDC'.
    // So new OIIO needs to read 'worldToNDC' to retrieve it.
    //
    // See https://github.com/OpenImageIO/oiio/pull/2609
    //
    // OIIO's change is correct -- the two metadata matrices have different
    // semantic meanings, and should not be conflated. Unfortunately, users will
    // have to continue to conflate them for a while as assets transition into
    // vfx2022 (which uses OIIO 2.3). So we will need to check for both.

    if (!img->GetMetadata(_tokens->worldtoscreen, &worldtoscreen)) {
        if (img->GetMetadata(_tokens->worldToNDC, &worldtoscreen)) {
            TF_WARN("The texture asset '%s' referenced at <%s> may have been "
            "authored by an earlier version of the VFX toolset. To silence this "
            "warning, please regenerate the asset with the current toolset.",
            file.c_str(), attr.GetPath().GetText());
        } else {
            TF_WARN("The texture asset '%s' referenced at <%s> lacks a "
            "worldtoscreen matrix in metadata. Cards draw mode may not appear "
            "as expected.", file.c_str(), attr.GetPath().GetText());
            return false;
        }
    }
    
    if (worldtoscreen.IsHolding<std::vector<float>>()) {
        return _ConvertToMatrix(
            worldtoscreen.UncheckedGet<std::vector<float>>(), mat);
    } else if (worldtoscreen.IsHolding<std::vector<double>>()) {
        return _ConvertToMatrix(
            worldtoscreen.UncheckedGet<std::vector<double>>(), mat);
    } else if (worldtoscreen.IsHolding<GfMatrix4f>()) {
        *mat = GfMatrix4d(worldtoscreen.UncheckedGet<GfMatrix4f>());
        return true;
    } else if (worldtoscreen.IsHolding<GfMatrix4d>()) {
        *mat = worldtoscreen.UncheckedGet<GfMatrix4d>();
        return true;
    }
    TF_WARN("worldtoscreen metadata holding unexpected type '%s'",
        worldtoscreen.GetTypeName().c_str());
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
UsdImagingDrawModeAdapter::_GenerateTextureCoordinates(
        VtValue *uv, uint8_t axes_mask) const
{
    // This function generates a UV quad per face, with the correct orientation.
    // The order is [X+, X-, Y+, Y-, Z+, Z-], possibly with some of
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
        uv_faces.push_back(
            (axes_mask & xNeg) ? uv_normal.data() : uv_flipped_s.data());
    }
    if (axes_mask & yAxis) {
        uv_faces.push_back(
            (axes_mask & yPos) ? uv_normal.data() : uv_flipped_s.data());
        uv_faces.push_back(
            (axes_mask & yNeg) ? uv_normal.data() : uv_flipped_s.data());
    }
    if (axes_mask & zAxis) {
        // (Z+) and (Z-) need to be flipped on the (t) axis instead of the (s)
        // axis when we're borrowing a texture from the other side of the axis.
        uv_faces.push_back(
            (axes_mask & zPos) ? uv_normal.data() : uv_flipped_t.data());
        uv_faces.push_back(
            (axes_mask & zNeg) ? uv_flipped_st.data() : uv_flipped_s.data());
    }

    VtVec2fArray faceUV = VtVec2fArray(uv_faces.size() * 4);
    for (size_t i = 0; i < uv_faces.size(); ++i) {
        memcpy(&faceUV[i*4], uv_faces[i], 4 * sizeof(GfVec2f));
    }
    *uv = VtValue(faceUV);
}

GfRange3d
UsdImagingDrawModeAdapter::_ComputeExtent(UsdPrim const& prim,
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
UsdImagingDrawModeAdapter::GetCullStyle(UsdPrim const& prim,
                                          SdfPath const& cachePath,
                                          UsdTimeCode time) const
{
    return HdCullStyleBack;
}

/*virtual*/
GfMatrix4d 
UsdImagingDrawModeAdapter::GetTransform(UsdPrim const& prim, 
                                          SdfPath const& cachePath,
                                          UsdTimeCode time,
                                          bool ignoreRootTransform) const
{
    // If the draw mode is instantiated on an instance, prim will be
    // the instance prim, but we want to ignore transforms on that
    // prim since the instance adapter will incorporate it into the per-instance
    // transform and we don't want to double-transform the prim.
    if (prim.IsInstance()) {
        return GfMatrix4d(1.0);
    } else {
        return BaseAdapter::GetTransform(
            prim, prim.GetPath(), time, ignoreRootTransform);
    }
}

PXR_NAMESPACE_CLOSE_SCOPE
