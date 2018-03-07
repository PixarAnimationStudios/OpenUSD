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
#include "pxr/usd/usdGeom/curves.h"
#include "pxr/usd/usd/schemaRegistry.h"
#include "pxr/usd/usd/typed.h"

#include "pxr/usd/sdf/types.h"
#include "pxr/usd/sdf/assetPath.h"

PXR_NAMESPACE_OPEN_SCOPE

// Register the schema with the TfType system.
TF_REGISTRY_FUNCTION(TfType)
{
    TfType::Define<UsdGeomCurves,
        TfType::Bases< UsdGeomPointBased > >();
    
}

/* virtual */
UsdGeomCurves::~UsdGeomCurves()
{
}

/* static */
UsdGeomCurves
UsdGeomCurves::Get(const UsdStagePtr &stage, const SdfPath &path)
{
    if (!stage) {
        TF_CODING_ERROR("Invalid stage");
        return UsdGeomCurves();
    }
    return UsdGeomCurves(stage->GetPrimAtPath(path));
}


/* static */
const TfType &
UsdGeomCurves::_GetStaticTfType()
{
    static TfType tfType = TfType::Find<UsdGeomCurves>();
    return tfType;
}

/* static */
bool 
UsdGeomCurves::_IsTypedSchema()
{
    static bool isTyped = _GetStaticTfType().IsA<UsdTyped>();
    return isTyped;
}

/* virtual */
const TfType &
UsdGeomCurves::_GetTfType() const
{
    return _GetStaticTfType();
}

UsdAttribute
UsdGeomCurves::GetCurveVertexCountsAttr() const
{
    return GetPrim().GetAttribute(UsdGeomTokens->curveVertexCounts);
}

UsdAttribute
UsdGeomCurves::CreateCurveVertexCountsAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdGeomTokens->curveVertexCounts,
                       SdfValueTypeNames->IntArray,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdGeomCurves::GetWidthsAttr() const
{
    return GetPrim().GetAttribute(UsdGeomTokens->widths);
}

UsdAttribute
UsdGeomCurves::CreateWidthsAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdGeomTokens->widths,
                       SdfValueTypeNames->FloatArray,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

namespace {
static inline TfTokenVector
_ConcatenateAttributeNames(const TfTokenVector& left,const TfTokenVector& right)
{
    TfTokenVector result;
    result.reserve(left.size() + right.size());
    result.insert(result.end(), left.begin(), left.end());
    result.insert(result.end(), right.begin(), right.end());
    return result;
}
}

/*static*/
const TfTokenVector&
UsdGeomCurves::GetSchemaAttributeNames(bool includeInherited)
{
    static TfTokenVector localNames = {
        UsdGeomTokens->curveVertexCounts,
        UsdGeomTokens->widths,
    };
    static TfTokenVector allNames =
        _ConcatenateAttributeNames(
            UsdGeomPointBased::GetSchemaAttributeNames(true),
            localNames);

    if (includeInherited)
        return allNames;
    else
        return localNames;
}

PXR_NAMESPACE_CLOSE_SCOPE

// ===================================================================== //
// Feel free to add custom code below this line. It will be preserved by
// the code generator.
//
// Just remember to wrap code in the appropriate delimiters:
// 'PXR_NAMESPACE_OPEN_SCOPE', 'PXR_NAMESPACE_CLOSE_SCOPE'.
// ===================================================================== //
// --(BEGIN CUSTOM CODE)--

#include "pxr/usd/usdGeom/boundableComputeExtent.h"
#include "pxr/base/tf/registryManager.h"
#include "pxr/usd/usdGeom/pointBased.h"
#include "pxr/usd/usdGeom/sphere.h"

PXR_NAMESPACE_OPEN_SCOPE

TfToken 
UsdGeomCurves::GetWidthsInterpolation() const
{
    // Because widths is a builtin, we don't need to check validity
    // of the attribute before using it
    TfToken interp;
    if (GetWidthsAttr().GetMetadata(UsdGeomTokens->interpolation, &interp)){
        return interp;
    }
    
    return UsdGeomTokens->varying;
}

