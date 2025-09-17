//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/usd/usdContrived/testReflectedAPIBase.h"
#include "pxr/usd/usd/schemaRegistry.h"
#include "pxr/usd/usd/typed.h"

#include "pxr/usd/sdf/types.h"
#include "pxr/usd/sdf/assetPath.h"

PXR_NAMESPACE_OPEN_SCOPE

// Register the schema with the TfType system.
TF_REGISTRY_FUNCTION(TfType)
{
    TfType::Define<UsdContrivedTestReflectedAPIBase,
        TfType::Bases< UsdTyped > >();
    
}

/* virtual */
UsdContrivedTestReflectedAPIBase::~UsdContrivedTestReflectedAPIBase()
{
}

/* static */
UsdContrivedTestReflectedAPIBase
UsdContrivedTestReflectedAPIBase::Get(const UsdStagePtr &stage, const SdfPath &path)
{
    if (!stage) {
        TF_CODING_ERROR("Invalid stage");
        return UsdContrivedTestReflectedAPIBase();
    }
    return UsdContrivedTestReflectedAPIBase(stage->GetPrimAtPath(path));
}


/* virtual */
UsdSchemaKind UsdContrivedTestReflectedAPIBase::_GetSchemaKind() const
{
    return UsdContrivedTestReflectedAPIBase::schemaKind;
}

/* static */
const TfType &
UsdContrivedTestReflectedAPIBase::_GetStaticTfType()
{
    static TfType tfType = TfType::Find<UsdContrivedTestReflectedAPIBase>();
    return tfType;
}

/* static */
bool 
UsdContrivedTestReflectedAPIBase::_IsTypedSchema()
{
    static bool isTyped = _GetStaticTfType().IsA<UsdTyped>();
    return isTyped;
}

/* virtual */
const TfType &
UsdContrivedTestReflectedAPIBase::_GetTfType() const
{
    return _GetStaticTfType();
}

/*static*/
const TfTokenVector&
UsdContrivedTestReflectedAPIBase::GetSchemaAttributeNames(bool includeInherited)
{
    static TfTokenVector localNames;
    static TfTokenVector allNames =
        UsdTyped::GetSchemaAttributeNames(true);

    if (includeInherited)
        return allNames;
    else
        return localNames;
}

UsdContrivedTestReflectedInternalAPI
UsdContrivedTestReflectedAPIBase::TestReflectedInternalAPI() const
{
    return UsdContrivedTestReflectedInternalAPI(GetPrim());
}

UsdAttribute
UsdContrivedTestReflectedAPIBase::GetTestAttrInternalAttr() const
{
    return TestReflectedInternalAPI().GetTestAttrInternalAttr();
}

UsdAttribute
UsdContrivedTestReflectedAPIBase::CreateTestAttrInternalAttr(
    VtValue const &defaultValue, bool writeSparsely) const
{
    return TestReflectedInternalAPI().CreateTestAttrInternalAttr(
        defaultValue, writeSparsely);
}

UsdAttribute
UsdContrivedTestReflectedAPIBase::GetTestAttrDuplicateAttr() const
{
    return TestReflectedInternalAPI().GetTestAttrDuplicateAttr();
}

UsdAttribute
UsdContrivedTestReflectedAPIBase::CreateTestAttrDuplicateAttr(
    VtValue const &defaultValue, bool writeSparsely) const
{
    return TestReflectedInternalAPI().CreateTestAttrDuplicateAttr(
        defaultValue, writeSparsely);
}

UsdRelationship
UsdContrivedTestReflectedAPIBase::GetTestRelInternalRel() const
{
    return TestReflectedInternalAPI().GetTestRelInternalRel();
}

UsdRelationship
UsdContrivedTestReflectedAPIBase::CreateTestRelInternalRel() const
{
    return TestReflectedInternalAPI().CreateTestRelInternalRel();
}

UsdRelationship
UsdContrivedTestReflectedAPIBase::GetTestRelDuplicateRel() const
{
    return TestReflectedInternalAPI().GetTestRelDuplicateRel();
}

UsdRelationship
UsdContrivedTestReflectedAPIBase::CreateTestRelDuplicateRel() const
{
    return TestReflectedInternalAPI().CreateTestRelDuplicateRel();
}

UsdTestReflectedExternalAPI
UsdContrivedTestReflectedAPIBase::TestReflectedExternalAPI() const
{
    return UsdTestReflectedExternalAPI(GetPrim());
}

UsdAttribute
UsdContrivedTestReflectedAPIBase::GetTestAttrExternalAttr() const
{
    return TestReflectedExternalAPI().GetTestAttrExternalAttr();
}

UsdAttribute
UsdContrivedTestReflectedAPIBase::CreateTestAttrExternalAttr(
    VtValue const &defaultValue, bool writeSparsely) const
{
    return TestReflectedExternalAPI().CreateTestAttrExternalAttr(
        defaultValue, writeSparsely);
}

UsdRelationship
UsdContrivedTestReflectedAPIBase::GetTestRelExternalRel() const
{
    return TestReflectedExternalAPI().GetTestRelExternalRel();
}

UsdRelationship
UsdContrivedTestReflectedAPIBase::CreateTestRelExternalRel() const
{
    return TestReflectedExternalAPI().CreateTestRelExternalRel();
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
