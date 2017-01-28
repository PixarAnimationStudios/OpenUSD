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
#include "pxr/usd/usdGeom/nurbsPatch.h"
#include "pxr/usd/usd/schemaRegistry.h"
#include "pxr/usd/usd/typed.h"

#include "pxr/usd/sdf/types.h"
#include "pxr/usd/sdf/assetPath.h"

PXR_NAMESPACE_OPEN_SCOPE

// Register the schema with the TfType system.
TF_REGISTRY_FUNCTION(TfType)
{
    TfType::Define<UsdGeomNurbsPatch,
        TfType::Bases< UsdGeomPointBased > >();
    
    // Register the usd prim typename as an alias under UsdSchemaBase. This
    // enables one to call
    // TfType::Find<UsdSchemaBase>().FindDerivedByName("NurbsPatch")
    // to find TfType<UsdGeomNurbsPatch>, which is how IsA queries are
    // answered.
    TfType::AddAlias<UsdSchemaBase, UsdGeomNurbsPatch>("NurbsPatch");
}

/* virtual */
UsdGeomNurbsPatch::~UsdGeomNurbsPatch()
{
}

/* static */
UsdGeomNurbsPatch
UsdGeomNurbsPatch::Get(const UsdStagePtr &stage, const SdfPath &path)
{
    if (!stage) {
        TF_CODING_ERROR("Invalid stage");
        return UsdGeomNurbsPatch();
    }
    return UsdGeomNurbsPatch(stage->GetPrimAtPath(path));
}

/* static */
UsdGeomNurbsPatch
UsdGeomNurbsPatch::Define(
    const UsdStagePtr &stage, const SdfPath &path)
{
    static TfToken usdPrimTypeName("NurbsPatch");
    if (!stage) {
        TF_CODING_ERROR("Invalid stage");
        return UsdGeomNurbsPatch();
    }
    return UsdGeomNurbsPatch(
        stage->DefinePrim(path, usdPrimTypeName));
}

/* static */
const TfType &
UsdGeomNurbsPatch::_GetStaticTfType()
{
    static TfType tfType = TfType::Find<UsdGeomNurbsPatch>();
    return tfType;
}

/* static */
bool 
UsdGeomNurbsPatch::_IsTypedSchema()
{
    static bool isTyped = _GetStaticTfType().IsA<UsdTyped>();
    return isTyped;
}

/* virtual */
const TfType &
UsdGeomNurbsPatch::_GetTfType() const
{
    return _GetStaticTfType();
}

UsdAttribute
UsdGeomNurbsPatch::GetUVertexCountAttr() const
{
    return GetPrim().GetAttribute(UsdGeomTokens->uVertexCount);
}

UsdAttribute
UsdGeomNurbsPatch::CreateUVertexCountAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdGeomTokens->uVertexCount,
                       SdfValueTypeNames->Int,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdGeomNurbsPatch::GetVVertexCountAttr() const
{
    return GetPrim().GetAttribute(UsdGeomTokens->vVertexCount);
}

UsdAttribute
UsdGeomNurbsPatch::CreateVVertexCountAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdGeomTokens->vVertexCount,
                       SdfValueTypeNames->Int,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdGeomNurbsPatch::GetUOrderAttr() const
{
    return GetPrim().GetAttribute(UsdGeomTokens->uOrder);
}

UsdAttribute
UsdGeomNurbsPatch::CreateUOrderAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdGeomTokens->uOrder,
                       SdfValueTypeNames->Int,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdGeomNurbsPatch::GetVOrderAttr() const
{
    return GetPrim().GetAttribute(UsdGeomTokens->vOrder);
}

UsdAttribute
UsdGeomNurbsPatch::CreateVOrderAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdGeomTokens->vOrder,
                       SdfValueTypeNames->Int,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdGeomNurbsPatch::GetUKnotsAttr() const
{
    return GetPrim().GetAttribute(UsdGeomTokens->uKnots);
}

UsdAttribute
UsdGeomNurbsPatch::CreateUKnotsAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdGeomTokens->uKnots,
                       SdfValueTypeNames->DoubleArray,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdGeomNurbsPatch::GetVKnotsAttr() const
{
    return GetPrim().GetAttribute(UsdGeomTokens->vKnots);
}

UsdAttribute
UsdGeomNurbsPatch::CreateVKnotsAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdGeomTokens->vKnots,
                       SdfValueTypeNames->DoubleArray,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdGeomNurbsPatch::GetUFormAttr() const
{
    return GetPrim().GetAttribute(UsdGeomTokens->uForm);
}

