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
#include "pxr/usd/usdGeom/mesh.h"
#include "pxr/usd/usd/schemaRegistry.h"
#include "pxr/usd/usd/typed.h"

#include "pxr/usd/sdf/types.h"
#include "pxr/usd/sdf/assetPath.h"

PXR_NAMESPACE_OPEN_SCOPE

// Register the schema with the TfType system.
TF_REGISTRY_FUNCTION(TfType)
{
    TfType::Define<UsdGeomMesh,
        TfType::Bases< UsdGeomPointBased > >();
    
    // Register the usd prim typename as an alias under UsdSchemaBase. This
    // enables one to call
    // TfType::Find<UsdSchemaBase>().FindDerivedByName("Mesh")
    // to find TfType<UsdGeomMesh>, which is how IsA queries are
    // answered.
    TfType::AddAlias<UsdSchemaBase, UsdGeomMesh>("Mesh");
}

/* virtual */
UsdGeomMesh::~UsdGeomMesh()
{
}

/* static */
UsdGeomMesh
UsdGeomMesh::Get(const UsdStagePtr &stage, const SdfPath &path)
{
    if (!stage) {
        TF_CODING_ERROR("Invalid stage");
        return UsdGeomMesh();
    }
    return UsdGeomMesh(stage->GetPrimAtPath(path));
}

/* static */
UsdGeomMesh
UsdGeomMesh::Define(
    const UsdStagePtr &stage, const SdfPath &path)
{
    static TfToken usdPrimTypeName("Mesh");
    if (!stage) {
        TF_CODING_ERROR("Invalid stage");
        return UsdGeomMesh();
    }
    return UsdGeomMesh(
        stage->DefinePrim(path, usdPrimTypeName));
}

/* virtual */
UsdSchemaType UsdGeomMesh::_GetSchemaType() const {
    return UsdGeomMesh::schemaType;
}

/* static */
const TfType &
UsdGeomMesh::_GetStaticTfType()
{
    static TfType tfType = TfType::Find<UsdGeomMesh>();
    return tfType;
}

/* static */
bool 
UsdGeomMesh::_IsTypedSchema()
{
    static bool isTyped = _GetStaticTfType().IsA<UsdTyped>();
    return isTyped;
}

/* virtual */
const TfType &
UsdGeomMesh::_GetTfType() const
{
    return _GetStaticTfType();
}

UsdAttribute
UsdGeomMesh::GetFaceVertexIndicesAttr() const
{
    return GetPrim().GetAttribute(UsdGeomTokens->faceVertexIndices);
}

UsdAttribute
UsdGeomMesh::CreateFaceVertexIndicesAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdGeomTokens->faceVertexIndices,
                       SdfValueTypeNames->IntArray,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdGeomMesh::GetFaceVertexCountsAttr() const
{
    return GetPrim().GetAttribute(UsdGeomTokens->faceVertexCounts);
}

UsdAttribute
UsdGeomMesh::CreateFaceVertexCountsAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdGeomTokens->faceVertexCounts,
                       SdfValueTypeNames->IntArray,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdGeomMesh::GetSubdivisionSchemeAttr() const
{
    return GetPrim().GetAttribute(UsdGeomTokens->subdivisionScheme);
}

UsdAttribute
UsdGeomMesh::CreateSubdivisionSchemeAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdGeomTokens->subdivisionScheme,
                       SdfValueTypeNames->Token,
                       /* custom = */ false,
                       SdfVariabilityUniform,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdGeomMesh::GetInterpolateBoundaryAttr() const
{
    return GetPrim().GetAttribute(UsdGeomTokens->interpolateBoundary);
}

UsdAttribute
UsdGeomMesh::CreateInterpolateBoundaryAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdGeomTokens->interpolateBoundary,
                       SdfValueTypeNames->Token,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdGeomMesh::GetFaceVaryingLinearInterpolationAttr() const
{
    return GetPrim().GetAttribute(UsdGeomTokens->faceVaryingLinearInterpolation);
}

UsdAttribute
UsdGeomMesh::CreateFaceVaryingLinearInterpolationAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdGeomTokens->faceVaryingLinearInterpolation,
                       SdfValueTypeNames->Token,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdGeomMesh::GetTriangleSubdivisionRuleAttr() const
{
    return GetPrim().GetAttribute(UsdGeomTokens->triangleSubdivisionRule);
}

