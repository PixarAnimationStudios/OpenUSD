//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/usd/usdGeom/subset.h"
#include "pxr/usd/usd/schemaRegistry.h"
#include "pxr/usd/usd/typed.h"

#include "pxr/usd/sdf/types.h"
#include "pxr/usd/sdf/assetPath.h"

PXR_NAMESPACE_OPEN_SCOPE

// Register the schema with the TfType system.
TF_REGISTRY_FUNCTION(TfType)
{
    TfType::Define<UsdGeomSubset,
        TfType::Bases< UsdTyped > >();
    
    // Register the usd prim typename as an alias under UsdSchemaBase. This
    // enables one to call
    // TfType::Find<UsdSchemaBase>().FindDerivedByName("GeomSubset")
    // to find TfType<UsdGeomSubset>, which is how IsA queries are
    // answered.
    TfType::AddAlias<UsdSchemaBase, UsdGeomSubset>("GeomSubset");
}

/* virtual */
UsdGeomSubset::~UsdGeomSubset()
{
}

/* static */
UsdGeomSubset
UsdGeomSubset::Get(const UsdStagePtr &stage, const SdfPath &path)
{
    if (!stage) {
        TF_CODING_ERROR("Invalid stage");
        return UsdGeomSubset();
    }
    return UsdGeomSubset(stage->GetPrimAtPath(path));
}

/* static */
UsdGeomSubset
UsdGeomSubset::Define(
    const UsdStagePtr &stage, const SdfPath &path)
{
    static TfToken usdPrimTypeName("GeomSubset");
    if (!stage) {
        TF_CODING_ERROR("Invalid stage");
        return UsdGeomSubset();
    }
    return UsdGeomSubset(
        stage->DefinePrim(path, usdPrimTypeName));
}

/* virtual */
UsdSchemaKind UsdGeomSubset::_GetSchemaKind() const
{
    return UsdGeomSubset::schemaKind;
}

/* static */
const TfType &
UsdGeomSubset::_GetStaticTfType()
{
    static TfType tfType = TfType::Find<UsdGeomSubset>();
    return tfType;
}

/* static */
bool 
UsdGeomSubset::_IsTypedSchema()
{
    static bool isTyped = _GetStaticTfType().IsA<UsdTyped>();
    return isTyped;
}

/* virtual */
const TfType &
UsdGeomSubset::_GetTfType() const
{
    return _GetStaticTfType();
}

UsdAttribute
UsdGeomSubset::GetElementTypeAttr() const
{
    return GetPrim().GetAttribute(UsdGeomTokens->elementType);
}

UsdAttribute
UsdGeomSubset::CreateElementTypeAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdGeomTokens->elementType,
                       SdfValueTypeNames->Token,
                       /* custom = */ false,
                       SdfVariabilityUniform,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdGeomSubset::GetIndicesAttr() const
{
    return GetPrim().GetAttribute(UsdGeomTokens->indices);
}

UsdAttribute
UsdGeomSubset::CreateIndicesAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdGeomTokens->indices,
                       SdfValueTypeNames->IntArray,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdGeomSubset::GetFamilyNameAttr() const
{
    return GetPrim().GetAttribute(UsdGeomTokens->familyName);
}

