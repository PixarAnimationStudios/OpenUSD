//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "./simple.h"
#include "pxr/usd/usd/schemaRegistry.h"
#include "pxr/usd/usd/typed.h"

#include "pxr/usd/sdf/types.h"
#include "pxr/usd/sdf/assetPath.h"

PXR_NAMESPACE_OPEN_SCOPE

// Register the schema with the TfType system.
TF_REGISTRY_FUNCTION(TfType)
{
    TfType::Define<UsdSchemaExamplesSimple,
        TfType::Bases< UsdTyped > >();
    
}

/* virtual */
UsdSchemaExamplesSimple::~UsdSchemaExamplesSimple()
{
}

/* static */
UsdSchemaExamplesSimple
UsdSchemaExamplesSimple::Get(const UsdStagePtr &stage, const SdfPath &path)
{
    if (!stage) {
        TF_CODING_ERROR("Invalid stage");
        return UsdSchemaExamplesSimple();
    }
    return UsdSchemaExamplesSimple(stage->GetPrimAtPath(path));
}


/* virtual */
UsdSchemaKind UsdSchemaExamplesSimple::_GetSchemaKind() const
{
    return UsdSchemaExamplesSimple::schemaKind;
}

/* static */
const TfType &
UsdSchemaExamplesSimple::_GetStaticTfType()
{
    static TfType tfType = TfType::Find<UsdSchemaExamplesSimple>();
    return tfType;
}

/* static */
bool 
UsdSchemaExamplesSimple::_IsTypedSchema()
{
    static bool isTyped = _GetStaticTfType().IsA<UsdTyped>();
    return isTyped;
}

/* virtual */
const TfType &
UsdSchemaExamplesSimple::_GetTfType() const
{
    return _GetStaticTfType();
}

UsdAttribute
UsdSchemaExamplesSimple::GetIntAttrAttr() const
{
    return GetPrim().GetAttribute(UsdSchemaExamplesTokens->intAttr);
}

UsdAttribute
UsdSchemaExamplesSimple::CreateIntAttrAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdSchemaExamplesTokens->intAttr,
                       SdfValueTypeNames->Int,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

UsdRelationship
UsdSchemaExamplesSimple::GetTargetRel() const
{
    return GetPrim().GetRelationship(UsdSchemaExamplesTokens->target);
}

UsdRelationship
UsdSchemaExamplesSimple::CreateTargetRel() const
{
    return GetPrim().CreateRelationship(UsdSchemaExamplesTokens->target,
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
UsdSchemaExamplesSimple::GetSchemaAttributeNames(bool includeInherited)
{
    static TfTokenVector localNames = {
        UsdSchemaExamplesTokens->intAttr,
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
