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
#include "pxr/usd/usdGeom/pointInstancer.h"
#include "pxr/usd/usd/schemaRegistry.h"
#include "pxr/usd/usd/typed.h"

#include "pxr/usd/sdf/types.h"
#include "pxr/usd/sdf/assetPath.h"

PXR_NAMESPACE_OPEN_SCOPE

// Register the schema with the TfType system.
TF_REGISTRY_FUNCTION(TfType)
{
    TfType::Define<UsdGeomPointInstancer,
        TfType::Bases< UsdGeomBoundable > >();
    
    // Register the usd prim typename as an alias under UsdSchemaBase. This
    // enables one to call
    // TfType::Find<UsdSchemaBase>().FindDerivedByName("PointInstancer")
    // to find TfType<UsdGeomPointInstancer>, which is how IsA queries are
    // answered.
    TfType::AddAlias<UsdSchemaBase, UsdGeomPointInstancer>("PointInstancer");
}

/* virtual */
UsdGeomPointInstancer::~UsdGeomPointInstancer()
{
}

/* static */
UsdGeomPointInstancer
UsdGeomPointInstancer::Get(const UsdStagePtr &stage, const SdfPath &path)
{
    if (!stage) {
        TF_CODING_ERROR("Invalid stage");
        return UsdGeomPointInstancer();
    }
    return UsdGeomPointInstancer(stage->GetPrimAtPath(path));
}

/* static */
UsdGeomPointInstancer
UsdGeomPointInstancer::Define(
    const UsdStagePtr &stage, const SdfPath &path)
{
    static TfToken usdPrimTypeName("PointInstancer");
    if (!stage) {
        TF_CODING_ERROR("Invalid stage");
        return UsdGeomPointInstancer();
    }
    return UsdGeomPointInstancer(
        stage->DefinePrim(path, usdPrimTypeName));
}

/* static */
const TfType &
UsdGeomPointInstancer::_GetStaticTfType()
{
    static TfType tfType = TfType::Find<UsdGeomPointInstancer>();
    return tfType;
}

/* static */
bool 
UsdGeomPointInstancer::_IsTypedSchema()
{
    static bool isTyped = _GetStaticTfType().IsA<UsdTyped>();
    return isTyped;
}

/* virtual */
const TfType &
UsdGeomPointInstancer::_GetTfType() const
{
    return _GetStaticTfType();
}

UsdAttribute
UsdGeomPointInstancer::GetProtoIndicesAttr() const
{
    return GetPrim().GetAttribute(UsdGeomTokens->protoIndices);
}

UsdAttribute
UsdGeomPointInstancer::CreateProtoIndicesAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdGeomTokens->protoIndices,
                       SdfValueTypeNames->IntArray,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdGeomPointInstancer::GetIdsAttr() const
{
    return GetPrim().GetAttribute(UsdGeomTokens->ids);
}

UsdAttribute
UsdGeomPointInstancer::CreateIdsAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdGeomTokens->ids,
                       SdfValueTypeNames->Int64Array,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdGeomPointInstancer::GetPositionsAttr() const
{
    return GetPrim().GetAttribute(UsdGeomTokens->positions);
}

UsdAttribute
UsdGeomPointInstancer::CreatePositionsAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdGeomTokens->positions,
                       SdfValueTypeNames->Point3fArray,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdGeomPointInstancer::GetOrientationsAttr() const
{
    return GetPrim().GetAttribute(UsdGeomTokens->orientations);
}

UsdAttribute
UsdGeomPointInstancer::CreateOrientationsAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdGeomTokens->orientations,
                       SdfValueTypeNames->QuathArray,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdGeomPointInstancer::GetScalesAttr() const
{
    return GetPrim().GetAttribute(UsdGeomTokens->scales);
}

UsdAttribute
UsdGeomPointInstancer::CreateScalesAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdGeomTokens->scales,
                       SdfValueTypeNames->Float3Array,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdGeomPointInstancer::GetVelocitiesAttr() const
{
    return GetPrim().GetAttribute(UsdGeomTokens->velocities);
}

UsdAttribute
UsdGeomPointInstancer::CreateVelocitiesAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdGeomTokens->velocities,
                       SdfValueTypeNames->Vector3fArray,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdGeomPointInstancer::GetAngularVelocitiesAttr() const
{
    return GetPrim().GetAttribute(UsdGeomTokens->angularVelocities);
}

UsdAttribute
UsdGeomPointInstancer::CreateAngularVelocitiesAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdGeomTokens->angularVelocities,
                       SdfValueTypeNames->Vector3fArray,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdGeomPointInstancer::GetInvisibleIdsAttr() const
{
    return GetPrim().GetAttribute(UsdGeomTokens->invisibleIds);
}

UsdAttribute
UsdGeomPointInstancer::CreateInvisibleIdsAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdGeomTokens->invisibleIds,
                       SdfValueTypeNames->Int64Array,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdGeomPointInstancer::GetPrototypeDrawModeAttr() const
{
    return GetPrim().GetAttribute(UsdGeomTokens->prototypeDrawMode);
}

