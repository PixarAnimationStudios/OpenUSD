//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/usd/usdGeom/capsule_1.h"
#include "pxr/usd/usd/schemaRegistry.h"
#include "pxr/usd/usd/typed.h"

#include "pxr/usd/sdf/types.h"
#include "pxr/usd/sdf/assetPath.h"

PXR_NAMESPACE_OPEN_SCOPE

// Register the schema with the TfType system.
TF_REGISTRY_FUNCTION(TfType)
{
    TfType::Define<UsdGeomCapsule_1,
        TfType::Bases< UsdGeomGprim > >();
    
    // Register the usd prim typename as an alias under UsdSchemaBase. This
    // enables one to call
    // TfType::Find<UsdSchemaBase>().FindDerivedByName("Capsule_1")
    // to find TfType<UsdGeomCapsule_1>, which is how IsA queries are
    // answered.
    TfType::AddAlias<UsdSchemaBase, UsdGeomCapsule_1>("Capsule_1");
}

/* virtual */
UsdGeomCapsule_1::~UsdGeomCapsule_1()
{
}

/* static */
UsdGeomCapsule_1
UsdGeomCapsule_1::Get(const UsdStagePtr &stage, const SdfPath &path)
{
    if (!stage) {
        TF_CODING_ERROR("Invalid stage");
        return UsdGeomCapsule_1();
    }
    return UsdGeomCapsule_1(stage->GetPrimAtPath(path));
}

/* static */
UsdGeomCapsule_1
UsdGeomCapsule_1::Define(
    const UsdStagePtr &stage, const SdfPath &path)
{
    static TfToken usdPrimTypeName("Capsule_1");
    if (!stage) {
        TF_CODING_ERROR("Invalid stage");
        return UsdGeomCapsule_1();
    }
    return UsdGeomCapsule_1(
        stage->DefinePrim(path, usdPrimTypeName));
}

/* virtual */
UsdSchemaKind UsdGeomCapsule_1::_GetSchemaKind() const
{
    return UsdGeomCapsule_1::schemaKind;
}

/* static */
const TfType &
UsdGeomCapsule_1::_GetStaticTfType()
{
    static TfType tfType = TfType::Find<UsdGeomCapsule_1>();
    return tfType;
}

/* static */
bool 
UsdGeomCapsule_1::_IsTypedSchema()
{
    static bool isTyped = _GetStaticTfType().IsA<UsdTyped>();
    return isTyped;
}

/* virtual */
const TfType &
UsdGeomCapsule_1::_GetTfType() const
{
    return _GetStaticTfType();
}

UsdAttribute
UsdGeomCapsule_1::GetHeightAttr() const
{
    return GetPrim().GetAttribute(UsdGeomTokens->height);
}

UsdAttribute
UsdGeomCapsule_1::CreateHeightAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdGeomTokens->height,
                       SdfValueTypeNames->Double,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdGeomCapsule_1::GetRadiusTopAttr() const
{
    return GetPrim().GetAttribute(UsdGeomTokens->radiusTop);
}

UsdAttribute
UsdGeomCapsule_1::CreateRadiusTopAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdGeomTokens->radiusTop,
                       SdfValueTypeNames->Double,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdGeomCapsule_1::GetRadiusBottomAttr() const
{
    return GetPrim().GetAttribute(UsdGeomTokens->radiusBottom);
}

UsdAttribute
UsdGeomCapsule_1::CreateRadiusBottomAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdGeomTokens->radiusBottom,
                       SdfValueTypeNames->Double,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdGeomCapsule_1::GetAxisAttr() const
{
    return GetPrim().GetAttribute(UsdGeomTokens->axis);
}

UsdAttribute
UsdGeomCapsule_1::CreateAxisAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdGeomTokens->axis,
                       SdfValueTypeNames->Token,
                       /* custom = */ false,
                       SdfVariabilityUniform,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdGeomCapsule_1::GetExtentAttr() const
{
    return GetPrim().GetAttribute(UsdGeomTokens->extent);
}

