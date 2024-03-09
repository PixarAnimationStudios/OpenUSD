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
PXR_NAMESPACE_OPEN_SCOPE

namespace {

GfVec3i
_Sorted(GfVec3i v)
{
    if (v[0] > v[1]) {
        std::swap(v[0], v[1]);
    }
    if (v[0] > v[2]) {
        std::swap(v[0], v[2]);
    }
    if (v[1] > v[2]) {
        std::swap(v[1], v[2]);
    }
    return v;
}

struct _Vec3iHash
{
    size_t operator()(const GfVec3i& v) const
    {
        return
            static_cast<size_t>(v[0]) << 42 ^
            static_cast<size_t>(v[1]) << 21 ^
            static_cast<size_t>(v[2]);
    }
};

struct _Vec3iCmp
{
    // Comparator function
    bool operator()(const GfVec3i& f1, const GfVec3i &f2)
    {
        if (f1[0] == f2[0])
        {
            if (f1[1] == f2[1])
            {
                return f1[2] < f2[2];
            }
            
            return f1[1] < f2[1];
        }
        
        return f1[0] < f2[0];
    }
};    

VtVec3iArray
_ComputeSurfaceFaces(const VtVec4iArray &tetVertexIndices)
{

    // The surface faces are made of triangles that are not shared between
    // tetrahedra and only occur once. We use a hashmap from triangles
    // to counting information to see whether a triangle occurs in the
    // hashmap just and thus is not shared. We then sweep the hashmap
    // to find all triangles.
    //
    // Recall that a triangle is a triple of indices. But two triangles are
    // shared if these two triples are related by a permutation. Thus, the
    // key into the hashmap is the sorted triple which we call the signature.
    //
    // The value of the hashmap is a pair of (count, triple). The triple
    // is stored next to the count so that we do not loose the orientation
    // information that was lost when sorting the triple.
    //
    using SigToCountAndTriangle =
        TfHashMap<GfVec3i, std::pair<size_t, GfVec3i>, _Vec3iHash>;

    SigToCountAndTriangle sigToCountAndTriangle;

    for (size_t t = 0; t < tetVertexIndices.size(); t++) {

        const GfVec4i& tet = tetVertexIndices[t];

        // The four triangles of a tetrahedron
        static int tetFaceIndices[4][3] = {
            {1,2,3},
            {0,3,2},
            {0,1,3},
            {0,2,1}
        };
        
        for (int tFace = 0; tFace < 4; tFace++) {

            // A triangle of this tetrahedron.
            const GfVec3i triangle(
                tet[tetFaceIndices[tFace][0]],
                tet[tetFaceIndices[tFace][1]],
                tet[tetFaceIndices[tFace][2]]);

            std::pair<size_t, GfVec3i> &item =
                sigToCountAndTriangle[_Sorted(triangle)];
            item.first++;
            item.second = triangle;
        }  
    }          

    VtVec3iArray result;
    // Reserve one surface face per tetrahedron.
    // A tetrahedron can contribute up to 4 faces, but typically,
    // most faces of tet mesh are shared. So this is really just
    // a guess.
    result.reserve(tetVertexIndices.size());

    for(auto && [sig, countAndTriangle] : sigToCountAndTriangle) {
        if (countAndTriangle.first == 1) {
            result.push_back(countAndTriangle.second);
        }
    }
    // Need to sort results for deterministic behavior across different 
    // compiler/OS versions 
    std::sort(result.begin(), result.end(), _Vec3iCmp());

    return result;
}

}

bool UsdGeomTetMesh::ComputeSurfaceFaces(const UsdGeomTetMesh& tetMesh,
                                         VtVec3iArray* surfaceFaceIndices,
                                         const UsdTimeCode timeCode) 
{
    
    if (surfaceFaceIndices == nullptr)
    {
        return false;
    }

    const UsdAttribute& tetVertexIndicesAttr = tetMesh.GetTetVertexIndicesAttr();
    VtVec4iArray tetVertexIndices;
    tetVertexIndicesAttr.Get(&tetVertexIndices, timeCode);
    
    *surfaceFaceIndices = _ComputeSurfaceFaces(tetVertexIndices);
    return true;
}

PXR_NAMESPACE_CLOSE_SCOPE    
