//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/usd/usdVol/fieldAsset.h"
#include "pxr/usd/usd/schemaRegistry.h"
#include "pxr/usd/usd/typed.h"

#include "pxr/usd/sdf/types.h"
#include "pxr/usd/sdf/assetPath.h"

PXR_NAMESPACE_OPEN_SCOPE

// Register the schema with the TfType system.
TF_REGISTRY_FUNCTION(TfType)
{
    TfType::Define<UsdVolFieldAsset,
        TfType::Bases< UsdVolVolumeFieldAsset > >();
    
}

/* virtual */
UsdVolFieldAsset::~UsdVolFieldAsset()
{
}

/* static */
UsdVolFieldAsset
UsdVolFieldAsset::Get(const UsdStagePtr &stage, const SdfPath &path)
{
    if (!stage) {
        TF_CODING_ERROR("Invalid stage");
        return UsdVolFieldAsset();
    }
    return UsdVolFieldAsset(stage->GetPrimAtPath(path));
}


/* virtual */
UsdSchemaKind UsdVolFieldAsset::_GetSchemaKind() const
{
    return UsdVolFieldAsset::schemaKind;
}

/* static */
const TfType &
UsdVolFieldAsset::_GetStaticTfType()
{
    static TfType tfType = TfType::Find<UsdVolFieldAsset>();
    return tfType;
}

/* static */
bool 
UsdVolFieldAsset::_IsTypedSchema()
{
    static bool isTyped = _GetStaticTfType().IsA<UsdTyped>();
    return isTyped;
}

/* virtual */
const TfType &
UsdVolFieldAsset::_GetTfType() const
{
    return _GetStaticTfType();
}

/*static*/
const TfTokenVector&
UsdVolFieldAsset::GetSchemaAttributeNames(bool includeInherited)
{
    static TfTokenVector localNames;
    static TfTokenVector allNames =
        UsdVolVolumeFieldAsset::GetSchemaAttributeNames(true);

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
