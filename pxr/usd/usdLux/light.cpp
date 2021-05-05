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
#include "pxr/usd/usdLux/light.h"
#include "pxr/usd/usd/schemaRegistry.h"
#include "pxr/usd/usd/typed.h"

#include "pxr/usd/sdf/types.h"
#include "pxr/usd/sdf/assetPath.h"

PXR_NAMESPACE_OPEN_SCOPE

// Register the schema with the TfType system.
TF_REGISTRY_FUNCTION(TfType)
{
    TfType::Define<UsdLuxLight,
        TfType::Bases< UsdGeomXformable > >();
    
}

/* virtual */
UsdLuxLight::~UsdLuxLight()
{
}

/* static */
UsdLuxLight
UsdLuxLight::Get(const UsdStagePtr &stage, const SdfPath &path)
{
    if (!stage) {
        TF_CODING_ERROR("Invalid stage");
        return UsdLuxLight();
    }
    return UsdLuxLight(stage->GetPrimAtPath(path));
}


/* virtual */
UsdSchemaKind UsdLuxLight::_GetSchemaKind() const {
    return UsdLuxLight::schemaKind;
}

/* virtual */
UsdSchemaKind UsdLuxLight::_GetSchemaType() const {
    return UsdLuxLight::schemaType;
}

/* static */
const TfType &
UsdLuxLight::_GetStaticTfType()
{
    static TfType tfType = TfType::Find<UsdLuxLight>();
    return tfType;
}

/* static */
bool 
UsdLuxLight::_IsTypedSchema()
{
    static bool isTyped = _GetStaticTfType().IsA<UsdTyped>();
    return isTyped;
}

/* virtual */
const TfType &
UsdLuxLight::_GetTfType() const
{
    return _GetStaticTfType();
}

UsdAttribute
UsdLuxLight::GetIntensityAttr() const
{
    return GetPrim().GetAttribute(UsdLuxTokens->inputsIntensity);
}

UsdAttribute
UsdLuxLight::CreateIntensityAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdLuxTokens->inputsIntensity,
                       SdfValueTypeNames->Float,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdLuxLight::GetExposureAttr() const
{
    return GetPrim().GetAttribute(UsdLuxTokens->inputsExposure);
}

UsdAttribute
UsdLuxLight::CreateExposureAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdLuxTokens->inputsExposure,
                       SdfValueTypeNames->Float,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdLuxLight::GetDiffuseAttr() const
{
    return GetPrim().GetAttribute(UsdLuxTokens->inputsDiffuse);
}

UsdAttribute
UsdLuxLight::CreateDiffuseAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdLuxTokens->inputsDiffuse,
                       SdfValueTypeNames->Float,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdLuxLight::GetSpecularAttr() const
{
    return GetPrim().GetAttribute(UsdLuxTokens->inputsSpecular);
}

UsdAttribute
UsdLuxLight::CreateSpecularAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdLuxTokens->inputsSpecular,
                       SdfValueTypeNames->Float,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdLuxLight::GetNormalizeAttr() const
{
    return GetPrim().GetAttribute(UsdLuxTokens->inputsNormalize);
}

UsdAttribute
UsdLuxLight::CreateNormalizeAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdLuxTokens->inputsNormalize,
                       SdfValueTypeNames->Bool,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdLuxLight::GetColorAttr() const
{
    return GetPrim().GetAttribute(UsdLuxTokens->inputsColor);
}

UsdAttribute
UsdLuxLight::CreateColorAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdLuxTokens->inputsColor,
                       SdfValueTypeNames->Color3f,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdLuxLight::GetEnableColorTemperatureAttr() const
{
    return GetPrim().GetAttribute(UsdLuxTokens->inputsEnableColorTemperature);
}

UsdAttribute
UsdLuxLight::CreateEnableColorTemperatureAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdLuxTokens->inputsEnableColorTemperature,
                       SdfValueTypeNames->Bool,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdLuxLight::GetColorTemperatureAttr() const
{
    return GetPrim().GetAttribute(UsdLuxTokens->inputsColorTemperature);
}

UsdAttribute
UsdLuxLight::CreateColorTemperatureAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdLuxTokens->inputsColorTemperature,
                       SdfValueTypeNames->Float,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

UsdRelationship
UsdLuxLight::GetFiltersRel() const
{
    return GetPrim().GetRelationship(UsdLuxTokens->filters);
}

