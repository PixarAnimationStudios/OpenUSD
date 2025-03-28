//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/usd/usdRender/var.h"
#include "pxr/usd/usd/schemaRegistry.h"
#include "pxr/usd/usd/typed.h"

#include "pxr/usd/sdf/types.h"
#include "pxr/usd/sdf/assetPath.h"

PXR_NAMESPACE_OPEN_SCOPE

// Register the schema with the TfType system.
TF_REGISTRY_FUNCTION(TfType)
{
    TfType::Define<UsdRenderVar,
        TfType::Bases< UsdTyped > >();
    
    // Register the usd prim typename as an alias under UsdSchemaBase. This
    // enables one to call
    // TfType::Find<UsdSchemaBase>().FindDerivedByName("RenderVar")
    // to find TfType<UsdRenderVar>, which is how IsA queries are
    // answered.
    TfType::AddAlias<UsdSchemaBase, UsdRenderVar>("RenderVar");
}

/* virtual */
UsdRenderVar::~UsdRenderVar()
{
}

/* static */
UsdRenderVar
UsdRenderVar::Get(const UsdStagePtr &stage, const SdfPath &path)
{
    if (!stage) {
        TF_CODING_ERROR("Invalid stage");
        return UsdRenderVar();
    }
    return UsdRenderVar(stage->GetPrimAtPath(path));
}

/* static */
UsdRenderVar
UsdRenderVar::Define(
    const UsdStagePtr &stage, const SdfPath &path)
{
    static TfToken usdPrimTypeName("RenderVar");
    if (!stage) {
        TF_CODING_ERROR("Invalid stage");
        return UsdRenderVar();
    }
    return UsdRenderVar(
        stage->DefinePrim(path, usdPrimTypeName));
}

/* virtual */
UsdSchemaKind UsdRenderVar::_GetSchemaKind() const
{
    return UsdRenderVar::schemaKind;
}

/* static */
const TfType &
UsdRenderVar::_GetStaticTfType()
{
    static TfType tfType = TfType::Find<UsdRenderVar>();
    return tfType;
}

/* static */
bool 
UsdRenderVar::_IsTypedSchema()
{
    static bool isTyped = _GetStaticTfType().IsA<UsdTyped>();
    return isTyped;
}

/* virtual */
const TfType &
UsdRenderVar::_GetTfType() const
{
    return _GetStaticTfType();
}

UsdAttribute
UsdRenderVar::GetDataTypeAttr() const
{
    return GetPrim().GetAttribute(UsdRenderTokens->dataType);
}

UsdAttribute
UsdRenderVar::CreateDataTypeAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdRenderTokens->dataType,
                       SdfValueTypeNames->Token,
                       /* custom = */ false,
                       SdfVariabilityUniform,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdRenderVar::GetSourceNameAttr() const
{
    return GetPrim().GetAttribute(UsdRenderTokens->sourceName);
}

UsdAttribute
UsdRenderVar::CreateSourceNameAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdRenderTokens->sourceName,
                       SdfValueTypeNames->String,
                       /* custom = */ false,
                       SdfVariabilityUniform,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdRenderVar::GetSourceTypeAttr() const
{
    return GetPrim().GetAttribute(UsdRenderTokens->sourceType);
}

UsdAttribute
UsdRenderVar::CreateSourceTypeAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdRenderTokens->sourceType,
                       SdfValueTypeNames->Token,
                       /* custom = */ false,
                       SdfVariabilityUniform,
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
UsdRenderVar::GetSchemaAttributeNames(bool includeInherited)
{
    static TfTokenVector localNames = {
        UsdRenderTokens->dataType,
        UsdRenderTokens->sourceName,
        UsdRenderTokens->sourceType,
    };
    static TfTokenVector allNames =
        _ConcatenateAttributeNames(
            UsdTyped::GetSchemaAttributeNames(true),
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