UsdAttribute
UsdGeomNurbsPatch::CreateUFormAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdGeomTokens->uForm,
                       SdfValueTypeNames->Token,
                       /* custom = */ false,
                       SdfVariabilityUniform,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdGeomNurbsPatch::GetVFormAttr() const
{
    return GetPrim().GetAttribute(UsdGeomTokens->vForm);
}

UsdAttribute
UsdGeomNurbsPatch::CreateVFormAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdGeomTokens->vForm,
                       SdfValueTypeNames->Token,
                       /* custom = */ false,
                       SdfVariabilityUniform,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdGeomNurbsPatch::GetURangeAttr() const
{
    return GetPrim().GetAttribute(UsdGeomTokens->uRange);
}

UsdAttribute
UsdGeomNurbsPatch::CreateURangeAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdGeomTokens->uRange,
                       SdfValueTypeNames->Double2,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdGeomNurbsPatch::GetVRangeAttr() const
{
    return GetPrim().GetAttribute(UsdGeomTokens->vRange);
}

UsdAttribute
UsdGeomNurbsPatch::CreateVRangeAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdGeomTokens->vRange,
                       SdfValueTypeNames->Double2,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdGeomNurbsPatch::GetPointWeightsAttr() const
{
    return GetPrim().GetAttribute(UsdGeomTokens->pointWeights);
}

UsdAttribute
UsdGeomNurbsPatch::CreatePointWeightsAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdGeomTokens->pointWeights,
                       SdfValueTypeNames->DoubleArray,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdGeomNurbsPatch::GetTrimCurveCountsAttr() const
{
    return GetPrim().GetAttribute(UsdGeomTokens->trimCurveCounts);
}

UsdAttribute
UsdGeomNurbsPatch::CreateTrimCurveCountsAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdGeomTokens->trimCurveCounts,
                       SdfValueTypeNames->IntArray,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdGeomNurbsPatch::GetTrimCurveOrdersAttr() const
{
    return GetPrim().GetAttribute(UsdGeomTokens->trimCurveOrders);
}

UsdAttribute
UsdGeomNurbsPatch::CreateTrimCurveOrdersAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdGeomTokens->trimCurveOrders,
                       SdfValueTypeNames->IntArray,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdGeomNurbsPatch::GetTrimCurveVertexCountsAttr() const
{
    return GetPrim().GetAttribute(UsdGeomTokens->trimCurveVertexCounts);
}

UsdAttribute
UsdGeomNurbsPatch::CreateTrimCurveVertexCountsAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdGeomTokens->trimCurveVertexCounts,
                       SdfValueTypeNames->IntArray,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdGeomNurbsPatch::GetTrimCurveKnotsAttr() const
{
    return GetPrim().GetAttribute(UsdGeomTokens->trimCurveKnots);
}

UsdAttribute
UsdGeomNurbsPatch::CreateTrimCurveKnotsAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdGeomTokens->trimCurveKnots,
                       SdfValueTypeNames->DoubleArray,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdGeomNurbsPatch::GetTrimCurveRangesAttr() const
{
    return GetPrim().GetAttribute(UsdGeomTokens->trimCurveRanges);
}

UsdAttribute
UsdGeomNurbsPatch::CreateTrimCurveRangesAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdGeomTokens->trimCurveRanges,
                       SdfValueTypeNames->Double2Array,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdGeomNurbsPatch::GetTrimCurvePointsAttr() const
{
    return GetPrim().GetAttribute(UsdGeomTokens->trimCurvePoints);
}

UsdAttribute
UsdGeomNurbsPatch::CreateTrimCurvePointsAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdGeomTokens->trimCurvePoints,
                       SdfValueTypeNames->Double3Array,
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
UsdGeomNurbsPatch::GetSchemaAttributeNames(bool includeInherited)
{
    static TfTokenVector localNames = {
        UsdGeomTokens->uVertexCount,
        UsdGeomTokens->vVertexCount,
        UsdGeomTokens->uOrder,
        UsdGeomTokens->vOrder,
        UsdGeomTokens->uKnots,
        UsdGeomTokens->vKnots,
        UsdGeomTokens->uForm,
        UsdGeomTokens->vForm,
        UsdGeomTokens->uRange,
        UsdGeomTokens->vRange,
        UsdGeomTokens->pointWeights,
        UsdGeomTokens->trimCurveCounts,
        UsdGeomTokens->trimCurveOrders,
        UsdGeomTokens->trimCurveVertexCounts,
        UsdGeomTokens->trimCurveKnots,
        UsdGeomTokens->trimCurveRanges,
        UsdGeomTokens->trimCurvePoints,
    };
    static TfTokenVector allNames =
        _ConcatenateAttributeNames(
            UsdGeomPointBased::GetSchemaAttributeNames(true),
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