UsdAttribute
UsdGeomPointInstancer::CreatePrototypeDrawModeAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdGeomTokens->prototypeDrawMode,
                       SdfValueTypeNames->Token,
                       /* custom = */ false,
                       SdfVariabilityUniform,
                       defaultValue,
                       writeSparsely);
}

UsdRelationship
UsdGeomPointInstancer::GetPrototypesRel() const
{
    return GetPrim().GetRelationship(UsdGeomTokens->prototypes);
}

UsdRelationship
UsdGeomPointInstancer::CreatePrototypesRel() const
{
    return GetPrim().CreateRelationship(UsdGeomTokens->prototypes,
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
UsdGeomPointInstancer::GetSchemaAttributeNames(bool includeInherited)
{
    static TfTokenVector localNames = {
        UsdGeomTokens->protoIndices,
        UsdGeomTokens->ids,
        UsdGeomTokens->positions,
        UsdGeomTokens->orientations,
        UsdGeomTokens->scales,
        UsdGeomTokens->velocities,
        UsdGeomTokens->angularVelocities,
        UsdGeomTokens->invisibleIds,
        UsdGeomTokens->prototypeDrawMode,
    };
    static TfTokenVector allNames =
        _ConcatenateAttributeNames(
            UsdGeomBoundable::GetSchemaAttributeNames(true),
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

#include "pxr/base/tf/enum.h"

PXR_NAMESPACE_OPEN_SCOPE

TF_REGISTRY_FUNCTION(TfEnum)
{
    TF_ADD_ENUM_NAME(UsdGeomPointInstancer::IncludeProtoXform);
    TF_ADD_ENUM_NAME(UsdGeomPointInstancer::ExcludeProtoXform);
    TF_ADD_ENUM_NAME(UsdGeomPointInstancer::ApplyMask);
    TF_ADD_ENUM_NAME(UsdGeomPointInstancer::IgnoreMask);
}

static
bool 
_SetOrMergeOverOp(std::vector<int64_t> const &items, SdfListOpType op,
                  UsdPrim const &prim)
{
    SdfInt64ListOp  proposed, current;
    UsdStagePtr stage = prim.GetStage();
    UsdEditTarget editTarget = stage->GetEditTarget();
    SdfPrimSpecHandle  primSpec = 
        editTarget.GetPrimSpecForScenePath(prim.GetPath());
    
    if (primSpec){
        VtValue  existingOp = primSpec->GetInfo(UsdGeomTokens->inactiveIds);
        if (existingOp.IsHolding<SdfInt64ListOp>()){
            current = existingOp.UncheckedGet<SdfInt64ListOp>();
        }
    }

    proposed.SetItems(items, op);
    if (current.IsExplicit()){
        std::vector<int64_t> explicitItems = current.GetExplicitItems();
        proposed.ApplyOperations(&explicitItems);
        current.SetExplicitItems(explicitItems);
    }
    else {
        // We can't use ApplyOperations on an extant, non-explicit listOp
        // because the result is always flat and explicit.
        current.ComposeOperations(proposed, op);
        // ComposeOperations() is too narrow in functionality - it does not
        // consider that if we "remove over" an existing set of added items,
        // we need to additionally ensure the removed items get removed
        // from the added in current, since when applying ops, we first
        // remove, then add.  Bug #139215 filed to track; when it gets fixed
        // we can remove this code!
        if (op == SdfListOpTypeDeleted){
            std::vector<int64_t> addedItems = current.GetAddedItems();
            if (!addedItems.empty()){
                std::set<int64_t> toRemove(items.begin(), items.end());
                std::vector<int64_t> newAdded;
                newAdded.reserve(addedItems.size());
                for (auto elt : addedItems){
                    if (!toRemove.count(elt))
                        newAdded.push_back(elt);
                }
                if (newAdded.size() != addedItems.size())
                    current.SetAddedItems(newAdded);
            }
        }
        else if (op == SdfListOpTypeAdded){
            std::vector<int64_t> deletedItems = current.GetDeletedItems();
            if (!deletedItems.empty()){
                std::set<int64_t> toAdd(items.begin(), items.end());
                std::vector<int64_t> newDeleted;
                newDeleted.reserve(deletedItems.size());
                for (auto elt : deletedItems){
                    if (!toAdd.count(elt))
                        newDeleted.push_back(elt);
                }
                if (newDeleted.size() != deletedItems.size())
                    current.SetDeletedItems(newDeleted);
            }
        }
    }
    return prim.SetMetadata(UsdGeomTokens->inactiveIds, current);
}

bool
UsdGeomPointInstancer::ActivateId(int64_t id) const
{
    std::vector<int64_t> toRemove(1, id);
    return _SetOrMergeOverOp(toRemove, SdfListOpTypeDeleted, GetPrim());
}

bool
UsdGeomPointInstancer::ActivateIds(VtInt64Array const &ids) const
{
    std::vector<int64_t> toRemove(ids.begin(), ids.end());
    return _SetOrMergeOverOp(toRemove, SdfListOpTypeDeleted, GetPrim());
}

bool
UsdGeomPointInstancer::ActivateAllIds() const
{
    SdfInt64ListOp  op;
    op.SetExplicitItems(std::vector<int64_t>());
    
    return GetPrim().SetMetadata(UsdGeomTokens->inactiveIds, op);
}

bool
UsdGeomPointInstancer::DeactivateId(int64_t id) const
{
    std::vector<int64_t> toAdd(1, id);
    return _SetOrMergeOverOp(toAdd, SdfListOpTypeAdded, GetPrim());
}

bool
UsdGeomPointInstancer::DeactivateIds(VtInt64Array const &ids) const
{
    std::vector<int64_t> toAdd(ids.begin(), ids.end());
    return _SetOrMergeOverOp(toAdd, SdfListOpTypeAdded, GetPrim());
}

bool
UsdGeomPointInstancer::VisId(int64_t id, UsdTimeCode const &time) const
{
    VtInt64Array ids(1);
    
    ids.push_back(id);
    return VisIds(ids, time);
}

bool
UsdGeomPointInstancer::VisIds(VtInt64Array const &ids, UsdTimeCode const &time) const
{
    VtInt64Array invised;

    if (!GetInvisibleIdsAttr().Get(&invised, time))
        return true;

    std::set<int64_t>  invisSet(invised.begin(), invised.end());
    size_t numRemoved = 0;

    for (int64_t id : ids){
        numRemoved += invisSet.erase(id);
    }

    if (numRemoved){
        invised.clear();
        invised.reserve(invisSet.size());
        for ( int64_t id : invisSet )
            invised.push_back(id);
    }

    return CreateInvisibleIdsAttr().Set(invised, time);
}

bool
UsdGeomPointInstancer::VisAllIds(UsdTimeCode const &time) const
{
    VtInt64Array invised(0);

    if (GetInvisibleIdsAttr().HasAuthoredValueOpinion())
        return CreateInvisibleIdsAttr().Set(invised, time);

    return true;
}

bool
UsdGeomPointInstancer::InvisId(int64_t id, UsdTimeCode const &time) const
{
    VtInt64Array ids(1);
    
    ids.push_back(id);
    return InvisIds(ids, time);
}

bool
UsdGeomPointInstancer::InvisIds(VtInt64Array const &ids, UsdTimeCode const &time) const
{
    VtInt64Array invised;

    if (!GetInvisibleIdsAttr().Get(&invised, time))
        return true;

    std::set<int64_t>  invisSet(invised.begin(), invised.end());

    for (int64_t id : ids){
        if (invisSet.find(id) == invisSet.end())
            invised.push_back(id);
    }

    return CreateInvisibleIdsAttr().Set(invised, time);
}

std::vector<bool> 
UsdGeomPointInstancer::ComputeMaskAtTime(UsdTimeCode time, 
                                         VtInt64Array const *ids) const
{
    VtInt64Array       idVals, invisedIds;
    std::vector<bool>  mask;
    SdfInt64ListOp     inactiveIdsListOp;

    // XXX Note we could be doing all three fetches in parallel
    GetPrim().GetMetadata(UsdGeomTokens->inactiveIds, &inactiveIdsListOp);
    std::vector<int64_t> inactiveIds = inactiveIdsListOp.GetExplicitItems();
    GetInvisibleIdsAttr().Get(&invisedIds, time);
    if (inactiveIds.size() > 0 || invisedIds.size() > 0){
        bool anyPruned = false;
        std::set<int64_t>  maskedIds(inactiveIds.begin(), inactiveIds.end());
        maskedIds.insert(invisedIds.begin(), invisedIds.end());
        if (!ids){
            if (GetIdsAttr().Get(&idVals, time)){
                ids = &idVals;
            }
            if (!ids){
                VtIntArray  protoIndices;
                if (!GetProtoIndicesAttr().Get(&protoIndices, time)){
                    // not a functional PointInstancer... just return 
                    // trivial pass
                    return mask;
                }
                size_t numInstances = protoIndices.size();
                idVals.reserve(numInstances);
                for (size_t i = 0; i < numInstances; ++i) {
                    idVals.push_back(i);
                }
                ids = &idVals;
            }
        }

        mask.reserve(ids->size());
        for (int64_t id : *ids){
            bool pruned = (maskedIds.find(id) != maskedIds.end());
            anyPruned = anyPruned || pruned;
            mask.push_back(!pruned);
        }
        
        if (!anyPruned){
            mask.resize(0);
        }
    }

    return mask;
}

bool
UsdGeomPointInstancer::ComputeInstanceTransformsAtTime(
    VtArray<GfMatrix4d>* xforms,
    UsdTimeCode time,
    UsdTimeCode baseTime,
    ProtoXformInclusion doProtoXforms,
    MaskApplication applyMask) const
{
    /// XXX STUB!
    (void)xforms;
    (void)time;
    (void)baseTime;
    (void)doProtoXforms;
    (void)applyMask;

    printf("Computing Instance transforms\n");

    return true;
}

PXR_NAMESPACE_CLOSE_SCOPE
