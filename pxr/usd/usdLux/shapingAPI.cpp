//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/usd/usdLux/shapingAPI.h"
#include "pxr/usd/usd/schemaRegistry.h"
#include "pxr/usd/usd/typed.h"

#include "pxr/usd/sdf/types.h"
#include "pxr/usd/sdf/assetPath.h"

PXR_NAMESPACE_OPEN_SCOPE

// Register the schema with the TfType system.
TF_REGISTRY_FUNCTION(TfType)
{
    TfType::Define<UsdLuxShapingAPI,
        TfType::Bases< UsdAPISchemaBase > >();
    
}

/* virtual */
UsdLuxShapingAPI::~UsdLuxShapingAPI()
{
}

/* static */
UsdLuxShapingAPI
UsdLuxShapingAPI::Get(const UsdStagePtr &stage, const SdfPath &path)
{
    if (!stage) {
        TF_CODING_ERROR("Invalid stage");
        return UsdLuxShapingAPI();
    }
    return UsdLuxShapingAPI(stage->GetPrimAtPath(path));
}


/* virtual */
UsdSchemaKind UsdLuxShapingAPI::_GetSchemaKind() const
{
    return UsdLuxShapingAPI::schemaKind;
}

/* static */
bool
UsdLuxShapingAPI::CanApply(
    const UsdPrim &prim, std::string *whyNot)
{
    return prim.CanApplyAPI<UsdLuxShapingAPI>(whyNot);
}

/* static */
UsdLuxShapingAPI
UsdLuxShapingAPI::Apply(const UsdPrim &prim)
{
    if (prim.ApplyAPI<UsdLuxShapingAPI>()) {
        return UsdLuxShapingAPI(prim);
    }
    return UsdLuxShapingAPI();
}

/* static */
const TfType &
UsdLuxShapingAPI::_GetStaticTfType()
{
    static TfType tfType = TfType::Find<UsdLuxShapingAPI>();
    return tfType;
}

/* static */
bool 
UsdLuxShapingAPI::_IsTypedSchema()
{
    static bool isTyped = _GetStaticTfType().IsA<UsdTyped>();
    return isTyped;
}

/* virtual */
const TfType &
UsdLuxShapingAPI::_GetTfType() const
{
    return _GetStaticTfType();
}

UsdAttribute
UsdLuxShapingAPI::GetShapingFocusAttr() const
{
    return GetPrim().GetAttribute(UsdLuxTokens->inputsShapingFocus);
}

UsdAttribute
UsdLuxShapingAPI::CreateShapingFocusAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdLuxTokens->inputsShapingFocus,
                       SdfValueTypeNames->Float,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdLuxShapingAPI::GetShapingFocusTintAttr() const
{
    return GetPrim().GetAttribute(UsdLuxTokens->inputsShapingFocusTint);
}

UsdAttribute
UsdLuxShapingAPI::CreateShapingFocusTintAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdLuxTokens->inputsShapingFocusTint,
                       SdfValueTypeNames->Color3f,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdLuxShapingAPI::GetShapingConeAngleAttr() const
{
    return GetPrim().GetAttribute(UsdLuxTokens->inputsShapingConeAngle);
}

UsdAttribute
UsdLuxShapingAPI::CreateShapingConeAngleAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdLuxTokens->inputsShapingConeAngle,
                       SdfValueTypeNames->Float,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdLuxShapingAPI::GetShapingConeSoftnessAttr() const
{
    return GetPrim().GetAttribute(UsdLuxTokens->inputsShapingConeSoftness);
}

UsdAttribute
UsdLuxShapingAPI::CreateShapingConeSoftnessAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdLuxTokens->inputsShapingConeSoftness,
                       SdfValueTypeNames->Float,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdLuxShapingAPI::GetShapingIesFileAttr() const
{
    return GetPrim().GetAttribute(UsdLuxTokens->inputsShapingIesFile);
}

UsdAttribute
UsdLuxShapingAPI::CreateShapingIesFileAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdLuxTokens->inputsShapingIesFile,
                       SdfValueTypeNames->Asset,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdLuxShapingAPI::GetShapingIesAngleScaleAttr() const
{
    return GetPrim().GetAttribute(UsdLuxTokens->inputsShapingIesAngleScale);
}

UsdAttribute
UsdLuxShapingAPI::CreateShapingIesAngleScaleAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdLuxTokens->inputsShapingIesAngleScale,
                       SdfValueTypeNames->Float,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdLuxShapingAPI::GetShapingIesNormalizeAttr() const
{
    return GetPrim().GetAttribute(UsdLuxTokens->inputsShapingIesNormalize);
}

UsdAttribute
UsdLuxShapingAPI::CreateShapingIesNormalizeAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdLuxTokens->inputsShapingIesNormalize,
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
UsdLuxShapingAPI::GetSchemaAttributeNames(bool includeInherited)
{
    static TfTokenVector localNames = {
        UsdLuxTokens->inputsShapingFocus,
        UsdLuxTokens->inputsShapingFocusTint,
        UsdLuxTokens->inputsShapingConeAngle,
        UsdLuxTokens->inputsShapingConeSoftness,
        UsdLuxTokens->inputsShapingIesFile,
        UsdLuxTokens->inputsShapingIesAngleScale,
        UsdLuxTokens->inputsShapingIesNormalize,
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

UsdLuxShapingAPI::UsdLuxShapingAPI(const UsdShadeConnectableAPI &connectable)
    : UsdLuxShapingAPI(connectable.GetPrim())
{
}

UsdShadeConnectableAPI 
UsdLuxShapingAPI::ConnectableAPI() const
{
    return UsdShadeConnectableAPI(GetPrim());
}

UsdShadeOutput
UsdLuxShapingAPI::CreateOutput(const TfToken& name,
                                const SdfValueTypeName& typeName)
{
    return UsdShadeConnectableAPI(GetPrim()).CreateOutput(name, typeName);
}

UsdShadeOutput
UsdLuxShapingAPI::GetOutput(const TfToken &name) const
{
    return UsdShadeConnectableAPI(GetPrim()).GetOutput(name);
}

std::vector<UsdShadeOutput>
UsdLuxShapingAPI::GetOutputs(bool onlyAuthored) const
{
    return UsdShadeConnectableAPI(GetPrim()).GetOutputs(onlyAuthored);
}

UsdShadeInput
UsdLuxShapingAPI::CreateInput(const TfToken& name,
                               const SdfValueTypeName& typeName)
{
    return UsdShadeConnectableAPI(GetPrim()).CreateInput(name, typeName);
}

UsdShadeInput
UsdLuxShapingAPI::GetInput(const TfToken &name) const
{
    return UsdShadeConnectableAPI(GetPrim()).GetInput(name);
}

std::vector<UsdShadeInput>
UsdLuxShapingAPI::GetInputs(bool onlyAuthored) const
{
    return UsdShadeConnectableAPI(GetPrim()).GetInputs(onlyAuthored);
}

PXR_NAMESPACE_CLOSE_SCOPE
