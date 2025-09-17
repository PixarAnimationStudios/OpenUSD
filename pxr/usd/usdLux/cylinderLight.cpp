//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/usd/usdLux/cylinderLight.h"
#include "pxr/usd/usd/schemaRegistry.h"
#include "pxr/usd/usd/typed.h"

#include "pxr/usd/sdf/types.h"
#include "pxr/usd/sdf/assetPath.h"

PXR_NAMESPACE_OPEN_SCOPE

// Register the schema with the TfType system.
TF_REGISTRY_FUNCTION(TfType)
{
    TfType::Define<UsdLuxCylinderLight,
        TfType::Bases< UsdLuxBoundableLightBase > >();
    
    // Register the usd prim typename as an alias under UsdSchemaBase. This
    // enables one to call
    // TfType::Find<UsdSchemaBase>().FindDerivedByName("CylinderLight")
    // to find TfType<UsdLuxCylinderLight>, which is how IsA queries are
    // answered.
    TfType::AddAlias<UsdSchemaBase, UsdLuxCylinderLight>("CylinderLight");
}

/* virtual */
UsdLuxCylinderLight::~UsdLuxCylinderLight()
{
}

/* static */
UsdLuxCylinderLight
UsdLuxCylinderLight::Get(const UsdStagePtr &stage, const SdfPath &path)
{
    if (!stage) {
        TF_CODING_ERROR("Invalid stage");
        return UsdLuxCylinderLight();
    }
    return UsdLuxCylinderLight(stage->GetPrimAtPath(path));
}

/* static */
UsdLuxCylinderLight
UsdLuxCylinderLight::Define(
    const UsdStagePtr &stage, const SdfPath &path)
{
    static TfToken usdPrimTypeName("CylinderLight");
    if (!stage) {
        TF_CODING_ERROR("Invalid stage");
        return UsdLuxCylinderLight();
    }
    return UsdLuxCylinderLight(
        stage->DefinePrim(path, usdPrimTypeName));
}

/* virtual */
UsdSchemaKind UsdLuxCylinderLight::_GetSchemaKind() const
{
    return UsdLuxCylinderLight::schemaKind;
}

/* static */
const TfType &
UsdLuxCylinderLight::_GetStaticTfType()
{
    static TfType tfType = TfType::Find<UsdLuxCylinderLight>();
    return tfType;
}

/* static */
bool 
UsdLuxCylinderLight::_IsTypedSchema()
{
    static bool isTyped = _GetStaticTfType().IsA<UsdTyped>();
    return isTyped;
}

/* virtual */
const TfType &
UsdLuxCylinderLight::_GetTfType() const
{
    return _GetStaticTfType();
}

UsdAttribute
UsdLuxCylinderLight::GetLengthAttr() const
{
    return GetPrim().GetAttribute(UsdLuxTokens->inputsLength);
}

UsdAttribute
UsdLuxCylinderLight::CreateLengthAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdLuxTokens->inputsLength,
                       SdfValueTypeNames->Float,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdLuxCylinderLight::GetRadiusAttr() const
{
    return GetPrim().GetAttribute(UsdLuxTokens->inputsRadius);
}

UsdAttribute
UsdLuxCylinderLight::CreateRadiusAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdLuxTokens->inputsRadius,
                       SdfValueTypeNames->Float,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdLuxCylinderLight::GetTreatAsLineAttr() const
{
    return GetPrim().GetAttribute(UsdLuxTokens->treatAsLine);
}

UsdAttribute
UsdLuxCylinderLight::CreateTreatAsLineAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdLuxTokens->treatAsLine,
                       SdfValueTypeNames->Bool,
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
UsdLuxCylinderLight::GetSchemaAttributeNames(bool includeInherited)
{
    static TfTokenVector localNames = {
        UsdLuxTokens->inputsLength,
        UsdLuxTokens->inputsRadius,
        UsdLuxTokens->treatAsLine,
    };
    static TfTokenVector allNames =
        _ConcatenateAttributeNames(
            UsdLuxBoundableLightBase::GetSchemaAttributeNames(true),
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

PXR_NAMESPACE_OPEN_SCOPE

static bool
_ComputeLocalExtent(const float radius, 
                    const float length, 
                    VtVec3fArray *extent)
{
    extent->resize(2);
    (*extent)[1] = GfVec3f(length * 0.5f, radius, radius);
    (*extent)[0] = -(*extent)[1];
    return true;
}

static bool 
_ComputeExtent(
    const UsdGeomBoundable &boundable,
    const UsdTimeCode &time,
    const GfMatrix4d *transform,
    VtVec3fArray *extent)
{
    const UsdLuxCylinderLight light(boundable);
    if (!TF_VERIFY(light)) {
        return false;
    }

    float radius;
    if (!light.GetRadiusAttr().Get(&radius, time)) {
        return false;
    }

    float length;
    if (!light.GetLengthAttr().Get(&length, time)) {
        return false;
    }

    if (!_ComputeLocalExtent(radius, length, extent)) {
        return false;
    }

    if (transform) {
        GfBBox3d bbox(GfRange3d((*extent)[0], (*extent)[1]), *transform);
        GfRange3d range = bbox.ComputeAlignedRange();
        (*extent)[0] = GfVec3f(range.GetMin());
        (*extent)[1] = GfVec3f(range.GetMax());
    }

    return true;
}

TF_REGISTRY_FUNCTION(UsdGeomBoundable)
{
    UsdGeomRegisterComputeExtentFunction<UsdLuxCylinderLight>(_ComputeExtent);
}

PXR_NAMESPACE_CLOSE_SCOPE
