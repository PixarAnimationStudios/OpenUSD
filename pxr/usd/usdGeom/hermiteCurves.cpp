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
#include "pxr/usd/usdGeom/hermiteCurves.h"
#include "pxr/usd/usd/schemaRegistry.h"
#include "pxr/usd/usd/typed.h"

#include "pxr/usd/sdf/types.h"
#include "pxr/usd/sdf/assetPath.h"

PXR_NAMESPACE_OPEN_SCOPE

// Register the schema with the TfType system.
TF_REGISTRY_FUNCTION(TfType)
{
    TfType::Define<UsdGeomHermiteCurves,
        TfType::Bases< UsdGeomCurves > >();
    
    // Register the usd prim typename as an alias under UsdSchemaBase. This
    // enables one to call
    // TfType::Find<UsdSchemaBase>().FindDerivedByName("HermiteCurves")
    // to find TfType<UsdGeomHermiteCurves>, which is how IsA queries are
    // answered.
    TfType::AddAlias<UsdSchemaBase, UsdGeomHermiteCurves>("HermiteCurves");
}

/* virtual */
UsdGeomHermiteCurves::~UsdGeomHermiteCurves()
{
}

/* static */
UsdGeomHermiteCurves
UsdGeomHermiteCurves::Get(const UsdStagePtr &stage, const SdfPath &path)
{
    if (!stage) {
        TF_CODING_ERROR("Invalid stage");
        return UsdGeomHermiteCurves();
    }
    return UsdGeomHermiteCurves(stage->GetPrimAtPath(path));
}

/* static */
UsdGeomHermiteCurves
UsdGeomHermiteCurves::Define(
    const UsdStagePtr &stage, const SdfPath &path)
{
    static TfToken usdPrimTypeName("HermiteCurves");
    if (!stage) {
        TF_CODING_ERROR("Invalid stage");
        return UsdGeomHermiteCurves();
    }
    return UsdGeomHermiteCurves(
        stage->DefinePrim(path, usdPrimTypeName));
}

/* virtual */
UsdSchemaKind UsdGeomHermiteCurves::_GetSchemaKind() const {
    return UsdGeomHermiteCurves::schemaKind;
}

/* virtual */
UsdSchemaKind UsdGeomHermiteCurves::_GetSchemaType() const {
    return UsdGeomHermiteCurves::schemaType;
}

/* static */
const TfType &
UsdGeomHermiteCurves::_GetStaticTfType()
{
    static TfType tfType = TfType::Find<UsdGeomHermiteCurves>();
    return tfType;
}

/* static */
bool 
UsdGeomHermiteCurves::_IsTypedSchema()
{
    static bool isTyped = _GetStaticTfType().IsA<UsdTyped>();
    return isTyped;
}

/* virtual */
const TfType &
UsdGeomHermiteCurves::_GetTfType() const
{
    return _GetStaticTfType();
}

UsdAttribute
UsdGeomHermiteCurves::GetTangentsAttr() const
{
    return GetPrim().GetAttribute(UsdGeomTokens->tangents);
}

UsdAttribute
UsdGeomHermiteCurves::CreateTangentsAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdGeomTokens->tangents,
                       SdfValueTypeNames->Vector3fArray,
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
UsdGeomHermiteCurves::GetSchemaAttributeNames(bool includeInherited)
{
    static TfTokenVector localNames = {
        UsdGeomTokens->tangents,
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

PXR_NAMESPACE_OPEN_SCOPE

UsdGeomHermiteCurves::PointAndTangentArrays::PointAndTangentArrays(
    const VtArray<GfVec3f>& interleaved)
{
    if (interleaved.empty()){
        return;
    }
    if (interleaved.size() % 2 != 0) {
        TF_CODING_ERROR(
            "Cannot separate odd-shaped interleaved points and tangents "
            "data.");
        return;
    }
    _points.resize(interleaved.size() / 2);
    _tangents.resize(interleaved.size() / 2);
    auto interleavedIt = interleaved.cbegin();
    auto pointsIt = _points.begin();
    auto tangentsIt = _tangents.begin();
    while (interleavedIt != interleaved.end()) {
        *pointsIt = *interleavedIt;
        std::advance(pointsIt, 1);
        std::advance(interleavedIt, 1);
        *tangentsIt = *interleavedIt;
        std::advance(tangentsIt, 1);
        std::advance(interleavedIt, 1);
    }
    TF_VERIFY(pointsIt == _points.end());
    TF_VERIFY(tangentsIt == _tangents.end());
}

VtVec3fArray UsdGeomHermiteCurves::PointAndTangentArrays::Interleave() const
{
    if (IsEmpty()) {
        return VtVec3fArray();
    }
    VtVec3fArray interleaved(GetPoints().size() * 2);
    auto interleavedIt = interleaved.begin();
    auto pointsIt = GetPoints().cbegin();
    auto tangentsIt = GetTangents().cbegin();
    while (interleavedIt != interleaved.end()) {
        *interleavedIt = *pointsIt;
        std::advance(pointsIt, 1);
        std::advance(interleavedIt, 1);
        *interleavedIt = *tangentsIt;
        std::advance(tangentsIt, 1);
        std::advance(interleavedIt, 1);
    }
    TF_VERIFY(pointsIt == GetPoints().cend());
    TF_VERIFY(tangentsIt == GetTangents().cend());
    return interleaved;
}

PXR_NAMESPACE_CLOSE_SCOPE
