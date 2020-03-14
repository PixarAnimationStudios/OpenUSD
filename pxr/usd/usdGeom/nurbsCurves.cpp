//
// Copyright 2016 Pixar
//
// Licensed under the Apache License, Version 2.0 (the "Apache License")
// with the following modification; you may not use this file except in
// compliance with the Apache License and the following modification to it:
// Section 6. Trademarks. is deleted and replaced with:
//
// 6. Trademarks. This License does not grant permission to use the trade
//    names, trademarks, service marks, or product names of the Licensor
//    and its affiliates, except as required to comply with Section 4(c) of
//    the License and to reproduce the content of the NOTICE file.
//
// You may obtain a copy of the Apache License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the Apache License with the above modification is
// distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
// KIND, either express or implied. See the Apache License for the specific
// language governing permissions and limitations under the Apache License.
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
UsdSchemaType UsdGeomNurbsCurves::_GetSchemaType() const {
    return UsdGeomNurbsCurves::schemaType;
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
