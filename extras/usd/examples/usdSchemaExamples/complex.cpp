//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "./complex.h"
#include "pxr/usd/usd/schemaRegistry.h"
#include "pxr/usd/usd/typed.h"

#include "pxr/usd/sdf/types.h"
#include "pxr/usd/sdf/assetPath.h"

PXR_NAMESPACE_OPEN_SCOPE

// Register the schema with the TfType system.
TF_REGISTRY_FUNCTION(TfType)
{
    TfType::Define<UsdSchemaExamplesComplex,
        TfType::Bases< UsdSchemaExamplesSimple > >();
    
    // Register the usd prim typename as an alias under UsdSchemaBase. This
    // enables one to call
    // TfType::Find<UsdSchemaBase>().FindDerivedByName("ComplexPrim")
    // to find TfType<UsdSchemaExamplesComplex>, which is how IsA queries are
    // answered.
    TfType::AddAlias<UsdSchemaBase, UsdSchemaExamplesComplex>("ComplexPrim");
}

/* virtual */
UsdSchemaExamplesComplex::~UsdSchemaExamplesComplex()
{
}

/* static */
UsdSchemaExamplesComplex
UsdSchemaExamplesComplex::Get(const UsdStagePtr &stage, const SdfPath &path)
{
    if (!stage) {
        TF_CODING_ERROR("Invalid stage");
        return UsdSchemaExamplesComplex();
    }
    return UsdSchemaExamplesComplex(stage->GetPrimAtPath(path));
}

/* static */
UsdSchemaExamplesComplex
UsdSchemaExamplesComplex::Define(
    const UsdStagePtr &stage, const SdfPath &path)
{
    static TfToken usdPrimTypeName("ComplexPrim");
    if (!stage) {
        TF_CODING_ERROR("Invalid stage");
        return UsdSchemaExamplesComplex();
    }
    return UsdSchemaExamplesComplex(
        stage->DefinePrim(path, usdPrimTypeName));
}

/* virtual */
UsdSchemaKind UsdSchemaExamplesComplex::_GetSchemaKind() const
{
    return UsdSchemaExamplesComplex::schemaKind;
}

/* static */
const TfType &
UsdSchemaExamplesComplex::_GetStaticTfType()
{
    static TfType tfType = TfType::Find<UsdSchemaExamplesComplex>();
    return tfType;
}

/* static */
bool 
UsdSchemaExamplesComplex::_IsTypedSchema()
{
    static bool isTyped = _GetStaticTfType().IsA<UsdTyped>();
    return isTyped;
}

/* virtual */
const TfType &
UsdSchemaExamplesComplex::_GetTfType() const
{
    return _GetStaticTfType();
}

UsdAttribute
UsdSchemaExamplesComplex::GetComplexStringAttr() const
{
    return GetPrim().GetAttribute(UsdSchemaExamplesTokens->complexString);
}

UsdAttribute
UsdSchemaExamplesComplex::CreateComplexStringAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdSchemaExamplesTokens->complexString,
                       SdfValueTypeNames->String,
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
UsdSchemaExamplesComplex::GetSchemaAttributeNames(bool includeInherited)
{
    static TfTokenVector localNames = {
        UsdSchemaExamplesTokens->complexString,
    };
    static TfTokenVector allNames =
        _ConcatenateAttributeNames(
            UsdSchemaExamplesSimple::GetSchemaAttributeNames(true),
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
