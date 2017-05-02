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
#include "pxr/usdImaging/usdImaging/gprimAdapter.h"

#include "pxr/usdImaging/usdImaging/debugCodes.h"
#include "pxr/usdImaging/usdImaging/delegate.h"
#include "pxr/usdImaging/usdImaging/instancerContext.h"
#include "pxr/usdImaging/usdImaging/tokens.h"

#include "pxr/imaging/hd/perfLog.h"

#include "pxr/usd/usdGeom/gprim.h"

#include "pxr/usd/usdShade/connectableAPI.h"
#include "pxr/usd/usdShade/material.h"
#include "pxr/usd/usdShade/pShader.h"
#include "pxr/usd/usdShade/shader.h"

#include "pxr/usd/usdHydra/shader.h"
#include "pxr/usd/usdHydra/primvar.h"

#include "pxr/base/tf/type.h"

// XXX: feels wrong
#include "pxr/imaging/glf/ptexTexture.h"

PXR_NAMESPACE_OPEN_SCOPE


TF_REGISTRY_FUNCTION(TfType)
{
    typedef UsdImagingGprimAdapter Adapter;
    TfType::Define<Adapter, TfType::Bases<Adapter::BaseAdapter> >();
    // No factory here, GprimAdapter is abstract.
}

UsdImagingGprimAdapter::~UsdImagingGprimAdapter() 
{
}

void 
UsdImagingGprimAdapter::TrackVariabilityPrep(UsdPrim const& prim,
                                             SdfPath const& cachePath,
                                             HdDirtyBits requestedBits,
                                             UsdImagingInstancerContext const* 
                                                 instancerContext)
{
    // Prepopulate cache entries to avoid mutation during multi-threaded data
    // fetch.
    UsdImagingValueCache* valueCache = _GetValueCache();

    if (requestedBits & HdChangeTracker::DirtyVisibility) {
        valueCache->GetVisible(cachePath);
        valueCache->GetPurpose(cachePath);
    }
    if (requestedBits & HdChangeTracker::DirtyPrimVar) {
        valueCache->GetPrimvars(cachePath);
    }
}

void 
UsdImagingGprimAdapter::TrackVariability(UsdPrim const& prim,
                                         SdfPath const& cachePath,
                                         HdDirtyBits requestedBits,
                                         HdDirtyBits* dirtyBits,
                                         UsdImagingInstancerContext const* 
                                             instancerContext)
{
    // WARNING: This method is executed from multiple threads, the value cache
    // has been carefully pre-populated to avoid mutating the underlying
    // container during update.
    
    // Why is this OK? 
    // Either the value is unvarying, in which case the time ordinate doesn't
    // matter; or the value is varying, in which case we will update it upon
    // first call to Delegate::SetTime(). 
    UsdTimeCode time(1.0);

    UsdImagingValueCache* valueCache = _GetValueCache();

    if (requestedBits & HdChangeTracker::DirtyPrimVar) {
        if (!_IsVarying(prim, 
                   UsdGeomTokens->primvarsDisplayColor, 
                   HdChangeTracker::DirtyPrimVar,
                   UsdImagingTokens->usdVaryingPrimVar,
                   dirtyBits,
                   false)) {
            // Only do this second check if the displayColor isn't already known
            // to be varying.
            _IsVarying(prim, 
                   UsdGeomTokens->primvarsDisplayOpacity, 
                   HdChangeTracker::DirtyPrimVar,
                   UsdImagingTokens->usdVaryingPrimVar,
                   dirtyBits,
                   false);
        }
    }

    if (requestedBits & HdChangeTracker::DirtyExtent) {
        // Discover time-varying extent.
        _IsVarying(prim, 
                   UsdGeomTokens->extent, 
                   HdChangeTracker::DirtyExtent,
                   UsdImagingTokens->usdVaryingExtent,
                   dirtyBits,
                   false);
    }

    if (requestedBits & HdChangeTracker::DirtyTransform) {
        // Discover time-varying transforms.
        _IsTransformVarying(prim, 
                   HdChangeTracker::DirtyTransform,
                   UsdImagingTokens->usdVaryingXform,
                   dirtyBits);
    } 

    if (requestedBits & HdChangeTracker::DirtyVisibility) {
        valueCache->GetVisible(cachePath) = GetVisible(prim, time);
        // Discover time-varying visibility.
        _IsVarying(prim, 
                   UsdGeomTokens->visibility, 
                   HdChangeTracker::DirtyVisibility,
                   UsdImagingTokens->usdVaryingVisibility,
                   dirtyBits,
                   true);

        TfToken purpose = _GetPurpose(prim, time);
        // Empty purpose means there is no opinion, fall back to geom.
        if (purpose.IsEmpty())
            purpose = UsdGeomTokens->default_;
        valueCache->GetPurpose(cachePath) = purpose;
    }
}

