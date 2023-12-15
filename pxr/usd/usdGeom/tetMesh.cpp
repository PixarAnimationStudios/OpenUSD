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
#include "pxr/usd/usdGeom/tetMesh.h"
#include "pxr/usd/usd/schemaRegistry.h"
#include "pxr/usd/usd/typed.h"

#include "pxr/usd/sdf/types.h"
#include "pxr/usd/sdf/assetPath.h"

PXR_NAMESPACE_OPEN_SCOPE

// Register the schema with the TfType system.
TF_REGISTRY_FUNCTION(TfType)
{
    TfType::Define<UsdGeomTetMesh,
        TfType::Bases< UsdGeomPointBased > >();
    
    // Register the usd prim typename as an alias under UsdSchemaBase. This
    // enables one to call
    // TfType::Find<UsdSchemaBase>().FindDerivedByName("TetMesh")
    // to find TfType<UsdGeomTetMesh>, which is how IsA queries are
    // answered.
    TfType::AddAlias<UsdSchemaBase, UsdGeomTetMesh>("TetMesh");
}

/* virtual */
UsdGeomTetMesh::~UsdGeomTetMesh()
{
}

/* static */
UsdGeomTetMesh
UsdGeomTetMesh::Get(const UsdStagePtr &stage, const SdfPath &path)
{
    if (!stage) {
        TF_CODING_ERROR("Invalid stage");
        return UsdGeomTetMesh();
    }
    return UsdGeomTetMesh(stage->GetPrimAtPath(path));
}

/* static */
UsdGeomTetMesh
UsdGeomTetMesh::Define(
    const UsdStagePtr &stage, const SdfPath &path)
{
    static TfToken usdPrimTypeName("TetMesh");
    if (!stage) {
        TF_CODING_ERROR("Invalid stage");
        return UsdGeomTetMesh();
    }
    return UsdGeomTetMesh(
        stage->DefinePrim(path, usdPrimTypeName));
}

/* virtual */
UsdSchemaKind UsdGeomTetMesh::_GetSchemaKind() const
{
    return UsdGeomTetMesh::schemaKind;
}

/* static */
const TfType &
UsdGeomTetMesh::_GetStaticTfType()
{
    static TfType tfType = TfType::Find<UsdGeomTetMesh>();
    return tfType;
}

/* static */
bool 
UsdGeomTetMesh::_IsTypedSchema()
{
    static bool isTyped = _GetStaticTfType().IsA<UsdTyped>();
    return isTyped;
}

/* virtual */
const TfType &
UsdGeomTetMesh::_GetTfType() const
{
    return _GetStaticTfType();
}

UsdAttribute
UsdGeomTetMesh::GetTetVertexIndicesAttr() const
{
    return GetPrim().GetAttribute(UsdGeomTokens->tetVertexIndices);
}

UsdAttribute
UsdGeomTetMesh::CreateTetVertexIndicesAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdGeomTokens->tetVertexIndices,
                       SdfValueTypeNames->Int4Array,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdGeomTetMesh::GetSurfaceFaceVertexIndicesAttr() const
{
    return GetPrim().GetAttribute(UsdGeomTokens->surfaceFaceVertexIndices);
}

UsdAttribute
UsdGeomTetMesh::CreateSurfaceFaceVertexIndicesAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdGeomTokens->surfaceFaceVertexIndices,
                       SdfValueTypeNames->Int3Array,
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
UsdGeomTetMesh::GetSchemaAttributeNames(bool includeInherited)
{
    static TfTokenVector localNames = {
        UsdGeomTokens->tetVertexIndices,
        UsdGeomTokens->surfaceFaceVertexIndices,
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
