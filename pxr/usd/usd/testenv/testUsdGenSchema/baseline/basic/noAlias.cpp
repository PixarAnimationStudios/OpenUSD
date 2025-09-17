//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/usd/usdContrived/noAlias.h"
#include "pxr/usd/usd/schemaRegistry.h"
#include "pxr/usd/usd/typed.h"

#include "pxr/usd/sdf/types.h"
#include "pxr/usd/sdf/assetPath.h"

PXR_NAMESPACE_OPEN_SCOPE

// Register the schema with the TfType system.
TF_REGISTRY_FUNCTION(TfType)
{
    TfType::Define<UsdContrivedNoAlias,
        TfType::Bases< UsdTyped > >();
    
    // Skip registering an alias as it would be the same as the class name.
}

/* virtual */
UsdContrivedNoAlias::~UsdContrivedNoAlias()
{
}

/* static */
UsdContrivedNoAlias
UsdContrivedNoAlias::Get(const UsdStagePtr &stage, const SdfPath &path)
{
    if (!stage) {
        TF_CODING_ERROR("Invalid stage");
        return UsdContrivedNoAlias();
    }
    return UsdContrivedNoAlias(stage->GetPrimAtPath(path));
}

/* static */
UsdContrivedNoAlias
UsdContrivedNoAlias::Define(
    const UsdStagePtr &stage, const SdfPath &path)
{
    static TfToken usdPrimTypeName("UsdContrivedNoAlias");
    if (!stage) {
        TF_CODING_ERROR("Invalid stage");
        return UsdContrivedNoAlias();
    }
    return UsdContrivedNoAlias(
        stage->DefinePrim(path, usdPrimTypeName));
}

/* virtual */
UsdSchemaKind UsdContrivedNoAlias::_GetSchemaKind() const
{
    return UsdContrivedNoAlias::schemaKind;
}

/* static */
const TfType &
UsdContrivedNoAlias::_GetStaticTfType()
{
    static TfType tfType = TfType::Find<UsdContrivedNoAlias>();
    return tfType;
}

/* static */
bool 
UsdContrivedNoAlias::_IsTypedSchema()
{
    static bool isTyped = _GetStaticTfType().IsA<UsdTyped>();
    return isTyped;
}

/* virtual */
const TfType &
UsdContrivedNoAlias::_GetTfType() const
{
    return _GetStaticTfType();
}

/*static*/
const TfTokenVector&
UsdContrivedNoAlias::GetSchemaAttributeNames(bool includeInherited)
{
    static TfTokenVector localNames;
    static TfTokenVector allNames =
        UsdTyped::GetSchemaAttributeNames(true);

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
