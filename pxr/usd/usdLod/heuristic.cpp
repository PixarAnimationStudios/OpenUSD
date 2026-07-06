//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/usd/usdLod/heuristic.h"
#include "pxr/usd/usd/schemaRegistry.h"
#include "pxr/usd/usd/typed.h"

#include "pxr/usd/sdf/types.h"
#include "pxr/usd/sdf/assetPath.h"

PXR_NAMESPACE_OPEN_SCOPE

// Register the schema with the TfType system.
TF_REGISTRY_FUNCTION(TfType)
{
    TfType::Define<UsdLodHeuristic,
        TfType::Bases< UsdTyped > >();
    
}

/* virtual */
UsdLodHeuristic::~UsdLodHeuristic()
{
}

/* static */
UsdLodHeuristic
UsdLodHeuristic::Get(const UsdStagePtr &stage, const SdfPath &path)
{
    if (!stage) {
        TF_CODING_ERROR("Invalid stage");
        return UsdLodHeuristic();
    }
    return UsdLodHeuristic(stage->GetPrimAtPath(path));
}


/* virtual */
UsdSchemaKind UsdLodHeuristic::_GetSchemaKind() const
{
    return UsdLodHeuristic::schemaKind;
}

/* static */
const TfType &
UsdLodHeuristic::_GetStaticTfType()
{
    static TfType tfType = TfType::Find<UsdLodHeuristic>();
    return tfType;
}

/* static */
bool 
UsdLodHeuristic::_IsTypedSchema()
{
    static bool isTyped = _GetStaticTfType().IsA<UsdTyped>();
    return isTyped;
}

/* virtual */
const TfType &
UsdLodHeuristic::_GetTfType() const
{
    return _GetStaticTfType();
}

UsdAttribute
UsdLodHeuristic::GetLodDomainAttr() const
{
    return GetPrim().GetAttribute(UsdLodTokens->lodDomain);
}

UsdAttribute
UsdLodHeuristic::CreateLodDomainAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdLodTokens->lodDomain,
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
UsdLodHeuristic::GetSchemaAttributeNames(bool includeInherited)
{
    static TfTokenVector localNames = {
        UsdLodTokens->lodDomain,
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