UsdAttribute
UsdGeomSubset::CreateFamilyNameAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdGeomTokens->familyName,
                       SdfValueTypeNames->Token,
                       /* custom = */ false,
                       SdfVariabilityUniform,
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
UsdGeomSubset::GetSchemaAttributeNames(bool includeInherited)
{
    static TfTokenVector localNames = {
        UsdGeomTokens->elementType,
        UsdGeomTokens->indices,
        UsdGeomTokens->familyName,
    };
    static TfTokenVector allNames =
        _ConcatenateAttributeNames(
            UsdTyped::GetSchemaAttributeNames(true),
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

TF_DEFINE_PRIVATE_TOKENS(
    _tokens,

    // Namespace prefix of the attribute used to encode the familyType of a 
    // family of GeomSubsets below an imageable prim.
    (subsetFamily)

    // Base name of token-valued attribute used to encode the type of family 
    // that a collection of GeomSubsets with a common familyName belong to.
    (familyType)
);

/// Comparator for edges. Returns true if edge \p x is numerically less
/// than edge \p y. Considers the first element of the edge the principal
/// dimension for comparison, and compares the second elements if there is a tie.
namespace {
struct cmpPair {
    bool operator()(const GfVec2i &x, const GfVec2i &y) const { 
        if (x[0] == y[0]) {
            return x[1] < y[1];
        }
        return x[0] < y[0];
    }
};
}

/* static */
UsdGeomSubset 
UsdGeomSubset::CreateGeomSubset(
    const UsdGeomImageable &geom, 
    const TfToken &subsetName,
    const TfToken &elementType,
    const VtIntArray &indices,
    const TfToken &familyName,
    const TfToken &familyType)
{
    SdfPath subsetPath = geom.GetPath().AppendChild(subsetName);
    UsdGeomSubset subset = UsdGeomSubset::Define(geom.GetPrim().GetStage(), 
                                                 subsetPath);
 
    subset.GetElementTypeAttr().Set(elementType);
    subset.GetIndicesAttr().Set(indices);
    subset.GetFamilyNameAttr().Set(familyName);

    // XXX: would be nice to do this just once per family rather than once per 
    // subset that's created.
    if (!familyName.IsEmpty() && !familyType.IsEmpty()) {
        UsdGeomSubset::SetFamilyType(geom, familyName, familyType);
    }

    return subset;   
}

static UsdGeomSubset
_CreateUniqueGeomSubset(
    UsdStagePtr stage,
    const SdfPath& parentPath,
    const std::string& baseName)
{
    std::string name = baseName;
    size_t idx = 0;
    while (true) {
        SdfPath childPath = parentPath.AppendChild(TfToken(name));
        auto subsetPrim = stage->GetPrimAtPath(childPath);
        if (!subsetPrim) {
            return UsdGeomSubset::Define(stage, childPath);
        }
        idx++;
        name = TfStringPrintf("%s_%zu", baseName.c_str(), idx);
    }
    return UsdGeomSubset();
}

/* static */
UsdGeomSubset 
UsdGeomSubset::CreateUniqueGeomSubset(
    const UsdGeomImageable &geom, 
    const TfToken &subsetName,
    const TfToken &elementType,
    const VtIntArray &indices,
    const TfToken &familyName,
    const TfToken &familyType)
{
    UsdGeomSubset subset = _CreateUniqueGeomSubset(
        geom.GetPrim().GetStage(), geom.GetPath(), /*baseName*/ subsetName);

    subset.GetElementTypeAttr().Set(elementType);
    subset.GetIndicesAttr().Set(indices);
    subset.GetFamilyNameAttr().Set(familyName);

    // XXX: would be nice to do this just once per family rather than once per 
    // subset.
    if (!familyName.IsEmpty() && !familyType.IsEmpty()) {
        UsdGeomSubset::SetFamilyType(geom, familyName, familyType);
    }

    return subset;
}

// This is the same as UsdPrimDefaultPredicate except IsDefined is replaced 
// with HasDefiningSpecifier.
static Usd_PrimFlagsConjunction 
_GetGeomSubsetPredicate()
{
    return UsdPrimIsActive && UsdPrimHasDefiningSpecifier && UsdPrimIsLoaded && 
        !UsdPrimIsAbstract;
}

/* static */
std::vector<UsdGeomSubset> 
UsdGeomSubset::GetAllGeomSubsets(const UsdGeomImageable &geom)
{
    std::vector<UsdGeomSubset> result;

    for (const auto &childPrim : 
         geom.GetPrim().GetFilteredChildren(_GetGeomSubsetPredicate())) {
        if (childPrim.IsA<UsdGeomSubset>()) {
            result.emplace_back(childPrim);
        }
    }
    
    return result;
}

/* static */
std::vector<UsdGeomSubset> 
UsdGeomSubset::GetGeomSubsets(
    const UsdGeomImageable &geom,
    const TfToken &elementType,
    const TfToken &familyName)
{
    std::vector<UsdGeomSubset> result;

    for (const auto &childPrim : 
         geom.GetPrim().GetFilteredChildren(_GetGeomSubsetPredicate())) {
        if (childPrim.IsA<UsdGeomSubset>()) {
            UsdGeomSubset subset(childPrim);

            TfToken subsetElementType, subsetFamilyName;
            subset.GetElementTypeAttr().Get(&subsetElementType);
            subset.GetFamilyNameAttr().Get(&subsetFamilyName);

            if ((elementType.IsEmpty() || subsetElementType == elementType) 
                &&
                (familyName.IsEmpty() || subsetFamilyName == familyName))
            {
                result.emplace_back(childPrim);
            }
        }
    }
    
    return result;
}

/* static */
TfToken::Set
UsdGeomSubset::GetAllGeomSubsetFamilyNames(const UsdGeomImageable &geom)
{
    TfToken::Set familyNames;
    
    for (const auto &childPrim : 
         geom.GetPrim().GetFilteredChildren(_GetGeomSubsetPredicate())) {
        if (childPrim.IsA<UsdGeomSubset>()) {
            UsdGeomSubset subset(childPrim);
            TfToken subsetFamilyName;
            subset.GetFamilyNameAttr().Get(&subsetFamilyName);
            if (!subsetFamilyName.IsEmpty()) {
                familyNames.insert(subsetFamilyName);
            }
        }
    }
    
    return familyNames;
}

static TfToken 
_GetFamilyTypeAttrName(const TfToken &familyName)
{
    return TfToken(TfStringJoin(std::vector<std::string>{
        _tokens->subsetFamily.GetString(),
        familyName.GetString(),
        _tokens->familyType.GetString()}, ":"));
}

/* static */
bool
UsdGeomSubset::SetFamilyType(
    const UsdGeomImageable &geom, 
    const TfToken &familyName, 
    const TfToken &familyType)
{
    UsdAttribute familyTypeAttr = geom.GetPrim().CreateAttribute(
            _GetFamilyTypeAttrName(familyName),
            SdfValueTypeNames->Token,
            /* custom */ false, 
            SdfVariabilityUniform);
    return familyTypeAttr.Set(familyType);
}

/* static */
TfToken
UsdGeomSubset::GetFamilyType(
    const UsdGeomImageable &geom, 
    const TfToken &familyName)
{
    UsdAttribute familyTypeAttr = geom.GetPrim().GetAttribute(
            _GetFamilyTypeAttrName(familyName));
    TfToken familyType;
    familyTypeAttr.Get(&familyType);
    
    return familyType.IsEmpty() ? UsdGeomTokens->unrestricted : familyType;
}

static bool _GetEdgesFromPrim(const UsdGeomImageable &geom, const UsdTimeCode &t,
        std::set<GfVec2i, cmpPair> &edgesOnPrim) {
    const UsdAttribute fvcAttr = geom.GetPrim().GetAttribute(
                    UsdGeomTokens->faceVertexCounts);
    const UsdAttribute fviAttr = geom.GetPrim().GetAttribute(
            UsdGeomTokens->faceVertexIndices);
    if (!fvcAttr || !fviAttr) {
        return false;
    }

    VtIntArray faceVertexCounts;
    VtIntArray faceVertexIndices;
    if (!fvcAttr.Get(&faceVertexCounts, t) || 
        !fviAttr.Get(&faceVertexIndices, t)) {
        return false;
    }

    // Interpret edges from faces on the prim
    // Store edges in the form (lowIndex, highIndex)
    int fviIndex = 0;
    for (int count : faceVertexCounts) {
        for (int i = 0; i < count - 1; ++i) {
            int pointA = faceVertexIndices[fviIndex];
            int pointB = faceVertexIndices[fviIndex + 1];
            edgesOnPrim.insert(GfVec2i(std::min(pointA, pointB), std::max(pointA, pointB)));
            fviIndex++;
        }
        int pointA = faceVertexIndices[fviIndex];
        int pointB = faceVertexIndices[fviIndex - (count - 1)];
        edgesOnPrim.insert(GfVec2i(std::min(pointA, pointB), std::max(pointA, pointB)));
        fviIndex++;
    }
    return true;
}

static bool _GetAllPossibleSegments(const UsdGeomImageable &geom, const UsdTimeCode &t,
                                    std::set<GfVec2i, cmpPair> &allSegments) {
    const UsdAttribute curvesAttr = geom.GetPrim().GetAttribute(
            UsdGeomTokens->curveVertexCounts);
    if (!curvesAttr) {
        return false;
    }

    VtIntArray curveVertexCounts;
    if (!curvesAttr.Get(&curveVertexCounts, t)) {
        return false;
    }

    // Compute all possible (curveIndex, segmentIndex) pairs based on the number
    // of curves and the segment count of each curve.
    VtIntArray segmentCounts = UsdGeomBasisCurves(geom.GetPrim()).ComputeSegmentCounts(t);
    for (size_t i = 0; i < curveVertexCounts.size(); i++) {
        for (int j = 0; j < segmentCounts[i]; j++) {
            allSegments.insert(GfVec2i(i, j));
        }
    }
    return true;
}

static size_t
_GetElementCountAtTime(
    const UsdGeomImageable& geom,
    const TfToken& elementType,
    UsdTimeCode time,
    bool* isCountTimeVarying=nullptr)
{
    size_t elementCount = 0u;
    if (isCountTimeVarying) {
        *isCountTimeVarying = false;
    }

    if (elementType == UsdGeomTokens->face) {
        // XXX: Use UsdGeomMesh schema to get the face count.
        const UsdPrim prim = geom.GetPrim();
        if (prim.IsA<UsdGeomMesh>()) {
            const UsdAttribute fvcAttr = prim.GetAttribute(
                UsdGeomTokens->faceVertexCounts);
            if (fvcAttr) {
                VtIntArray faceVertexCounts;
                if (fvcAttr.Get(&faceVertexCounts, time)) {
                    elementCount = faceVertexCounts.size();
                }
                if (isCountTimeVarying) {
                    *isCountTimeVarying = fvcAttr.ValueMightBeTimeVarying();
                }
            }
        } else if (prim.IsA<UsdGeomTetMesh>()) {
            const UsdAttribute sfviAttr = prim.GetAttribute(
                UsdGeomTokens->surfaceFaceVertexIndices);
            
            if (sfviAttr) {
                VtVec3iArray surfaceFaceVertexIndices;
                if (sfviAttr.Get(&surfaceFaceVertexIndices, time)) {
                    elementCount = surfaceFaceVertexIndices.size();
                }
                if (isCountTimeVarying) {
                    *isCountTimeVarying = sfviAttr.ValueMightBeTimeVarying();
                }
            }
        }
    } else if (elementType == UsdGeomTokens->point) {
        const UsdAttribute ptAttr = geom.GetPrim().GetAttribute(
            UsdGeomTokens->points);
        if (ptAttr) {
            VtArray<GfVec3f> points;
            if (ptAttr.Get(&points, time)) {
                elementCount = points.size();
            } 
            if (isCountTimeVarying) {
                *isCountTimeVarying = ptAttr.ValueMightBeTimeVarying();
            }
        } 
    } else if (elementType == UsdGeomTokens->edge) {
        std::set<GfVec2i, cmpPair> edgesOnPrim;
        if (_GetEdgesFromPrim(geom, time, edgesOnPrim)) {
            elementCount = edgesOnPrim.size();

            const UsdAttribute fvcAttr = geom.GetPrim().GetAttribute(
                UsdGeomTokens->faceVertexCounts);
            const UsdAttribute fviAttr = geom.GetPrim().GetAttribute(
                UsdGeomTokens->faceVertexIndices);
            if (fvcAttr && fviAttr && isCountTimeVarying) {
                *isCountTimeVarying = fvcAttr.ValueMightBeTimeVarying() ||
                                        fviAttr.ValueMightBeTimeVarying();
            }
        }
    } else if (elementType == UsdGeomTokens->segment) {
        std::set<GfVec2i, cmpPair> segmentsOnPrim;
        const UsdAttribute curvesAttr = geom.GetPrim().GetAttribute(
            UsdGeomTokens->curveVertexCounts);
        if (curvesAttr) {
            const UsdGeomBasisCurves curves = UsdGeomBasisCurves(geom.GetPrim());
            VtIntArray segmentCounts = curves.ComputeSegmentCounts(time);
            for (int segmentCount : segmentCounts) {
                elementCount += segmentCount;
            }
            if (isCountTimeVarying) {
                *isCountTimeVarying = curvesAttr.ValueMightBeTimeVarying();
            }
        }
    } else if (elementType == UsdGeomTokens->tetrahedron) {
        const UsdAttribute tviAttr = geom.GetPrim().GetAttribute(
            UsdGeomTokens->tetVertexIndices);
        if (tviAttr) {
            VtVec4iArray tetVertexIndices;
            if (tviAttr.Get(&tetVertexIndices, time)) {
                elementCount = tetVertexIndices.size();
            }
            if (isCountTimeVarying) {
                *isCountTimeVarying = tviAttr.ValueMightBeTimeVarying();
            }
        }
    } else {
        TF_CODING_ERROR("Unsupported element type '%s'.",
                        elementType.GetText());
    }

    return elementCount;
}

static bool _ValidateGeomType(const UsdGeomImageable &geom, const TfToken &elementType) {
    const UsdPrim prim = geom.GetPrim();
    if (prim.IsA<UsdGeomMesh>()) {
        if (elementType != UsdGeomTokens->face && elementType != UsdGeomTokens->point 
            && elementType != UsdGeomTokens->edge) {
            TF_CODING_ERROR("Unsupported element type '%s' for prim type Mesh.",
                            elementType.GetText());
            return false;
        }
    } else if (prim.IsA<UsdGeomTetMesh>()) {
        if (elementType != UsdGeomTokens->face && elementType != UsdGeomTokens->tetrahedron) {
            TF_CODING_ERROR("Unsupported element type '%s' for prim type TetMesh.",
                            elementType.GetText());
            return false;
        }
    } else if (prim.IsA<UsdGeomBasisCurves>()) {
        if (elementType != UsdGeomTokens->segment) {
            TF_CODING_ERROR("Unsupported element type '%s' for prim type BasisCurves.",
                            elementType.GetText());
            return false;
        }
    } else {
        TF_CODING_ERROR("Unsupported prim type '%s'.",
                        elementType.GetText());
        return false;
    }
    return true;
}

VtVec2iArray UsdGeomSubset::_GetIndexPairs(const UsdTimeCode t, 
                                            bool preserveOrder=false) const {
    VtVec2iArray subsetIndices;
    VtIntArray indicesAttr;
    this->GetIndicesAttr().Get(&indicesAttr, t);

    subsetIndices.reserve(indicesAttr.size() / 2);
    for (size_t i = 0; i < indicesAttr.size() / 2; ++i) {
        int pointA = indicesAttr[2*i];
        int pointB = indicesAttr[2*i+1];
        if (preserveOrder) {
            subsetIndices.emplace_back(GfVec2i(pointA, pointB));
        } else {
            subsetIndices.emplace_back(GfVec2i(std::min(pointA, pointB),
                                            std::max(pointA, pointB)));
        }
    }

    return subsetIndices;
}

/* static */
VtIntArray
UsdGeomSubset::GetUnassignedIndices(
    const UsdGeomImageable &geom, 
    const TfToken &elementType,
    const TfToken &familyName,
    const UsdTimeCode &time /* UsdTimeCode::EarliestTime() */)
{
    VtIntArray result;
    if (!_ValidateGeomType(geom, elementType)) {
        return result;
    }

    std::vector<UsdGeomSubset> subsets = UsdGeomSubset::GetGeomSubsets(
            geom, elementType, familyName);

    const size_t elementCount = _GetElementCountAtTime(geom, elementType, time);

    if (elementType != UsdGeomTokens->edge && elementType != UsdGeomTokens->segment) {
        std::set<int> assignedIndices;
        for (const auto &subset : subsets) {
            VtIntArray indices;
            subset.GetIndicesAttr().Get(&indices, time);
            assignedIndices.insert(indices.begin(), indices.end());
        }

        // This is protection against the possibility that any of the subsets can
        // erroneously contain negative valued indices. Even though negative indices
        // are invalid, their presence breaks the assumption in the rest of this 
        // function that all indices are nonnegative. This can lead to crashes.
        // 
        // Negative indices should be extremely rare which is why it's better to 
        // check and remove them after the collection of assigned indices rather 
        // than during.
        while (!assignedIndices.empty() && *assignedIndices.begin() < 0) {
            assignedIndices.erase(assignedIndices.begin());
        }

        if (assignedIndices.empty()) {
            result.reserve(elementCount);
            for (size_t idx = 0 ; idx < elementCount ; ++idx) 
                result.push_back(idx);
        } else {
            std::vector<int> allIndices;
            allIndices.reserve(elementCount);
            for (size_t idx = 0 ; idx < elementCount ; ++idx) 
                allIndices.push_back(idx);

            const unsigned int lastAssigned = *assignedIndices.rbegin();
            if (elementCount > lastAssigned) {
                result.reserve(elementCount - assignedIndices.size());
            } else {
                result.reserve(std::min(
                    elementCount, (lastAssigned + 1) - assignedIndices.size()));
            }
            std::set_difference(allIndices.begin(), allIndices.end(), 
                assignedIndices.begin(), assignedIndices.end(), 
                std::back_inserter(result));
        }
    } else {
        std::set<GfVec2i, cmpPair> possiblePairs;
        bool preserveOrder = false;
        if (elementType == UsdGeomTokens->segment) {
            preserveOrder = true;
            if (!_GetAllPossibleSegments(geom, time, possiblePairs)) {
                TF_WARN("Unable to determine curve vertex count or segment count.");
                return result;
            }
        } else {
            if (!_GetEdgesFromPrim(geom, time, possiblePairs)) {
                TF_WARN("Unable to interpret edges from prim.");
                return result;
            }
        }
        VtVec2iArray pairsInFamily;
        for (const auto &subset : subsets) {
            VtVec2iArray subsetPairs = subset._GetIndexPairs(time, preserveOrder);
            std::move(subsetPairs.begin(), subsetPairs.end(), 
                    std::back_inserter(pairsInFamily));
        }

        struct cmpPair e;
        std::vector<GfVec2i> unassignedIndexPairs;
        std::set_difference(possiblePairs.begin(), possiblePairs.end(), 
                pairsInFamily.begin(), pairsInFamily.end(), 
                std::inserter(unassignedIndexPairs, unassignedIndexPairs.begin()), e);

        result.reserve(elementCount);
        for (GfVec2i indexPair : unassignedIndexPairs) {
            result.push_back(indexPair[0]);
            result.push_back(indexPair[1]);
        }
    }

    return result;
}

/* static */
VtIntArray
UsdGeomSubset::GetUnassignedIndices(
    const std::vector<UsdGeomSubset> &subsets,
    const size_t elementCount,
    const UsdTimeCode &time /* UsdTimeCode::EarliestTime() */)
{
    std::set<int> assignedIndices;
    for (const auto &subset : subsets) {
        VtIntArray indices;
        subset.GetIndicesAttr().Get(&indices, time);
        assignedIndices.insert(indices.begin(), indices.end());
    }

    // This is protection against the possibility that any of the subsets can
    // erroneously contain negative valued indices. Even though negative indices
    // are invalid, their presence breaks the assumption in the rest of this 
    // function that all indices are nonnegative. This can lead to crashes.
    // 
    // Negative indices should be extremely rare which is why it's better to 
    // check and remove them after the collection of assigned indices rather 
    // than during.
    while (!assignedIndices.empty() && *assignedIndices.begin() < 0) {
        assignedIndices.erase(assignedIndices.begin());
    }

    VtIntArray result;
    if (assignedIndices.empty()) {
        result.reserve(elementCount);
        for (size_t idx = 0 ; idx < elementCount ; ++idx) 
            result.push_back(idx);
    } else {
        std::vector<int> allIndices;
        allIndices.reserve(elementCount);
        for (size_t idx = 0 ; idx < elementCount ; ++idx) 
            allIndices.push_back(idx);

        const unsigned int lastAssigned = *assignedIndices.rbegin();
        if (elementCount > lastAssigned) {
            result.reserve(elementCount - assignedIndices.size());
        } else {
            result.reserve(std::min(
                elementCount, (lastAssigned + 1) - assignedIndices.size()));
        }
        std::set_difference(allIndices.begin(), allIndices.end(), 
            assignedIndices.begin(), assignedIndices.end(), 
            std::back_inserter(result));
    }

    return result;
}

/* static */
bool 
UsdGeomSubset::ValidateSubsets(
    const std::vector<UsdGeomSubset> &subsets,
    const size_t elementCount,
    const TfToken &familyType,
    std::string * const reason)
{
    if (subsets.empty())
        return true;

    TfToken elementType;
    subsets[0].GetElementTypeAttr().Get(&elementType);

    std::set<double> allTimeSamples;
    for (const auto &subset : subsets) {
        TfToken subsetElementType;
        subset.GetElementTypeAttr().Get(&subsetElementType);
        if (subsetElementType != elementType) {
            if (reason) {
                *reason = TfStringPrintf("Subset at path <%s> has elementType "
                    "%s, which does not match '%s'.", 
                    subset.GetPath().GetText(), subsetElementType.GetText(),
                    elementType.GetText());
            }
            // Return early if all the subsets don't have the same element type.
            return false;
        }

        std::vector<double> subsetTimeSamples;
        subset.GetIndicesAttr().GetTimeSamples(&subsetTimeSamples);
        allTimeSamples.insert(subsetTimeSamples.begin(), subsetTimeSamples.end());
    }

    std::vector<UsdTimeCode> allTimeCodes(1, UsdTimeCode::Default());
    allTimeCodes.reserve(1 + allTimeSamples.size());
    for (const double t: allTimeSamples) {
        allTimeCodes.emplace_back(t);
    }

    bool valid = true;
    for (const UsdTimeCode &t : allTimeCodes) {
        std::set<int> indicesInFamily;

        for (const UsdGeomSubset &subset : subsets) {
            VtIntArray subsetIndices;
            subset.GetIndicesAttr().Get(&subsetIndices, t);

            for (const int index : subsetIndices) {
                if (!indicesInFamily.insert(index).second &&
                    familyType != UsdGeomTokens->unrestricted) 
                {
                    valid = false;
                    if (reason) {
                        *reason += TfStringPrintf("Found overlapping index %d "
                            "in GeomSubset at path <%s> at time %s.", index,
                            subset.GetPath().GetText(), TfStringify(t).c_str());
                    }
                }
            }
        }

        // Make sure every index appears exactly once if it's a partition.
        if (familyType == UsdGeomTokens->partition &&
            indicesInFamily.size() != elementCount) {
            valid = false;
            if (reason) {
                *reason += TfStringPrintf("Number of unique indices at time %s "
                    "does not match the element count %ld.", 
                    TfStringify(t).c_str(), elementCount);
            }
        }

        // Ensure that the indices are in the range [0, elementCount).
        if (elementCount > 0 && 
            static_cast<size_t>(*indicesInFamily.rbegin()) >= elementCount) {
            valid = false;
            if (reason) {
                *reason += TfStringPrintf("Found one or more indices that are "
                    "greater than the element count %ld at time %s.",
                    elementCount, TfStringify(t).c_str());
            }
        }
        if (*indicesInFamily.begin() < 0) {
            valid = false;
            if (reason) {
                *reason += TfStringPrintf("Found one or more indices that are "
                    "less than 0 at time %s.", TfStringify(t).c_str());
            }
        }
    }

    return valid;
}

/* static */
bool
UsdGeomSubset::ValidateFamily(
    const UsdGeomImageable &geom,
    const TfToken &elementType,
    const TfToken &familyName,
    std::string * const reason)
{
    if (!_ValidateGeomType(geom, elementType)) {
        *reason += TfStringPrintf("Invalid geom type for elementType %s.",
                elementType.GetText());
        return false;
    }

    // Get subsets of *all* element types in the family so we can detect
    // whether any are authored that do not match the given element type.
    const std::vector<UsdGeomSubset> familySubsets =
        UsdGeomSubset::GetGeomSubsets(
            geom, /* elementType = */ TfToken(), familyName);
    const TfToken familyType = GetFamilyType(geom, familyName);
    const bool familyIsRestricted = (familyType != UsdGeomTokens->unrestricted);

    bool valid = true;

    bool isElementCountTimeVarying = false;
    const size_t earliestTimeElementCount = _GetElementCountAtTime(
        geom, elementType, UsdTimeCode::EarliestTime(),
        &isElementCountTimeVarying);
    if (!isElementCountTimeVarying && earliestTimeElementCount == 0u) {
        valid = false;
        if (reason) {
            *reason += TfStringPrintf("Unable to determine element count "
                "at earliest time for geom <%s>.", geom.GetPath().GetText());
        }
    }

    std::set<double> allTimeSamples;
    for (const auto &subset : familySubsets) {
        TfToken subsetElementType;
        subset.GetElementTypeAttr().Get(&subsetElementType);
        if (subsetElementType != elementType) {
            if (reason) {
                *reason = TfStringPrintf("GeomSubset at path <%s> has "
                    "elementType '%s', which does not match '%s'.",
                    subset.GetPath().GetText(), subsetElementType.GetText(),
                    elementType.GetText());
            }
            // Return early if all the subsets don't have the same element type.
            return false;
        }

        std::vector<double> subsetTimeSamples;
        subset.GetIndicesAttr().GetTimeSamples(&subsetTimeSamples);
        allTimeSamples.insert(subsetTimeSamples.begin(), subsetTimeSamples.end());
    }

    std::vector<UsdTimeCode> allTimeCodes(1, UsdTimeCode::Default());
    allTimeCodes.reserve(1 + allTimeSamples.size());
    for (const double t : allTimeSamples) {
        allTimeCodes.emplace_back(t);
    }

    bool hasIndicesAtAnyTime = false;

    for (const UsdTimeCode &t : allTimeCodes) {
        std::set<int> indicesInFamily;

        for (const UsdGeomSubset &subset : familySubsets) {
            VtIntArray subsetIndices;
            subset.GetIndicesAttr().Get(&subsetIndices, t);
            if (elementType == UsdGeomTokens->edge || 
                elementType == UsdGeomTokens->segment) {
                if (subsetIndices.size() % 2 != 0) {
                    valid = false;
                    if (reason) {
                        *reason += TfStringPrintf("Indices attribute has an "
                            "odd number of elements in GeomSubset at path <%s> "
                            "at time %s with elementType %s.",
                            subset.GetPath().GetText(), TfStringify(t).c_str(),
                            elementType.GetText());
                    }
                }
            }

            // Check for duplicate indices if the family is restricted.
            // This check is not applicable to edges and segments as the same  
            // index may be part of multiple distinct edges or segments.
            if (!familyIsRestricted || elementType == UsdGeomTokens->edge || 
                elementType == UsdGeomTokens->segment) {
                indicesInFamily.insert(subsetIndices.begin(), subsetIndices.end());
            } else {
                for (const int index : subsetIndices) {
                    if (!indicesInFamily.insert(index).second) {
                        valid = false;
                        if (reason) {
                            *reason += TfStringPrintf("Found duplicate index %d "
                                "in GeomSubset at path <%s> at time %s.", index,
                                subset.GetPath().GetText(), TfStringify(t).c_str());
                        }
                    }
                }
            }
        }

        // Topologically varying geometry may not have any elements at some
        // times. In that case, only mark the family invalid if it has indices
        // but we have no elements for this time.
        const size_t elementCount = isElementCountTimeVarying ?
            _GetElementCountAtTime(geom, elementType, t) :
            earliestTimeElementCount;
        if (!indicesInFamily.empty() &&
                isElementCountTimeVarying && elementCount == 0u) {
            valid = false;
            if (reason) {
                *reason += TfStringPrintf("Geometry <%s> has no elements at "
                    "time %s, but the \"%s\" GeomSubset family contains "
                    "indices.", geom.GetPath().GetText(),
                    TfStringify(t).c_str(), familyName.GetText());
            }
        }

        if (elementType != UsdGeomTokens->edge && elementType != UsdGeomTokens->segment) {
            // Make sure every index appears exactly once if it's a partition.
            if (familyType == UsdGeomTokens->partition &&
                indicesInFamily.size() != elementCount)
            {
                valid = false;
                if (reason) {
                    *reason += TfStringPrintf("Number of unique indices at time %s "
                        "does not match the element count %ld.",
                        TfStringify(t).c_str(), elementCount);
                }
            }
        } else {
            // Check for duplicate pairs if elementType is edge or segment
            std::set<GfVec2i, cmpPair> pairsInFamily;
            for (const UsdGeomSubset &subset : familySubsets) {
                // Segment index ordering matters, while edge index ordering does not:
                // (a, b) = (b, a) for edges.
                bool preserveOrder = (elementType == UsdGeomTokens->segment);
                VtVec2iArray subsetIndices = subset._GetIndexPairs(t, preserveOrder);

                if (!familyIsRestricted) {
                    pairsInFamily.insert(subsetIndices.begin(), subsetIndices.end());
                } else {
                    for (const GfVec2i &edge : subsetIndices) {
                        if (!pairsInFamily.insert(edge).second) {
                            valid = false;
                            if (reason) {
                                *reason += TfStringPrintf("Found duplicate %s (%d, %d) "
                                    "in GeomSubset at path <%s> at time %s.", 
                                    elementType.GetText(),edge[0], edge[1],
                                    subset.GetPath().GetText(), TfStringify(t).c_str());
                            }
                        }
                    }
                }
            }

            std::set<GfVec2i, cmpPair> possiblePairs;
            if (elementType == UsdGeomTokens->edge) {
                // Check if every edge in the subset is a valid edge on the prim
                if (_GetEdgesFromPrim(geom, t, possiblePairs)) {
                    struct cmpPair e;
                    bool hasValidEdges = std::includes(possiblePairs.begin(), possiblePairs.end(), 
                            pairsInFamily.begin(), pairsInFamily.end(), e);
                    
                    if (!hasValidEdges) {
                        valid = false;
                        if (reason) {
                            *reason += TfStringPrintf("At least one edge in family %s at time %s "
                                "does not exist on the parent prim.",
                                familyName.GetText(), TfStringify(t).c_str());
                        }
                    }
                }
            } else if (elementType == UsdGeomTokens->segment) {
                _GetAllPossibleSegments(geom, t, possiblePairs);

                const UsdAttribute curvesAttr = geom.GetPrim().GetAttribute(
                    UsdGeomTokens->curveVertexCounts);
                if (curvesAttr) {
                    VtIntArray curveVertexCounts;
                    if (curvesAttr.Get(&curveVertexCounts, t)) {
                        VtIntArray segmentCounts = 
                            UsdGeomBasisCurves(geom.GetPrim()).ComputeSegmentCounts(t);

                        // Verify curveIndex is within bound of curveVertexCounts and
                        // that segmentIndex is within bound of segmentCounts
                        for (const GfVec2i& segment : pairsInFamily) {
                            int curveIndex = segment[0];
                            if (curveIndex < 0) {
                                // Verification that all indices are >= 0 happens later,
                                // so skip without producing errors for now
                                continue;
                            }
                            if (curveIndex >= static_cast<int>(curveVertexCounts.size())) {
                                valid = false;
                                if (reason) {
                                    *reason += TfStringPrintf("Found one or more indices that are "
                                        "greater than the curve vertex count %lu at time %s.",
                                        curveVertexCounts.size(), TfStringify(t).c_str());
                                }
                            } else if (segment[1] >= segmentCounts[curveIndex]) {
                                valid = false;
                                if (reason) {
                                    *reason += TfStringPrintf("Found one or more indices that are "
                                        "greater than the segment count %d at time %s.",
                                        segmentCounts[curveIndex], TfStringify(t).c_str());
                                }
                            }
                        }
                    }
                }
            }

            // Make sure every edge/segment appears exactly once if it's a partition.
            if (familyType == UsdGeomTokens->partition)
            {
                struct cmpPair e;
                std::vector<GfVec2i> res;
                std::set_difference(possiblePairs.begin(), possiblePairs.end(), 
                        pairsInFamily.begin(), pairsInFamily.end(), 
                        std::inserter(res, res.begin()), e);
            
                if (res.size() != 0) {
                    valid = false;
                    if (reason) {
                        *reason += TfStringPrintf("Number of unique indices at time %s "
                            "does not match the element count %ld.",
                            TfStringify(t).c_str(), elementCount);
                    }
                }
            }
        }

        if (indicesInFamily.empty()) {
            // Skip the bounds checking below if there are no indices at this
            // time. This does not invalidate the subset family.
            continue;
        }

        hasIndicesAtAnyTime = true;

        // Make sure the indices are valid and don't exceed the elementCount.
        if (elementType != UsdGeomTokens->segment) {
            const int lastIndex = *indicesInFamily.rbegin();
            if (elementCount > 0 && lastIndex >= 0 &&
                static_cast<size_t>(lastIndex) >= elementCount) {
                valid = false;
                if (reason) {
                    *reason += TfStringPrintf("Found one or more indices that are "
                        "greater than the element count %ld at time %s.",
                        elementCount, TfStringify(t).c_str());
                }
            }
        }

        // Ensure there are no negative indices.
        if (*indicesInFamily.begin() < 0) {
            valid = false;
            if (reason) {
                *reason += TfStringPrintf("Found one or more indices that are "
                    "less than 0 at time %s.", TfStringify(t).c_str());
            }
        }
    }

    if (!hasIndicesAtAnyTime) {
        valid = false;
        if (reason) {
            *reason += TfStringPrintf("No indices in family at any time.");
        }
    }

    return valid;
}

PXR_NAMESPACE_CLOSE_SCOPE