void 
UsdImagingGprimAdapter::UpdateForTimePrep(UsdPrim const& prim,
                                   SdfPath const& cachePath, 
                                   UsdTimeCode time,
                                   HdDirtyBits requestedBits,
                                   UsdImagingInstancerContext const* 
                                       instancerContext)
{
    UsdImagingValueCache* valueCache = _GetValueCache();
    // Prepopulate cache entries to avoid mutation during multi-threaded data
    // fetch.
    if (requestedBits & HdChangeTracker::DirtyPrimVar)
        valueCache->GetColor(cachePath);

    if (requestedBits & HdChangeTracker::DirtyDoubleSided)
        valueCache->GetDoubleSided(cachePath);

    if (requestedBits & HdChangeTracker::DirtyTransform)
        valueCache->GetTransform(cachePath);

    if (requestedBits & HdChangeTracker::DirtyExtent)
        valueCache->GetExtent(cachePath);

    if (requestedBits & HdChangeTracker::DirtyVisibility)
        valueCache->GetVisible(cachePath);

    if (requestedBits & HdChangeTracker::DirtySurfaceShader)
        valueCache->GetSurfaceShader(cachePath);

    valueCache->GetPrimvars(cachePath);
}

void
UsdImagingGprimAdapter::_DiscoverPrimvars(
        UsdGeomGprim const& gprim,
        SdfPath const& cachePath,
        SdfPath const& shaderPath,
        UsdTimeCode time,
        UsdImagingValueCache* valueCache)
{
    // Check if each parameter/input is bound to a texture or primvar, if so,
    // collect that primvar from this gprim.
    // XXX: Should move this into ShaderAdapter
    if (UsdPrim const& shaderPrim =
                        gprim.GetPrim().GetStage()->GetPrimAtPath(shaderPath)) {
        if (UsdShadeShader s = UsdShadeShader(shaderPrim)) {
            _DiscoverPrimvarsFromShaderNetwork(gprim, cachePath, s, time, valueCache);
        } else {
            _DiscoverPrimvarsDeprecated(gprim, cachePath, 
                                        shaderPrim, time, valueCache);
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

void
UsdImagingGprimAdapter::_DiscoverPrimvarsFromShaderNetwork(UsdGeomGprim const& gprim,
                                          SdfPath const& cachePath, 
                                          UsdShadeShader const& shader,
                                          UsdTimeCode time,
                                          UsdImagingValueCache* valueCache)
{
    // TODO: It might be convenient to implicitly wire up PtexFaceOffset and
    // PtexFaceIndex primvars.
    
    TF_DEBUG(USDIMAGING_SHADERS).Msg("\t Looking for <%s> primvars at <%s>\n",
                            gprim.GetPrim().GetPath().GetText(),
                            shader.GetPrim().GetPath().GetText());
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
                VtValue v;
                UsdGeomPrimvar primvarAttr;
                if (UsdHydraPrimvar(sourceShader).GetVarnameAttr().Get(&t, 
                                            UsdTimeCode::Default())) {
                    primvarAttr = gprim.GetPrimvar(t);
                    if (primvarAttr.ComputeFlattened(&v, time)) {
                        TF_DEBUG(USDIMAGING_SHADERS).Msg("Found primvar %s\n",
                            t.GetText());

                        UsdImagingValueCache::PrimvarInfo primvar;
                        primvar.name = t;
                        primvar.interpolation = primvarAttr.GetInterpolation();
                        valueCache->GetPrimvar(cachePath, t) = v;
                        _MergePrimvar(primvar, &valueCache->GetPrimvars(cachePath));
                    } else {
                        TF_DEBUG(USDIMAGING_SHADERS).Msg(
                            "\t\t No primvar on <%s> named %s\n",
                            gprim.GetPath().GetText(),
                            t.GetText());

                    }
                }
            } else {
                // Recursively look for more primvars
                _DiscoverPrimvarsFromShaderNetwork(gprim, cachePath, UsdShadeShader(source), time, valueCache);
            }
        }
    }
}
void
UsdImagingGprimAdapter::_DiscoverPrimvarsDeprecated(UsdGeomGprim const& gprim,
                                          SdfPath const& cachePath, 
                                          UsdPrim const& shaderPrim,
                                          UsdTimeCode time,
                                          UsdImagingValueCache* valueCache)
{
    UsdImagingValueCache::PrimvarInfo primvar;

    UsdShadeShader shader(shaderPrim);
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
            VtValue v;
            UsdGeomPrimvar primvarAttr;
            texAttr.Get(&ap, UsdTimeCode::Default());

            bool isPtex = GlfIsSupportedPtexTexture(TfToken(ap.GetAssetPath()));
            if (isPtex) {
                t = UsdImagingTokens->ptexFaceIndex;
                // Allow the client to override this name
                texAttr.GetMetadata(UsdImagingTokens->faceIndexPrimvar, &t);
                primvarAttr = gprim.GetPrimvar(t);
                if (primvarAttr) {
                    if (primvarAttr.ComputeFlattened(&v, time)) {
                        primvar.name = t;
                        primvar.interpolation = primvarAttr.GetInterpolation();
                        valueCache->GetPrimvar(cachePath, t) = v;
                        _MergePrimvar(primvar, &valueCache->GetPrimvars(cachePath));
                    }
                }
                t = UsdImagingTokens->ptexFaceOffset;
                // Allow the client to override this name
                texAttr.GetMetadata(UsdImagingTokens->faceOffsetPrimvar, &t);
                primvarAttr = gprim.GetPrimvar(t);
                if (primvarAttr) {
                    primvar.name = t;
                    primvar.interpolation = primvarAttr.GetInterpolation();
                    if (primvarAttr.ComputeFlattened(&v, time)) {
                        valueCache->GetPrimvar(cachePath, t) = v;
                        _MergePrimvar(primvar, &valueCache->GetPrimvars(cachePath));
                    }
                }
            } else {
                texAttr.GetMetadata(UsdImagingTokens->uvPrimvar, &t);
                primvarAttr = gprim.GetPrimvar(t);
                if (TF_VERIFY(primvarAttr, "%s\n", t.GetText())) {
                    if (TF_VERIFY(primvarAttr.ComputeFlattened(&v, time))) {
                        primvar.name = t; // does not include primvars:
                        primvar.interpolation = primvarAttr.GetInterpolation();
                        // Convert double to float, we don't need double precision.
                        if (v.IsHolding<VtVec2dArray>()) {
                            v = VtValue::Cast<VtVec2fArray>(v);
                        }
                        valueCache->GetPrimvar(cachePath, t) = v;
                        _MergePrimvar(primvar, &valueCache->GetPrimvars(cachePath));
                    }
                }
            }
        } else if (UsdAttribute pvAttr = shaderPrim.GetAttribute(
                                        TfToken(attr.GetPath().GetName() 
                                                + ":primvar"))) {
            TfToken t;
            VtValue v;
            UsdGeomPrimvar primvarAttr;
            if (TF_VERIFY(pvAttr.Get(&t, UsdTimeCode::Default()))) {
                primvarAttr = gprim.GetPrimvar(t);
                if (TF_VERIFY(primvarAttr.ComputeFlattened(&v, time))) {
                    primvar.name = t; // does not include primvars:
                    primvar.interpolation = primvarAttr.GetInterpolation();
                    valueCache->GetPrimvar(cachePath, t) = v;
                   _MergePrimvar(primvar, &valueCache->GetPrimvars(cachePath));
                }
            }
        }
    }
}

