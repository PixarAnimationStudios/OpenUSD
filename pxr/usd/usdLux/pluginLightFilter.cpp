//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/usd/usdLux/pluginLightFilter.h"
#include "pxr/usd/usd/schemaRegistry.h"
#include "pxr/usd/usd/typed.h"

#include "pxr/usd/sdf/types.h"
#include "pxr/usd/sdf/assetPath.h"

PXR_NAMESPACE_OPEN_SCOPE

// Register the schema with the TfType system.
TF_REGISTRY_FUNCTION(TfType)
{
    TfType::Define<UsdLuxPluginLightFilter,
        TfType::Bases< UsdLuxLightFilter > >();
    
    // Register the usd prim typename as an alias under UsdSchemaBase. This
    // enables one to call
    // TfType::Find<UsdSchemaBase>().FindDerivedByName("PluginLightFilter")
    // to find TfType<UsdLuxPluginLightFilter>, which is how IsA queries are
    // answered.
    TfType::AddAlias<UsdSchemaBase, UsdLuxPluginLightFilter>("PluginLightFilter");
}

/* virtual */
UsdLuxPluginLightFilter::~UsdLuxPluginLightFilter()
{
}

/* static */
UsdLuxPluginLightFilter
UsdLuxPluginLightFilter::Get(const UsdStagePtr &stage, const SdfPath &path)
{
    if (!stage) {
        TF_CODING_ERROR("Invalid stage");
        return UsdLuxPluginLightFilter();
    }
    return UsdLuxPluginLightFilter(stage->GetPrimAtPath(path));
}

/* static */
UsdLuxPluginLightFilter
UsdLuxPluginLightFilter::Define(
    const UsdStagePtr &stage, const SdfPath &path)
{
    static TfToken usdPrimTypeName("PluginLightFilter");
    if (!stage) {
        TF_CODING_ERROR("Invalid stage");
        return UsdLuxPluginLightFilter();
    }
    return UsdLuxPluginLightFilter(
        stage->DefinePrim(path, usdPrimTypeName));
}

/* virtual */
UsdSchemaKind UsdLuxPluginLightFilter::_GetSchemaKind() const
{
    return UsdLuxPluginLightFilter::schemaKind;
}

/* static */
const TfType &
UsdLuxPluginLightFilter::_GetStaticTfType()
{
    static TfType tfType = TfType::Find<UsdLuxPluginLightFilter>();
    return tfType;
}

/* static */
bool 
UsdLuxPluginLightFilter::_IsTypedSchema()
{
    static bool isTyped = _GetStaticTfType().IsA<UsdTyped>();
    return isTyped;
}

/* virtual */
const TfType &
UsdLuxPluginLightFilter::_GetTfType() const
{
    return _GetStaticTfType();
}

/*static*/
const TfTokenVector&
UsdLuxPluginLightFilter::GetSchemaAttributeNames(bool includeInherited)
{
    static TfTokenVector localNames;
    static TfTokenVector allNames =
        UsdLuxLightFilter::GetSchemaAttributeNames(true);

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
UsdLuxPluginLightFilter::GetNodeDefAPI() const
{
    return UsdShadeNodeDefAPI(GetPrim());
}

PXR_NAMESPACE_CLOSE_SCOPE
