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
#include "pxr/usd/usdPhysics/collisionGroup.h"
#include "pxr/usd/usd/schemaRegistry.h"
#include "pxr/usd/usd/typed.h"

#include "pxr/usd/sdf/types.h"
#include "pxr/usd/sdf/assetPath.h"

PXR_NAMESPACE_OPEN_SCOPE

// Register the schema with the TfType system.
TF_REGISTRY_FUNCTION(TfType)
{
    TfType::Define<UsdPhysicsCollisionGroup,
        TfType::Bases< UsdTyped > >();
    
    // Register the usd prim typename as an alias under UsdSchemaBase. This
    // enables one to call
    // TfType::Find<UsdSchemaBase>().FindDerivedByName("PhysicsCollisionGroup")
    // to find TfType<UsdPhysicsCollisionGroup>, which is how IsA queries are
    // answered.
    TfType::AddAlias<UsdSchemaBase, UsdPhysicsCollisionGroup>("PhysicsCollisionGroup");
}

/* virtual */
UsdPhysicsCollisionGroup::~UsdPhysicsCollisionGroup()
{
}

/* static */
UsdPhysicsCollisionGroup
UsdPhysicsCollisionGroup::Get(const UsdStagePtr &stage, const SdfPath &path)
{
    if (!stage) {
        TF_CODING_ERROR("Invalid stage");
        return UsdPhysicsCollisionGroup();
    }
    return UsdPhysicsCollisionGroup(stage->GetPrimAtPath(path));
}

/* static */
UsdPhysicsCollisionGroup
UsdPhysicsCollisionGroup::Define(
    const UsdStagePtr &stage, const SdfPath &path)
{
    static TfToken usdPrimTypeName("PhysicsCollisionGroup");
    if (!stage) {
        TF_CODING_ERROR("Invalid stage");
        return UsdPhysicsCollisionGroup();
    }
    return UsdPhysicsCollisionGroup(
        stage->DefinePrim(path, usdPrimTypeName));
}

/* virtual */
UsdSchemaKind UsdPhysicsCollisionGroup::_GetSchemaKind() const
{
    return UsdPhysicsCollisionGroup::schemaKind;
}

/* static */
const TfType &
UsdPhysicsCollisionGroup::_GetStaticTfType()
{
    static TfType tfType = TfType::Find<UsdPhysicsCollisionGroup>();
    return tfType;
}

/* static */
bool 
UsdPhysicsCollisionGroup::_IsTypedSchema()
{
    static bool isTyped = _GetStaticTfType().IsA<UsdTyped>();
    return isTyped;
}

/* virtual */
const TfType &
UsdPhysicsCollisionGroup::_GetTfType() const
{
    return _GetStaticTfType();
}

UsdAttribute
UsdPhysicsCollisionGroup::GetMergeGroupNameAttr() const
{
    return GetPrim().GetAttribute(UsdPhysicsTokens->physicsMergeGroup);
}

UsdAttribute
UsdPhysicsCollisionGroup::CreateMergeGroupNameAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdPhysicsTokens->physicsMergeGroup,
                       SdfValueTypeNames->String,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdPhysicsCollisionGroup::GetInvertFilteredGroupsAttr() const
{
    return GetPrim().GetAttribute(UsdPhysicsTokens->physicsInvertFilteredGroups);
}

UsdAttribute
UsdPhysicsCollisionGroup::CreateInvertFilteredGroupsAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdPhysicsTokens->physicsInvertFilteredGroups,
                       SdfValueTypeNames->Bool,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

UsdRelationship
UsdPhysicsCollisionGroup::GetFilteredGroupsRel() const
{
    return GetPrim().GetRelationship(UsdPhysicsTokens->physicsFilteredGroups);
}