void 
UsdImagingGprimAdapter::UpdateForTime(UsdPrim const& prim,
                               SdfPath const& cachePath, 
                               UsdTimeCode time,
                               HdDirtyBits requestedBits,
                               HdDirtyBits* resultBits,
                               UsdImagingInstancerContext const* 
                                   instancerContext)
{
    UsdImagingValueCache* valueCache = _GetValueCache();

    if (requestedBits & HdChangeTracker::DirtyPrimVar) {
        // XXX: need to validate gprim schema
        UsdGeomGprim gprim(prim);
        UsdImagingValueCache::PrimvarInfo primvar;
        valueCache->GetColor(cachePath) =
                        GetColorAndOpacity(prim, &primvar, time);
        _MergePrimvar(primvar, &valueCache->GetPrimvars(cachePath));

        // Collect shader required primvars
        SdfPath usdShaderPath = GetShaderBinding(prim);

        // If we're processing this gprim on behalf of an instancer,
        // use the shader binding specified by the instancer if we
        // aren't able to find a shader binding for this prim itself.
        if (instancerContext && usdShaderPath.IsEmpty()) {
            usdShaderPath = instancerContext->instanceSurfaceShaderPath;
        }
        TF_DEBUG(USDIMAGING_SHADERS).Msg("Shader for <%s> is <%s>\n",
                prim.GetPath().GetText(), usdShaderPath.GetText());

        if (!usdShaderPath.IsEmpty()) {
            _DiscoverPrimvars(gprim, cachePath, usdShaderPath, time, valueCache);
        }
    }

    if (requestedBits & HdChangeTracker::DirtyDoubleSided)
        valueCache->GetDoubleSided(cachePath) = _GetDoubleSided(prim);

    if (requestedBits & HdChangeTracker::DirtyTransform) {
        valueCache->GetTransform(cachePath) = GetTransform(prim, time);
    }

    if (requestedBits & HdChangeTracker::DirtyExtent)
        valueCache->GetExtent(cachePath) = _GetExtent(prim, time);

    if (requestedBits & HdChangeTracker::DirtyVisibility)
        valueCache->GetVisible(cachePath) = GetVisible(prim, time);

    if (requestedBits & HdChangeTracker::DirtySurfaceShader)
        valueCache->GetSurfaceShader(cachePath) = _GetSurfaceShader(prim);
}

