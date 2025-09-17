//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/usd/usdLux/shadowAPI.h"
#include "pxr/usd/usd/schemaRegistry.h"
#include "pxr/usd/usd/typed.h"

#include "pxr/usd/sdf/types.h"
#include "pxr/usd/sdf/assetPath.h"

PXR_NAMESPACE_OPEN_SCOPE

// Register the schema with the TfType system.
TF_REGISTRY_FUNCTION(TfType)
{
    TfType::Define<UsdLuxShadowAPI,
        TfType::Bases< UsdAPISchemaBase > >();
    
}

/* virtual */
UsdLuxShadowAPI::~UsdLuxShadowAPI()
{
}

/* static */
UsdLuxShadowAPI
UsdLuxShadowAPI::Get(const UsdStagePtr &stage, const SdfPath &path)
{
    if (!stage) {
        TF_CODING_ERROR("Invalid stage");
        return UsdLuxShadowAPI();
    }
    return UsdLuxShadowAPI(stage->GetPrimAtPath(path));
}


/* virtual */
UsdSchemaKind UsdLuxShadowAPI::_GetSchemaKind() const
{
    return UsdLuxShadowAPI::schemaKind;
}

/* static */
bool
UsdLuxShadowAPI::CanApply(
    const UsdPrim &prim, std::string *whyNot)
{
    return prim.CanApplyAPI<UsdLuxShadowAPI>(whyNot);
}

/* static */
UsdLuxShadowAPI
UsdLuxShadowAPI::Apply(const UsdPrim &prim)
{
    if (prim.ApplyAPI<UsdLuxShadowAPI>()) {
        return UsdLuxShadowAPI(prim);
    }
    return UsdLuxShadowAPI();
}

/* static */
const TfType &
UsdLuxShadowAPI::_GetStaticTfType()
{
    static TfType tfType = TfType::Find<UsdLuxShadowAPI>();
    return tfType;
}

/* static */
bool 
UsdLuxShadowAPI::_IsTypedSchema()
{
    static bool isTyped = _GetStaticTfType().IsA<UsdTyped>();
    return isTyped;
}

/* virtual */
const TfType &
UsdLuxShadowAPI::_GetTfType() const
{
    return _GetStaticTfType();
}

UsdAttribute
UsdLuxShadowAPI::GetShadowEnableAttr() const
{
    return GetPrim().GetAttribute(UsdLuxTokens->inputsShadowEnable);
}

UsdAttribute
UsdLuxShadowAPI::CreateShadowEnableAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdLuxTokens->inputsShadowEnable,
                       SdfValueTypeNames->Bool,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdLuxShadowAPI::GetShadowColorAttr() const
{
    return GetPrim().GetAttribute(UsdLuxTokens->inputsShadowColor);
}

UsdAttribute
UsdLuxShadowAPI::CreateShadowColorAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdLuxTokens->inputsShadowColor,
                       SdfValueTypeNames->Color3f,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdLuxShadowAPI::GetShadowDistanceAttr() const
{
    return GetPrim().GetAttribute(UsdLuxTokens->inputsShadowDistance);
}

UsdAttribute
UsdLuxShadowAPI::CreateShadowDistanceAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdLuxTokens->inputsShadowDistance,
                       SdfValueTypeNames->Float,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdLuxShadowAPI::GetShadowFalloffAttr() const
{
    return GetPrim().GetAttribute(UsdLuxTokens->inputsShadowFalloff);
}

UsdAttribute
UsdLuxShadowAPI::CreateShadowFalloffAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdLuxTokens->inputsShadowFalloff,
                       SdfValueTypeNames->Float,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdLuxShadowAPI::GetShadowFalloffGammaAttr() const
{
    return GetPrim().GetAttribute(UsdLuxTokens->inputsShadowFalloffGamma);
}

UsdAttribute
UsdLuxShadowAPI::CreateShadowFalloffGammaAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdLuxTokens->inputsShadowFalloffGamma,
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
UsdLuxShadowAPI::GetSchemaAttributeNames(bool includeInherited)
{
    static TfTokenVector localNames = {
        UsdLuxTokens->inputsShadowEnable,
        UsdLuxTokens->inputsShadowColor,
        UsdLuxTokens->inputsShadowDistance,
        UsdLuxTokens->inputsShadowFalloff,
        UsdLuxTokens->inputsShadowFalloffGamma,
    };
    static TfTokenVector allNames =
        _ConcatenateAttributeNames(
            UsdAPISchemaBase::GetSchemaAttributeNames(true),
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

#include "pxr/usd/usdShade/connectableAPI.h"

PXR_NAMESPACE_OPEN_SCOPE

UsdLuxShadowAPI::UsdLuxShadowAPI(const UsdShadeConnectableAPI &connectable)
    : UsdLuxShadowAPI(connectable.GetPrim())
{
}

UsdShadeConnectableAPI 
UsdLuxShadowAPI::ConnectableAPI() const
{
    return UsdShadeConnectableAPI(GetPrim());
}

UsdShadeOutput
UsdLuxShadowAPI::CreateOutput(const TfToken& name,
                                const SdfValueTypeName& typeName)
{
    return UsdShadeConnectableAPI(GetPrim()).CreateOutput(name, typeName);
}

UsdShadeOutput
UsdLuxShadowAPI::GetOutput(const TfToken &name) const
{
    return UsdShadeConnectableAPI(GetPrim()).GetOutput(name);
}

std::vector<UsdShadeOutput>
UsdLuxShadowAPI::GetOutputs(bool onlyAuthored) const
{
    return UsdShadeConnectableAPI(GetPrim()).GetOutputs(onlyAuthored);
}

UsdShadeInput
UsdLuxShadowAPI::CreateInput(const TfToken& name,
                               const SdfValueTypeName& typeName)
{
    return UsdShadeConnectableAPI(GetPrim()).CreateInput(name, typeName);
}

UsdShadeInput
UsdLuxShadowAPI::GetInput(const TfToken &name) const
{
    return UsdShadeConnectableAPI(GetPrim()).GetInput(name);
}

std::vector<UsdShadeInput>
UsdLuxShadowAPI::GetInputs(bool onlyAuthored) const
{
    return UsdShadeConnectableAPI(GetPrim()).GetInputs(onlyAuthored);
}

PXR_NAMESPACE_CLOSE_SCOPE
