//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/usd/usdLux/boundableLightBase.h"
#include "pxr/usd/usd/schemaRegistry.h"
#include "pxr/usd/usd/typed.h"

#include "pxr/usd/sdf/types.h"
#include "pxr/usd/sdf/assetPath.h"

PXR_NAMESPACE_OPEN_SCOPE

// Register the schema with the TfType system.
TF_REGISTRY_FUNCTION(TfType)
{
    TfType::Define<UsdLuxBoundableLightBase,
        TfType::Bases< UsdGeomBoundable > >();
    
}

/* virtual */
UsdLuxBoundableLightBase::~UsdLuxBoundableLightBase()
{
}

/* static */
UsdLuxBoundableLightBase
UsdLuxBoundableLightBase::Get(const UsdStagePtr &stage, const SdfPath &path)
{
    if (!stage) {
        TF_CODING_ERROR("Invalid stage");
        return UsdLuxBoundableLightBase();
    }
    return UsdLuxBoundableLightBase(stage->GetPrimAtPath(path));
}


/* virtual */
UsdSchemaKind UsdLuxBoundableLightBase::_GetSchemaKind() const
{
    return UsdLuxBoundableLightBase::schemaKind;
}

/* static */
const TfType &
UsdLuxBoundableLightBase::_GetStaticTfType()
{
    static TfType tfType = TfType::Find<UsdLuxBoundableLightBase>();
    return tfType;
}

/* static */
bool 
UsdLuxBoundableLightBase::_IsTypedSchema()
{
    static bool isTyped = _GetStaticTfType().IsA<UsdTyped>();
    return isTyped;
}

/* virtual */
const TfType &
UsdLuxBoundableLightBase::_GetTfType() const
{
    return _GetStaticTfType();
}

/*static*/
const TfTokenVector&
UsdLuxBoundableLightBase::GetSchemaAttributeNames(bool includeInherited)
{
    static TfTokenVector localNames;
    static TfTokenVector allNames =
        UsdGeomBoundable::GetSchemaAttributeNames(true);

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

PXR_NAMESPACE_OPEN_SCOPE

UsdAttribute
UsdLuxBoundableLightBase::GetIntensityAttr() const
{
    return LightAPI().GetIntensityAttr();
}

UsdAttribute
UsdLuxBoundableLightBase::CreateIntensityAttr(
    VtValue const &defaultValue, bool writeSparsely) const
{
    return LightAPI().CreateIntensityAttr(
        defaultValue, writeSparsely);
}

UsdAttribute
UsdLuxBoundableLightBase::GetExposureAttr() const
{
    return LightAPI().GetExposureAttr();
}

UsdAttribute
UsdLuxBoundableLightBase::CreateExposureAttr(
    VtValue const &defaultValue, bool writeSparsely) const
{
    return LightAPI().CreateExposureAttr(
        defaultValue, writeSparsely);
}

UsdAttribute
UsdLuxBoundableLightBase::GetDiffuseAttr() const
{
    return LightAPI().GetDiffuseAttr();
}

UsdAttribute
UsdLuxBoundableLightBase::CreateDiffuseAttr(
    VtValue const &defaultValue, bool writeSparsely) const
{
    return LightAPI().CreateDiffuseAttr(
        defaultValue, writeSparsely);
}

UsdAttribute
UsdLuxBoundableLightBase::GetSpecularAttr() const
{
    return LightAPI().GetSpecularAttr();
}

UsdAttribute
UsdLuxBoundableLightBase::CreateSpecularAttr(
    VtValue const &defaultValue, bool writeSparsely) const
{
    return LightAPI().CreateSpecularAttr(
        defaultValue, writeSparsely);
}

UsdAttribute
UsdLuxBoundableLightBase::GetNormalizeAttr() const
{
    return LightAPI().GetNormalizeAttr();
}

UsdAttribute
UsdLuxBoundableLightBase::CreateNormalizeAttr(
    VtValue const &defaultValue, bool writeSparsely) const
{
    return LightAPI().CreateNormalizeAttr(
        defaultValue, writeSparsely);
}

UsdAttribute
UsdLuxBoundableLightBase::GetColorAttr() const
{
    return LightAPI().GetColorAttr();
}

UsdAttribute
UsdLuxBoundableLightBase::CreateColorAttr(
    VtValue const &defaultValue, bool writeSparsely) const
{
    return LightAPI().CreateColorAttr(
        defaultValue, writeSparsely);
}

UsdAttribute
UsdLuxBoundableLightBase::GetEnableColorTemperatureAttr() const
{
    return LightAPI().GetEnableColorTemperatureAttr();
}

UsdAttribute
UsdLuxBoundableLightBase::CreateEnableColorTemperatureAttr(
    VtValue const &defaultValue, bool writeSparsely) const
{
    return LightAPI().CreateEnableColorTemperatureAttr(
        defaultValue, writeSparsely);
}

UsdAttribute
UsdLuxBoundableLightBase::GetColorTemperatureAttr() const
{
    return LightAPI().GetColorTemperatureAttr();
}

UsdAttribute
UsdLuxBoundableLightBase::CreateColorTemperatureAttr(
    VtValue const &defaultValue, bool writeSparsely) const
{
    return LightAPI().CreateColorTemperatureAttr(
        defaultValue, writeSparsely);
}

UsdRelationship
UsdLuxBoundableLightBase::GetFiltersRel() const
{
    return LightAPI().GetFiltersRel();
}

UsdRelationship
UsdLuxBoundableLightBase::CreateFiltersRel() const
{
    return LightAPI().CreateFiltersRel();
}

UsdLuxLightAPI 
UsdLuxBoundableLightBase::LightAPI() const
{
    return UsdLuxLightAPI(GetPrim());
}

PXR_NAMESPACE_CLOSE_SCOPE
