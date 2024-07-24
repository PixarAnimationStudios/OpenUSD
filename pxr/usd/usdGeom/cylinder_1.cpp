//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/usd/usdGeom/cylinder_1.h"
#include "pxr/usd/usd/schemaRegistry.h"
#include "pxr/usd/usd/typed.h"

#include "pxr/usd/sdf/types.h"
#include "pxr/usd/sdf/assetPath.h"

PXR_NAMESPACE_OPEN_SCOPE

// Register the schema with the TfType system.
TF_REGISTRY_FUNCTION(TfType)
{
    TfType::Define<UsdGeomCylinder_1,
        TfType::Bases< UsdGeomGprim > >();
    
    // Register the usd prim typename as an alias under UsdSchemaBase. This
    // enables one to call
    // TfType::Find<UsdSchemaBase>().FindDerivedByName("Cylinder_1")
    // to find TfType<UsdGeomCylinder_1>, which is how IsA queries are
    // answered.
    TfType::AddAlias<UsdSchemaBase, UsdGeomCylinder_1>("Cylinder_1");
}

/* virtual */
UsdGeomCylinder_1::~UsdGeomCylinder_1()
{
}

/* static */
UsdGeomCylinder_1
UsdGeomCylinder_1::Get(const UsdStagePtr &stage, const SdfPath &path)
{
    if (!stage) {
        TF_CODING_ERROR("Invalid stage");
        return UsdGeomCylinder_1();
    }
    return UsdGeomCylinder_1(stage->GetPrimAtPath(path));
}

/* static */
UsdGeomCylinder_1
UsdGeomCylinder_1::Define(
    const UsdStagePtr &stage, const SdfPath &path)
{
    static TfToken usdPrimTypeName("Cylinder_1");
    if (!stage) {
        TF_CODING_ERROR("Invalid stage");
        return UsdGeomCylinder_1();
    }
    return UsdGeomCylinder_1(
        stage->DefinePrim(path, usdPrimTypeName));
}

/* virtual */
UsdSchemaKind UsdGeomCylinder_1::_GetSchemaKind() const
{
    return UsdGeomCylinder_1::schemaKind;
}

/* static */
const TfType &
UsdGeomCylinder_1::_GetStaticTfType()
{
    static TfType tfType = TfType::Find<UsdGeomCylinder_1>();
    return tfType;
}

/* static */
bool 
UsdGeomCylinder_1::_IsTypedSchema()
{
    static bool isTyped = _GetStaticTfType().IsA<UsdTyped>();
    return isTyped;
}

/* virtual */
const TfType &
UsdGeomCylinder_1::_GetTfType() const
{
    return _GetStaticTfType();
}

UsdAttribute
UsdGeomCylinder_1::GetHeightAttr() const
{
    return GetPrim().GetAttribute(UsdGeomTokens->height);
}

UsdAttribute
UsdGeomCylinder_1::CreateHeightAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdGeomTokens->height,
                       SdfValueTypeNames->Double,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdGeomCylinder_1::GetRadiusTopAttr() const
{
    return GetPrim().GetAttribute(UsdGeomTokens->radiusTop);
}

UsdAttribute
UsdGeomCylinder_1::CreateRadiusTopAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdGeomTokens->radiusTop,
                       SdfValueTypeNames->Double,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdGeomCylinder_1::GetRadiusBottomAttr() const
{
    return GetPrim().GetAttribute(UsdGeomTokens->radiusBottom);
}

UsdAttribute
UsdGeomCylinder_1::CreateRadiusBottomAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdGeomTokens->radiusBottom,
                       SdfValueTypeNames->Double,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdGeomCylinder_1::GetAxisAttr() const
{
    return GetPrim().GetAttribute(UsdGeomTokens->axis);
}

UsdAttribute
UsdGeomCylinder_1::CreateAxisAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdGeomTokens->axis,
                       SdfValueTypeNames->Token,
                       /* custom = */ false,
                       SdfVariabilityUniform,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdGeomCylinder_1::GetExtentAttr() const
{
    return GetPrim().GetAttribute(UsdGeomTokens->extent);
}

UsdAttribute
UsdGeomCylinder_1::CreateExtentAttr(VtValue const &defaultValue, bool writeSparsely) const
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
UsdGeomCylinder_1::GetSchemaAttributeNames(bool includeInherited)
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
    const TfToken &axis, GfVec3f *max)
{
    const double radiusForBox = std::max(radiusTop, radiusBottom);
    if (axis == UsdGeomTokens->x) {
        *max = GfVec3f(height * 0.5, radiusForBox, radiusForBox);
    } else if (axis == UsdGeomTokens->y) {
        *max = GfVec3f(radiusForBox, height * 0.5, radiusForBox);
    } else if (axis == UsdGeomTokens->z) {
        *max = GfVec3f(radiusForBox, radiusForBox, height * 0.5);
    } else {
        return false; // invalid axis
    }
    
    return true;
}

bool
UsdGeomCylinder_1::ComputeExtent(double height, double radiusTop, 
    double radiusBottom, const TfToken &axis, VtVec3fArray *extent)
{
    // Create Sized extent
    extent->resize(2);

    GfVec3f max;
    if (!_ComputeExtentMax(height, radiusTop, radiusBottom, axis, &max)) {
        return false;
    }

    (*extent)[0] = -max;
    (*extent)[1] = max;

    return true;
}

bool
UsdGeomCylinder_1::ComputeExtent(double height, double radiusTop, 
    double radiusBottom, const TfToken &axis, const GfMatrix4d &transform,
    VtVec3fArray *extent)
{
    // Create sized extent
    extent->resize(2);

    GfVec3f max;
    if (!_ComputeExtentMax(height, radiusTop, radiusBottom, axis, &max)) {
        return false;
    }

    GfBBox3d bbox = GfBBox3d(GfRange3d(-max, max), transform);
    GfRange3d range = bbox.ComputeAlignedRange();
    (*extent)[0] = GfVec3f(range.GetMin());
    (*extent)[1] = GfVec3f(range.GetMax());

    return true;
}

static bool
_ComputeExtentForCylinder(
    const UsdGeomBoundable &boundable,
    const UsdTimeCode &time,
    const GfMatrix4d *transform,
    VtVec3fArray *extent)
{
    const UsdGeomCylinder_1 cylinderSchema(boundable);
    if (!TF_VERIFY(cylinderSchema)) {
        return false;
    }

    double height;
    if (!cylinderSchema.GetHeightAttr().Get(&height, time)) {
        return false;
    }

    double radiusTop;
    if (!cylinderSchema.GetRadiusTopAttr().Get(&radiusTop, time)) {
        return false;
    }

    double radiusBottom;
    if (!cylinderSchema.GetRadiusBottomAttr().Get(&radiusBottom, time)) {
        return false;
    }

    TfToken axis;
    if (!cylinderSchema.GetAxisAttr().Get(&axis, time)) {
        return false;
    }

    if (transform) {
        return UsdGeomCylinder_1::ComputeExtent(
            height, radiusTop, radiusBottom, axis, *transform, extent);
    } else {
        return UsdGeomCylinder_1::ComputeExtent(height, radiusTop, radiusBottom, 
                axis, extent);
    }
}

TF_REGISTRY_FUNCTION(UsdGeomBoundable)
{
    UsdGeomRegisterComputeExtentFunction<UsdGeomCylinder_1>(
            _ComputeExtentForCylinder);
}

PXR_NAMESPACE_CLOSE_SCOPE
