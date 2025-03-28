//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/usd/usdLux/geometryLight.h"
#include "pxr/usd/usd/schemaRegistry.h"
#include "pxr/usd/usd/typed.h"

#include "pxr/usd/sdf/types.h"
#include "pxr/usd/sdf/assetPath.h"

PXR_NAMESPACE_OPEN_SCOPE

// Register the schema with the TfType system.
TF_REGISTRY_FUNCTION(TfType)
{
    TfType::Define<UsdLuxGeometryLight,
        TfType::Bases< UsdLuxNonboundableLightBase > >();
    
    // Register the usd prim typename as an alias under UsdSchemaBase. This
    // enables one to call
    // TfType::Find<UsdSchemaBase>().FindDerivedByName("GeometryLight")
    // to find TfType<UsdLuxGeometryLight>, which is how IsA queries are
    // answered.
    TfType::AddAlias<UsdSchemaBase, UsdLuxGeometryLight>("GeometryLight");
}

/* virtual */
UsdLuxGeometryLight::~UsdLuxGeometryLight()
{
}

/* static */
UsdLuxGeometryLight
UsdLuxGeometryLight::Get(const UsdStagePtr &stage, const SdfPath &path)
{
    if (!stage) {
        TF_CODING_ERROR("Invalid stage");
        return UsdLuxGeometryLight();
    }
    return UsdLuxGeometryLight(stage->GetPrimAtPath(path));
}

/* static */
UsdLuxGeometryLight
UsdLuxGeometryLight::Define(
    const UsdStagePtr &stage, const SdfPath &path)
{
    static TfToken usdPrimTypeName("GeometryLight");
    if (!stage) {
        TF_CODING_ERROR("Invalid stage");
        return UsdLuxGeometryLight();
    }
    return UsdLuxGeometryLight(
        stage->DefinePrim(path, usdPrimTypeName));
}

/* virtual */
UsdSchemaKind UsdLuxGeometryLight::_GetSchemaKind() const
{
    return UsdLuxGeometryLight::schemaKind;
}

/* static */
const TfType &
UsdLuxGeometryLight::_GetStaticTfType()
{
    static TfType tfType = TfType::Find<UsdLuxGeometryLight>();
    return tfType;
}

/* static */
bool 
UsdLuxGeometryLight::_IsTypedSchema()
{
    static bool isTyped = _GetStaticTfType().IsA<UsdTyped>();
    return isTyped;
}

/* virtual */
const TfType &
UsdLuxGeometryLight::_GetTfType() const
{
    return _GetStaticTfType();
}

UsdRelationship
UsdLuxGeometryLight::GetGeometryRel() const
{
    return GetPrim().GetRelationship(UsdLuxTokens->geometry);
}

UsdRelationship
UsdLuxGeometryLight::CreateGeometryRel() const
{
    return GetPrim().CreateRelationship(UsdLuxTokens->geometry,
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
UsdLuxGeometryLight::GetSchemaAttributeNames(bool includeInherited)
{
    static TfTokenVector localNames = {
    };
    static TfTokenVector allNames =
        _ConcatenateAttributeNames(
            UsdLuxNonboundableLightBase::GetSchemaAttributeNames(true),
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