UsdAttribute
UsdGeomCapsule_1::CreateExtentAttr(VtValue const &defaultValue, bool writeSparsely) const
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
UsdGeomCapsule_1::GetSchemaAttributeNames(bool includeInherited)
{
    static TfTokenVector localNames = {
        UsdGeomTokens->height,
        UsdGeomTokens->radiusTop,
        UsdGeomTokens->radiusBottom,
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
_ComputeExtentMax(double height, double radiusTop, double radiusBottom, 
    const TfToken &axis, GfVec3f *min, GfVec3f *max)
{
    if (height < 0.0 || radiusTop < 0.0 || radiusBottom < 0.0) {
        return false;
    }

    const double halfH = 0.5 * height;

    // In order to create a continuous surface, the slant line of the conical
    // frustum of the capsule must be tangent to the radii of the end spheres
    // at height/2 and -height/2.
    // Consider the triangle formed by the following points:
        // (assume axis = Y and radiusBottom > radiusTop)
            // A: (-radiusTop, height / 2, 0),
            // B: (-radiusTop, -height / 2, 0),
            // C: (-radiusBottom, -height / 2, 0).
        // This is a similar triangle to
            // A: (-radiusTop, height / 2, 0),
            // D: (0, height / 2, 0),
            // E: (0, topSphereCenter.y, 0).
        // Let topSphereCenter.y = height / 2 - offsetTop.
    // We therefore get the equation:
        // BC/AB = DE/AD
        // => (radiusBottom - radiusTop) / height = offsetTop / radiusTop
    // Similarly, we can derive the following equation for radiusBottom:
        // (radiusBottom - radiusTop) / height = offsetBottom / radiusBottom
    // We can similarly derive the same equations for radiusTop >= radiusBottom
        // and for axis = X or Z.
    const double slope = height > 0.0 ? (radiusBottom - radiusTop) / height : 0.0;
    const double offsetTop = slope * radiusTop;
    const double offsetBottom = slope * radiusBottom;

    // sec^2(theta) = tan^2(theta) + 1,
        // where theta = 90deg - the base angle of the conical frustum.
        // In the example above,
            // theta = angle between AB and AC = angle between AD and AE.
    const double secant = GfSqrt(GfSqr(slope) + 1);

    // Simplified form of sphereTopRadius^2 = offsetTop^2 + radiusTop^2
    const double sphereTopRadius = radiusTop * secant;
    const double sphereTopCenterCoord = halfH - offsetTop;

    const double sphereBottomRadius = radiusBottom * secant;
    const double sphereBottomCenterCoord = -halfH - offsetBottom;
    
    // sphereRadius >= offset => the absolute value of these is always >= halfH
    const double maxHCoord = sphereTopCenterCoord + sphereTopRadius;
    const double minHCoord = sphereBottomCenterCoord - sphereBottomRadius;

    const double radiusForBox = std::max(sphereTopRadius, sphereBottomRadius);

    if (axis == UsdGeomTokens->x) {
        *max = GfVec3f(maxHCoord, radiusForBox, radiusForBox);
        *min = GfVec3f(minHCoord, -radiusForBox, -radiusForBox);
    } else if (axis == UsdGeomTokens->y) {
        *max = GfVec3f(radiusForBox, maxHCoord, radiusForBox);
        *min = GfVec3f(-radiusForBox, minHCoord, -radiusForBox);
    } else if (axis == UsdGeomTokens->z) {
        *max = GfVec3f(radiusForBox, radiusForBox, maxHCoord);
        *min = GfVec3f(-radiusForBox, -radiusForBox, minHCoord);
    } else {
      return false; // invalid axis
    }

    return true;
}

bool
UsdGeomCapsule_1::ComputeExtent(double height, double radiusTop, 
    double radiusBottom, const TfToken &axis, VtVec3fArray *extent)
{
    // Create Sized Extent
    extent->resize(2);

    GfVec3f max;
    GfVec3f min;
    if (!_ComputeExtentMax(height, radiusTop, radiusBottom, axis, &min, &max)) {
        return false;
    }

    (*extent)[0] = min;
    (*extent)[1] = max;

    return true;
}

bool
UsdGeomCapsule_1::ComputeExtent(double height, double radiusTop, 
    double radiusBottom, const TfToken &axis, const GfMatrix4d &transform, 
    VtVec3fArray *extent)
{
    // Create Sized Extent
    extent->resize(2);

    GfVec3f max;
    GfVec3f min;
    if (!_ComputeExtentMax(height, radiusTop, radiusBottom, axis, &min, &max)) {
        return false;
    }

    GfBBox3d bbox = GfBBox3d(GfRange3d(min, max), transform);
    GfRange3d range = bbox.ComputeAlignedRange();
    (*extent)[0] = GfVec3f(range.GetMin());
    (*extent)[1] = GfVec3f(range.GetMax());

    return true;
}

static bool
_ComputeExtentForCapsule(
    const UsdGeomBoundable &boundable,
    const UsdTimeCode &time,
    const GfMatrix4d *transform,
    VtVec3fArray *extent)
{
    const UsdGeomCapsule_1 capsuleSchema(boundable);
    if (!TF_VERIFY(capsuleSchema)) {
        return false;
    }

    double height;
    if (!capsuleSchema.GetHeightAttr().Get(&height, time)) {
        return false;
    }

    double radiusTop;
    if (!capsuleSchema.GetRadiusTopAttr().Get(&radiusTop, time)) {
        return false;
    }

    double radiusBottom;
    if (!capsuleSchema.GetRadiusBottomAttr().Get(&radiusBottom, time)) {
        return false;
    }

    TfToken axis;
    if (!capsuleSchema.GetAxisAttr().Get(&axis, time)) {
        return false;
    }

    if (transform) {
        return UsdGeomCapsule_1::ComputeExtent(
            height, radiusTop, radiusBottom, axis, *transform, extent);
    } else {
        return UsdGeomCapsule_1::ComputeExtent(height, radiusTop, radiusBottom, 
            axis, extent);
    }
}

TF_REGISTRY_FUNCTION(UsdGeomBoundable)
{
    UsdGeomRegisterComputeExtentFunction<UsdGeomCapsule_1>(
        _ComputeExtentForCapsule);
}

PXR_NAMESPACE_CLOSE_SCOPE
