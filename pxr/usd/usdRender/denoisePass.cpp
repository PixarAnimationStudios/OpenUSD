//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/usd/usdRender/denoisePass.h"
#include "pxr/usd/usd/schemaRegistry.h"
#include "pxr/usd/usd/typed.h"

#include "pxr/usd/sdf/types.h"
#include "pxr/usd/sdf/assetPath.h"

PXR_NAMESPACE_OPEN_SCOPE

// Register the schema with the TfType system.
TF_REGISTRY_FUNCTION(TfType)
{
    TfType::Define<UsdRenderDenoisePass,
        TfType::Bases< UsdTyped > >();
    
    // Register the usd prim typename as an alias under UsdSchemaBase. This
    // enables one to call
    // TfType::Find<UsdSchemaBase>().FindDerivedByName("RenderDenoisePass")
    // to find TfType<UsdRenderDenoisePass>, which is how IsA queries are
    // answered.
    TfType::AddAlias<UsdSchemaBase, UsdRenderDenoisePass>("RenderDenoisePass");
}

/* virtual */
UsdRenderDenoisePass::~UsdRenderDenoisePass()
{
}

/* static */
UsdRenderDenoisePass
UsdRenderDenoisePass::Get(const UsdStagePtr &stage, const SdfPath &path)
{
    if (!stage) {
        TF_CODING_ERROR("Invalid stage");
        return UsdRenderDenoisePass();
    }
    return UsdRenderDenoisePass(stage->GetPrimAtPath(path));
}

/* static */
UsdRenderDenoisePass
UsdRenderDenoisePass::Define(
    const UsdStagePtr &stage, const SdfPath &path)
{
    static TfToken usdPrimTypeName("RenderDenoisePass");
    if (!stage) {
        TF_CODING_ERROR("Invalid stage");
        return UsdRenderDenoisePass();
    }
    return UsdRenderDenoisePass(
        stage->DefinePrim(path, usdPrimTypeName));
}

/* virtual */
UsdSchemaKind UsdRenderDenoisePass::_GetSchemaKind() const
{
    return UsdRenderDenoisePass::schemaKind;
}

/* static */
const TfType &
UsdRenderDenoisePass::_GetStaticTfType()
{
    static TfType tfType = TfType::Find<UsdRenderDenoisePass>();
    return tfType;
}

/* static */
bool 
UsdRenderDenoisePass::_IsTypedSchema()
{
    static bool isTyped = _GetStaticTfType().IsA<UsdTyped>();
    return isTyped;
}

/* virtual */
const TfType &
UsdRenderDenoisePass::_GetTfType() const
{
    return _GetStaticTfType();
}

/*static*/
const TfTokenVector&
UsdRenderDenoisePass::GetSchemaAttributeNames(bool includeInherited)
{
    static TfTokenVector localNames;
    static TfTokenVector allNames =
        UsdTyped::GetSchemaAttributeNames(true);

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
