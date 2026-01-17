//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/usd/usdVol/particleField.h"
#include "pxr/usd/usd/schemaRegistry.h"
#include "pxr/usd/usd/typed.h"

#include "pxr/usd/sdf/types.h"
#include "pxr/usd/sdf/assetPath.h"

PXR_NAMESPACE_OPEN_SCOPE

// Register the schema with the TfType system.
TF_REGISTRY_FUNCTION(TfType)
{
    TfType::Define<UsdVolParticleField,
        TfType::Bases< UsdGeomGprim > >();
    
    // Register the usd prim typename as an alias under UsdSchemaBase. This
    // enables one to call
    // TfType::Find<UsdSchemaBase>().FindDerivedByName("ParticleField")
    // to find TfType<UsdVolParticleField>, which is how IsA queries are
    // answered.
    TfType::AddAlias<UsdSchemaBase, UsdVolParticleField>("ParticleField");
}

/* virtual */
UsdVolParticleField::~UsdVolParticleField()
{
}

/* static */
UsdVolParticleField
UsdVolParticleField::Get(const UsdStagePtr &stage, const SdfPath &path)
{
    if (!stage) {
        TF_CODING_ERROR("Invalid stage");
        return UsdVolParticleField();
    }
    return UsdVolParticleField(stage->GetPrimAtPath(path));
}

/* static */
UsdVolParticleField
UsdVolParticleField::Define(
    const UsdStagePtr &stage, const SdfPath &path)
{
    static TfToken usdPrimTypeName("ParticleField");
    if (!stage) {
        TF_CODING_ERROR("Invalid stage");
        return UsdVolParticleField();
    }
    return UsdVolParticleField(
        stage->DefinePrim(path, usdPrimTypeName));
}

/* virtual */
UsdSchemaKind UsdVolParticleField::_GetSchemaKind() const
{
    return UsdVolParticleField::schemaKind;
}

/* static */
const TfType &
UsdVolParticleField::_GetStaticTfType()
{
    static TfType tfType = TfType::Find<UsdVolParticleField>();
    return tfType;
}

/* static */
bool 
UsdVolParticleField::_IsTypedSchema()
{
    static bool isTyped = _GetStaticTfType().IsA<UsdTyped>();
    return isTyped;
}

/* virtual */
const TfType &
UsdVolParticleField::_GetTfType() const
{
    return _GetStaticTfType();
}

/*static*/
const TfTokenVector&
UsdVolParticleField::GetSchemaAttributeNames(bool includeInherited)
{
    static TfTokenVector localNames;
    static TfTokenVector allNames =
        UsdGeomGprim::GetSchemaAttributeNames(true);

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