UsdRelationship
UsdPhysicsCollisionGroup::CreateFilteredGroupsRel() const
{
    return GetPrim().CreateRelationship(UsdPhysicsTokens->physicsFilteredGroups,
                       /* custom = */ false);
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
UsdPhysicsCollisionGroup::GetSchemaAttributeNames(bool includeInherited)
{
    static TfTokenVector localNames = {
        UsdPhysicsTokens->physicsMergeGroup,
        UsdPhysicsTokens->physicsInvertFilteredGroups,
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

#include "pxr/usd/usd/primRange.h"
#include <algorithm>

PXR_NAMESPACE_OPEN_SCOPE

UsdCollectionAPI
UsdPhysicsCollisionGroup::GetCollidersCollectionAPI() const
{
    return UsdCollectionAPI(GetPrim(), UsdPhysicsTokens->colliders);
}

const SdfPathVector&
UsdPhysicsCollisionGroup::CollisionGroupTable::GetCollisionGroups() const
{
    return groups;
}

bool
UsdPhysicsCollisionGroup::CollisionGroupTable::IsCollisionEnabled(int idxA, int idxB) const
{
    if (idxA >= 0 && idxB >= 0 && idxA < groups.size() && idxB < groups.size())
    {
        int minGroup = std::min(idxA, idxB);
        int maxGroup = std::max(idxA, idxB);
        int numSkippedEntries = (minGroup * minGroup + minGroup) / 2;
        return enabled[minGroup * groups.size() - numSkippedEntries + maxGroup];
    }

    // If the groups aren't in the table or we've been passed invalid groups,
    // return true, as groups will collide by default.
    return true;
}

bool
UsdPhysicsCollisionGroup::CollisionGroupTable::IsCollisionEnabled(const SdfPath& primA, const SdfPath& primB) const
{
    auto a = std::find(groups.begin(), groups.end(), primA);
    auto b = std::find(groups.begin(), groups.end(), primB);
    return IsCollisionEnabled(std::distance(groups.begin(), a), std::distance(groups.begin(), b));
}


UsdPhysicsCollisionGroup::CollisionGroupTable
UsdPhysicsCollisionGroup::ComputeCollisionGroupTable(const UsdStage& stage)
{
    // First, collect all the collision groups, as we want to iterate over them several times
    std::vector<UsdPhysicsCollisionGroup> allSceneGroups;
    const UsdPrimRange range(stage.GetPseudoRoot());
    for (const UsdPrim& prim : range)
    {
        if (prim.IsA<UsdPhysicsCollisionGroup>())
        {
            allSceneGroups.emplace_back(prim);
        }
    }

    // Assign a number to every collision group; in order to handle merge groups, some prims will have non-unique indices
    std::map<SdfPath, size_t> primPathToIndex; // Using SdfPath, rather than prim, as the filtered groups rel gives us a path.
    std::map<std::string, size_t> mergeGroupNameToIndex;
    int nextPrimId = 0;

    for (const UsdPhysicsCollisionGroup& collisionGroup : allSceneGroups)
    {
        UsdAttribute mergeGroupAttr = collisionGroup.GetMergeGroupNameAttr();

        // If the group doesn't have a merge group, we can just add it to the table:
        if (!mergeGroupAttr.IsAuthored())
        {
            primPathToIndex[collisionGroup.GetPath()] = nextPrimId;
            nextPrimId++;
        }
        else
        {
            std::string mergeGroupName;
            mergeGroupAttr.Get(&mergeGroupName);
            auto foundGroup = mergeGroupNameToIndex.find(mergeGroupName);
            if (foundGroup != mergeGroupNameToIndex.end())
            {
                primPathToIndex[collisionGroup.GetPath()] = foundGroup->second;
            }
            else
            {
                mergeGroupNameToIndex[mergeGroupName] = nextPrimId;
                primPathToIndex[collisionGroup.GetPath()] = nextPrimId;
                nextPrimId++;
            }
        }
    }

    // Now, we've seen "nextPrimId" different unique groups after accounting for the merge groups.
    // Calculate the collision table for those groups.

    // First, resize the table and set to default-collide. We're only going to use the upper diagonal,
    // as the table is symmetric:
    std::vector<bool> mergedTable;
    mergedTable.resize( ((nextPrimId + 1) * nextPrimId) / 2, true);

    for (const UsdPhysicsCollisionGroup& groupA : allSceneGroups)
    {
        int groupAIdx = primPathToIndex[groupA.GetPath()];

        // Extract the indices for each filtered group in "prim":
        std::vector<int> filteredGroupIndices;
        {
            UsdRelationship filteredGroups = groupA.GetFilteredGroupsRel();
            SdfPathVector filteredTargets;
            filteredGroups.GetTargets(&filteredTargets);
            for(const SdfPath& path : filteredTargets)
            {
                filteredGroupIndices.push_back(primPathToIndex[path]);
            }
        }

        bool invertedFilter;
        UsdAttribute invertedAttr = groupA.GetInvertFilteredGroupsAttr();
        invertedAttr.Get(&invertedFilter);

        // Now, we are ready to apply the filter rules for "prim":
        if (!invertedAttr.IsAuthored() || !invertedFilter)
        {
            // This is the usual case; collisions against all the filteredTargets should be disabled
            for (int groupBIdx : filteredGroupIndices)
            {
                // Disable aIdx -v- bIdx
                int minGroup = std::min(groupAIdx, groupBIdx);
                int maxGroup = std::max(groupAIdx, groupBIdx);
                int numSkippedEntries = (minGroup * minGroup + minGroup) / 2;
                mergedTable[minGroup * nextPrimId - numSkippedEntries  + maxGroup] = false;
            }
        }
        else
        {
            // This is the less common case; disable collisions against all groups except the filtered targets
            std::set<int> requestedGroups(filteredGroupIndices.begin(), filteredGroupIndices.end());
            for(int groupBIdx = 0; groupBIdx < nextPrimId; groupBIdx++)
            {
                if (requestedGroups.find(groupBIdx) == requestedGroups.end())
                {
                    // Disable aIdx -v- bIdx
                    int minGroup = std::min(groupAIdx, groupBIdx);
                    int maxGroup = std::max(groupAIdx, groupBIdx);
                    int numSkippedEntries = (minGroup * minGroup + minGroup) / 2;
                    mergedTable[minGroup * nextPrimId - numSkippedEntries + maxGroup] = false;
                }
            }
        }
    }

    // Finally, we can calculate the output table, based on the merged table
    CollisionGroupTable res;
    for (const UsdPhysicsCollisionGroup& collisionGroup : allSceneGroups)
    {
        res.groups.push_back(collisionGroup.GetPrim().GetPrimPath());
    }
    res.enabled.resize(((res.groups.size() + 1) * res.groups.size()) / 2, true);

    // Iterate over every group A and B, and use the merged table to determine if they collide.
    for (int iA = 0; iA < allSceneGroups.size(); iA++)
    {
        for (int iB = iA; iB < allSceneGroups.size(); iB++)
        {
            // Determine if the groups at iA and iB collide:
            int groupAId = primPathToIndex[allSceneGroups[iA].GetPath()];
            int groupBId = primPathToIndex[allSceneGroups[iB].GetPath()];
            int numSkippedMergeEntries = (groupAId * groupAId + groupAId) / 2;
            bool enabledInMergeTable = mergedTable[groupAId * nextPrimId - numSkippedMergeEntries + groupBId];

            // And use that to populate the output table:
            int minGroup = std::min(iA, iB);
            int maxGroup = std::max(iA, iB);
            int numSkippedEntries = (minGroup * minGroup + minGroup) / 2;
            res.enabled[minGroup * allSceneGroups.size() - numSkippedEntries + maxGroup] = enabledInMergeTable;
        }
    }

    return res;
}

PXR_NAMESPACE_CLOSE_SCOPE
