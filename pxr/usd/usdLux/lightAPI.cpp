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
#include "pxr/usd/usdLux/lightAPI.h"
#include "pxr/usd/usd/schemaRegistry.h"
#include "pxr/usd/usd/typed.h"

#include "pxr/usd/sdf/types.h"
#include "pxr/usd/sdf/assetPath.h"

PXR_NAMESPACE_OPEN_SCOPE

// Register the schema with the TfType system.
TF_REGISTRY_FUNCTION(TfType)
{
    TfType::Define<UsdLuxLightAPI,
        TfType::Bases< UsdAPISchemaBase > >();
    
}

/* virtual */
UsdLuxLightAPI::~UsdLuxLightAPI()
{
}

/* static */
UsdLuxLightAPI
UsdLuxLightAPI::Get(const UsdStagePtr &stage, const SdfPath &path)
{
    if (!stage) {
        TF_CODING_ERROR("Invalid stage");
        return UsdLuxLightAPI();
    }
    return UsdLuxLightAPI(stage->GetPrimAtPath(path));
}


/* virtual */
UsdSchemaKind UsdLuxLightAPI::_GetSchemaKind() const
{
    return UsdLuxLightAPI::schemaKind;
}

/* static */
bool
UsdLuxLightAPI::CanApply(
    const UsdPrim &prim, std::string *whyNot)
{
    return prim.CanApplyAPI<UsdLuxLightAPI>(whyNot);
}

/* static */
UsdLuxLightAPI
UsdLuxLightAPI::Apply(const UsdPrim &prim)
{
    if (prim.ApplyAPI<UsdLuxLightAPI>()) {
        return UsdLuxLightAPI(prim);
    }
    return UsdLuxLightAPI();
}

/* static */
const TfType &
UsdLuxLightAPI::_GetStaticTfType()
{
    static TfType tfType = TfType::Find<UsdLuxLightAPI>();
    return tfType;
}

/* static */
bool 
UsdLuxLightAPI::_IsTypedSchema()
{
    static bool isTyped = _GetStaticTfType().IsA<UsdTyped>();
    return isTyped;
}

/* virtual */
const TfType &
UsdLuxLightAPI::_GetTfType() const
{
    return _GetStaticTfType();
}

UsdAttribute
UsdLuxLightAPI::GetShaderIdAttr() const
{
    return GetPrim().GetAttribute(UsdLuxTokens->lightShaderId);
}

UsdAttribute
UsdLuxLightAPI::CreateShaderIdAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdLuxTokens->lightShaderId,
                       SdfValueTypeNames->Token,
                       /* custom = */ false,
                       SdfVariabilityUniform,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdLuxLightAPI::GetMaterialSyncModeAttr() const
{
    return GetPrim().GetAttribute(UsdLuxTokens->lightMaterialSyncMode);
}

UsdAttribute
UsdLuxLightAPI::CreateMaterialSyncModeAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdLuxTokens->lightMaterialSyncMode,
                       SdfValueTypeNames->Token,
                       /* custom = */ false,
                       SdfVariabilityUniform,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdLuxLightAPI::GetIntensityAttr() const
{
    return GetPrim().GetAttribute(UsdLuxTokens->inputsIntensity);
}

UsdAttribute
UsdLuxLightAPI::CreateIntensityAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdLuxTokens->inputsIntensity,
                       SdfValueTypeNames->Float,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdLuxLightAPI::GetExposureAttr() const
{
    return GetPrim().GetAttribute(UsdLuxTokens->inputsExposure);
}

UsdAttribute
UsdLuxLightAPI::CreateExposureAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdLuxTokens->inputsExposure,
                       SdfValueTypeNames->Float,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdLuxLightAPI::GetDiffuseAttr() const
{
    return GetPrim().GetAttribute(UsdLuxTokens->inputsDiffuse);
}

