//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/usd/usdPhysics/sphericalJoint.h"
#include "pxr/usd/usd/schemaRegistry.h"
#include "pxr/usd/usd/typed.h"

#include "pxr/usd/sdf/types.h"
#include "pxr/usd/sdf/assetPath.h"

PXR_NAMESPACE_OPEN_SCOPE

// Register the schema with the TfType system.
TF_REGISTRY_FUNCTION(TfType)
{
    TfType::Define<UsdPhysicsSphericalJoint,
        TfType::Bases< UsdPhysicsJoint > >();
    
    // Register the usd prim typename as an alias under UsdSchemaBase. This
    // enables one to call
    // TfType::Find<UsdSchemaBase>().FindDerivedByName("PhysicsSphericalJoint")
    // to find TfType<UsdPhysicsSphericalJoint>, which is how IsA queries are
    // answered.
    TfType::AddAlias<UsdSchemaBase, UsdPhysicsSphericalJoint>("PhysicsSphericalJoint");
}

/* virtual */
UsdPhysicsSphericalJoint::~UsdPhysicsSphericalJoint()
{
}

/* static */
UsdPhysicsSphericalJoint
UsdPhysicsSphericalJoint::Get(const UsdStagePtr &stage, const SdfPath &path)
{
    if (!stage) {
        TF_CODING_ERROR("Invalid stage");
        return UsdPhysicsSphericalJoint();
    }
    return UsdPhysicsSphericalJoint(stage->GetPrimAtPath(path));
}

/* static */
UsdPhysicsSphericalJoint
UsdPhysicsSphericalJoint::Define(
    const UsdStagePtr &stage, const SdfPath &path)
{
    static TfToken usdPrimTypeName("PhysicsSphericalJoint");
    if (!stage) {
        TF_CODING_ERROR("Invalid stage");
        return UsdPhysicsSphericalJoint();
    }
    return UsdPhysicsSphericalJoint(
        stage->DefinePrim(path, usdPrimTypeName));
}

/* virtual */
UsdSchemaKind UsdPhysicsSphericalJoint::_GetSchemaKind() const
{
    return UsdPhysicsSphericalJoint::schemaKind;
}

/* static */
const TfType &
UsdPhysicsSphericalJoint::_GetStaticTfType()
{
    static TfType tfType = TfType::Find<UsdPhysicsSphericalJoint>();
    return tfType;
}

/* static */
bool 
UsdPhysicsSphericalJoint::_IsTypedSchema()
{
    static bool isTyped = _GetStaticTfType().IsA<UsdTyped>();
    return isTyped;
}

/* virtual */
const TfType &
UsdPhysicsSphericalJoint::_GetTfType() const
{
    return _GetStaticTfType();
}

UsdAttribute
UsdPhysicsSphericalJoint::GetAxisAttr() const
{
    return GetPrim().GetAttribute(UsdPhysicsTokens->physicsAxis);
}

UsdAttribute
UsdPhysicsSphericalJoint::CreateAxisAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdPhysicsTokens->physicsAxis,
                       SdfValueTypeNames->Token,
                       /* custom = */ false,
                       SdfVariabilityUniform,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdPhysicsSphericalJoint::GetConeAngle0LimitAttr() const
{
    return GetPrim().GetAttribute(UsdPhysicsTokens->physicsConeAngle0Limit);
}

UsdAttribute
UsdPhysicsSphericalJoint::CreateConeAngle0LimitAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdPhysicsTokens->physicsConeAngle0Limit,
                       SdfValueTypeNames->Float,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdPhysicsSphericalJoint::GetConeAngle1LimitAttr() const
{
    return GetPrim().GetAttribute(UsdPhysicsTokens->physicsConeAngle1Limit);
}

UsdAttribute
UsdPhysicsSphericalJoint::CreateConeAngle1LimitAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdPhysicsTokens->physicsConeAngle1Limit,
                       SdfValueTypeNames->Float,
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
UsdPhysicsSphericalJoint::GetSchemaAttributeNames(bool includeInherited)
{
    static TfTokenVector localNames = {
        UsdPhysicsTokens->physicsAxis,
        UsdPhysicsTokens->physicsConeAngle0Limit,
        UsdPhysicsTokens->physicsConeAngle1Limit,
    };
    static TfTokenVector allNames =
        _ConcatenateAttributeNames(
            UsdPhysicsJoint::GetSchemaAttributeNames(true),
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
