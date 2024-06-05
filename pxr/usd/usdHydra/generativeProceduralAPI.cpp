//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/usd/usdHydra/generativeProceduralAPI.h"
#include "pxr/usd/usd/schemaRegistry.h"
#include "pxr/usd/usd/typed.h"

#include "pxr/usd/sdf/types.h"
#include "pxr/usd/sdf/assetPath.h"

PXR_NAMESPACE_OPEN_SCOPE

// Register the schema with the TfType system.
TF_REGISTRY_FUNCTION(TfType)
{
    TfType::Define<UsdHydraGenerativeProceduralAPI,
        TfType::Bases< UsdAPISchemaBase > >();
    
}

/* virtual */
UsdHydraGenerativeProceduralAPI::~UsdHydraGenerativeProceduralAPI()
{
}

/* static */
UsdHydraGenerativeProceduralAPI
UsdHydraGenerativeProceduralAPI::Get(const UsdStagePtr &stage, const SdfPath &path)
{
    if (!stage) {
        TF_CODING_ERROR("Invalid stage");
        return UsdHydraGenerativeProceduralAPI();
    }
    return UsdHydraGenerativeProceduralAPI(stage->GetPrimAtPath(path));
}


/* virtual */
UsdSchemaKind UsdHydraGenerativeProceduralAPI::_GetSchemaKind() const
{
    return UsdHydraGenerativeProceduralAPI::schemaKind;
}

/* static */
bool
UsdHydraGenerativeProceduralAPI::CanApply(
    const UsdPrim &prim, std::string *whyNot)
{
    return prim.CanApplyAPI<UsdHydraGenerativeProceduralAPI>(whyNot);
}

/* static */
UsdHydraGenerativeProceduralAPI
UsdHydraGenerativeProceduralAPI::Apply(const UsdPrim &prim)
{
    if (prim.ApplyAPI<UsdHydraGenerativeProceduralAPI>()) {
        return UsdHydraGenerativeProceduralAPI(prim);
    }
    return UsdHydraGenerativeProceduralAPI();
}

/* static */
const TfType &
UsdHydraGenerativeProceduralAPI::_GetStaticTfType()
{
    static TfType tfType = TfType::Find<UsdHydraGenerativeProceduralAPI>();
    return tfType;
}

/* static */
bool 
UsdHydraGenerativeProceduralAPI::_IsTypedSchema()
{
    static bool isTyped = _GetStaticTfType().IsA<UsdTyped>();
    return isTyped;
}

/* virtual */
const TfType &
UsdHydraGenerativeProceduralAPI::_GetTfType() const
{
    return _GetStaticTfType();
}

UsdAttribute
UsdHydraGenerativeProceduralAPI::GetProceduralTypeAttr() const
{
    return GetPrim().GetAttribute(UsdHydraTokens->primvarsHdGpProceduralType);
}

UsdAttribute
UsdHydraGenerativeProceduralAPI::CreateProceduralTypeAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdHydraTokens->primvarsHdGpProceduralType,
                       SdfValueTypeNames->Token,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdHydraGenerativeProceduralAPI::GetProceduralSystemAttr() const
{
    return GetPrim().GetAttribute(UsdHydraTokens->proceduralSystem);
}

UsdAttribute
UsdHydraGenerativeProceduralAPI::CreateProceduralSystemAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdHydraTokens->proceduralSystem,
                       SdfValueTypeNames->Token,
                       /* custom = */ false,
                       SdfVariabilityVarying,
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
UsdHydraGenerativeProceduralAPI::GetSchemaAttributeNames(bool includeInherited)
{
    static TfTokenVector localNames = {
        UsdHydraTokens->primvarsHdGpProceduralType,
        UsdHydraTokens->proceduralSystem,
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
