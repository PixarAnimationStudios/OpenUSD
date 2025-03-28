//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/usd/usdContrived/singleApplyAPI_1.h"
#include "pxr/usd/usd/schemaRegistry.h"
#include "pxr/usd/usd/typed.h"

#include "pxr/usd/sdf/types.h"
#include "pxr/usd/sdf/assetPath.h"

PXR_NAMESPACE_OPEN_SCOPE

// Register the schema with the TfType system.
TF_REGISTRY_FUNCTION(TfType)
{
    TfType::Define<UsdContrivedSingleApplyAPI_1,
        TfType::Bases< UsdAPISchemaBase > >();
    
}

/* virtual */
UsdContrivedSingleApplyAPI_1::~UsdContrivedSingleApplyAPI_1()
{
}

/* static */
UsdContrivedSingleApplyAPI_1
UsdContrivedSingleApplyAPI_1::Get(const UsdStagePtr &stage, const SdfPath &path)
{
    if (!stage) {
        TF_CODING_ERROR("Invalid stage");
        return UsdContrivedSingleApplyAPI_1();
    }
    return UsdContrivedSingleApplyAPI_1(stage->GetPrimAtPath(path));
}


/* virtual */
UsdSchemaKind UsdContrivedSingleApplyAPI_1::_GetSchemaKind() const
{
    return UsdContrivedSingleApplyAPI_1::schemaKind;
}

/* static */
bool
UsdContrivedSingleApplyAPI_1::CanApply(
    const UsdPrim &prim, std::string *whyNot)
{
    return prim.CanApplyAPI<UsdContrivedSingleApplyAPI_1>(whyNot);
}

/* static */
UsdContrivedSingleApplyAPI_1
UsdContrivedSingleApplyAPI_1::Apply(const UsdPrim &prim)
{
    if (prim.ApplyAPI<UsdContrivedSingleApplyAPI_1>()) {
        return UsdContrivedSingleApplyAPI_1(prim);
    }
    return UsdContrivedSingleApplyAPI_1();
}

/* static */
const TfType &
UsdContrivedSingleApplyAPI_1::_GetStaticTfType()
{
    static TfType tfType = TfType::Find<UsdContrivedSingleApplyAPI_1>();
    return tfType;
}

/* static */
bool 
UsdContrivedSingleApplyAPI_1::_IsTypedSchema()
{
    static bool isTyped = _GetStaticTfType().IsA<UsdTyped>();
    return isTyped;
}

/* virtual */
const TfType &
UsdContrivedSingleApplyAPI_1::_GetTfType() const
{
    return _GetStaticTfType();
}

/*static*/
const TfTokenVector&
UsdContrivedSingleApplyAPI_1::GetSchemaAttributeNames(bool includeInherited)
{
    static TfTokenVector localNames;
    static TfTokenVector allNames =
        UsdAPISchemaBase::GetSchemaAttributeNames(true);

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
