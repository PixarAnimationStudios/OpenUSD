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
UsdSchemaType UsdGeomSubset::_GetSchemaType() const {
    return UsdGeomSubset::schemaType;
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

/* static */
std::vector<UsdGeomSubset> 
UsdGeomSubset::GetAllGeomSubsets(const UsdGeomImageable &geom)
{
    std::vector<UsdGeomSubset> result;

    for (const auto &childPrim : geom.GetPrim().GetChildren()) {
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

    for (const auto &childPrim : geom.GetPrim().GetChildren()) {
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
    
    for (const auto &childPrim : geom.GetPrim().GetChildren()) {
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
    std::vector<int> allIndices;
    allIndices.reserve(elementCount);
    for (size_t idx = 0 ; idx < elementCount ; ++idx) 
        allIndices.push_back(idx);

    VtIntArray result;
    result.reserve(elementCount - assignedIndices.size());
    std::set_difference(allIndices.begin(), allIndices.end(), 
        assignedIndices.begin(), assignedIndices.end(), 
        std::back_inserter(result));

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
                            "in GeomSubset at path <%s> at time %s.\n", index,
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

        // Ensure that the indices are in the range [0, faceCount).
        if (elementCount > 0 && 
            static_cast<size_t>(*indicesInFamily.rbegin()) >= elementCount) {
            valid = false;
            if (reason) {
                *reason += TfStringPrintf("Found one or more indices that are "
                    "greater than the element count %ld at time %s.\n", 
                    elementCount, TfStringify(t).c_str());
            }
        }
        if (*indicesInFamily.begin() < 0) {
            valid = false;
            if (reason) {
                *reason += TfStringPrintf("Found one or more indices that are "
                    "less than 0 at time %s.\n", TfStringify(t).c_str());
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
    std::vector<UsdGeomSubset> familySubsets =      
        UsdGeomSubset::GetGeomSubsets(geom, elementType, familyName);

    bool valid = true;

    size_t faceCount = 0;
    if (elementType == UsdGeomTokens->face) {
        // XXX: Use UsdGeomMesh schema to get the face count.
        UsdAttribute fvcAttr = geom.GetPrim().GetAttribute(
            UsdGeomTokens->faceVertexCounts);
        if (fvcAttr) {
            VtIntArray faceVertexCounts;
            if (fvcAttr.Get(&faceVertexCounts)) {
                faceCount = faceVertexCounts.size();
            }
        }
    } else {
        TF_CODING_ERROR("Unsupported element type '%s'.", 
                        elementType.GetText());
        return false;
    }

    if (faceCount == 0) {
        valid = false;
        if (reason) {
            *reason += TfStringPrintf("Unable to determine face-count for geom"
                " <%s>", geom.GetPath().GetText());
        }
    }

    TfToken familyType = GetFamilyType(geom, familyName);
    
    bool familyIsRestricted = (familyType != UsdGeomTokens->unrestricted);

    std::set<double> allTimeSamples;
    for (const auto &subset : familySubsets) {
        std::vector<double> subsetTimeSamples;
        subset.GetIndicesAttr().GetTimeSamples(&subsetTimeSamples);
        allTimeSamples.insert(subsetTimeSamples.begin(), subsetTimeSamples.end());
    }

    std::vector<UsdTimeCode> allTimeCodes(1, UsdTimeCode::Default());
    allTimeCodes.reserve(1 + allTimeSamples.size());
    for (const double t: allTimeSamples) {
        allTimeCodes.emplace_back(t);
    }

    for (const UsdTimeCode &t : allTimeCodes) {
        std::set<int> indicesInFamily;

        for (const UsdGeomSubset &subset : familySubsets) {
            VtIntArray subsetIndices;
            subset.GetIndicesAttr().Get(&subsetIndices, t);
            if (!familyIsRestricted) {
                indicesInFamily.insert(subsetIndices.begin(), subsetIndices.end());
            } else {
                for (const int index : subsetIndices) {
                    if (!indicesInFamily.insert(index).second) {
                        valid = false;
                        if (reason) {
                            *reason += TfStringPrintf("Found duplicate index %d "
                                "in GeomSubset at path <%s>.\n", index,
                                subset.GetPath().GetText());
                        }
                    }
                }
            }
        }

        // Make sure every index appears exactly once if it's a partition.
        if (familyType == UsdGeomTokens->partition && 
            indicesInFamily.size() != faceCount) 
        {
            valid = false;
            if (reason) {
                *reason += TfStringPrintf("Number of unique indices at time %s "
                    "does not match the face count %ld.", 
                    TfStringify(t).c_str(), faceCount);
            }
        }

        // Make sure the indices are valid and don't exceed the faceCount.
        if (faceCount > 0 && 
            static_cast<size_t>(*indicesInFamily.rbegin()) >= faceCount) {
            valid = false;
            if (reason) {
                *reason += TfStringPrintf("Found one or more indices that are "
                    "greater than the face-count %ld at time %s.\n", 
                    faceCount, TfStringify(t).c_str());
            }
        }

        // Ensure there are no negative indices.
        if (*indicesInFamily.begin() < 0) {
            valid = false;
            if (reason) {
                *reason += TfStringPrintf("Found one or more indices that are "
                    "less than 0 at time %s.\n", TfStringify(t).c_str());
            }
        }
    }

    return valid;
}

PXR_NAMESPACE_CLOSE_SCOPE