int
UsdImagingGprimAdapter::ProcessPropertyChange(UsdPrim const& prim,
                                      SdfPath const& cachePath, 
                                      TfToken const& propertyName)
{
    if(propertyName == UsdGeomTokens->visibility 
          || propertyName == UsdGeomTokens->purpose)
        return HdChangeTracker::DirtyVisibility;

    else if (UsdGeomXformable::IsTransformationAffectedByAttrNamed(propertyName))
        return HdChangeTracker::DirtyTransform;

    else if (propertyName == UsdGeomTokens->extent) 
        return HdChangeTracker::DirtyExtent;

    else if (propertyName == UsdGeomTokens->doubleSided) 
        return HdChangeTracker::DirtyDoubleSided;
    
    // TODO: support sparse displayColor updates

    return HdChangeTracker::AllDirty;
}


// -------------------------------------------------------------------------- //

GfRange3d 
UsdImagingGprimAdapter::_GetExtent(UsdPrim const& prim, UsdTimeCode time)
{
    HD_TRACE_FUNCTION();
    HF_MALLOC_TAG_FUNCTION();
    UsdGeomGprim gprim(prim);
    VtVec3fArray extent;
    if (gprim.GetExtentAttr().Get(&extent, time)) {
        // Note:
        // Usd stores extent as 2 float vecs. We do an implicit 
        // conversion to doubles
        return GfRange3d(extent[0], extent[1]);
    } else {
        // Return empty range if no value was found.
        // TODO: Should this compute the extent based on the points instead?
        return GfRange3d();
    }
}

