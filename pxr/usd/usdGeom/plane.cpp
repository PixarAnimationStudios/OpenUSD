//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/usd/usdGeom/plane.h"
#include "pxr/usd/usd/schemaRegistry.h"
#include "pxr/usd/usd/typed.h"

#include "pxr/usd/sdf/types.h"
#include "pxr/usd/sdf/assetPath.h"

PXR_NAMESPACE_OPEN_SCOPE

// Register the schema with the TfType system.
TF_REGISTRY_FUNCTION(TfType)
{
    TfType::Define<UsdGeomPlane,
        TfType::Bases< UsdGeomGprim > >();
    
    // Register the usd prim typename as an alias under UsdSchemaBase. This
    // enables one to call
    // TfType::Find<UsdSchemaBase>().FindDerivedByName("Plane")
    // to find TfType<UsdGeomPlane>, which is how IsA queries are
    // answered.
    TfType::AddAlias<UsdSchemaBase, UsdGeomPlane>("Plane");
}

/* virtual */
UsdGeomPlane::~UsdGeomPlane()
{
}

/* static */
UsdGeomPlane
UsdGeomPlane::Get(const UsdStagePtr &stage, const SdfPath &path)
{
    if (!stage) {
        TF_CODING_ERROR("Invalid stage");
        return UsdGeomPlane();
    }
    return UsdGeomPlane(stage->GetPrimAtPath(path));
}

/* static */
UsdGeomPlane
UsdGeomPlane::Define(
    const UsdStagePtr &stage, const SdfPath &path)
{
    static TfToken usdPrimTypeName("Plane");
    if (!stage) {
        TF_CODING_ERROR("Invalid stage");
        return UsdGeomPlane();
    }
    return UsdGeomPlane(
        stage->DefinePrim(path, usdPrimTypeName));
}

/* virtual */
UsdSchemaKind UsdGeomPlane::_GetSchemaKind() const
{
    return UsdGeomPlane::schemaKind;
}

/* static */
const TfType &
UsdGeomPlane::_GetStaticTfType()
{
    static TfType tfType = TfType::Find<UsdGeomPlane>();
    return tfType;
}

/* static */
bool 
UsdGeomPlane::_IsTypedSchema()
{
    static bool isTyped = _GetStaticTfType().IsA<UsdTyped>();
    return isTyped;
}

/* virtual */
const TfType &
UsdGeomPlane::_GetTfType() const
{
    return _GetStaticTfType();
}

UsdAttribute
UsdGeomPlane::GetDoubleSidedAttr() const
{
    return GetPrim().GetAttribute(UsdGeomTokens->doubleSided);
}

UsdAttribute
UsdGeomPlane::CreateDoubleSidedAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdGeomTokens->doubleSided,
                       SdfValueTypeNames->Bool,
                       /* custom = */ false,
                       SdfVariabilityUniform,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdGeomPlane::GetWidthAttr() const
{
    return GetPrim().GetAttribute(UsdGeomTokens->width);
}

UsdAttribute
UsdGeomPlane::CreateWidthAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdGeomTokens->width,
                       SdfValueTypeNames->Double,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdGeomPlane::GetLengthAttr() const
{
    return GetPrim().GetAttribute(UsdGeomTokens->length);
}

UsdAttribute
UsdGeomPlane::CreateLengthAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdGeomTokens->length,
                       SdfValueTypeNames->Double,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdGeomPlane::GetAxisAttr() const
{
    return GetPrim().GetAttribute(UsdGeomTokens->axis);
}

UsdAttribute
UsdGeomPlane::CreateAxisAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdGeomTokens->axis,
                       SdfValueTypeNames->Token,
                       /* custom = */ false,
                       SdfVariabilityUniform,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdGeomPlane::GetExtentAttr() const
{
    return GetPrim().GetAttribute(UsdGeomTokens->extent);
}

UsdAttribute
UsdGeomPlane::CreateExtentAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdGeomTokens->extent,
                       SdfValueTypeNames->Float3Array,
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
UsdGeomPlane::GetSchemaAttributeNames(bool includeInherited)
{
    static TfTokenVector localNames = {
        UsdGeomTokens->doubleSided,
        UsdGeomTokens->width,
        UsdGeomTokens->length,
        UsdGeomTokens->axis,
        UsdGeomTokens->extent,
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

#include "pxr/usd/usdGeom/boundableComputeExtent.h"
#include "pxr/base/tf/registryManager.h"

PXR_NAMESPACE_OPEN_SCOPE

static bool
_ComputeExtentMax(double width, double length, const TfToken& axis, GfVec3f* max)
{
    float halfWidth = width * 0.5;
    float halfLength = length * 0.5;

    if (axis == UsdGeomTokens->x) {
        *max = GfVec3f(0, halfLength, halfWidth);
    } else if (axis == UsdGeomTokens->y) {
        *max = GfVec3f(halfWidth, 0, halfLength);
    } else if (axis == UsdGeomTokens->z) {
        *max = GfVec3f(halfWidth, halfLength, 0);
    } else {
        return false; // invalid axis
    }

    return true;
}

bool
UsdGeomPlane::ComputeExtent(double width, double length, const TfToken& axis, 
    VtVec3fArray* extent)
{
    // Create Sized Extent
    extent->resize(2);

    GfVec3f max;
    if (!_ComputeExtentMax(width, length, axis, &max)) {
        return false;
    }

    (*extent)[0] = -max;
    (*extent)[1] = max;

    return true;
}

bool
UsdGeomPlane::ComputeExtent(double width, double length, const TfToken& axis, 
    const GfMatrix4d& transform, VtVec3fArray* extent)
{
    // Create Sized Extent
    extent->resize(2);

    GfVec3f max;
    if (!_ComputeExtentMax(width, length, axis, &max)) {
        return false;
    }

    GfBBox3d bbox = GfBBox3d(GfRange3d(-max, max), transform);
    GfRange3d range = bbox.ComputeAlignedRange();
    (*extent)[0] = GfVec3f(range.GetMin());
    (*extent)[1] = GfVec3f(range.GetMax());

    return true;
}

static bool
_ComputeExtentForPlane(
    const UsdGeomBoundable& boundable,
    const UsdTimeCode& time,
    const GfMatrix4d* transform,
    VtVec3fArray* extent)
{
    const UsdGeomPlane planeSchema(boundable);
    if (!TF_VERIFY(planeSchema)) {
        return false;
    }

    double width;
    if (!planeSchema.GetWidthAttr().Get(&width)) {
        return false;
    }

    double length;
    if (!planeSchema.GetLengthAttr().Get(&length)) {
        return false;
    }

    TfToken axis;
    if (!planeSchema.GetAxisAttr().Get(&axis)) {
        return false;
    }

    if (transform) {
        return UsdGeomPlane::ComputeExtent(width, length, axis, *transform, extent);
    } else {
        return UsdGeomPlane::ComputeExtent(width, length, axis, extent);
    }
}

TF_REGISTRY_FUNCTION(UsdGeomBoundable)
{
    UsdGeomRegisterComputeExtentFunction<UsdGeomPlane>(
        _ComputeExtentForPlane);
}

PXR_NAMESPACE_CLOSE_SCOPE