UsdAttribute
UsdLuxLightAPI::CreateDiffuseAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdLuxTokens->inputsDiffuse,
                       SdfValueTypeNames->Float,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdLuxLightAPI::GetSpecularAttr() const
{
    return GetPrim().GetAttribute(UsdLuxTokens->inputsSpecular);
}

UsdAttribute
UsdLuxLightAPI::CreateSpecularAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdLuxTokens->inputsSpecular,
                       SdfValueTypeNames->Float,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdLuxLightAPI::GetNormalizeAttr() const
{
    return GetPrim().GetAttribute(UsdLuxTokens->inputsNormalize);
}

UsdAttribute
UsdLuxLightAPI::CreateNormalizeAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdLuxTokens->inputsNormalize,
                       SdfValueTypeNames->Bool,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdLuxLightAPI::GetColorAttr() const
{
    return GetPrim().GetAttribute(UsdLuxTokens->inputsColor);
}

UsdAttribute
UsdLuxLightAPI::CreateColorAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdLuxTokens->inputsColor,
                       SdfValueTypeNames->Color3f,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdLuxLightAPI::GetEnableColorTemperatureAttr() const
{
    return GetPrim().GetAttribute(UsdLuxTokens->inputsEnableColorTemperature);
}

UsdAttribute
UsdLuxLightAPI::CreateEnableColorTemperatureAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdLuxTokens->inputsEnableColorTemperature,
                       SdfValueTypeNames->Bool,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdLuxLightAPI::GetColorTemperatureAttr() const
{
    return GetPrim().GetAttribute(UsdLuxTokens->inputsColorTemperature);
}

UsdAttribute
UsdLuxLightAPI::CreateColorTemperatureAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdLuxTokens->inputsColorTemperature,
                       SdfValueTypeNames->Float,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

UsdRelationship
UsdLuxLightAPI::GetFiltersRel() const
{
    return GetPrim().GetRelationship(UsdLuxTokens->lightFilters);
}

