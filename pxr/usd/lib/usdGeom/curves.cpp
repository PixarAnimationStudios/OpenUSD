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

// ===================================================================== //
// Feel free to add custom code below this line. It will be preserved by
// the code generator.
// ===================================================================== //
// --(BEGIN CUSTOM CODE)--

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

#include "pxr/usd/usdGeom/pointBased.h"

bool
UsdGeomCurves::ComputeExtent(const VtVec3fArray& points, 
    const VtFloatArray& widths, VtVec3fArray* extent)
{
    // We know nothing about the curve basis. Compute the extent as if it were 
    // a point cloud with some max width (convex hull).
    float maxWidth = (widths.size() > 0 ? 
        *(std::max_element(widths.begin(), widths.end())) : 0);
    
    if (not UsdGeomPointBased::ComputeExtent(points, extent)) { 
        return false;
    }
 
    GfVec3f widthVec = GfVec3f(maxWidth/2.);
    (*extent)[0] -= widthVec;
    (*extent)[1] += widthVec;

    return true;
}
