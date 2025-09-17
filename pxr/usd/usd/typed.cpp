//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/usd/usd/typed.h"
#include "pxr/usd/usd/schemaRegistry.h"
#include "pxr/usd/usd/typed.h"

#include "pxr/usd/sdf/types.h"
#include "pxr/usd/sdf/assetPath.h"

PXR_NAMESPACE_OPEN_SCOPE

// Register the schema with the TfType system.
TF_REGISTRY_FUNCTION(TfType)
{
    TfType::Define<UsdTyped,
        TfType::Bases< UsdSchemaBase > >();
    
}

/* virtual */
UsdTyped::~UsdTyped()
{
}

/* static */
UsdTyped
UsdTyped::Get(const UsdStagePtr &stage, const SdfPath &path)
{
    if (!stage) {
        TF_CODING_ERROR("Invalid stage");
        return UsdTyped();
    }
    return UsdTyped(stage->GetPrimAtPath(path));
}


/* virtual */
UsdSchemaKind UsdTyped::_GetSchemaKind() const
{
    return UsdTyped::schemaKind;
}

/* static */
const TfType &
UsdTyped::_GetStaticTfType()
{
    static TfType tfType = TfType::Find<UsdTyped>();
    return tfType;
}

/* static */
bool 
UsdTyped::_IsTypedSchema()
{
    static bool isTyped = _GetStaticTfType().IsA<UsdTyped>();
    return isTyped;
}

/* virtual */
const TfType &
UsdTyped::_GetTfType() const
{
    return _GetStaticTfType();
}

/*static*/
const TfTokenVector&
UsdTyped::GetSchemaAttributeNames(bool includeInherited)
{
    static TfTokenVector localNames;
    static TfTokenVector allNames =
        UsdSchemaBase::GetSchemaAttributeNames(true);

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
UsdTyped::_IsCompatible() const
{
    if (!UsdSchemaBase::_IsCompatible())
        return false;

    return GetPrim().IsA(_GetType());
}

PXR_NAMESPACE_CLOSE_SCOPE
