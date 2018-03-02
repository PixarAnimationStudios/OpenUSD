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
#include "pxr/usdImaging/usdImagingGL/textureUtils.h"

#include "pxr/usdImaging/usdImaging/debugCodes.h"
#include "pxr/usdImaging/usdImaging/delegate.h"
#include "pxr/usdImaging/usdImaging/gprimAdapter.h"
#include "pxr/usdImaging/usdImaging/instancerContext.h"
#include "pxr/usdImaging/usdImaging/tokens.h"

#include "pxr/imaging/hd/perfLog.h"
#include "pxr/imaging/hd/material.h"

#include "pxr/imaging/glf/glslfx.h"
#include "pxr/imaging/glf/image.h"
#include "pxr/imaging/pxOsd/tokens.h"

#include "pxr/usd/usdGeom/modelAPI.h"

#include "pxr/base/tf/type.h"

PXR_NAMESPACE_OPEN_SCOPE

TF_DEFINE_PRIVATE_TOKENS(
    _tokens,

    (material)

    (cardsUv)
    (cardsTexAssign)

    (textureXPos)
    (textureYPos)
    (textureZPos)
    (textureXNeg)
    (textureYNeg)
    (textureZNeg)

    (worldtoscreen)
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

UsdImagingGLDrawModeAdapter::~UsdImagingGLDrawModeAdapter()
{
}

SdfPath
UsdImagingGLDrawModeAdapter::Populate(UsdPrim const& prim,
                                    UsdImagingIndexProxy* index,
                                    UsdImagingInstancerContext const*
                                       instancerContext)
{
    // The draw mode adapter only supports models. This is enforced in
    // UsdImagingDelegate::_IsDrawModeApplied.
    if (!TF_VERIFY(prim.IsModel(), "<%s>", prim.GetPath().GetText())) {
        return SdfPath();
    }

    // There should have been a non-default draw mode applied for this
    // adapter to be called; this is enforced in
    // UsdImagingDelegate::_IsDrawModeApplied.
    TfToken drawMode = GetModelDrawMode(prim);
    if (!TF_VERIFY(drawMode != UsdGeomTokens->default_, "<%s>",
        prim.GetPath().GetText())) {
        return SdfPath();
    }

    SdfPath cachePath = UsdImagingGprimAdapter::_ResolveCachePath(
        prim.GetPath(), instancerContext);
    SdfPath instancer = instancerContext ?
        instancerContext->instancerId : SdfPath();
    UsdPrim cachePrim = _GetPrim(cachePath.GetAbsoluteRootOrPrimPath());

    // If this object is instanced, we need to use the instancer adapter for the
    // rprim, which will forward to the draw mode adapter but additionally
    // handle instancer attributes like instance index.
    UsdImagingPrimAdapterSharedPtr rprimAdapter =
        (instancerContext && instancerContext->instancerAdapter) ?
        instancerContext->instancerAdapter :
        shared_from_this();

    if (drawMode == UsdGeomTokens->origin ||
        drawMode == UsdGeomTokens->bounds) {
        // Origin and bounds both draw as basis curves
        if (!index->IsRprimTypeSupported(HdPrimTypeTokens->basisCurves)) {
            TF_WARN("Unable to load cards for model %s, "
                    "basis curves not supported", cachePath.GetText());
            return SdfPath();
        }
        index->InsertRprim(HdPrimTypeTokens->basisCurves,
            cachePath, instancer, cachePrim, rprimAdapter);
        HD_PERF_COUNTER_INCR(UsdImagingTokens->usdPopulatedPrimCount);
    } else if (drawMode == UsdGeomTokens->cards) {
        // Cards draw as a mesh
        if (!index->IsRprimTypeSupported(HdPrimTypeTokens->mesh)) {
            TF_WARN("Unable to load cards for model %s, "
                    "meshes not supported", cachePath.GetText());
            return SdfPath();
        }
        index->InsertRprim(HdPrimTypeTokens->mesh,
            cachePath, instancer, cachePrim, rprimAdapter);
        HD_PERF_COUNTER_INCR(UsdImagingTokens->usdPopulatedPrimCount);

        // Add all of the texture dependencies.
        TfToken textureAttrs[6] = {
            UsdGeomTokens->modelCardTextureXPos,
            UsdGeomTokens->modelCardTextureYPos,
            UsdGeomTokens->modelCardTextureZPos,
            UsdGeomTokens->modelCardTextureXNeg,
            UsdGeomTokens->modelCardTextureYNeg,
            UsdGeomTokens->modelCardTextureZNeg,
        };

        for (int i = 0; i < 6; ++i) {
            UsdAttribute attr = prim.GetAttribute(textureAttrs[i]);
            if (attr && index->IsBprimTypeSupported(HdPrimTypeTokens->texture)
                     && !index->IsPopulated(attr.GetPath())) {
                index->InsertBprim(HdPrimTypeTokens->texture,
                    attr.GetPath(), prim, shared_from_this());
                HD_PERF_COUNTER_INCR(UsdImagingTokens->usdPopulatedPrimCount);
            }
        }
    } else {
        TF_CODING_ERROR("Model <%s> has unsupported drawMode '%s'",
            prim.GetPath().GetText(), drawMode.GetText());
        return SdfPath();
    }

    // Additionally, insert the material.
    SdfPath materialPath = prim.GetPath().AppendProperty(_tokens->material);
    if (index->IsSprimTypeSupported(HdPrimTypeTokens->material) &&
        !index->IsPopulated(materialPath)) {
        index->InsertSprim(HdPrimTypeTokens->material,
            materialPath, prim, shared_from_this());
        HD_PERF_COUNTER_INCR(UsdImagingTokens->usdPopulatedPrimCount);
    }
    return cachePath;
}

bool
UsdImagingGLDrawModeAdapter::_IsMaterialPath(SdfPath const& path)
{
    return IsChildPath(path) && path.GetNameToken() == _tokens->material;
}

bool
UsdImagingGLDrawModeAdapter::_IsTexturePath(SdfPath const& path)
{
    if (!IsChildPath(path)) {
        return false;
    }
    TfToken name = path.GetNameToken();
    return
        name == UsdGeomTokens->modelCardTextureXPos ||
        name == UsdGeomTokens->modelCardTextureYPos ||
        name == UsdGeomTokens->modelCardTextureZPos ||
        name == UsdGeomTokens->modelCardTextureXNeg ||
        name == UsdGeomTokens->modelCardTextureYNeg ||
        name == UsdGeomTokens->modelCardTextureZNeg;
}

void
UsdImagingGLDrawModeAdapter::_RemovePrim(SdfPath const& cachePath,
                                       UsdImagingIndexProxy* index)
{
    if (_IsMaterialPath(cachePath)) {
        index->RemoveSprim(HdPrimTypeTokens->material, cachePath);
    } else if (_IsTexturePath(cachePath)) {
        index->RemoveBprim(HdPrimTypeTokens->texture, cachePath);
    } else {
        index->RemoveRprim(cachePath);
    }
}

void
UsdImagingGLDrawModeAdapter::MarkDirty(UsdPrim const& prim,
                                     SdfPath const& cachePath,
                                     HdDirtyBits dirty,
                                     UsdImagingIndexProxy* index)
{
    if (_IsMaterialPath(cachePath)) {
        index->MarkSprimDirty(cachePath, dirty);
    } else if (_IsTexturePath(cachePath)) {
        index->MarkBprimDirty(cachePath, dirty);
    } else {
        index->MarkRprimDirty(cachePath, dirty);
    }
}

void
UsdImagingGLDrawModeAdapter::MarkTransformDirty(UsdPrim const& prim,
                                              SdfPath const& cachePath,
                                              UsdImagingIndexProxy* index)
{
    if (!_IsMaterialPath(cachePath) && !_IsTexturePath(cachePath)) {
        index->MarkRprimDirty(cachePath, HdChangeTracker::DirtyTransform);
    }
}

void
UsdImagingGLDrawModeAdapter::MarkVisibilityDirty(UsdPrim const& prim,
                                               SdfPath const& cachePath,
                                               UsdImagingIndexProxy* index)
{
    if (!_IsMaterialPath(cachePath) && !_IsTexturePath(cachePath)) {
        index->MarkRprimDirty(cachePath, HdChangeTracker::DirtyVisibility);
    }
}

void
UsdImagingGLDrawModeAdapter::TrackVariability(UsdPrim const& prim,
                                            SdfPath const& cachePath,
                                            HdDirtyBits* timeVaryingBits,
                                            UsdImagingInstancerContext const*
                                               instancerContext)
{
    if (_IsMaterialPath(cachePath) || _IsTexturePath(cachePath)) {
        // Shader/texture aspects aren't time-varying.
        return;
    }

    // WARNING: This method is executed from multiple threads, the value cache
    // has been carefully pre-populated to avoid mutating the underlying
    // container during update.

    // Why is this OK?
    // Either the value is unvarying, in which case the time ordinate doesn't
    // matter; or the value is varying, in which case we will update it upon
    // first call to Delegate::SetTime().
    UsdTimeCode time(1.0);

    UsdImagingValueCache* valueCache = _GetValueCache();

    // Discover time-varying transforms.
    _IsTransformVarying(prim,
            HdChangeTracker::DirtyTransform,
            UsdImagingTokens->usdVaryingXform,
            timeVaryingBits);

    valueCache->GetVisible(cachePath) = GetVisible(prim, time);
    // Discover time-varying visibility.
    _IsVarying(prim,
            UsdGeomTokens->visibility,
            HdChangeTracker::DirtyVisibility,
            UsdImagingTokens->usdVaryingVisibility,
            timeVaryingBits,
            true);

    TfToken purpose = _GetPurpose(prim, time);
    // Empty purpose means there is no opinion, fall back to geom.
    if (purpose.IsEmpty())
        purpose = UsdGeomTokens->default_;
    valueCache->GetPurpose(cachePath) = purpose;
}

void
UsdImagingGLDrawModeAdapter::UpdateForTime(UsdPrim const& prim,
                                         SdfPath const& cachePath,
                                         UsdTimeCode time,
                                         HdDirtyBits requestedBits,
                                         UsdImagingInstancerContext const*
                                            instancerContext)
{
    UsdImagingValueCache* valueCache = _GetValueCache();
    UsdGeomModelAPI model(prim);

    if (_IsTexturePath(cachePath)) {
        // textures don't currently use UpdateForTime().
        return;
    }

    if (_IsMaterialPath(cachePath)) {
        // DirtySurfaceShader indicates we should return the shader source.
        if (requestedBits & HdMaterial::DirtySurfaceShader) {
            valueCache->GetSurfaceShaderSource(cachePath) =
                _GetSurfaceShaderSource();
            valueCache->GetDisplacementShaderSource(cachePath) =
                std::string();
        }

        // DirtyParams indicates we should return material bindings;
        // in our case, loop through the texture attributes to see
        // which ones to add. Use the draw mode color as a fallback value.
        if (requestedBits & HdMaterial::DirtyParams) {
            HdMaterialParamVector params;
            UsdAttribute attr;

            TfToken textureAttrs[6] = {
                UsdGeomTokens->modelCardTextureXPos,
                UsdGeomTokens->modelCardTextureYPos,
                UsdGeomTokens->modelCardTextureZPos,
                UsdGeomTokens->modelCardTextureXNeg,
                UsdGeomTokens->modelCardTextureYNeg,
                UsdGeomTokens->modelCardTextureZNeg,
            };
            TfToken textureNames[6] = {
                _tokens->textureXPos,
                _tokens->textureYPos,
                _tokens->textureZPos,
                _tokens->textureXNeg,
                _tokens->textureYNeg,
                _tokens->textureZNeg,
            };

            GfVec3f schemaColor = GfVec3f(0.18f, 0.18f, 0.18f);
            UsdAttribute drawModeColorAttr =
                model.GetModelDrawModeColorAttr();
            if (drawModeColorAttr) {
                drawModeColorAttr.Get(&schemaColor);
            }
            VtValue fallback = VtValue(GfVec4f(
                schemaColor[0], schemaColor[1], schemaColor[2], 1.0f));

            TfTokenVector samplerParams = { _tokens->cardsUv };
            for (int i = 0; i < 6; ++i) {
                attr = prim.GetAttribute(textureAttrs[i]);
                if (attr) {
                    params.push_back(HdMaterialParam(
                                textureNames[i], fallback,
                                attr.GetPath(), samplerParams, false));
                }
            }
            valueCache->GetMaterialParams(cachePath) = params;
        }
    }

    // Geometry aspect
    PrimvarInfoVector& primvars = valueCache->GetPrimvars(cachePath);

    if (requestedBits & HdChangeTracker::DirtyTransform) {
        valueCache->GetTransform(cachePath) = GetTransform(prim, time);
    }

    if (requestedBits & HdChangeTracker::DirtyVisibility) {
        valueCache->GetVisible(cachePath) = GetVisible(prim, time);
    }

    if (requestedBits & HdChangeTracker::DirtyDoubleSided) {
        valueCache->GetDoubleSided(cachePath) = true;
    }

    if (requestedBits & HdChangeTracker::DirtyMaterialId) {
        SdfPath materialPath = prim.GetPath().AppendProperty(_tokens->material);
        valueCache->GetMaterialId(cachePath) = materialPath;
    }

    if (requestedBits & HdChangeTracker::DirtyWidths) {
        VtFloatArray widths = VtFloatArray(1);
        widths[0] = 1.0f;
        valueCache->GetWidths(cachePath) = VtValue(widths);

        UsdImagingValueCache::PrimvarInfo primvar;
        primvar.name = UsdGeomTokens->widths;
        primvar.interpolation = UsdGeomTokens->constant;
        _MergePrimvar(primvar, &primvars);
    }

    if (requestedBits & HdChangeTracker::DirtyPrimVar) {
        VtVec4fArray color = VtVec4fArray(1);
        // Default color to 18% gray.
        GfVec3f schemaColor= GfVec3f(0.18f, 0.18f, 0.18f);
        UsdAttribute drawModeColorAttr = model.GetModelDrawModeColorAttr();
        if (drawModeColorAttr) {
            drawModeColorAttr.Get(&schemaColor);
        }
        color[0] = GfVec4f(schemaColor[0], schemaColor[1], schemaColor[2], 1);
        valueCache->GetColor(cachePath) = color;

        UsdImagingValueCache::PrimvarInfo primvar;
        primvar.name = HdTokens->color;
        primvar.interpolation = UsdGeomTokens->constant;
        _MergePrimvar(primvar, &primvars);
    }

    // If we're updating geometry, we need to compute the draw mode.
    TfToken drawMode = UsdGeomTokens->default_, 
            cardGeometry = UsdGeomTokens->cross;

    // We compute all of the below items together, since their derivations
    // aren't easily separable.
    HdDirtyBits geometryBits = HdChangeTracker::DirtyTopology |
                               HdChangeTracker::DirtyPoints |
                               HdChangeTracker::DirtyPrimVar |
                               HdChangeTracker::DirtyExtent;

    if (requestedBits & geometryBits) {
        drawMode = GetModelDrawMode(prim);
        UsdAttribute cardGeometryAttr = model.GetModelCardGeometryAttr();
        if (cardGeometryAttr)
            cardGeometryAttr.Get(&cardGeometry);

        VtValue& topology = valueCache->GetTopology(cachePath);
        VtValue& points = valueCache->GetPoints(cachePath);
        GfRange3d& extent = valueCache->GetExtent(cachePath);

        // Unless we're in cards "fromTexture" mode, compute the extents.
        if (!(drawMode == UsdGeomTokens->cards &&
              cardGeometry == UsdGeomTokens->fromTexture)) {
            extent = _ComputeExtent(prim);
        }

        if (drawMode == UsdGeomTokens->origin) {
            _GenerateOriginGeometry(&topology, &points, extent);
        } else if (drawMode == UsdGeomTokens->bounds) {
            _GenerateBoundsGeometry(&topology, &points, extent);
        } else if (drawMode == UsdGeomTokens->cards) {
            VtValue& uv = valueCache->GetPrimvar(cachePath,
                    _tokens->cardsUv);
            VtValue& assign = valueCache->GetPrimvar(cachePath,
                    _tokens->cardsTexAssign);

            if (cardGeometry == UsdGeomTokens->fromTexture) {
                // In "fromTexture" mode, read all the geometry data in from
                // the textures.
                _GenerateCardsFromTextureGeometry(&topology, &points,
                        &uv, &assign, &extent, prim);
            } else {
                // Generate mask for suppressing axes with no textures
                uint8_t axes_mask = 0;
                if (model.GetModelCardTextureXPosAttr()) axes_mask |= xPos;
                if (model.GetModelCardTextureXNegAttr()) axes_mask |= xNeg;
                if (model.GetModelCardTextureYPosAttr()) axes_mask |= yPos;
                if (model.GetModelCardTextureYNegAttr()) axes_mask |= yNeg;
                if (model.GetModelCardTextureZPosAttr()) axes_mask |= zPos;
                if (model.GetModelCardTextureZNegAttr()) axes_mask |= zNeg;

                // If no textures are bound, generate the full geometry.
                if (axes_mask == 0) { axes_mask = xAxis | yAxis | zAxis; }

                // Generate UVs.
                _GenerateTextureCoordinates(&uv, &assign, axes_mask);

                // Generate geometry based on card type.
                if (cardGeometry == UsdGeomTokens->cross) {
                    _GenerateCardsCrossGeometry(&topology, &points, extent,
                        axes_mask);
                } else if (cardGeometry == UsdGeomTokens->box) {
                    _GenerateCardsBoxGeometry(&topology, &points, extent,
                        axes_mask);
                } else {
                    TF_CODING_ERROR("<%s> Unexpected card geometry mode %s",
                        prim.GetPath().GetText(), cardGeometry.GetText());
                }
            }

            // Merge "cardsUv" and "cardsTexAssign" primvars
            UsdImagingValueCache::PrimvarInfo primvar;
            primvar.name = _tokens->cardsUv;
            primvar.interpolation = UsdGeomTokens->faceVarying;
            _MergePrimvar(primvar, &primvars);

            primvar.name = _tokens->cardsTexAssign;
            primvar.interpolation = UsdGeomTokens->uniform;
            _MergePrimvar(primvar, &primvars);
        } else {
            TF_CODING_ERROR("<%s> Unexpected draw mode %s",
                prim.GetPath().GetText(), drawMode.GetText());
        }

        // Merge "points" primvar
        UsdImagingValueCache::PrimvarInfo primvar;
        primvar.name = HdTokens->points;
        primvar.interpolation = UsdGeomTokens->vertex;
        _MergePrimvar(primvar, &primvars);
    }
}

HdDirtyBits
UsdImagingGLDrawModeAdapter::ProcessPropertyChange(UsdPrim const& prim,
                                                 SdfPath const& cachePath,
                                                 TfToken const& propertyName)
{
    if (propertyName == UsdGeomTokens->modelDrawModeColor)
        return HdChangeTracker::DirtyPrimVar;
    else if (propertyName == UsdGeomTokens->modelCardGeometry)
        return (HdChangeTracker::DirtyTopology | HdChangeTracker::DirtyPoints);
    else if (propertyName == UsdGeomTokens->extent)
        return (HdChangeTracker::DirtyExtent | HdChangeTracker::DirtyPoints);
    else if (propertyName == UsdGeomTokens->visibility ||
             propertyName == UsdGeomTokens->purpose)
        return HdChangeTracker::DirtyVisibility;
    else if (propertyName == UsdGeomTokens->doubleSided)
        return HdChangeTracker::Clean;
    else if (UsdGeomXformable::IsTransformationAffectedByAttrNamed(
                propertyName))
        return HdChangeTracker::DirtyTransform;

    return HdChangeTracker::AllDirty;
}

void
UsdImagingGLDrawModeAdapter::_GenerateOriginGeometry(
        VtValue *topo, VtValue *points, GfRange3d const& extents)
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
        VtValue *topo, VtValue *points, GfRange3d const& extents)
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
        uint8_t axes_mask)
{
    // Cards (Cross) vertices:
    // - +/-X vertices (CCW wrt +X)
    // - +/-Y vertices (CCW wrt +Y)
    // - +/-Z vertices (CCW wrt +Z)
    GfVec3f min = GfVec3f(extents.GetMin()),
            max = GfVec3f(extents.GetMax()),
            mid = (min+max)/2.0f;
    VtVec3fArray pt = VtVec3fArray(24);
    float eps = 0.00001f;

    // +X
    pt[ 0] = GfVec3f(mid[0] + eps, min[1], min[2]);
    pt[ 1] = GfVec3f(mid[0] + eps, max[1], min[2]);
    pt[ 2] = GfVec3f(mid[0] + eps, max[1], max[2]);
    pt[ 3] = GfVec3f(mid[0] + eps, min[1], max[2]);

    // -X
    pt[ 4] = GfVec3f(mid[0] - eps, min[1], min[2]);
    pt[ 5] = GfVec3f(mid[0] - eps, max[1], min[2]);
    pt[ 6] = GfVec3f(mid[0] - eps, max[1], max[2]);
    pt[ 7] = GfVec3f(mid[0] - eps, min[1], max[2]);

    // +Y
    pt[ 8] = GfVec3f(min[0], mid[1] + eps, min[2]);
    pt[ 9] = GfVec3f(max[0], mid[1] + eps, min[2]);
    pt[10] = GfVec3f(max[0], mid[1] + eps, max[2]);
    pt[11] = GfVec3f(min[0], mid[1] + eps, max[2]);

    // -Y
    pt[12] = GfVec3f(min[0], mid[1] - eps, min[2]);
    pt[13] = GfVec3f(max[0], mid[1] - eps, min[2]);
    pt[14] = GfVec3f(max[0], mid[1] - eps, max[2]);
    pt[15] = GfVec3f(min[0], mid[1] - eps, max[2]);

    // +Z
    pt[16] = GfVec3f(min[0], min[1], mid[2] + eps);
    pt[17] = GfVec3f(max[0], min[1], mid[2] + eps);
    pt[18] = GfVec3f(max[0], max[1], mid[2] + eps);
    pt[19] = GfVec3f(min[0], max[1], mid[2] + eps);

    // -Z
    pt[20] = GfVec3f(min[0], min[1], mid[2] - eps);
    pt[21] = GfVec3f(max[0], min[1], mid[2] - eps);
    pt[22] = GfVec3f(max[0], max[1], mid[2] - eps);
    pt[23] = GfVec3f(min[0], max[1], mid[2] - eps);

    // Generate one face per axis direction, for included axes.
    int numFaces = ((axes_mask & zAxis) ? 2 : 0) +
                   ((axes_mask & yAxis) ? 2 : 0) +
                   ((axes_mask & xAxis) ? 2 : 0);

    VtIntArray faceCounts = VtIntArray(numFaces);
    for (int i = 0; i < numFaces; ++i) { faceCounts[i] = 4; }

    const int x_indices[8] = {  2,  3,  0,  1,  7,  6,  5,  4 };
    const int y_indices[8] = { 11, 10,  9,  8, 14, 15, 12, 13 };
    const int z_indices[8] = { 19, 18, 17, 16, 23, 22, 21, 20 };
    VtIntArray faceIndices = VtIntArray(numFaces * 4);
    int dest = 0;
    if (axes_mask & xAxis) {
        memcpy(&faceIndices[dest], x_indices, 8 * sizeof(int));
        dest += 8;
    }
    if (axes_mask & yAxis) {
        memcpy(&faceIndices[dest], y_indices, 8 * sizeof(int));
        dest += 8;
    }
    if (axes_mask & zAxis) {
        memcpy(&faceIndices[dest], z_indices, 8 * sizeof(int));
        dest += 8;
    }

    VtIntArray holeIndices(0);

    HdMeshTopology topology(
        PxOsdOpenSubdivTokens->none, PxOsdOpenSubdivTokens->rightHanded,
        faceCounts, faceIndices, holeIndices);

    // Hydra expects the points buffer to be as big as the largest index,
    // so if we suppressed certain faces we may need to resize "points".
    if (!(axes_mask & zAxis)) {
        if (!(axes_mask & yAxis)) {
            pt.resize(8);
        } else {
            pt.resize(16);
        }
    }

    *points = VtValue(pt);
    *topo = VtValue(topology);
}

