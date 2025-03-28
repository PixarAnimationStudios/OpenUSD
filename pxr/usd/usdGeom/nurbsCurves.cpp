//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/usd/usdGeom/nurbsCurves.h"
#include "pxr/usd/usd/schemaRegistry.h"
#include "pxr/usd/usd/typed.h"

#include "pxr/usd/sdf/types.h"
#include "pxr/usd/sdf/assetPath.h"

PXR_NAMESPACE_OPEN_SCOPE

// Register the schema with the TfType system.
TF_REGISTRY_FUNCTION(TfType)
{
    TfType::Define<UsdGeomNurbsCurves,
        TfType::Bases< UsdGeomCurves > >();
    
    // Register the usd prim typename as an alias under UsdSchemaBase. This
    // enables one to call
    // TfType::Find<UsdSchemaBase>().FindDerivedByName("NurbsCurves")
    // to find TfType<UsdGeomNurbsCurves>, which is how IsA queries are
    // answered.
    TfType::AddAlias<UsdSchemaBase, UsdGeomNurbsCurves>("NurbsCurves");
}

/* virtual */
UsdGeomNurbsCurves::~UsdGeomNurbsCurves()
{
}

/* static */
UsdGeomNurbsCurves
UsdGeomNurbsCurves::Get(const UsdStagePtr &stage, const SdfPath &path)
{
    if (!stage) {
        TF_CODING_ERROR("Invalid stage");
        return UsdGeomNurbsCurves();
    }
    return UsdGeomNurbsCurves(stage->GetPrimAtPath(path));
}

/* static */
UsdGeomNurbsCurves
UsdGeomNurbsCurves::Define(
    const UsdStagePtr &stage, const SdfPath &path)
{
    static TfToken usdPrimTypeName("NurbsCurves");
    if (!stage) {
        TF_CODING_ERROR("Invalid stage");
        return UsdGeomNurbsCurves();
    }
    return UsdGeomNurbsCurves(
        stage->DefinePrim(path, usdPrimTypeName));
}

/* virtual */
UsdSchemaKind UsdGeomNurbsCurves::_GetSchemaKind() const
{
    return UsdGeomNurbsCurves::schemaKind;
}

/* static */
const TfType &
UsdGeomNurbsCurves::_GetStaticTfType()
{
    static TfType tfType = TfType::Find<UsdGeomNurbsCurves>();
    return tfType;
}

/* static */
bool 
UsdGeomNurbsCurves::_IsTypedSchema()
{
    static bool isTyped = _GetStaticTfType().IsA<UsdTyped>();
    return isTyped;
}

/* virtual */
const TfType &
UsdGeomNurbsCurves::_GetTfType() const
{
    return _GetStaticTfType();
}

UsdAttribute
UsdGeomNurbsCurves::GetOrderAttr() const
{
    return GetPrim().GetAttribute(UsdGeomTokens->order);
}

UsdAttribute
UsdGeomNurbsCurves::CreateOrderAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdGeomTokens->order,
                       SdfValueTypeNames->IntArray,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdGeomNurbsCurves::GetKnotsAttr() const
{
    return GetPrim().GetAttribute(UsdGeomTokens->knots);
}

UsdAttribute
UsdGeomNurbsCurves::CreateKnotsAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdGeomTokens->knots,
                       SdfValueTypeNames->DoubleArray,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdGeomNurbsCurves::GetRangesAttr() const
{
    return GetPrim().GetAttribute(UsdGeomTokens->ranges);
}

UsdAttribute
UsdGeomNurbsCurves::CreateRangesAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdGeomTokens->ranges,
                       SdfValueTypeNames->Double2Array,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdGeomNurbsCurves::GetPointWeightsAttr() const
{
    return GetPrim().GetAttribute(UsdGeomTokens->pointWeights);
}

UsdAttribute
UsdGeomNurbsCurves::CreatePointWeightsAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdGeomTokens->pointWeights,
                       SdfValueTypeNames->DoubleArray,
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
UsdGeomNurbsCurves::GetSchemaAttributeNames(bool includeInherited)
{
    static TfTokenVector localNames = {
        UsdGeomTokens->order,
        UsdGeomTokens->knots,
        UsdGeomTokens->ranges,
        UsdGeomTokens->pointWeights,
    };
    static TfTokenVector allNames =
        _ConcatenateAttributeNames(
            UsdGeomCurves::GetSchemaAttributeNames(true),
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
