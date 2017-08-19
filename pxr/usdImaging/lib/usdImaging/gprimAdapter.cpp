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
UsdImagingGprimAdapter::_ComputeAndMergePrimvar(
    UsdGeomGprim const& gprim,
    SdfPath const& cachePath,
    TfToken const& primvarName,
    UsdTimeCode time,
    UsdImagingValueCache* valueCache)
{
    UsdGeomPrimvar primvarAttr = gprim.GetPrimvar(primvarName);

    if (!primvarAttr) {
        return;
    }

    VtValue v;
    if (primvarAttr.ComputeFlattened(&v, time)) {

        TF_DEBUG(USDIMAGING_SHADERS).Msg("Found primvar %s\n",
            primvarName.GetText());

        UsdImagingValueCache::PrimvarInfo primvar;
        primvar.name = primvarName;
        primvar.interpolation = primvarAttr.GetInterpolation();
        valueCache->GetPrimvar(cachePath, primvar.name) = v;
        _MergePrimvar(primvar, &valueCache->GetPrimvars(cachePath));

    } else {

        TF_DEBUG(USDIMAGING_SHADERS).Msg( "\t\t No primvar on <%s> named %s\n",
            gprim.GetPath().GetText(), primvarName.GetText());
    }
}

void
UsdImagingGprimAdapter::_DiscoverPrimvarsFromShaderNetwork(
    UsdGeomGprim const& gprim,
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
                if (UsdHydraPrimvar(sourceShader).GetVarnameAttr().Get(&t, 
                                            UsdTimeCode::Default())) {
                    _ComputeAndMergePrimvar(
                        gprim, cachePath, t, time, valueCache);
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
            texAttr.Get(&ap, UsdTimeCode::Default());

            bool isPtex = GlfIsSupportedPtexTexture(TfToken(ap.GetAssetPath()));
            if (isPtex) {

                t = UsdImagingTokens->ptexFaceIndex;
                // Allow the client to override this name
                texAttr.GetMetadata(UsdImagingTokens->faceIndexPrimvar, &t);
                _ComputeAndMergePrimvar(gprim, cachePath, t, time, valueCache);

                t = UsdImagingTokens->ptexFaceOffset;
                // Allow the client to override this name
                texAttr.GetMetadata(UsdImagingTokens->faceOffsetPrimvar, &t);
                _ComputeAndMergePrimvar(gprim, cachePath, t, time, valueCache);

            } else {
                texAttr.GetMetadata(UsdImagingTokens->uvPrimvar, &t);
                UsdGeomPrimvar primvarAttr = gprim.GetPrimvar(t);
                if (TF_VERIFY(primvarAttr, "%s\n", t.GetText())) {
                    VtValue v;
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
            if (TF_VERIFY(pvAttr.Get(&t, UsdTimeCode::Default()))) {
                _ComputeAndMergePrimvar(gprim, cachePath, t, time, valueCache);
            }
        }
    }
}

void 
UsdImagingGprimAdapter::UpdateForTime(UsdPrim const& prim,
                               SdfPath const& cachePath, 
                               UsdTimeCode time,
                               HdDirtyBits requestedBits,
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
    const GfVec3f defaultColor(0.5f);
    const float defaultOpacity = 1.0f;
    VtVec4fArray result(1);
    result[0] = GfVec4f(defaultColor[0],
                        defaultColor[1],
                        defaultColor[2],
                        defaultOpacity);

    size_t numColors = 1, numOpacities = 1;
    TfToken colorInterp, opacityInterp;
    TfToken colorPrimvarName, opacityPrimvarName;

    // for a prim's color & opacity, we use the following precedence:
    // material rel >  shader rel > local prim vars   
    {
        // -- Material --        
        // XXX: Primvar values that come from shaders should not be part of
        // the Rprim data, it should live as part of the shader so it can be 
        // shared, though that poses some interesting questions for vertex & 
        // varying rate shader provided primvars.
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

                GfVec3f matColor;
                if (matPrim &&
                    matPrim.GetAttribute(displayColorToken)
                        .Get(&matColor, time)) {
                    colorInterp = UsdGeomTokens->constant; 
                    colorPrimvarName = displayColorToken;
                    result[0][0] = matColor[0];
                    result[0][1] = matColor[1];
                    result[0][2] = matColor[2];
                }

                float matOpacity;
                if (matPrim &&
                    matPrim.GetAttribute(displayOpacityToken)
                        .Get(&matOpacity, time)) {
                    opacityInterp = UsdGeomTokens->constant;
                    opacityPrimvarName = displayOpacityToken;
                    result[0][3] = matOpacity;
                }
            }
        }
    }

    {
        // -- Prim local prim var --
        UsdGeomGprim gprimSchema(prim);
 
        if (colorInterp.IsEmpty()) {
            VtArray<GfVec3f> colorArray;
                const UsdGeomPrimvar& primvar = 
                    gprimSchema.GetDisplayColorPrimvar();
            if (primvar.ComputeFlattened(&colorArray, time)) {
                colorInterp = primvar.GetInterpolation();
                colorPrimvarName = primvar.GetName();
                numColors = colorArray.size();
                result.resize(numColors);

                if (colorInterp == UsdGeomTokens->constant) {
                    result[0][0] = colorArray[0][0];
                    result[0][1] = colorArray[0][1];
                    result[0][2] = colorArray[0][2];

                    if (numColors > 1) {
                        // warn and copy default color for remaining elements
                         TF_WARN("Prim %s has %lu elements for %s even "
                                 "though it is marked constant.",
                                 prim.GetPath().GetText(), numColors,
                                 colorPrimvarName.GetText());
                        
                        for (size_t ii = 1; ii < numColors; ii++) {
                            result[ii][0] = defaultColor[0];
                            result[ii][1] = defaultColor[1];
                            result[ii][2] = defaultColor[2];
                        }
                    }
                } else {
                    for (size_t ii = 0; ii < numColors; ii++) {
                        result[ii][0] = colorArray[ii][0];
                        result[ii][1] = colorArray[ii][1];
                        result[ii][2] = colorArray[ii][2];
                    }
                }
            } else {
                // displayColor is treated as a special primvar -- if it isn't
                // authored by the user, the schema defaults it to constant 
                // interp. 
                // if authored with no data (allowed for non-constant interp),
                // we should return an empty result.
                colorInterp = primvar.GetInterpolation();
                if (colorInterp != UsdGeomTokens->constant) {
                    numColors = 0;
                }
            }
        }

        // Guaranteed to have set either material/shader/primvar/default color
        TF_VERIFY(!colorInterp.IsEmpty());

        if (opacityInterp.IsEmpty()) {
                // did not get opacity from either material or shader
            VtArray<float> opacityArray;
                const UsdGeomPrimvar& primvar = 
                    gprimSchema.GetDisplayOpacityPrimvar();
            if (primvar.ComputeFlattened(&opacityArray, time)) {
                opacityInterp = primvar.GetInterpolation();
                opacityPrimvarName = primvar.GetName();
                numOpacities = opacityArray.size();
                if (numOpacities > result.size()) {
                    result.resize(numOpacities);
                }

                // copy just the opacities; color is populated in the 
                // consolidation step
                if (opacityInterp == UsdGeomTokens->constant) {
                    result[0][3] = opacityArray[0];

                    if (numOpacities > 1) {
                        // warn and copy default opacity for remaining elements
                         TF_WARN("Prim %s has %lu elements for %s even "
                                 "though it is marked constant.",
                                 prim.GetPath().GetText(), numOpacities,
                                 opacityPrimvarName.GetText());
                        
                        for (size_t ii = 1; ii < numOpacities; ii++) {
                            result[ii][3] = defaultOpacity;
                        }
                    }
                } else {
                    for (size_t ii = 0; ii < numOpacities; ii++) {
                        result[ii][3] = opacityArray[ii];
                    }
                }
            } else {
                // displayOpacity is treated as a special primvar -- if it isn't
                // authored by the user, the schema defaults it to constant 
                // interp. 
                // if authored with no data (allowed for non-constant interp),
                // we should return an empty result.
                opacityInterp = primvar.GetInterpolation();
                if (opacityInterp != UsdGeomTokens->constant) {
                    numOpacities = 0;
                }
            }
        }
        // Guaranteed to have set either material/shader/primvar/default opacity
        TF_VERIFY(!opacityInterp.IsEmpty());
    }

    {
        // --  Cases where we can surely issue warnings --
        if (colorInterp == opacityInterp && numColors != numOpacities) {
            // interp modes same but lengths different for primvars is surely
            // an input error
            TF_WARN("Prim %s has %lu elements for %s and %lu "
                    "elements for %s even though they have the "
                    "same interpolation mode %s", prim.GetPath().GetText(), 
                    numColors, colorPrimvarName.GetText(),
                    numOpacities, opacityPrimvarName.GetText(),
                    colorInterp.GetText());

        } else if (colorInterp != opacityInterp && 
                   (colorInterp != UsdGeomTokens->constant &&
                    opacityInterp != UsdGeomTokens->constant)) {
            // we can sensibly handle the case of different interp modes with
            // one of them being constant by splatting it across. 
            // for everything else, issue a warning.
            TF_WARN("Prim %s has %s interpolation for %s and %s "
                    "interpolation for %s; this combination is not "
                    "supported by UsdImaging", prim.GetPath().GetText(),
                    colorInterp.GetText(), colorPrimvarName.GetText(),
                    opacityInterp.GetText(), opacityPrimvarName.GetText());
        }
    }

    {
        // -- Consolidate missing color or opacity values in result --
        if (numColors == 0 || numOpacities == 0) {
            // remove default value that was filled in
            result.resize(0);
            // override the (default) color interp mode if opacity was authored
            // and empty
            if (numOpacities == 0) {
                colorInterp = opacityInterp;
            }
        } else {
            const size_t resultSize = result.size();
            if (numColors < numOpacities) {
                GfVec3f splatColor = defaultColor;
                if (colorInterp == UsdGeomTokens->constant) {
                    // override color interp mode and splat first color
                    colorInterp = opacityInterp;
                    splatColor = GfVec3f(
                        result[0][0], result[0][1], result[0][2]);
                }
                for(size_t ii = numColors; ii < resultSize; ii++) {
                    result[ii][0] = splatColor[0];
                    result[ii][1] = splatColor[1];
                    result[ii][2] = splatColor[2];
                }
            }
            else {
                float splatOpacity = defaultOpacity;
                // resultSize may be 0 (if empty color primvar array), so don't 
                // splat 
                if (opacityInterp == UsdGeomTokens->constant) {
                    // splat first opacity
                    splatOpacity = result[0][3];
                }
                for(size_t ii = numOpacities; ii < resultSize; ii++) {
                    result[ii][3] = splatOpacity;
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

