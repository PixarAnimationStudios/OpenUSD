//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/usd/usdVol/volume.h"
#include "pxr/usd/usd/schemaRegistry.h"
#include "pxr/usd/usd/typed.h"

#include "pxr/usd/sdf/types.h"
#include "pxr/usd/sdf/assetPath.h"

PXR_NAMESPACE_OPEN_SCOPE

// Register the schema with the TfType system.
TF_REGISTRY_FUNCTION(TfType)
{
    TfType::Define<UsdVolVolume,
        TfType::Bases< UsdGeomGprim > >();
    
    // Register the usd prim typename as an alias under UsdSchemaBase. This
    // enables one to call
    // TfType::Find<UsdSchemaBase>().FindDerivedByName("Volume")
    // to find TfType<UsdVolVolume>, which is how IsA queries are
    // answered.
    TfType::AddAlias<UsdSchemaBase, UsdVolVolume>("Volume");
}

/* virtual */
UsdVolVolume::~UsdVolVolume()
{
}

/* static */
UsdVolVolume
UsdVolVolume::Get(const UsdStagePtr &stage, const SdfPath &path)
{
    if (!stage) {
        TF_CODING_ERROR("Invalid stage");
        return UsdVolVolume();
    }
    return UsdVolVolume(stage->GetPrimAtPath(path));
}

/* static */
UsdVolVolume
UsdVolVolume::Define(
    const UsdStagePtr &stage, const SdfPath &path)
{
    static TfToken usdPrimTypeName("Volume");
    if (!stage) {
        TF_CODING_ERROR("Invalid stage");
        return UsdVolVolume();
    }
    return UsdVolVolume(
        stage->DefinePrim(path, usdPrimTypeName));
}

/* virtual */
UsdSchemaKind UsdVolVolume::_GetSchemaKind() const
{
    return UsdVolVolume::schemaKind;
}

/* static */
const TfType &
UsdVolVolume::_GetStaticTfType()
{
    static TfType tfType = TfType::Find<UsdVolVolume>();
    return tfType;
}

/* static */
bool 
UsdVolVolume::_IsTypedSchema()
{
    static bool isTyped = _GetStaticTfType().IsA<UsdTyped>();
    return isTyped;
}

/* virtual */
const TfType &
UsdVolVolume::_GetTfType() const
{
    return _GetStaticTfType();
}

/*static*/
const TfTokenVector&
UsdVolVolume::GetSchemaAttributeNames(bool includeInherited)
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

PXR_NAMESPACE_OPEN_SCOPE

TF_DEFINE_PRIVATE_TOKENS(
    _tokens,
    ((fieldPrefix, "field:"))
);

TfToken
UsdVolVolume::_MakeNamespaced(const TfToken& name)
{
    TfToken result;

    if (TfStringStartsWith(name, _tokens->fieldPrefix)) {
	result = name;
    }
    else {
	result = TfToken(_tokens->fieldPrefix.GetString() + name.GetString());
    }

    return result;
}

UsdVolVolume::FieldMap
UsdVolVolume::GetFieldPaths() const
{
    std::map<TfToken, SdfPath> fieldMap;
    const UsdPrim &prim = GetPrim();

    if (prim) {
        std::vector<UsdProperty> fieldProps =
            prim.GetPropertiesInNamespace(_tokens->fieldPrefix);
        for (const UsdProperty &fieldProp : fieldProps) {
            UsdRelationship fieldRel = fieldProp.As<UsdRelationship>();
            SdfPathVector targets;

            // All relationships starting with "field:" should point to
            // UsdVolFieldBase primitives.
            if (fieldRel && fieldRel.GetForwardedTargets(&targets)) {
                if (targets.size() == 1 && 
                    targets.front().IsPrimPath()) {
                    fieldMap.emplace(fieldRel.GetBaseName(), targets.front());
                }
            }
        }
    }

    return fieldMap;
}

bool
UsdVolVolume::HasFieldRelationship(const TfToken &name) const
{
    return GetPrim().HasRelationship(_MakeNamespaced(name));
}

SdfPath
UsdVolVolume::GetFieldPath(const TfToken &name) const
{
    UsdRelationship fieldRel =  GetPrim().GetRelationship(_MakeNamespaced(name));
    SdfPathVector targets;
    
    if (fieldRel && fieldRel.GetForwardedTargets(&targets)) {
        if (targets.size() == 1 && 
            targets.front().IsPrimPath()) {
            return targets.front();
        }
    }

    return SdfPath::EmptyPath();
}

bool
UsdVolVolume::CreateFieldRelationship(const TfToken &name,
	const SdfPath &fieldPath) const
{
    if (!fieldPath.IsPrimPath() && !fieldPath.IsPrimPropertyPath()){
        return false;
    }
    UsdRelationship fieldRel =
	GetPrim().CreateRelationship(_MakeNamespaced(name), /*custom*/true);

    if (fieldRel) {
	return fieldRel.SetTargets({fieldPath});
    }

    return false;
}

bool
UsdVolVolume::BlockFieldRelationship(const TfToken &name) const
{
    UsdRelationship fieldRel =  GetPrim().GetRelationship(_MakeNamespaced(name));
    
    if (fieldRel){
        fieldRel.SetTargets({});
        return true;
    }
    else {
        return false;
    }
}

PXR_NAMESPACE_CLOSE_SCOPE