void
UsdImagingGLDrawModeAdapter::_GenerateCardsBoxGeometry(
        VtValue *topo, VtValue *points, GfRange3d const& extents,
        uint8_t axes_mask)
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

    // Generate one face per axis direction, for included axes.
    int numFaces = ((axes_mask & zAxis) ? 2 : 0) +
                   ((axes_mask & yAxis) ? 2 : 0) +
                   ((axes_mask & xAxis) ? 2 : 0);

    VtIntArray faceCounts = VtIntArray(numFaces);
    for (int i = 0; i < numFaces; ++i) { faceCounts[i] = 4; }

    const int x_indices[8] = { 7, 5, 4, 6, 1, 3, 2, 0 };
    const int y_indices[8] = { 3, 7, 6, 2, 5, 1, 0, 4 };
    const int z_indices[8] = { 3, 7, 5, 1, 2, 6, 4, 0 };
    VtIntArray faceIndices = VtIntArray(numFaces * 4);
    int dest = 0;
    if (axes_mask & xAxis) {
        memcpy(&faceIndices[dest], x_indices, 8 * sizeof(int));
        dest += 8;
    }
    if (axes_mask & yAxis) {
        memcpy(&faceIndices[dest], y_indices, 8 * sizeof(int));
        dest += 8;
    }
    if (axes_mask & zAxis) {
        memcpy(&faceIndices[dest], z_indices, 8 * sizeof(int));
        dest += 8;
    }

    VtIntArray holeIndices(0);

    HdMeshTopology topology(
        UsdGeomTokens->none, UsdGeomTokens->rightHanded,
        faceCounts, faceIndices, holeIndices);
    *topo = VtValue(topology);
}