UsdAttribute
UsdGeomMesh::CreateTriangleSubdivisionRuleAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdGeomTokens->triangleSubdivisionRule,
                       SdfValueTypeNames->Token,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdGeomMesh::GetHoleIndicesAttr() const
{
    return GetPrim().GetAttribute(UsdGeomTokens->holeIndices);
}

UsdAttribute
UsdGeomMesh::CreateHoleIndicesAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdGeomTokens->holeIndices,
                       SdfValueTypeNames->IntArray,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdGeomMesh::GetCornerIndicesAttr() const
{
    return GetPrim().GetAttribute(UsdGeomTokens->cornerIndices);
}

UsdAttribute
UsdGeomMesh::CreateCornerIndicesAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdGeomTokens->cornerIndices,
                       SdfValueTypeNames->IntArray,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdGeomMesh::GetCornerSharpnessesAttr() const
{
    return GetPrim().GetAttribute(UsdGeomTokens->cornerSharpnesses);
}

UsdAttribute
UsdGeomMesh::CreateCornerSharpnessesAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdGeomTokens->cornerSharpnesses,
                       SdfValueTypeNames->FloatArray,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdGeomMesh::GetCreaseIndicesAttr() const
{
    return GetPrim().GetAttribute(UsdGeomTokens->creaseIndices);
}

UsdAttribute
UsdGeomMesh::CreateCreaseIndicesAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdGeomTokens->creaseIndices,
                       SdfValueTypeNames->IntArray,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdGeomMesh::GetCreaseLengthsAttr() const
{
    return GetPrim().GetAttribute(UsdGeomTokens->creaseLengths);
}

UsdAttribute
UsdGeomMesh::CreateCreaseLengthsAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdGeomTokens->creaseLengths,
                       SdfValueTypeNames->IntArray,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdGeomMesh::GetCreaseSharpnessesAttr() const
{
    return GetPrim().GetAttribute(UsdGeomTokens->creaseSharpnesses);
}

UsdAttribute
UsdGeomMesh::CreateCreaseSharpnessesAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdGeomTokens->creaseSharpnesses,
                       SdfValueTypeNames->FloatArray,
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
UsdGeomMesh::GetSchemaAttributeNames(bool includeInherited)
{
    static TfTokenVector localNames = {
        UsdGeomTokens->faceVertexIndices,
        UsdGeomTokens->faceVertexCounts,
        UsdGeomTokens->subdivisionScheme,
        UsdGeomTokens->interpolateBoundary,
        UsdGeomTokens->faceVaryingLinearInterpolation,
        UsdGeomTokens->triangleSubdivisionRule,
        UsdGeomTokens->holeIndices,
        UsdGeomTokens->cornerIndices,
        UsdGeomTokens->cornerSharpnesses,
        UsdGeomTokens->creaseIndices,
        UsdGeomTokens->creaseLengths,
        UsdGeomTokens->creaseSharpnesses,
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

#include "pxr/base/arch/hints.h"
#include "pxr/base/tf/stringUtils.h"

#include <numeric>

PXR_NAMESPACE_OPEN_SCOPE

bool
UsdGeomMesh::ValidateTopology(const VtIntArray& faceVertexIndices,
                              const VtIntArray& faceVertexCounts,
                              size_t numPoints,
                              std::string* reason)
{
    // Sum of the vertex counts should be equal to the number of vertex indices.
    size_t vertCountsSum = std::accumulate(faceVertexCounts.cbegin(),
                                           faceVertexCounts.cend(), 0);
    
    if(vertCountsSum != faceVertexIndices.size()) {
        if(reason) {
            *reason = TfStringPrintf("Sum of faceVertexCounts [%zu] != "
                                     "size of faceVertexIndices [%zu].",
                                     vertCountsSum, faceVertexIndices.size());
        }
        return false;
    }

    // Make sure all verts are within the range of the point count.
    for(int vertexIndex : faceVertexIndices) {
        if(ARCH_UNLIKELY(vertexIndex < 0 || (size_t)vertexIndex >= numPoints)) {
            if(reason) {
                *reason = TfStringPrintf("Out of range face vertex index %d: "
                                         "Vertex must be in the range [0,%zu).",
                                         vertexIndex, numPoints);
            }
            return false;
        }
    }
    return true;
}


const float UsdGeomMesh::SHARPNESS_INFINITE = 1e38;

PXR_NAMESPACE_CLOSE_SCOPE
