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
#include "pxr/usd/usdGeom/pointBased.h"
#include "pxr/usd/usd/schemaRegistry.h"
#include "pxr/usd/usd/typed.h"

#include "pxr/usd/sdf/types.h"
#include "pxr/usd/sdf/assetPath.h"

PXR_NAMESPACE_OPEN_SCOPE

// Register the schema with the TfType system.
TF_REGISTRY_FUNCTION(TfType)
{
    TfType::Define<UsdGeomPointBased,
        TfType::Bases< UsdGeomGprim > >();
    
}

/* virtual */
UsdGeomPointBased::~UsdGeomPointBased()
{
}

/* static */
UsdGeomPointBased
UsdGeomPointBased::Get(const UsdStagePtr &stage, const SdfPath &path)
{
    if (!stage) {
        TF_CODING_ERROR("Invalid stage");
        return UsdGeomPointBased();
    }
    return UsdGeomPointBased(stage->GetPrimAtPath(path));
}


/* static */
const TfType &
UsdGeomPointBased::_GetStaticTfType()
{
    static TfType tfType = TfType::Find<UsdGeomPointBased>();
    return tfType;
}

/* static */
bool 
UsdGeomPointBased::_IsTypedSchema()
{
    static bool isTyped = _GetStaticTfType().IsA<UsdTyped>();
    return isTyped;
}

/* virtual */
const TfType &
UsdGeomPointBased::_GetTfType() const
{
    return _GetStaticTfType();
}

UsdAttribute
UsdGeomPointBased::GetPointsAttr() const
{
    return GetPrim().GetAttribute(UsdGeomTokens->points);
}

UsdAttribute
UsdGeomPointBased::CreatePointsAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdGeomTokens->points,
                       SdfValueTypeNames->Point3fArray,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdGeomPointBased::GetVelocitiesAttr() const
{
    return GetPrim().GetAttribute(UsdGeomTokens->velocities);
}

UsdAttribute
UsdGeomPointBased::CreateVelocitiesAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdGeomTokens->velocities,
                       SdfValueTypeNames->Vector3fArray,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdGeomPointBased::GetNormalsAttr() const
{
    return GetPrim().GetAttribute(UsdGeomTokens->normals);
}

UsdAttribute
UsdGeomPointBased::CreateNormalsAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdGeomTokens->normals,
                       SdfValueTypeNames->Normal3fArray,
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
UsdGeomPointBased::GetSchemaAttributeNames(bool includeInherited)
{
    static TfTokenVector localNames = {
        UsdGeomTokens->points,
        UsdGeomTokens->velocities,
        UsdGeomTokens->normals,
    };
    static TfTokenVector allNames =
        _ConcatenateAttributeNames(
            UsdGeomGprim::GetSchemaAttributeNames(true),
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

PXR_NAMESPACE_OPEN_SCOPE

TfToken 
UsdGeomPointBased::GetNormalsInterpolation() const
{
    // Because normals is a builtin, we don't need to check validity
    // of the attribute before using it
    TfToken interp;
    if (GetNormalsAttr().GetMetadata(UsdGeomTokens->interpolation, &interp)){
        return interp;
    }
    
    return UsdGeomTokens->varying;
}

bool
UsdGeomPointBased::SetNormalsInterpolation(TfToken const &interpolation)
{
    if (UsdGeomPrimvar::IsValidInterpolation(interpolation)){
        return GetNormalsAttr().SetMetadata(UsdGeomTokens->interpolation, 
                                            interpolation);
    }

    TF_CODING_ERROR("Attempt to set invalid interpolation "
                     "\"%s\" for normals attr on prim %s",
                     interpolation.GetText(),
                     GetPrim().GetPath().GetString().c_str());
    
    return false;
}

bool
UsdGeomPointBased::ComputeExtent(const VtVec3fArray& points, 
    VtVec3fArray* extent)  
{
    // Create Sized Extent
    extent->resize(2);

    // Calculate bounds
    GfRange3d bbox;
    TF_FOR_ALL(pointsItr, points) {
        bbox.UnionWith(GfVec3f(*pointsItr));
    }

    (*extent)[0] = GfVec3f(bbox.GetMin());
    (*extent)[1] = GfVec3f(bbox.GetMax());
    
    return true;
}

PXR_NAMESPACE_CLOSE_SCOPE
