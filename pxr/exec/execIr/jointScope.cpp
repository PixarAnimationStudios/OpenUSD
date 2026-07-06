//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/exec/execIr/jointScope.h"
#include "pxr/usd/usd/schemaRegistry.h"
#include "pxr/usd/usd/typed.h"

#include "pxr/usd/sdf/types.h"
#include "pxr/usd/sdf/assetPath.h"

PXR_NAMESPACE_OPEN_SCOPE

// Register the schema with the TfType system.
TF_REGISTRY_FUNCTION(TfType)
{
    TfType::Define<ExecIrJointScope,
        TfType::Bases< ExecIrXformable > >();
    
    // Register the usd prim typename as an alias under UsdSchemaBase. This
    // enables one to call
    // TfType::Find<UsdSchemaBase>().FindDerivedByName("IrJointScope")
    // to find TfType<ExecIrJointScope>, which is how IsA queries are
    // answered.
    TfType::AddAlias<UsdSchemaBase, ExecIrJointScope>("IrJointScope");
}

/* virtual */
ExecIrJointScope::~ExecIrJointScope()
{
}

/* static */
ExecIrJointScope
ExecIrJointScope::Get(const UsdStagePtr &stage, const SdfPath &path)
{
    if (!stage) {
        TF_CODING_ERROR("Invalid stage");
        return ExecIrJointScope();
    }
    return ExecIrJointScope(stage->GetPrimAtPath(path));
}

/* static */
ExecIrJointScope
ExecIrJointScope::Define(
    const UsdStagePtr &stage, const SdfPath &path)
{
    static TfToken usdPrimTypeName("IrJointScope");
    if (!stage) {
        TF_CODING_ERROR("Invalid stage");
        return ExecIrJointScope();
    }
    return ExecIrJointScope(
        stage->DefinePrim(path, usdPrimTypeName));
}

/* virtual */
UsdSchemaKind ExecIrJointScope::_GetSchemaKind() const
{
    return ExecIrJointScope::schemaKind;
}

/* static */
const TfType &
ExecIrJointScope::_GetStaticTfType()
{
    static TfType tfType = TfType::Find<ExecIrJointScope>();
    return tfType;
}

/* static */
bool 
ExecIrJointScope::_IsTypedSchema()
{
    static bool isTyped = _GetStaticTfType().IsA<UsdTyped>();
    return isTyped;
}

/* virtual */
const TfType &
ExecIrJointScope::_GetTfType() const
{
    return _GetStaticTfType();
}

UsdAttribute
ExecIrJointScope::GetGuideLengthAttr() const
{
    return GetPrim().GetAttribute(ExecIrTokens->guideLength);
}

UsdAttribute
ExecIrJointScope::CreateGuideLengthAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(ExecIrTokens->guideLength,
                       SdfValueTypeNames->Double,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
ExecIrJointScope::GetGuideDisplayColorAttr() const
{
    return GetPrim().GetAttribute(ExecIrTokens->guideDisplayColor);
}

UsdAttribute
ExecIrJointScope::CreateGuideDisplayColorAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(ExecIrTokens->guideDisplayColor,
                       SdfValueTypeNames->Color3f,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
ExecIrJointScope::GetGuideDisplayOpacityAttr() const
{
    return GetPrim().GetAttribute(ExecIrTokens->guideDisplayOpacity);
}

UsdAttribute
ExecIrJointScope::CreateGuideDisplayOpacityAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(ExecIrTokens->guideDisplayOpacity,
                       SdfValueTypeNames->Float,
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
ExecIrJointScope::GetSchemaAttributeNames(bool includeInherited)
{
    static TfTokenVector localNames = {
        ExecIrTokens->guideLength,
        ExecIrTokens->guideDisplayColor,
        ExecIrTokens->guideDisplayOpacity,
    };
    static TfTokenVector allNames =
        _ConcatenateAttributeNames(
            ExecIrXformable::GetSchemaAttributeNames(true),
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
