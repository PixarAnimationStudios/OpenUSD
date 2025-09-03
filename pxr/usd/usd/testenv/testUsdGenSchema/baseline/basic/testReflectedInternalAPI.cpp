//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/usd/usdContrived/testReflectedInternalAPI.h"
#include "pxr/usd/usd/schemaRegistry.h"
#include "pxr/usd/usd/typed.h"

#include "pxr/usd/sdf/types.h"
#include "pxr/usd/sdf/assetPath.h"

PXR_NAMESPACE_OPEN_SCOPE

// Register the schema with the TfType system.
TF_REGISTRY_FUNCTION(TfType)
{
    TfType::Define<UsdContrivedTestReflectedInternalAPI,
        TfType::Bases< UsdAPISchemaBase > >();
    
}

/* virtual */
UsdContrivedTestReflectedInternalAPI::~UsdContrivedTestReflectedInternalAPI()
{
}

/* static */
UsdContrivedTestReflectedInternalAPI
UsdContrivedTestReflectedInternalAPI::Get(const UsdStagePtr &stage, const SdfPath &path)
{
    if (!stage) {
        TF_CODING_ERROR("Invalid stage");
        return UsdContrivedTestReflectedInternalAPI();
    }
    return UsdContrivedTestReflectedInternalAPI(stage->GetPrimAtPath(path));
}


/* virtual */
UsdSchemaKind UsdContrivedTestReflectedInternalAPI::_GetSchemaKind() const
{
    return UsdContrivedTestReflectedInternalAPI::schemaKind;
}

/* static */
bool
UsdContrivedTestReflectedInternalAPI::CanApply(
    const UsdPrim &prim, std::string *whyNot)
{
    return prim.CanApplyAPI<UsdContrivedTestReflectedInternalAPI>(whyNot);
}

/* static */
UsdContrivedTestReflectedInternalAPI
UsdContrivedTestReflectedInternalAPI::Apply(const UsdPrim &prim)
{
    if (prim.ApplyAPI<UsdContrivedTestReflectedInternalAPI>()) {
        return UsdContrivedTestReflectedInternalAPI(prim);
    }
    return UsdContrivedTestReflectedInternalAPI();
}

/* static */
const TfType &
UsdContrivedTestReflectedInternalAPI::_GetStaticTfType()
{
    static TfType tfType = TfType::Find<UsdContrivedTestReflectedInternalAPI>();
    return tfType;
}

/* static */
bool 
UsdContrivedTestReflectedInternalAPI::_IsTypedSchema()
{
    static bool isTyped = _GetStaticTfType().IsA<UsdTyped>();
    return isTyped;
}

/* virtual */
const TfType &
UsdContrivedTestReflectedInternalAPI::_GetTfType() const
{
    return _GetStaticTfType();
}

UsdAttribute
UsdContrivedTestReflectedInternalAPI::GetTestAttrInternalAttr() const
{
    return GetPrim().GetAttribute(UsdContrivedTokens->testAttrInternal);
}

UsdAttribute
UsdContrivedTestReflectedInternalAPI::CreateTestAttrInternalAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdContrivedTokens->testAttrInternal,
                       SdfValueTypeNames->Int,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdContrivedTestReflectedInternalAPI::GetTestAttrDuplicateAttr() const
{
    return GetPrim().GetAttribute(UsdContrivedTokens->testAttrDuplicate);
}

UsdAttribute
UsdContrivedTestReflectedInternalAPI::CreateTestAttrDuplicateAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdContrivedTokens->testAttrDuplicate,
                       SdfValueTypeNames->Int,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

UsdRelationship
UsdContrivedTestReflectedInternalAPI::GetTestRelInternalRel() const
{
    return GetPrim().GetRelationship(UsdContrivedTokens->testRelInternal);
}

UsdRelationship
UsdContrivedTestReflectedInternalAPI::CreateTestRelInternalRel() const
{
    return GetPrim().CreateRelationship(UsdContrivedTokens->testRelInternal,
                       /* custom = */ false);
}

UsdRelationship
UsdContrivedTestReflectedInternalAPI::GetTestRelDuplicateRel() const
{
    return GetPrim().GetRelationship(UsdContrivedTokens->testRelDuplicate);
}

UsdRelationship
UsdContrivedTestReflectedInternalAPI::CreateTestRelDuplicateRel() const
{
    return GetPrim().CreateRelationship(UsdContrivedTokens->testRelDuplicate,
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
UsdContrivedTestReflectedInternalAPI::GetSchemaAttributeNames(bool includeInherited)
{
    static TfTokenVector localNames = {
        UsdContrivedTokens->testAttrInternal,
        UsdContrivedTokens->testAttrDuplicate,
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
