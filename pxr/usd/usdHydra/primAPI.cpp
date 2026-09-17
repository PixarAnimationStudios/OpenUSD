//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/usd/usdHydra/primAPI.h"
#include "pxr/usd/usd/schemaRegistry.h"
#include "pxr/usd/usd/typed.h"

#include "pxr/usd/sdf/types.h"
#include "pxr/usd/sdf/assetPath.h"

PXR_NAMESPACE_OPEN_SCOPE

// Register the schema with the TfType system.
TF_REGISTRY_FUNCTION(TfType)
{
    TfType::Define<UsdHydraPrimAPI,
        TfType::Bases< UsdAPISchemaBase > >();
    
}

/* virtual */
UsdHydraPrimAPI::~UsdHydraPrimAPI()
{
}

/* static */
UsdHydraPrimAPI
UsdHydraPrimAPI::Get(const UsdStagePtr &stage, const SdfPath &path)
{
    if (!stage) {
        TF_CODING_ERROR("Invalid stage");
        return UsdHydraPrimAPI();
    }
    return UsdHydraPrimAPI(stage->GetPrimAtPath(path));
}


/* virtual */
UsdSchemaKind UsdHydraPrimAPI::_GetSchemaKind() const
{
    return UsdHydraPrimAPI::schemaKind;
}

/* static */
bool
UsdHydraPrimAPI::CanApply(
    const UsdPrim &prim, std::string *whyNot)
{
    return prim.CanApplyAPI<UsdHydraPrimAPI>(whyNot);
}

/* static */
UsdHydraPrimAPI
UsdHydraPrimAPI::Apply(const UsdPrim &prim)
{
    if (prim.ApplyAPI<UsdHydraPrimAPI>()) {
        return UsdHydraPrimAPI(prim);
    }
    return UsdHydraPrimAPI();
}

/* static */
const TfType &
UsdHydraPrimAPI::_GetStaticTfType()
{
    static TfType tfType = TfType::Find<UsdHydraPrimAPI>();
    return tfType;
}

/* static */
bool 
UsdHydraPrimAPI::_IsTypedSchema()
{
    static bool isTyped = _GetStaticTfType().IsA<UsdTyped>();
    return isTyped;
}

/* virtual */
const TfType &
UsdHydraPrimAPI::_GetTfType() const
{
    return _GetStaticTfType();
}

UsdAttribute
UsdHydraPrimAPI::GetHydraExpandInstancesAttr() const
{
    return GetPrim().GetAttribute(UsdHydraTokens->hydraExpandInstances);
}

UsdAttribute
UsdHydraPrimAPI::CreateHydraExpandInstancesAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdHydraTokens->hydraExpandInstances,
                       SdfValueTypeNames->Bool,
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
UsdHydraPrimAPI::GetSchemaAttributeNames(bool includeInherited)
{
    static TfTokenVector localNames = {
        UsdHydraTokens->hydraExpandInstances,
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

PXR_NAMESPACE_OPEN_SCOPE

bool
UsdHydraPrimAPI::ShouldExpandInstancesForPrim(
    UsdPrim const& prim,
    bool includeAncestors)
{
    for (UsdPrim p = prim; p && !p.IsPseudoRoot(); p = p.GetParent()) {
        if (const UsdAttribute attr =
            UsdHydraPrimAPI(p).GetHydraExpandInstancesAttr()) {
            bool value = false;
            if (attr.Get(&value) && value) {
                return true;
            }
            // If the value is false, we must continue walking up,
            // since there may be an ancestor that requests expansion
            // of its descendants.
            if (!includeAncestors) {
                return false;
            }
        }
    }
    return false;
}

PXR_NAMESPACE_CLOSE_SCOPE
