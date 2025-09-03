//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/usd/usdContrived/testPropertyOrderSingleApplyAPI.h"
#include "pxr/usd/usd/schemaRegistry.h"
#include "pxr/usd/usd/typed.h"

#include "pxr/usd/sdf/types.h"
#include "pxr/usd/sdf/assetPath.h"

PXR_NAMESPACE_OPEN_SCOPE

// Register the schema with the TfType system.
TF_REGISTRY_FUNCTION(TfType)
{
    TfType::Define<UsdContrivedTestPropertyOrderSingleApplyAPI,
        TfType::Bases< UsdAPISchemaBase > >();
    
}

/* virtual */
UsdContrivedTestPropertyOrderSingleApplyAPI::~UsdContrivedTestPropertyOrderSingleApplyAPI()
{
}

/* static */
UsdContrivedTestPropertyOrderSingleApplyAPI
UsdContrivedTestPropertyOrderSingleApplyAPI::Get(const UsdStagePtr &stage, const SdfPath &path)
{
    if (!stage) {
        TF_CODING_ERROR("Invalid stage");
        return UsdContrivedTestPropertyOrderSingleApplyAPI();
    }
    return UsdContrivedTestPropertyOrderSingleApplyAPI(stage->GetPrimAtPath(path));
}


/* virtual */
UsdSchemaKind UsdContrivedTestPropertyOrderSingleApplyAPI::_GetSchemaKind() const
{
    return UsdContrivedTestPropertyOrderSingleApplyAPI::schemaKind;
}

/* static */
bool
UsdContrivedTestPropertyOrderSingleApplyAPI::CanApply(
    const UsdPrim &prim, std::string *whyNot)
{
    return prim.CanApplyAPI<UsdContrivedTestPropertyOrderSingleApplyAPI>(whyNot);
}

/* static */
UsdContrivedTestPropertyOrderSingleApplyAPI
UsdContrivedTestPropertyOrderSingleApplyAPI::Apply(const UsdPrim &prim)
{
    if (prim.ApplyAPI<UsdContrivedTestPropertyOrderSingleApplyAPI>()) {
        return UsdContrivedTestPropertyOrderSingleApplyAPI(prim);
    }
    return UsdContrivedTestPropertyOrderSingleApplyAPI();
}

/* static */
const TfType &
UsdContrivedTestPropertyOrderSingleApplyAPI::_GetStaticTfType()
{
    static TfType tfType = TfType::Find<UsdContrivedTestPropertyOrderSingleApplyAPI>();
    return tfType;
}

/* static */
bool 
UsdContrivedTestPropertyOrderSingleApplyAPI::_IsTypedSchema()
{
    static bool isTyped = _GetStaticTfType().IsA<UsdTyped>();
    return isTyped;
}

/* virtual */
const TfType &
UsdContrivedTestPropertyOrderSingleApplyAPI::_GetTfType() const
{
    return _GetStaticTfType();
}

UsdAttribute
UsdContrivedTestPropertyOrderSingleApplyAPI::GetTestAttrOneAttr() const
{
    return GetPrim().GetAttribute(UsdContrivedTokens->testAttrOne);
}

UsdAttribute
UsdContrivedTestPropertyOrderSingleApplyAPI::CreateTestAttrOneAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdContrivedTokens->testAttrOne,
                       SdfValueTypeNames->Int,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdContrivedTestPropertyOrderSingleApplyAPI::GetTestAttrTwoAttr() const
{
    return GetPrim().GetAttribute(UsdContrivedTokens->testAttrTwo);
}

UsdAttribute
UsdContrivedTestPropertyOrderSingleApplyAPI::CreateTestAttrTwoAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdContrivedTokens->testAttrTwo,
                       SdfValueTypeNames->Int,
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
UsdContrivedTestPropertyOrderSingleApplyAPI::GetSchemaAttributeNames(bool includeInherited)
{
    static TfTokenVector localNames = {
        UsdContrivedTokens->testAttrOne,
        UsdContrivedTokens->testAttrTwo,
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