void
UsdImagingGLDrawModeAdapter::_GenerateCardsFromTextureGeometry(
        VtValue *topo, VtValue *points, VtValue *uv, VtValue *assign,
        GfRange3d *extents, UsdPrim const& prim)
{
    UsdGeomModelAPI model(prim);
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

    GfVec3f corners[4] = { GfVec3f(-1, -1, 0), GfVec3f(1, -1, 0),
                           GfVec3f(1, 1, 0), GfVec3f(-1, 1, 0) };
    GfVec2f std_uvs[4] = { GfVec2f(0,0), GfVec2f(1,0),
                           GfVec2f(1,1), GfVec2f(0,1) };

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

bool
UsdImagingGLDrawModeAdapter::_GetMatrixFromImageMetadata(
    UsdAttribute const& attr, GfMatrix4d *mat)
{
    // This function expects the input attribute to be an image asset path.
    SdfAssetPath asset;
    if (!attr || !attr.Get(&asset)) {
        return false;
    }

    std::string file = asset.GetResolvedPath();
    // Fallback to the literal path if it couldn't be resolved.
    if (file.empty()) {
        file = asset.GetAssetPath();
    }

    GlfImageSharedPtr img = GlfImage::OpenForReading(file);
    if (!img) {
        return false;
    }

    // Read the "worldtoscreen" metadata, as a vector that we expect to be
    // of size 16, and matrixify it.
    std::vector<float> mvec;
    if (!(img->GetMetadata(_tokens->worldtoscreen, &mvec)) ||
        mvec.size() != 16) {
        return false;
    }
    mat->Set(mvec[ 0], mvec[ 1], mvec[ 2], mvec[ 3],
             mvec[ 4], mvec[ 5], mvec[ 6], mvec[ 7],
             mvec[ 8], mvec[ 9], mvec[10], mvec[11],
             mvec[12], mvec[13], mvec[14], mvec[15]);
    return true;
}

void
UsdImagingGLDrawModeAdapter::_GenerateTextureCoordinates(
        VtValue *uv, VtValue *assign, uint8_t axes_mask)
{
    // Note: this function depends on the vertex order of the generated
    // card faces. Per spec, the texture axes are:
    //  card +x: local (-y,-z)
    //  card -x: local (y,-z)
    //  card +y: local (x,-z)
    //  card -y: local (-x,-z)
    //  card +z: local (x,-y)
    //  card -z: local (x,-y)
    // .. meaning, for example: card x uv (0,0) maps to (y,z), and uv (1,1)
    // maps to (-y,-z).
    //
    // This function generates face-varying UVs, and also uniform indices
    // for each face specifying which texture to sample.

    const GfVec2f uv_normal[4] =
        { GfVec2f(0,0), GfVec2f(1,0), GfVec2f(1,1), GfVec2f(0,1) };
    const GfVec2f uv_flipped[4] =
        { GfVec2f(1,0), GfVec2f(0,0), GfVec2f(0,1), GfVec2f(1,1) };

    std::vector<const GfVec2f*> uv_faces;
    std::vector<int> face_assign;
    if (axes_mask & xAxis) {
        uv_faces.push_back((axes_mask & xPos) ? uv_normal : uv_flipped);
        face_assign.push_back((axes_mask & xPos) ? xPos : xNeg);
        uv_faces.push_back((axes_mask & xNeg) ? uv_normal : uv_flipped);
        face_assign.push_back((axes_mask & xNeg) ? xNeg : xPos);
    }
    if (axes_mask & yAxis) {
        uv_faces.push_back((axes_mask & yPos) ? uv_normal : uv_flipped);
        face_assign.push_back((axes_mask & yPos) ? yPos : yNeg);
        uv_faces.push_back((axes_mask & yNeg) ? uv_normal : uv_flipped);
        face_assign.push_back((axes_mask & yNeg) ? yNeg : yPos);
    }
    if (axes_mask & zAxis) {
        uv_faces.push_back((axes_mask & zPos) ? uv_normal : uv_flipped);
        face_assign.push_back((axes_mask & zPos) ? zPos : zNeg);
        uv_faces.push_back((axes_mask & zNeg) ? uv_normal : uv_flipped);
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

std::string
UsdImagingGLDrawModeAdapter::_GetSurfaceShaderSource()
{
    GlfGLSLFX gfx (UsdImagingGLPackageDrawModeShader());
    if (!gfx.IsValid()) {
        TF_CODING_ERROR("Couldn't load UsdImagingPackageDrawModeShader");
        return std::string();
    }
    return gfx.GetSurfaceSource();
}

GfRange3d
UsdImagingGLDrawModeAdapter::_ComputeExtent(UsdPrim const& prim)
{
    HD_TRACE_FUNCTION();
    HF_MALLOC_TAG_FUNCTION();

    TfTokenVector purposes = { UsdGeomTokens->default_, UsdGeomTokens->proxy,
                               UsdGeomTokens->render };
    UsdGeomBBoxCache bboxCache(UsdTimeCode::EarliestTime(), purposes, true);
    return bboxCache.ComputeUntransformedBound(prim).ComputeAlignedBox();
}

TfToken
UsdImagingGLDrawModeAdapter::_GetPurpose(UsdPrim const& prim, UsdTimeCode time)
{
    HD_TRACE_FUNCTION();
    // PERFORMANCE: Make this more efficient, see http://bug/90497
    return UsdGeomImageable(prim).ComputePurpose();
}


HdTextureResource::ID
UsdImagingGLDrawModeAdapter::GetTextureResourceID(UsdPrim const& usdPrim,
                                                       SdfPath const &id,
                                                       UsdTimeCode time,
                                                       size_t salt) const
{
    return UsdImagingGL_GetTextureResourceID(usdPrim, id, time, salt);
}

HdTextureResourceSharedPtr
UsdImagingGLDrawModeAdapter::GetTextureResource(UsdPrim const& usdPrim,
                                                     SdfPath const &id,
                                                     UsdTimeCode time) const
{
    return UsdImagingGL_GetTextureResource(usdPrim, id, time);
}

PXR_NAMESPACE_CLOSE_SCOPE