UsdRelationship
UsdLuxLightAPI::CreateFiltersRel() const
{
    return GetPrim().CreateRelationship(UsdLuxTokens->lightFilters,
                       /* custom = */ false);
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
UsdLuxLightAPI::GetSchemaAttributeNames(bool includeInherited)
{
    static TfTokenVector localNames = {
        UsdLuxTokens->lightShaderId,
        UsdLuxTokens->lightMaterialSyncMode,
        UsdLuxTokens->inputsIntensity,
        UsdLuxTokens->inputsExposure,
        UsdLuxTokens->inputsDiffuse,
        UsdLuxTokens->inputsSpecular,
        UsdLuxTokens->inputsNormalize,
        UsdLuxTokens->inputsColor,
        UsdLuxTokens->inputsEnableColorTemperature,
        UsdLuxTokens->inputsColorTemperature,
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
#include "pxr/usd/usdShade/connectableAPIBehavior.h"

PXR_NAMESPACE_OPEN_SCOPE

class UsdLuxLightAPI_ConnectableAPIBehavior : public UsdShadeConnectableAPIBehavior
{
public:
    // By default all UsdLuxLightAPI Connectable Behavior should be
    // container and not exhibit encapsulation behavior, as we expect lights to
    // be connected across multiple scopes, hence ignoring encapsulation rules.
    UsdLuxLightAPI_ConnectableAPIBehavior() : 
        UsdShadeConnectableAPIBehavior(
                true /*isContainer*/, false /*requiresEncapsulation*/) {}

    bool
    CanConnectInputToSource(const UsdShadeInput &input,
                            const UsdAttribute &source,
                            std::string *reason) const override
    {
        return _CanConnectInputToSource(input, source, reason, 
                ConnectableNodeTypes::DerivedContainerNodes);
    }

    bool
    CanConnectOutputToSource(const UsdShadeOutput &output,
                             const UsdAttribute &source,
                             std::string *reason) const override
    {
        return _CanConnectOutputToSource(output, source, reason,
                ConnectableNodeTypes::DerivedContainerNodes);
    }

};

TF_REGISTRY_FUNCTION(UsdShadeConnectableAPI)
{
    UsdShadeRegisterConnectableAPIBehavior<
        UsdLuxLightAPI, UsdLuxLightAPI_ConnectableAPIBehavior>();
}

UsdLuxLightAPI::UsdLuxLightAPI(const UsdShadeConnectableAPI &connectable)
    : UsdLuxLightAPI(connectable.GetPrim())
{
}

UsdShadeConnectableAPI 
UsdLuxLightAPI::ConnectableAPI() const
{
    return UsdShadeConnectableAPI(GetPrim());
}

UsdShadeOutput
UsdLuxLightAPI::CreateOutput(const TfToken& name,
                          const SdfValueTypeName& typeName)
{
    return UsdShadeConnectableAPI(GetPrim()).CreateOutput(name, typeName);
}

UsdShadeOutput
UsdLuxLightAPI::GetOutput(const TfToken &name) const
{
    return UsdShadeConnectableAPI(GetPrim()).GetOutput(name);
}

std::vector<UsdShadeOutput>
UsdLuxLightAPI::GetOutputs(bool onlyAuthored) const
{
    return UsdShadeConnectableAPI(GetPrim()).GetOutputs(onlyAuthored);
}

UsdShadeInput
UsdLuxLightAPI::CreateInput(const TfToken& name,
                         const SdfValueTypeName& typeName)
{
    return UsdShadeConnectableAPI(GetPrim()).CreateInput(name, typeName);
}

UsdShadeInput
UsdLuxLightAPI::GetInput(const TfToken &name) const
{
    return UsdShadeConnectableAPI(GetPrim()).GetInput(name);
}

std::vector<UsdShadeInput>
UsdLuxLightAPI::GetInputs(bool onlyAuthored) const
{
    return UsdShadeConnectableAPI(GetPrim()).GetInputs(onlyAuthored);
}

UsdCollectionAPI
UsdLuxLightAPI::GetLightLinkCollectionAPI() const
{
    return UsdCollectionAPI(GetPrim(), UsdLuxTokens->lightLink);
}

UsdCollectionAPI
UsdLuxLightAPI::GetShadowLinkCollectionAPI() const
{
    return UsdCollectionAPI(GetPrim(), UsdLuxTokens->shadowLink);
}

static TfToken 
_GetShaderIdAttrName(const TfToken &renderContext)
{
    if (renderContext.IsEmpty()) {
        return UsdLuxTokens->lightShaderId;
    }
    return TfToken(
        SdfPath::JoinIdentifier(renderContext, UsdLuxTokens->lightShaderId));
}

UsdAttribute 
UsdLuxLightAPI::GetShaderIdAttrForRenderContext(
    const TfToken &renderContext) const
{
    return GetPrim().GetAttribute(_GetShaderIdAttrName(renderContext));
}

UsdAttribute 
UsdLuxLightAPI::CreateShaderIdAttrForRenderContext(
    const TfToken &renderContext, 
    VtValue const &defaultValue, 
    bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(_GetShaderIdAttrName(renderContext),
                       SdfValueTypeNames->Token,
                       /* custom = */ false,
                       SdfVariabilityUniform,
                       defaultValue,
                       writeSparsely);
}

TfToken 
UsdLuxLightAPI::GetShaderId(const TfTokenVector &renderContexts) const
{
    TfToken shaderId;
    // The passed in render contexts are in priority order so return the shader
    // ID from the first render context specific shaderId attribute that has a
    // a non-empty value.
    for (const TfToken &renderContext : renderContexts) {
        if (UsdAttribute shaderIdAttr = 
                GetShaderIdAttrForRenderContext(renderContext)) {
            shaderIdAttr.Get(&shaderId);
            if (!shaderId.IsEmpty()) {
                return shaderId;
            }
        }
    }
    // Return the default shaderId attributes values if we couldn't get a value
    // for any of the render contexts.
    GetShaderIdAttr().Get(&shaderId);
    return shaderId;
}

PXR_NAMESPACE_CLOSE_SCOPE
