//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/usd/usdLux/distantLight.h"
#include "pxr/usd/usd/schemaRegistry.h"
#include "pxr/usd/usd/typed.h"

#include "pxr/usd/sdf/types.h"
#include "pxr/usd/sdf/assetPath.h"

PXR_NAMESPACE_OPEN_SCOPE

// Register the schema with the TfType system.
TF_REGISTRY_FUNCTION(TfType)
{
    TfType::Define<UsdLuxDistantLight,
        TfType::Bases< UsdLuxNonboundableLightBase > >();
    
    // Register the usd prim typename as an alias under UsdSchemaBase. This
    // enables one to call
    // TfType::Find<UsdSchemaBase>().FindDerivedByName("DistantLight")
    // to find TfType<UsdLuxDistantLight>, which is how IsA queries are
    // answered.
    TfType::AddAlias<UsdSchemaBase, UsdLuxDistantLight>("DistantLight");
}

/* virtual */
UsdLuxDistantLight::~UsdLuxDistantLight()
{
}

/* static */
UsdLuxDistantLight
UsdLuxDistantLight::Get(const UsdStagePtr &stage, const SdfPath &path)
{
    if (!stage) {
        TF_CODING_ERROR("Invalid stage");
        return UsdLuxDistantLight();
    }
    return UsdLuxDistantLight(stage->GetPrimAtPath(path));
}

/* static */
UsdLuxDistantLight
UsdLuxDistantLight::Define(
    const UsdStagePtr &stage, const SdfPath &path)
{
    static TfToken usdPrimTypeName("DistantLight");
    if (!stage) {
        TF_CODING_ERROR("Invalid stage");
        return UsdLuxDistantLight();
    }
    return UsdLuxDistantLight(
        stage->DefinePrim(path, usdPrimTypeName));
}

/* virtual */
UsdSchemaKind UsdLuxDistantLight::_GetSchemaKind() const
{
    return UsdLuxDistantLight::schemaKind;
}

/* static */
const TfType &
UsdLuxDistantLight::_GetStaticTfType()
{
    static TfType tfType = TfType::Find<UsdLuxDistantLight>();
    return tfType;
}

/* static */
bool 
UsdLuxDistantLight::_IsTypedSchema()
{
    static bool isTyped = _GetStaticTfType().IsA<UsdTyped>();
    return isTyped;
}

/* virtual */
const TfType &
UsdLuxDistantLight::_GetTfType() const
{
    return _GetStaticTfType();
}

UsdAttribute
UsdLuxDistantLight::GetAngleAttr() const
{
    return GetPrim().GetAttribute(UsdLuxTokens->inputsAngle);
}

UsdAttribute
UsdLuxDistantLight::CreateAngleAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdLuxTokens->inputsAngle,
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
UsdLuxDistantLight::GetSchemaAttributeNames(bool includeInherited)
{
    static TfTokenVector localNames = {
        UsdLuxTokens->inputsAngle,
    };
    static TfTokenVector allNames =
        _ConcatenateAttributeNames(
            UsdLuxNonboundableLightBase::GetSchemaAttributeNames(true),
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
