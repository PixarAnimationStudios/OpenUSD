//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/usd/usdProc/generativeProcedural.h"
#include "pxr/usd/usd/schemaRegistry.h"
#include "pxr/usd/usd/typed.h"

#include "pxr/usd/sdf/types.h"
#include "pxr/usd/sdf/assetPath.h"

PXR_NAMESPACE_OPEN_SCOPE

// Register the schema with the TfType system.
TF_REGISTRY_FUNCTION(TfType)
{
    TfType::Define<UsdProcGenerativeProcedural,
        TfType::Bases< UsdGeomBoundable > >();
    
    // Register the usd prim typename as an alias under UsdSchemaBase. This
    // enables one to call
    // TfType::Find<UsdSchemaBase>().FindDerivedByName("GenerativeProcedural")
    // to find TfType<UsdProcGenerativeProcedural>, which is how IsA queries are
    // answered.
    TfType::AddAlias<UsdSchemaBase, UsdProcGenerativeProcedural>("GenerativeProcedural");
}

/* virtual */
UsdProcGenerativeProcedural::~UsdProcGenerativeProcedural()
{
}

/* static */
UsdProcGenerativeProcedural
UsdProcGenerativeProcedural::Get(const UsdStagePtr &stage, const SdfPath &path)
{
    if (!stage) {
        TF_CODING_ERROR("Invalid stage");
        return UsdProcGenerativeProcedural();
    }
    return UsdProcGenerativeProcedural(stage->GetPrimAtPath(path));
}

/* static */
UsdProcGenerativeProcedural
UsdProcGenerativeProcedural::Define(
    const UsdStagePtr &stage, const SdfPath &path)
{
    static TfToken usdPrimTypeName("GenerativeProcedural");
    if (!stage) {
        TF_CODING_ERROR("Invalid stage");
        return UsdProcGenerativeProcedural();
    }
    return UsdProcGenerativeProcedural(
        stage->DefinePrim(path, usdPrimTypeName));
}

/* virtual */
UsdSchemaKind UsdProcGenerativeProcedural::_GetSchemaKind() const
{
    return UsdProcGenerativeProcedural::schemaKind;
}

/* static */
const TfType &
UsdProcGenerativeProcedural::_GetStaticTfType()
{
    static TfType tfType = TfType::Find<UsdProcGenerativeProcedural>();
    return tfType;
}

/* static */
bool 
UsdProcGenerativeProcedural::_IsTypedSchema()
{
    static bool isTyped = _GetStaticTfType().IsA<UsdTyped>();
    return isTyped;
}

/* virtual */
const TfType &
UsdProcGenerativeProcedural::_GetTfType() const
{
    return _GetStaticTfType();
}

UsdAttribute
UsdProcGenerativeProcedural::GetProceduralSystemAttr() const
{
    return GetPrim().GetAttribute(UsdProcTokens->proceduralSystem);
}

UsdAttribute
UsdProcGenerativeProcedural::CreateProceduralSystemAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdProcTokens->proceduralSystem,
                       SdfValueTypeNames->Token,
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
UsdProcGenerativeProcedural::GetSchemaAttributeNames(bool includeInherited)
{
    static TfTokenVector localNames = {
        UsdProcTokens->proceduralSystem,
    };
    static TfTokenVector allNames =
        _ConcatenateAttributeNames(
            UsdGeomBoundable::GetSchemaAttributeNames(true),
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