VtValue
UsdImagingGprimAdapter::GetColorAndOpacity(UsdPrim const& prim,
                        UsdImagingValueCache::PrimvarInfo* primvarInfo,
                        UsdTimeCode time)
{
    HD_TRACE_FUNCTION();
    HF_MALLOC_TAG_FUNCTION();
    GfVec3f color(0.5f);
    float opacity = 1.0f;
    VtVec4fArray result(1);
    result[0] = GfVec4f(color[0], color[1], color[2], opacity);
    UsdGeomGprim gprimSchema(prim);

    // XXX: Need to define surface relationship in UsdGeomGprim.
    TfToken colorInterp;
    TfToken opacityInterp;


    // XXX: Primvar values that come from shaders should not be part of
    // the Rprim data, it should live as part of the shader so it can be shared,
    // though that poses some interesting questions for vertex & varying rate
    // shader provided primvars.
    static TfToken displayColorToken("displayColor");
    static TfToken displayOpacityToken("displayOpacity");

    UsdRelationship mat = UsdShadeMaterial::GetBindingRel(prim);
    SdfPathVector matTargets;
    if (mat.GetForwardedTargets(&matTargets)) {
        if (!matTargets.empty()) {
            if (matTargets.size() > 1) {
                TF_WARN("<%s> has more than one material target; "\
                        "using first one found: <%s>",
                        prim.GetPath().GetText(),
                        matTargets.front().GetText());
            }
            UsdPrim matPrim(
                prim.GetStage()->GetPrimAtPath(matTargets.front()));

            if (matPrim &&
                matPrim.GetAttribute(displayColorToken).Get(&color, time)) {
                colorInterp = UsdGeomTokens->constant; 
                result[0] = GfVec4f(color[0], color[1], color[2], opacity);
            }

            if (matPrim &&
                matPrim.GetAttribute(displayOpacityToken).Get(&opacity, time)) {
                opacityInterp = UsdGeomTokens->constant;
                result[0][3] = opacity;
            }
        }
    }

    // XXX: Primvar values that come from shaders should not be part of
    // the Rprim data, it should live as part of the shader so it can be shared,
    // though that poses some interesting questions for vertex & varying rate
    // shader provided primvars.
    // XXX SHOULD OBSOLETE THIS WHEN LOOK IS THE DEFAULT SHADING MODEL
    if (colorInterp.IsEmpty() && opacityInterp.IsEmpty()) {
        static TfToken surfaceToken("surface");
        UsdRelationship surface = prim.GetRelationship(surfaceToken);
        SdfPathVector surfaceTargets;
        if (surface.GetForwardedTargets(&surfaceTargets)) {
            if (!surfaceTargets.empty()) {
                if (surfaceTargets.size() > 1) {
                    TF_WARN("<%s> has more than one surface target; "\
                        "using first one found: <%s>",
                        prim.GetPath().GetText(),
                        surfaceTargets.front().GetText());
                }
                UsdShadePShader shaderSchema(
                    prim.GetStage()->GetPrimAtPath(surfaceTargets.front()));
                if (shaderSchema &&
                    shaderSchema.GetDisplayColorAttr().Get(&color, time)) {
                    colorInterp = UsdGeomTokens->constant; 
                    result[0] = GfVec4f(color[0], color[1], color[2], opacity);
                }
                if (shaderSchema &&
                    shaderSchema.GetDisplayOpacityAttr().Get(&opacity, time)) {
                    opacityInterp = UsdGeomTokens->constant;
                    result[0][3] = opacity;
                }
            }
        }
    }
    if (colorInterp.IsEmpty()) {
        VtArray<GfVec3f> colorArray;
        UsdGeomPrimvar primvar = gprimSchema.GetDisplayColorPrimvar();
        if (primvar.ComputeFlattened(&colorArray, time)) {
            colorInterp = primvar.GetInterpolation();
            result.resize(colorArray.size());
            for (size_t i = 0; i < colorArray.size(); i++) {
                GfVec3f& c = colorArray[i];
                result[i] = GfVec4f(c[0], c[1], c[2], opacity);
            }
        } else {
            colorInterp = UsdGeomTokens->constant;
            result[0] = GfVec4f(color[0], color[1], color[2], opacity);
        }
    }
    if (opacityInterp.IsEmpty()) {
        VtArray<float> opacityArray;
        UsdGeomPrimvar primvar = gprimSchema.GetDisplayOpacityPrimvar();
        if (primvar.ComputeFlattened(&opacityArray, time)) {
            opacityInterp = primvar.GetInterpolation();
            if (colorInterp == opacityInterp 
                || result.size() == opacityArray.size()) {
                if (TF_VERIFY(result.size() == opacityArray.size(),
                            "Color and Opacity have the same interpolation, "
                            "but different lengths (color:%lu, opacity:%lu) "
                            "for prim: %s",
                            result.size(), opacityArray.size(),
                            prim.GetPath().GetText())) {
                    for (size_t i = 0; i < result.size(); ++i) {
                        result[i][3] = opacityArray[i];
                    }
                }
            } else {
                // We have conflicting interpolation modes, must find LCD.
                if (result.size() > opacityArray.size()) {
                    if (opacityArray.size() == 1) {
                        for (size_t i = 0; i < result.size(); ++i) {
                            result[i][3] = opacityArray[0];
                        }
                    } else {
                        // TODO: splice the primvars together, this logic
                        // depends on the prim type.
                        for (size_t i = 0; i < result.size(); ++i) {
                            result[i][3] = opacityArray[0];
                        }
                        TF_WARN("color has interpolation %s but opacity has "
                                "interpolation %s, this combination not yet "
                                "supported by UsdImaging",
                                colorInterp.GetText(), opacityInterp.GetText());
                    }
                } else {
                    if (result.size() == 1) {
                        result.resize(opacityArray.size());
                        for (size_t i = 0; i < result.size(); ++i) {
                            result[i] = GfVec4f(result[0][0], result[0][1], 
                                                result[0][2], opacityArray[0]);
                        }
                        // We just changed the interpolation, so we need to
                        // change the declared interpolation as well.
                        colorInterp = opacityInterp;
                    } else {
                        // TODO: splice the primvars together, this logic
                        // depends on the prim type.
                        for (size_t i = 0; i < result.size(); ++i) {
                            result[i][3] = opacityArray[0];
                        }
                        TF_WARN("color has interpolation %s but opacity has "
                                "interpolation %s, this combination not yet "
                                "supported by UsdImaging",
                                colorInterp.GetText(), opacityInterp.GetText());
                    }
                }
            }
        }
    }
    if (primvarInfo) {
        primvarInfo->name = HdTokens->color;
        primvarInfo->interpolation = colorInterp;
    }
    return VtValue(result);
}

TfToken
UsdImagingGprimAdapter::_GetPurpose(UsdPrim const& prim, UsdTimeCode time)
{
    HD_TRACE_FUNCTION();
    // PERFORMANCE: Make this more efficient, see http://bug/90497
    return UsdGeomImageable(prim).ComputePurpose();
}

bool 
UsdImagingGprimAdapter::_GetDoubleSided(UsdPrim const& prim)
{
    HD_TRACE_FUNCTION();
    HF_MALLOC_TAG_FUNCTION();

    if(!TF_VERIFY(prim.IsA<UsdGeomGprim>(), "%s\n",
                prim.GetPath().GetText()))
        return false;

    return _Get<bool>(prim, UsdGeomTokens->doubleSided, UsdTimeCode::Default());
}

SdfPath 
UsdImagingGprimAdapter::_GetSurfaceShader(UsdPrim const& prim)
{
    HD_TRACE_FUNCTION();
    HF_MALLOC_TAG_FUNCTION();

    return GetShaderBinding(prim);
}

PXR_NAMESPACE_CLOSE_SCOPE

