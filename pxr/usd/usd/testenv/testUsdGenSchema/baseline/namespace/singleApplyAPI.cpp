//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/usd/usdContrived/singleApplyAPI.h"
#include "pxr/usd/usd/schemaRegistry.h"
#include "pxr/usd/usd/typed.h"

#include "pxr/usd/sdf/types.h"
#include "pxr/usd/sdf/assetPath.h"

namespace foo {

// Register the schema with the TfType system.
TF_REGISTRY_FUNCTION(TfType)
{
    TfType::Define<UsdContrivedSingleApplyAPI,
        TfType::Bases< UsdAPISchemaBase > >();
    
}

/* virtual */
UsdContrivedSingleApplyAPI::~UsdContrivedSingleApplyAPI()
{
}

/* static */
UsdContrivedSingleApplyAPI
UsdContrivedSingleApplyAPI::Get(const UsdStagePtr &stage, const SdfPath &path)
{
    if (!stage) {
        TF_CODING_ERROR("Invalid stage");
        return UsdContrivedSingleApplyAPI();
    }
    return UsdContrivedSingleApplyAPI(stage->GetPrimAtPath(path));
}


/* virtual */
UsdSchemaKind UsdContrivedSingleApplyAPI::_GetSchemaKind() const
{
    return UsdContrivedSingleApplyAPI::schemaKind;
}

/* static */
bool
UsdContrivedSingleApplyAPI::CanApply(
    const UsdPrim &prim, std::string *whyNot)
{
    return prim.CanApplyAPI<UsdContrivedSingleApplyAPI>(whyNot);
}

/* static */
UsdContrivedSingleApplyAPI
UsdContrivedSingleApplyAPI::Apply(const UsdPrim &prim)
{
    if (prim.ApplyAPI<UsdContrivedSingleApplyAPI>()) {
        return UsdContrivedSingleApplyAPI(prim);
    }
    return UsdContrivedSingleApplyAPI();
}

/* static */
const TfType &
UsdContrivedSingleApplyAPI::_GetStaticTfType()
{
    static TfType tfType = TfType::Find<UsdContrivedSingleApplyAPI>();
    return tfType;
}

/* static */
bool 
UsdContrivedSingleApplyAPI::_IsTypedSchema()
{
    static bool isTyped = _GetStaticTfType().IsA<UsdTyped>();
    return isTyped;
}

/* virtual */
const TfType &
UsdContrivedSingleApplyAPI::_GetTfType() const
{
    return _GetStaticTfType();
}

UsdAttribute
UsdContrivedSingleApplyAPI::GetTestAttrOneAttr() const
{
    return GetPrim().GetAttribute(UsdContrivedTokens->testAttrOne);
}

UsdAttribute
UsdContrivedSingleApplyAPI::CreateTestAttrOneAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdContrivedTokens->testAttrOne,
                       SdfValueTypeNames->Int,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdContrivedSingleApplyAPI::GetTestAttrTwoAttr() const
{
    return GetPrim().GetAttribute(UsdContrivedTokens->testAttrTwo);
}

UsdAttribute
UsdContrivedSingleApplyAPI::CreateTestAttrTwoAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdContrivedTokens->testAttrTwo,
                       SdfValueTypeNames->Double,
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
UsdContrivedSingleApplyAPI::GetSchemaAttributeNames(bool includeInherited)
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

}

// ===================================================================== //
// Feel free to add custom code below this line. It will be preserved by
// the code generator.
//
// Just remember to wrap code in the appropriate delimiters:
// 'namespace foo {', '}'.
// ===================================================================== //
// --(BEGIN CUSTOM CODE)--
