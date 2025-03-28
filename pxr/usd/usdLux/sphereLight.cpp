//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/usd/usdLux/sphereLight.h"
#include "pxr/usd/usd/schemaRegistry.h"
#include "pxr/usd/usd/typed.h"

#include "pxr/usd/sdf/types.h"
#include "pxr/usd/sdf/assetPath.h"

PXR_NAMESPACE_OPEN_SCOPE

// Register the schema with the TfType system.
TF_REGISTRY_FUNCTION(TfType)
{
    TfType::Define<UsdLuxSphereLight,
        TfType::Bases< UsdLuxBoundableLightBase > >();
    
    // Register the usd prim typename as an alias under UsdSchemaBase. This
    // enables one to call
    // TfType::Find<UsdSchemaBase>().FindDerivedByName("SphereLight")
    // to find TfType<UsdLuxSphereLight>, which is how IsA queries are
    // answered.
    TfType::AddAlias<UsdSchemaBase, UsdLuxSphereLight>("SphereLight");
}

/* virtual */
UsdLuxSphereLight::~UsdLuxSphereLight()
{
}

/* static */
UsdLuxSphereLight
UsdLuxSphereLight::Get(const UsdStagePtr &stage, const SdfPath &path)
{
    if (!stage) {
        TF_CODING_ERROR("Invalid stage");
        return UsdLuxSphereLight();
    }
    return UsdLuxSphereLight(stage->GetPrimAtPath(path));
}

/* static */
UsdLuxSphereLight
UsdLuxSphereLight::Define(
    const UsdStagePtr &stage, const SdfPath &path)
{
    static TfToken usdPrimTypeName("SphereLight");
    if (!stage) {
        TF_CODING_ERROR("Invalid stage");
        return UsdLuxSphereLight();
    }
    return UsdLuxSphereLight(
        stage->DefinePrim(path, usdPrimTypeName));
}

/* virtual */
UsdSchemaKind UsdLuxSphereLight::_GetSchemaKind() const
{
    return UsdLuxSphereLight::schemaKind;
}

/* static */
const TfType &
UsdLuxSphereLight::_GetStaticTfType()
{
    static TfType tfType = TfType::Find<UsdLuxSphereLight>();
    return tfType;
}

/* static */
bool 
UsdLuxSphereLight::_IsTypedSchema()
{
    static bool isTyped = _GetStaticTfType().IsA<UsdTyped>();
    return isTyped;
}

/* virtual */
const TfType &
UsdLuxSphereLight::_GetTfType() const
{
    return _GetStaticTfType();
}

UsdAttribute
UsdLuxSphereLight::GetRadiusAttr() const
{
    return GetPrim().GetAttribute(UsdLuxTokens->inputsRadius);
}

UsdAttribute
UsdLuxSphereLight::CreateRadiusAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdLuxTokens->inputsRadius,
                       SdfValueTypeNames->Float,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdLuxSphereLight::GetTreatAsPointAttr() const
{
    return GetPrim().GetAttribute(UsdLuxTokens->treatAsPoint);
}

UsdAttribute
UsdLuxSphereLight::CreateTreatAsPointAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdLuxTokens->treatAsPoint,
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
UsdLuxSphereLight::GetSchemaAttributeNames(bool includeInherited)
{
    static TfTokenVector localNames = {
        UsdLuxTokens->inputsRadius,
        UsdLuxTokens->treatAsPoint,
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
_ComputeLocalExtent(const float radius, VtVec3fArray *extent)
{
    extent->resize(2);
    (*extent)[1] = GfVec3f(radius);
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
    const UsdLuxSphereLight light(boundable);
    if (!TF_VERIFY(light)) {
        return false;
    }

    float radius;
    if (!light.GetRadiusAttr().Get(&radius, time)) {
        return false;
    }

    if (!_ComputeLocalExtent(radius, extent)) {
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
    UsdGeomRegisterComputeExtentFunction<UsdLuxSphereLight>(_ComputeExtent);
}

PXR_NAMESPACE_CLOSE_SCOPE
