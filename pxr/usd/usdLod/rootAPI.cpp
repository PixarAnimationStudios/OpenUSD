//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/usd/usdLod/rootAPI.h"
#include "pxr/usd/usd/schemaRegistry.h"
#include "pxr/usd/usd/typed.h"

#include "pxr/usd/sdf/types.h"
#include "pxr/usd/sdf/assetPath.h"

PXR_NAMESPACE_OPEN_SCOPE

// Register the schema with the TfType system.
TF_REGISTRY_FUNCTION(TfType)
{
    TfType::Define<UsdLodRootAPI,
        TfType::Bases< UsdAPISchemaBase > >();
    
}

/* virtual */
UsdLodRootAPI::~UsdLodRootAPI()
{
}

/* static */
UsdLodRootAPI
UsdLodRootAPI::Get(const UsdStagePtr &stage, const SdfPath &path)
{
    if (!stage) {
        TF_CODING_ERROR("Invalid stage");
        return UsdLodRootAPI();
    }
    return UsdLodRootAPI(stage->GetPrimAtPath(path));
}


/* virtual */
UsdSchemaKind UsdLodRootAPI::_GetSchemaKind() const
{
    return UsdLodRootAPI::schemaKind;
}

/* static */
bool
UsdLodRootAPI::CanApply(
    const UsdPrim &prim, std::string *whyNot)
{
    return prim.CanApplyAPI<UsdLodRootAPI>(whyNot);
}

/* static */
UsdLodRootAPI
UsdLodRootAPI::Apply(const UsdPrim &prim)
{
    if (prim.ApplyAPI<UsdLodRootAPI>()) {
        return UsdLodRootAPI(prim);
    }
    return UsdLodRootAPI();
}

/* static */
const TfType &
UsdLodRootAPI::_GetStaticTfType()
{
    static TfType tfType = TfType::Find<UsdLodRootAPI>();
    return tfType;
}

/* static */
bool 
UsdLodRootAPI::_IsTypedSchema()
{
    static bool isTyped = _GetStaticTfType().IsA<UsdTyped>();
    return isTyped;
}

/* virtual */
const TfType &
UsdLodRootAPI::_GetTfType() const
{
    return _GetStaticTfType();
}

UsdAttribute
UsdLodRootAPI::GetLodDefaultIndexAttr() const
{
    return GetPrim().GetAttribute(UsdLodTokens->lodDefaultIndex);
}

UsdAttribute
UsdLodRootAPI::CreateLodDefaultIndexAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdLodTokens->lodDefaultIndex,
                       SdfValueTypeNames->Int,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

UsdRelationship
UsdLodRootAPI::GetLodHeuristicsRel() const
{
    return GetPrim().GetRelationship(UsdLodTokens->lodHeuristics);
}

UsdRelationship
UsdLodRootAPI::CreateLodHeuristicsRel() const
{
    return GetPrim().CreateRelationship(UsdLodTokens->lodHeuristics,
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
UsdLodRootAPI::GetSchemaAttributeNames(bool includeInherited)
{
    static TfTokenVector localNames = {
        UsdLodTokens->lodDefaultIndex,
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