UsdRelationship
UsdLuxLight::CreateFiltersRel() const
{
    return GetPrim().CreateRelationship(UsdLuxTokens->filters,
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
UsdLuxLight::GetSchemaAttributeNames(bool includeInherited)
{
    static TfTokenVector localNames = {
        UsdLuxTokens->collectionLightLinkIncludeRoot,
        UsdLuxTokens->collectionShadowLinkIncludeRoot,
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
            UsdGeomXformable::GetSchemaAttributeNames(true),
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

#include "pxr/usd/usdLux/blackbody.h"

#include "pxr/usd/usdShade/connectableAPI.h"
#include "pxr/usd/usdShade/connectableAPIBehavior.h"

PXR_NAMESPACE_OPEN_SCOPE

class UsdLuxLight_ConnectableAPIBehavior : public UsdShadeConnectableAPIBehavior
{
    bool
    CanConnectInputToSource(const UsdShadeInput &input,
                            const UsdAttribute &source,
                            std::string *reason) override
    {
        return _CanConnectInputToSource(input, source, reason, 
                ConnectableNodeTypes::DerivedContainerNodes);
    }

    bool IsContainer() const
    {
        return true;
    }

    // Note that Light's outputs are not connectable (different from
    // UsdShadeNodeGraph default behavior) as there are no known use-case for 
    // these right now.
};

TF_REGISTRY_FUNCTION(UsdShadeConnectableAPI)
{
    // UsdLuxLight prims are connectable, with special behavior requiring 
    // connection source to be encapsulated under the light.
    UsdShadeRegisterConnectableAPIBehavior<
        UsdLuxLight, UsdLuxLight_ConnectableAPIBehavior>();
}

UsdLuxLight::UsdLuxLight(const UsdShadeConnectableAPI &connectable)
    : UsdLuxLight(connectable.GetPrim())
{
}

UsdShadeConnectableAPI 
UsdLuxLight::ConnectableAPI() const
{
    return UsdShadeConnectableAPI(GetPrim());
}

UsdShadeOutput
UsdLuxLight::CreateOutput(const TfToken& name,
                          const SdfValueTypeName& typeName)
{
    return UsdShadeConnectableAPI(GetPrim()).CreateOutput(name, typeName);
}

UsdShadeOutput
UsdLuxLight::GetOutput(const TfToken &name) const
{
    return UsdShadeConnectableAPI(GetPrim()).GetOutput(name);
}

std::vector<UsdShadeOutput>
UsdLuxLight::GetOutputs(bool onlyAuthored) const
{
    return UsdShadeConnectableAPI(GetPrim()).GetOutputs(onlyAuthored);
}

UsdShadeInput
UsdLuxLight::CreateInput(const TfToken& name,
                         const SdfValueTypeName& typeName)
{
    return UsdShadeConnectableAPI(GetPrim()).CreateInput(name, typeName);
}

UsdShadeInput
UsdLuxLight::GetInput(const TfToken &name) const
{
    return UsdShadeConnectableAPI(GetPrim()).GetInput(name);
}

std::vector<UsdShadeInput>
UsdLuxLight::GetInputs(bool onlyAuthored) const
{
    return UsdShadeConnectableAPI(GetPrim()).GetInputs(onlyAuthored);
}

GfVec3f
UsdLuxLight::ComputeBaseEmission() const
{
    GfVec3f e(1.0);

    float intensity = 1.0;
    GetIntensityAttr().Get(&intensity);
    e *= intensity;

    float exposure = 0.0;
    GetExposureAttr().Get(&exposure);
    e *= exp2(exposure);

    GfVec3f color(1.0);
    GetColorAttr().Get(&color);
    e = GfCompMult(e, color);

    bool enableColorTemp = false;
    GetEnableColorTemperatureAttr().Get(&enableColorTemp);
    if (enableColorTemp) {
        float colorTemp = 6500;
        if (GetColorTemperatureAttr().Get(&colorTemp)) {
            e = GfCompMult(e, UsdLuxBlackbodyTemperatureAsRgb(colorTemp));
        }
    }

    return e;
}

UsdCollectionAPI
UsdLuxLight::GetLightLinkCollectionAPI() const
{
    return UsdCollectionAPI(GetPrim(), UsdLuxTokens->lightLink);
}

UsdCollectionAPI
UsdLuxLight::GetShadowLinkCollectionAPI() const
{
    return UsdCollectionAPI(GetPrim(), UsdLuxTokens->shadowLink);
}

PXR_NAMESPACE_CLOSE_SCOPE
