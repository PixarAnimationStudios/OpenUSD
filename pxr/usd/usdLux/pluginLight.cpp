//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/usd/usdLux/pluginLight.h"
#include "pxr/usd/usd/schemaRegistry.h"
#include "pxr/usd/usd/typed.h"

#include "pxr/usd/sdf/types.h"
#include "pxr/usd/sdf/assetPath.h"

PXR_NAMESPACE_OPEN_SCOPE

// Register the schema with the TfType system.
TF_REGISTRY_FUNCTION(TfType)
{
    TfType::Define<UsdLuxPluginLight,
        TfType::Bases< UsdGeomXformable > >();
    
    // Register the usd prim typename as an alias under UsdSchemaBase. This
    // enables one to call
    // TfType::Find<UsdSchemaBase>().FindDerivedByName("PluginLight")
    // to find TfType<UsdLuxPluginLight>, which is how IsA queries are
    // answered.
    TfType::AddAlias<UsdSchemaBase, UsdLuxPluginLight>("PluginLight");
}

/* virtual */
UsdLuxPluginLight::~UsdLuxPluginLight()
{
}

/* static */
UsdLuxPluginLight
UsdLuxPluginLight::Get(const UsdStagePtr &stage, const SdfPath &path)
{
    if (!stage) {
        TF_CODING_ERROR("Invalid stage");
        return UsdLuxPluginLight();
    }
    return UsdLuxPluginLight(stage->GetPrimAtPath(path));
}

/* static */
UsdLuxPluginLight
UsdLuxPluginLight::Define(
    const UsdStagePtr &stage, const SdfPath &path)
{
    static TfToken usdPrimTypeName("PluginLight");
    if (!stage) {
        TF_CODING_ERROR("Invalid stage");
        return UsdLuxPluginLight();
    }
    return UsdLuxPluginLight(
        stage->DefinePrim(path, usdPrimTypeName));
}

/* virtual */
UsdSchemaKind UsdLuxPluginLight::_GetSchemaKind() const
{
    return UsdLuxPluginLight::schemaKind;
}

/* static */
const TfType &
UsdLuxPluginLight::_GetStaticTfType()
{
    static TfType tfType = TfType::Find<UsdLuxPluginLight>();
    return tfType;
}

/* static */
bool 
UsdLuxPluginLight::_IsTypedSchema()
{
    static bool isTyped = _GetStaticTfType().IsA<UsdTyped>();
    return isTyped;
}

/* virtual */
const TfType &
UsdLuxPluginLight::_GetTfType() const
{
    return _GetStaticTfType();
}

/*static*/
const TfTokenVector&
UsdLuxPluginLight::GetSchemaAttributeNames(bool includeInherited)
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

PXR_NAMESPACE_OPEN_SCOPE

UsdShadeNodeDefAPI 
UsdLuxPluginLight::GetNodeDefAPI() const
{
    return UsdShadeNodeDefAPI(GetPrim());
}

PXR_NAMESPACE_CLOSE_SCOPE
