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
UsdSchemaKind UsdLuxLight::_GetSchemaKind() const
{
    return UsdLuxLight::schemaKind;
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

/*static*/
const TfTokenVector&
UsdLuxLight::GetSchemaAttributeNames(bool includeInherited)
{
    static TfTokenVector localNames;
    static TfTokenVector allNames =
        UsdGeomXformable::GetSchemaAttributeNames(true);

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
#include "pxr/usd/usdLux/lightAPI.h"

#include "pxr/usd/usdShade/connectableAPI.h"

PXR_NAMESPACE_OPEN_SCOPE

UsdAttribute
UsdLuxLight::GetIntensityAttr() const
{
    return UsdLuxLightAPI(GetPrim()).GetIntensityAttr();
}

UsdAttribute
UsdLuxLight::CreateIntensityAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdLuxLightAPI(GetPrim()).CreateIntensityAttr(
        defaultValue, writeSparsely);
}

UsdAttribute
UsdLuxLight::GetExposureAttr() const
{
    return UsdLuxLightAPI(GetPrim()).GetExposureAttr();
}

UsdAttribute
UsdLuxLight::CreateExposureAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdLuxLightAPI(GetPrim()).CreateExposureAttr(
        defaultValue, writeSparsely);
}

UsdAttribute
UsdLuxLight::GetDiffuseAttr() const
{
    return UsdLuxLightAPI(GetPrim()).GetDiffuseAttr();
}

UsdAttribute
UsdLuxLight::CreateDiffuseAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdLuxLightAPI(GetPrim()).CreateDiffuseAttr(
        defaultValue, writeSparsely);
}

UsdAttribute
UsdLuxLight::GetSpecularAttr() const
{
    return UsdLuxLightAPI(GetPrim()).GetSpecularAttr();
}

UsdAttribute
UsdLuxLight::CreateSpecularAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdLuxLightAPI(GetPrim()).CreateSpecularAttr(
        defaultValue, writeSparsely);
}

UsdAttribute
UsdLuxLight::GetNormalizeAttr() const
{
    return UsdLuxLightAPI(GetPrim()).GetNormalizeAttr();
}

UsdAttribute
UsdLuxLight::CreateNormalizeAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdLuxLightAPI(GetPrim()).CreateNormalizeAttr(
        defaultValue, writeSparsely);
}

UsdAttribute
UsdLuxLight::GetColorAttr() const
{
    return UsdLuxLightAPI(GetPrim()).GetColorAttr();
}

UsdAttribute
UsdLuxLight::CreateColorAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdLuxLightAPI(GetPrim()).CreateColorAttr(
        defaultValue, writeSparsely);
}

UsdAttribute
UsdLuxLight::GetEnableColorTemperatureAttr() const
{
    return UsdLuxLightAPI(GetPrim()).GetEnableColorTemperatureAttr();
}

UsdAttribute
UsdLuxLight::CreateEnableColorTemperatureAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdLuxLightAPI(GetPrim()).CreateEnableColorTemperatureAttr(
        defaultValue, writeSparsely);
}

UsdAttribute
UsdLuxLight::GetColorTemperatureAttr() const
{
    return UsdLuxLightAPI(GetPrim()).GetColorTemperatureAttr();
}

UsdAttribute
UsdLuxLight::CreateColorTemperatureAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdLuxLightAPI(GetPrim()).CreateColorTemperatureAttr(
        defaultValue, writeSparsely);
}

UsdRelationship
UsdLuxLight::GetFiltersRel() const
{
    return UsdLuxLightAPI(GetPrim()).GetFiltersRel();
}

UsdRelationship
UsdLuxLight::CreateFiltersRel() const
{
    return UsdLuxLightAPI(GetPrim()).CreateFiltersRel();
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
    return UsdLuxLightAPI(GetPrim()).CreateOutput(name, typeName);
}

UsdShadeOutput
UsdLuxLight::GetOutput(const TfToken &name) const
{
    return UsdLuxLightAPI(GetPrim()).GetOutput(name);
}

std::vector<UsdShadeOutput>
UsdLuxLight::GetOutputs(bool onlyAuthored) const
{
    return UsdLuxLightAPI(GetPrim()).GetOutputs(onlyAuthored);
}

UsdShadeInput
UsdLuxLight::CreateInput(const TfToken& name,
                         const SdfValueTypeName& typeName)
{
    return UsdLuxLightAPI(GetPrim()).CreateInput(name, typeName);
}

UsdShadeInput
UsdLuxLight::GetInput(const TfToken &name) const
{
    return UsdLuxLightAPI(GetPrim()).GetInput(name);
}

std::vector<UsdShadeInput>
UsdLuxLight::GetInputs(bool onlyAuthored) const
{
    return UsdLuxLightAPI(GetPrim()).GetInputs(onlyAuthored);
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
    return UsdLuxLightAPI(GetPrim()).GetLightLinkCollectionAPI();
}

UsdCollectionAPI
UsdLuxLight::GetShadowLinkCollectionAPI() const
{
    return UsdLuxLightAPI(GetPrim()).GetShadowLinkCollectionAPI();
}

PXR_NAMESPACE_CLOSE_SCOPE