bool
UsdGeomCurves::SetWidthsInterpolation(TfToken const &interpolation)
{
    if (UsdGeomPrimvar::IsValidInterpolation(interpolation)){
        return GetWidthsAttr().SetMetadata(UsdGeomTokens->interpolation, 
                                            interpolation);
    }

    TF_CODING_ERROR("Attempt to set invalid interpolation "
                     "\"%s\" for widths attr on prim %s",
                     interpolation.GetText(),
                     GetPrim().GetPath().GetString().c_str());
    
    return false;
}

bool
UsdGeomCurves::ComputeExtent(const VtVec3fArray& points, 
    const VtFloatArray& widths, VtVec3fArray* extent)
{
    // XXX: All curves can be bounded by their control points, excluding
    //      catmull rom and hermite. For now, we treat hermite and catmull
    //      rom curves like their convex-hull counterparts. While there are
    //      some bounds approximations we could perform, hermite's
    //      implementation is not fully supported and catmull rom splines
    //      are very rare. For simplicity, we ignore these odd corner cases
    //      and provide a still reasonable approximation, but we also 
    //      recognize there could be some out-of-bounds error. 

    // We know nothing about the curve basis. Compute the extent as if it were 
    // a point cloud with some max width (convex hull).
    float maxWidth = (widths.size() > 0 ? 
        *(std::max_element(widths.begin(), widths.end())) : 0);
    
    if (!UsdGeomPointBased::ComputeExtent(points, extent)) { 
        return false;
    }

    GfVec3f widthVec = GfVec3f(maxWidth * 0.5);
    (*extent)[0] -= widthVec;
    (*extent)[1] += widthVec;

    return true;
}

bool
UsdGeomCurves::ComputeExtent(const VtVec3fArray& points, 
    const VtFloatArray& widths, const GfMatrix4d& transform,
    VtVec3fArray* extent)
{
    // XXX: All curves can be bounded by their control points, excluding
    //      catmull rom and hermite. For now, we treat hermite and catmull
    //      rom curves like their convex-hull counterparts. While there are
    //      some bounds approximations we could perform, hermite's
    //      implementation is not fully supported and catmull rom splines
    //      are very rare. For simplicity, we ignore these odd corner cases
    //      and provide a still reasonable approximation, but we also 
    //      recognize there could be some out-of-bounds error. 

    // We know nothing about the curve basis. Compute the extent as if it were 
    // a point cloud with some max width (convex hull).
    float maxWidth = (widths.size() > 0 ? 
        *(std::max_element(widths.begin(), widths.end())) : 0);
    
    if (!UsdGeomPointBased::ComputeExtent(points, transform, extent)) { 
        return false;
    }

    VtVec3fArray sphereExtent;

    // We want to transform the sphere without translation. The translation
    // was already applied to each point, so we just need to find the extent
    // of each point.
    GfMatrix4d transformDir(transform);
    transformDir.SetTranslateOnly(GfVec3d(0.0));

    if (!UsdGeomSphere::ComputeExtent(maxWidth * 0.5,
                                      transformDir,
                                      &sphereExtent)) {
        return false;
    }
    (*extent)[0] += sphereExtent[0];
    (*extent)[1] += sphereExtent[1];

    return true;
}

static bool
_ComputeExtentForCurves(
    const UsdGeomBoundable& boundable,
    const UsdTimeCode& time,
    const GfMatrix4d* transform,
    VtVec3fArray* extent)
{
    const UsdGeomCurves curves(boundable);
    if (!TF_VERIFY(curves)) {
        return false;
    }

    VtVec3fArray points;
    if (!curves.GetPointsAttr().Get(&points, time)) {
        return false;
    }

    VtFloatArray widths;
    curves.GetWidthsAttr().Get(&widths, time);
    
    if (transform) {
        return UsdGeomCurves::ComputeExtent(points, widths, *transform, extent);
    } else {
        return UsdGeomCurves::ComputeExtent(points, widths, extent);
    }
}

TF_REGISTRY_FUNCTION(UsdGeomBoundable)
{
    UsdGeomRegisterComputeExtentFunction<UsdGeomCurves>(
        _ComputeExtentForCurves);
}

PXR_NAMESPACE_CLOSE_SCOPE
