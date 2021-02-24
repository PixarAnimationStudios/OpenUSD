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
#include "pxr/usd/usdLux/shadowAPI.h"
#include "pxr/usd/usd/schemaRegistry.h"
#include "pxr/usd/usd/typed.h"
#include "pxr/usd/usd/tokens.h"

#include "pxr/usd/sdf/types.h"
#include "pxr/usd/sdf/assetPath.h"

PXR_NAMESPACE_OPEN_SCOPE

// Register the schema with the TfType system.
TF_REGISTRY_FUNCTION(TfType)
{
    TfType::Define<UsdLuxShadowAPI,
        TfType::Bases< UsdAPISchemaBase > >();
    
}

TF_DEFINE_PRIVATE_TOKENS(
    _schemaTokens,
    (ShadowAPI)
);

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
UsdSchemaKind UsdLuxShadowAPI::_GetSchemaKind() const {
    return UsdLuxShadowAPI::schemaKind;
}

/* virtual */
UsdSchemaKind UsdLuxShadowAPI::_GetSchemaType() const {
    return UsdLuxShadowAPI::schemaType;
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
