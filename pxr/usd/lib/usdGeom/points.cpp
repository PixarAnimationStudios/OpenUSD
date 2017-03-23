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
#include "pxr/usd/usdGeom/points.h"
#include "pxr/usd/usd/schemaRegistry.h"
#include "pxr/usd/usd/typed.h"

#include "pxr/usd/sdf/types.h"
#include "pxr/usd/sdf/assetPath.h"

PXR_NAMESPACE_OPEN_SCOPE

// Register the schema with the TfType system.
TF_REGISTRY_FUNCTION(TfType)
{
    TfType::Define<UsdGeomPoints,
        TfType::Bases< UsdGeomPointBased > >();
    
    // Register the usd prim typename as an alias under UsdSchemaBase. This
    // enables one to call
    // TfType::Find<UsdSchemaBase>().FindDerivedByName("Points")
    // to find TfType<UsdGeomPoints>, which is how IsA queries are
    // answered.
    TfType::AddAlias<UsdSchemaBase, UsdGeomPoints>("Points");
}

/* virtual */
UsdGeomPoints::~UsdGeomPoints()
{
}

/* static */
UsdGeomPoints
UsdGeomPoints::Get(const UsdStagePtr &stage, const SdfPath &path)
{
    if (!stage) {
        TF_CODING_ERROR("Invalid stage");
        return UsdGeomPoints();
    }
    return UsdGeomPoints(stage->GetPrimAtPath(path));
}

/* static */
UsdGeomPoints
UsdGeomPoints::Define(
    const UsdStagePtr &stage, const SdfPath &path)
{
    static TfToken usdPrimTypeName("Points");
    if (!stage) {
        TF_CODING_ERROR("Invalid stage");
        return UsdGeomPoints();
    }
    return UsdGeomPoints(
        stage->DefinePrim(path, usdPrimTypeName));
}

/* static */
const TfType &
UsdGeomPoints::_GetStaticTfType()
{
    static TfType tfType = TfType::Find<UsdGeomPoints>();
    return tfType;
}

/* static */
bool 
UsdGeomPoints::_IsTypedSchema()
{
    static bool isTyped = _GetStaticTfType().IsA<UsdTyped>();
    return isTyped;
}

/* virtual */
const TfType &
UsdGeomPoints::_GetTfType() const
{
    return _GetStaticTfType();
}

UsdAttribute
UsdGeomPoints::GetWidthsAttr() const
{
    return GetPrim().GetAttribute(UsdGeomTokens->widths);
}

UsdAttribute
UsdGeomPoints::CreateWidthsAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdGeomTokens->widths,
                       SdfValueTypeNames->FloatArray,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdGeomPoints::GetIdsAttr() const
{
    return GetPrim().GetAttribute(UsdGeomTokens->ids);
}

UsdAttribute
UsdGeomPoints::CreateIdsAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdGeomTokens->ids,
                       SdfValueTypeNames->Int64Array,
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
UsdGeomPoints::GetSchemaAttributeNames(bool includeInherited)
{
    static TfTokenVector localNames = {
        UsdGeomTokens->widths,
        UsdGeomTokens->ids,
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

PXR_NAMESPACE_OPEN_SCOPE

bool
UsdGeomPoints::ComputeExtent(const VtVec3fArray& points, 
    const VtFloatArray& widths, VtVec3fArray* extent)
{
    // Check for Valid Widths/Points Attributes Size 
    if (points.size() != widths.size()) {
        return false;
    }

    // Create Sized Extent
    extent->resize(2);

    // Calculate bounds
    GfRange3d bbox;
    TfIterator<const VtFloatArray> widthsItr(widths);
    TF_FOR_ALL(pointsItr, points) {
        float halfWidth = *widthsItr/2;
        GfVec3f widthVec(halfWidth);
        bbox.UnionWith(GfVec3f(*pointsItr) + widthVec);
        bbox.UnionWith(GfVec3f(*pointsItr) - widthVec);

        widthsItr++;
    }

    (*extent)[0] = GfVec3f(bbox.GetMin());
    (*extent)[1] = GfVec3f(bbox.GetMax());

    return true;
}

static bool
_ComputeExtentForPoints(
    const UsdGeomBoundable& boundable, 
    const UsdTimeCode& time, 
    VtVec3fArray* extent)
{
    const UsdGeomPoints pointsSchema(boundable);
    if (!TF_VERIFY(pointsSchema)) {
        return false;
    }

    VtVec3fArray points;
    if (!pointsSchema.GetPointsAttr().Get(&points, time)) {
        return false;
    }

    VtFloatArray widths;
    if (!pointsSchema.GetWidthsAttr().Get(&widths, time)) {
        return UsdGeomPointBased::ComputeExtent(points, extent);
    }
    
    return UsdGeomPoints::ComputeExtent(points, widths, extent);
}

TF_REGISTRY_FUNCTION(UsdGeomBoundable)
{
    UsdGeomRegisterComputeExtentFunction<UsdGeomPoints>(
        _ComputeExtentForPoints);
}

PXR_NAMESPACE_CLOSE_SCOPE
