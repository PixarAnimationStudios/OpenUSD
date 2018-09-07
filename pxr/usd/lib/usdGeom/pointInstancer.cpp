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

/* virtual */
UsdSchemaType UsdGeomPointInstancer::_GetSchemaType() const {
    return UsdGeomPointInstancer::schemaType;
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
#include "pxr/base/tf/envSetting.h"
#include "pxr/base/gf/transform.h"
#include "pxr/usd/usdGeom/bboxCache.h"
#include "pxr/usd/usdGeom/debugCodes.h"
#include "pxr/usd/usdGeom/xformCache.h"
#include "pxr/usd/usdGeom/motionAPI.h"

#include "pxr/usd/usdGeom/boundableComputeExtent.h"
#include "pxr/base/work/loops.h"
#include "pxr/base/tf/registryManager.h"

PXR_NAMESPACE_OPEN_SCOPE

// XXX Bug 139215: When we enable this, we can remove
// SdfListOp::ComposeOperations().
TF_DEFINE_ENV_SETTING(
    USDGEOM_POINTINSTANCER_NEW_APPLYOPS, false,
    "Set to true to use SdfListOp::ApplyOperations() instead of "
    "ComposeOperations().");

TF_REGISTRY_FUNCTION(TfEnum)
{
    TF_ADD_ENUM_NAME(UsdGeomPointInstancer::IncludeProtoXform);
    TF_ADD_ENUM_NAME(UsdGeomPointInstancer::ExcludeProtoXform);
    TF_ADD_ENUM_NAME(UsdGeomPointInstancer::ApplyMask);
    TF_ADD_ENUM_NAME(UsdGeomPointInstancer::IgnoreMask);
}

// Convert a list-op to a canonical order, treating it as an
// operation on a set rather than a list.  A side effect is
// ensuring that it does not use added or ordered items,
// and can therefore be used with ApplyOperations().
template <typename T>
static SdfListOp<T>
_CanonicalizeListOp(const SdfListOp<T> &op) {
    if (op.IsExplicit()) {
        return op;
    } else {
        std::vector<T> items;
        op.ApplyOperations(&items);
        std::sort(items.begin(), items.end());
        SdfListOp<T> r;
        r.SetPrependedItems(std::vector<T>(items.begin(), items.end()));
        r.SetDeletedItems(op.GetDeletedItems());
        return r;
    }
}

bool
UsdGeomPointInstancerApplyNewStyleListOps()
{
    return TfGetEnvSetting(USDGEOM_POINTINSTANCER_NEW_APPLYOPS);
}

bool 
UsdGeomPointInstancerSetOrMergeOverOp(std::vector<int64_t> const &items, 
                                      SdfListOpType op,
                                      UsdPrim const &prim,
                                      TfToken const &metadataName)
{
    SdfInt64ListOp  proposed, current;
    UsdStagePtr stage = prim.GetStage();
    UsdEditTarget editTarget = stage->GetEditTarget();
    SdfPrimSpecHandle  primSpec = 
        editTarget.GetPrimSpecForScenePath(prim.GetPath());
    
    if (primSpec){
        VtValue  existingOp = primSpec->GetInfo(metadataName);
        if (existingOp.IsHolding<SdfInt64ListOp>()){
            current = existingOp.UncheckedGet<SdfInt64ListOp>();
        }
    }

    proposed.SetItems(items, op);

    if (TfGetEnvSetting(USDGEOM_POINTINSTANCER_NEW_APPLYOPS)) {
        current = _CanonicalizeListOp(current);
        return prim.SetMetadata(UsdGeomTokens->inactiveIds,
                                *proposed.ApplyOperations(current));
    }

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
    return prim.SetMetadata(metadataName, current);
}

bool
UsdGeomPointInstancer::ActivateId(int64_t id) const
{
    std::vector<int64_t> toRemove(1, id);
    return UsdGeomPointInstancerSetOrMergeOverOp(
        toRemove, SdfListOpTypeDeleted, GetPrim(), UsdGeomTokens->inactiveIds);
}

bool
UsdGeomPointInstancer::ActivateIds(VtInt64Array const &ids) const
{
    std::vector<int64_t> toRemove(ids.begin(), ids.end());
    return UsdGeomPointInstancerSetOrMergeOverOp(
        toRemove, SdfListOpTypeDeleted, GetPrim(), UsdGeomTokens->inactiveIds);
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
    return UsdGeomPointInstancerSetOrMergeOverOp(toAdd,
        TfGetEnvSetting(USDGEOM_POINTINSTANCER_NEW_APPLYOPS) ?
        SdfListOpTypeAppended : SdfListOpTypeAdded, GetPrim(),
        UsdGeomTokens->inactiveIds);
}

bool
UsdGeomPointInstancer::DeactivateIds(VtInt64Array const &ids) const
{
    std::vector<int64_t> toAdd(ids.begin(), ids.end());
    return UsdGeomPointInstancerSetOrMergeOverOp(toAdd,
        TfGetEnvSetting(USDGEOM_POINTINSTANCER_NEW_APPLYOPS) ?
        SdfListOpTypeAppended : SdfListOpTypeAdded, GetPrim(),
        UsdGeomTokens->inactiveIds);
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

// Get the authored data of an attribute at the lower bracketing timesample of a
// given base time. Fails if the attribute is not authored. If baseTime is
// UsdTimeCode.Default() or the attribute has no time samples, the attribute is
// sampled at the UsdTimeCode.Default().
template<class T>
static bool
_GetAttrForInstanceTransforms(
    const UsdAttribute& attr,
    UsdTimeCode baseTime,
    UsdTimeCode* attrSampleTime,
    bool* attrHasSamples,
    T* attrData)
{

    if (baseTime.IsNumeric()) {

        double sampleTimeValue;
        double upperTimeValue;
        bool hasSamples;
        if (!attr.GetBracketingTimeSamples(
                baseTime.GetValue(),
                &sampleTimeValue,
                &upperTimeValue,
                &hasSamples)) {
            return false;
        }

        UsdTimeCode sampleTime = UsdTimeCode::Default();
        if (hasSamples) {
            sampleTime = UsdTimeCode(sampleTimeValue);
        }

        if (!attr.Get(attrData, sampleTime)) {
            return false;
        }

        *attrSampleTime = sampleTime;
        *attrHasSamples = hasSamples;

    } else {

        // baseTime is UsdTimeCode.Default()
        if (!attr.Get(attrData, baseTime)) {
            return false;
        }
        *attrSampleTime = baseTime;
        *attrHasSamples = false;

    }

    return true;
}

bool
UsdGeomPointInstancer::_GetProtoIndicesForInstanceTransforms(
    UsdTimeCode baseTime,
    VtIntArray* protoIndices) const
{
    UsdTimeCode sampleTime;
    bool hasSamples;
    if (!_GetAttrForInstanceTransforms<VtIntArray>(
            GetProtoIndicesAttr(),
            baseTime,
            &sampleTime,
            &hasSamples,
            protoIndices)) {
        // We don't TF_WARN here because computing transforms on an empty
        // PointInstancer should return an empty result without error.
        return false;
    }

    return true;
}

bool
UsdGeomPointInstancer::_GetPositionsForInstanceTransforms(
    UsdTimeCode baseTime,
    size_t numInstances,
    UsdTimeCode* positionsSampleTime,
    bool* positionsHasSamples,
    VtVec3fArray* positions) const
{
    UsdTimeCode sampleTime;
    bool hasSamples;
    VtVec3fArray positionData;
    if (!_GetAttrForInstanceTransforms<VtVec3fArray>(
            GetPositionsAttr(),
            baseTime,
            &sampleTime,
            &hasSamples,
            &positionData)) {
        TF_WARN("%s -- no positions", GetPrim().GetPath().GetText());
        return false;
    }

    if (positionData.size() != numInstances) {
        TF_WARN("%s -- found [%zu] positions, but expected [%zu]",
            GetPrim().GetPath().GetText(),
            positionData.size(),
            numInstances);
        return false;
    }

    *positionsSampleTime = sampleTime;
    *positionsHasSamples = hasSamples;
    *positions = positionData;
    return true;
}

bool
UsdGeomPointInstancer::_GetVelocitiesForInstanceTransforms(
    UsdTimeCode baseTime,
    size_t numInstances,
    UsdTimeCode positionsSampleTime,
    UsdTimeCode* velocitiesSampleTime,
    VtVec3fArray* velocities) const
{
    UsdTimeCode sampleTime;
    bool hasSamples;
    VtVec3fArray velocityData;
    if (!_GetAttrForInstanceTransforms<VtVec3fArray>(
            GetVelocitiesAttr(),
            baseTime,
            &sampleTime,
            &hasSamples,
            &velocityData)) {
        return false;
    }

    if (!hasSamples || !GfIsClose(
            sampleTime.GetValue(),
            positionsSampleTime.GetValue(),
            std::numeric_limits<double>::epsilon())) {
        TF_WARN("%s -- velocity samples are not aligned with position samples",
            GetPrim().GetPath().GetText());
        return false;
    }

    if (velocityData.size() != numInstances) {
        TF_WARN("%s -- found [%zu] velocities, but expected [%zu]",
            GetPrim().GetPath().GetText(),
            velocityData.size(),
            numInstances);
        return false;
    }

    *velocitiesSampleTime = sampleTime;
    *velocities = velocityData;
    return true;
}

bool
UsdGeomPointInstancer::_GetPositionsAndVelocitiesForInstanceTransforms(
    UsdTimeCode baseTime,
    size_t numInstances,
    VtVec3fArray* positions,
    VtVec3fArray* velocities,
    UsdTimeCode* velocitiesSampleTime) const
{
    UsdTimeCode positionsSampleTime;
    bool positionsHasSamples;
    if (!_GetPositionsForInstanceTransforms(
            baseTime,
            numInstances,
            &positionsSampleTime,
            &positionsHasSamples,
            positions)) {
        return false;
    }

    if (!positionsHasSamples || !_GetVelocitiesForInstanceTransforms(
            baseTime,
            numInstances,
            positionsSampleTime,
            velocitiesSampleTime,
            velocities)) {
        velocities->clear();
    }

    return true;
}

bool
UsdGeomPointInstancer::_GetScalesForInstanceTransforms(
    UsdTimeCode baseTime,
    size_t numInstances,
    VtVec3fArray* scales) const
{
    UsdTimeCode scalesSampleTime;
    bool scalesHasSamples;
    VtVec3fArray scaleData;
    if (!_GetAttrForInstanceTransforms<VtVec3fArray>(
            GetScalesAttr(),
            baseTime,
            &scalesSampleTime,
            &scalesHasSamples,
            &scaleData)) {
        return false;
    }

    if (scaleData.size() != numInstances) {
        TF_WARN("%s -- found [%zu] scales, but expected [%zu]",
            GetPrim().GetPath().GetText(),
            scaleData.size(),
            numInstances);
        return false;
    }

    *scales = scaleData;
    return true;
}

bool
UsdGeomPointInstancer::_GetOrientationsForInstanceTransforms(
    UsdTimeCode baseTime,
    size_t numInstances,
    UsdTimeCode* orientationsSampleTime,
    bool* orientationsHasSamples,
    VtQuathArray* orientations) const
{
    UsdTimeCode sampleTime;
    bool hasSamples;
    VtQuathArray orientationData;
    if (!_GetAttrForInstanceTransforms<VtQuathArray>(
            GetOrientationsAttr(),
            baseTime,
            &sampleTime,
            &hasSamples,
            &orientationData)) {
        return false;
    }

    if (orientationData.size() != numInstances) {
        TF_WARN("%s -- found [%zu] orientations, but expected [%zu]",
            GetPrim().GetPath().GetText(),
            orientationData.size(),
            numInstances);
        return false;
    }

    *orientationsSampleTime = sampleTime;
    *orientationsHasSamples = hasSamples;
    *orientations = orientationData;
    return true;
}

bool
UsdGeomPointInstancer::_GetAngularVelocitiesForInstanceTransforms(
    UsdTimeCode baseTime,
    size_t numInstances,
    UsdTimeCode orientationsSampleTime,
    UsdTimeCode* angularVelocitiesSampleTime,
    VtVec3fArray* angularVelocities) const
{
    UsdTimeCode sampleTime;
    bool hasSamples;
    VtVec3fArray angularVelocityData;
    if (!_GetAttrForInstanceTransforms<VtVec3fArray>(
            GetAngularVelocitiesAttr(),
            baseTime,
            &sampleTime,
            &hasSamples,
            &angularVelocityData)) {
        return false;
    }

    if (!hasSamples || !GfIsClose(
            sampleTime.GetValue(),
            orientationsSampleTime.GetValue(),
            std::numeric_limits<double>::epsilon())) {
        TF_WARN(
            "%s -- angular velocity samples are not aligned with orientation samples",
            GetPrim().GetPath().GetText());
        return false;
    }

    if (angularVelocityData.size() != numInstances) {
        TF_WARN(
            "%s -- found [%zu] angular velocities, but expected [%zu]",
            GetPrim().GetPath().GetText(),
            angularVelocityData.size(),
            numInstances);
        return false;
    }

    *angularVelocitiesSampleTime = sampleTime;
    *angularVelocities = angularVelocityData;
    return true;
}

bool
UsdGeomPointInstancer::_GetOrientationsAndAngularVelocitiesForInstanceTransforms(
    UsdTimeCode baseTime,
    size_t numInstances,
    VtQuathArray* orientations,
    VtVec3fArray* angularVelocities,
    UsdTimeCode* angularVelocitiesSampleTime) const
{
    UsdTimeCode orientationsSampleTime;
    bool orientationsHasSamples;
    if (!_GetOrientationsForInstanceTransforms(
            baseTime,
            numInstances,
            &orientationsSampleTime,
            &orientationsHasSamples,
            orientations)) {
        return false;
    }

    if (!orientationsHasSamples || !_GetAngularVelocitiesForInstanceTransforms(
            baseTime,
            numInstances,
            orientationsSampleTime,
            angularVelocitiesSampleTime,
            angularVelocities)) {
        angularVelocities->clear();
    }

    return true;
}

bool
UsdGeomPointInstancer::_GetPrototypePathsForInstanceTransforms(
    const VtIntArray& protoIndices,
    SdfPathVector* protoPaths) const
{
    SdfPathVector protoPathData;
    if (!GetPrototypesRel().GetTargets(&protoPathData) || protoPathData.empty()) {
        TF_WARN("%s -- no prototypes",
                GetPrim().GetPath().GetText());
        return false;
    }

    for (const auto& protoIndex : protoIndices) {
        if (protoIndex < 0
                || static_cast<size_t>(protoIndex) >= protoPathData.size()) {
            TF_WARN("%s -- invalid prototype index: %d. Should be in [0, %zu)",
                    GetPrim().GetPath().GetText(),
                    protoIndex,
                    protoPathData.size());
            return false;
        }
    }

    *protoPaths = protoPathData;
    return true;
}

bool
UsdGeomPointInstancer::_ComputeInstanceTransformsAtTimePreamble(
    const UsdTimeCode baseTime,
    const ProtoXformInclusion doProtoXforms,
    const MaskApplication applyMask,
    VtIntArray* protoIndices,
    VtVec3fArray* positions,
    VtVec3fArray* velocities,
    UsdTimeCode* velocitiesSampleTime,
    VtVec3fArray* scales,
    VtQuathArray* orientations,
    VtVec3fArray* angularVelocities,
    UsdTimeCode* angularVelocitiesSampleTime,
    SdfPathVector* protoPaths,
    std::vector<bool>* mask,
    float* velocityScale) const
{
    TRACE_FUNCTION();

    if (!_GetProtoIndicesForInstanceTransforms(
            baseTime,
            protoIndices)) {
        return false;
    }

    // We determine the number of instances from the number of prototype
    // indices. All other data (positions, velocities, orientations, etc.) is
    // invalid if it does not conform to this count.
    size_t numInstances = protoIndices->size();

    if (numInstances == 0) {
        return true;
    }

    if (!_GetPositionsAndVelocitiesForInstanceTransforms(
            baseTime,
            numInstances,
            positions,
            velocities,
            velocitiesSampleTime)) {
        return false;
    }

    // We don't currently support an attribute which linearly changes the
    // scale (as velocity does for position). Instead, we lock the scale to
    // the last authored value without performing any interpolation.
    _GetScalesForInstanceTransforms(
        baseTime,
        numInstances,
        scales);

    _GetOrientationsAndAngularVelocitiesForInstanceTransforms(
            baseTime,
            numInstances,
            orientations,
            angularVelocities,
            angularVelocitiesSampleTime);

    if (doProtoXforms == IncludeProtoXform) {
        if (!_GetPrototypePathsForInstanceTransforms(
                *protoIndices,
                protoPaths)) {
            return false;
        }
    }

    if (applyMask == ApplyMask) {
        *mask = ComputeMaskAtTime(baseTime);
        if (!(mask->empty() || mask->size() == numInstances)) {
            TF_WARN(
                "%s -- found mask of size [%zu], but expected size [%zu]",
                GetPrim().GetPath().GetText(),
                mask->size(),
                numInstances);
            return false;
        }
    }

    *velocityScale = UsdGeomMotionAPI(GetPrim()).ComputeVelocityScale(
        baseTime);

    return true;
}

bool
UsdGeomPointInstancer::ComputeInstanceTransformsAtTime(
    VtArray<GfMatrix4d>* xforms,
    const UsdTimeCode time,
    const UsdTimeCode baseTime,
    const ProtoXformInclusion doProtoXforms,
    const MaskApplication applyMask) const
{
    TRACE_FUNCTION();

    if (!xforms) {
        TF_WARN(
            "%s -- null container passed to ComputeInstanceTransformsAtTime()",
            GetPrim().GetPath().GetText());
        return false;
    }

    if (time.IsNumeric() != baseTime.IsNumeric()) {
        TF_CODING_ERROR(
            "%s -- time and baseTime must either both be numeric or both be default",
            GetPrim().GetPath().GetText());
    }

    VtIntArray protoIndices;
    VtVec3fArray positions;
    VtVec3fArray velocities;
    UsdTimeCode velocitiesSampleTime;
    VtVec3fArray scales;
    VtQuathArray orientations;
    VtVec3fArray angularVelocities;
    UsdTimeCode angularVelocitiesSampleTime;
    SdfPathVector protoPaths;
    std::vector<bool> mask;
    float velocityScale;
    if (!_ComputeInstanceTransformsAtTimePreamble(
            baseTime,
            doProtoXforms,
            applyMask,
            &protoIndices,
            &positions,
            &velocities,
            &velocitiesSampleTime,
            &scales,
            &orientations,
            &angularVelocities,
            &angularVelocitiesSampleTime,
            &protoPaths,
            &mask,
            &velocityScale)) {
        return false;
    }

    size_t numInstances = protoIndices.size();
    if (numInstances == 0) {
        xforms->clear();
        return true;
    }

    UsdStageWeakPtr stage = GetPrim().GetStage();

    // If there are no valid velocities or angular velocities, we fallback to
    // "standard" computation logic (linear interpolation between samples).
    if (velocities.empty() && angularVelocities.empty()) {

        // Try to fetch the positions, scales, and orientations at the sample
        // time. If this fails or the fetched data don't have the correct
        // topology, we fallback to the data from the base time.

        VtVec3fArray interpolatedPositions;
        if (GetPositionsAttr().Get(&interpolatedPositions, time)
                && interpolatedPositions.size() == numInstances) {
            positions = interpolatedPositions;
        }

        VtVec3fArray interpolatedScales;
        if (GetScalesAttr().Get(&interpolatedScales, time)
                && interpolatedScales.size() == numInstances) {
            scales = interpolatedScales;
        }

        VtQuathArray interpolatedOrientations;
        if (GetOrientationsAttr().Get(&interpolatedOrientations, time)
                && interpolatedOrientations.size() == numInstances) {
            orientations = interpolatedOrientations;
        }

    }

    return UsdGeomPointInstancer::ComputeInstanceTransformsAtTime(
        xforms,
        stage,
        time,
        protoIndices,
        positions,
        velocities,
        velocitiesSampleTime,
        scales,
        orientations,
        angularVelocities,
        angularVelocitiesSampleTime,
        protoPaths,
        mask,
        velocityScale);
}

bool
UsdGeomPointInstancer::ComputeInstanceTransformsAtTimes(
    std::vector<VtArray<GfMatrix4d>>* xformsArray,
    const std::vector<UsdTimeCode>& times,
    const UsdTimeCode baseTime,
    const ProtoXformInclusion doProtoXforms,
    const MaskApplication applyMask) const
{
    size_t numSamples = times.size();
    for (auto time : times) {
        if (time.IsNumeric() != baseTime.IsNumeric()) {
            TF_CODING_ERROR(
                "%s -- all sample times in times and baseTime must either all "
                "be numeric or all be default",
                GetPrim().GetPath().GetText());
        }
    }

    VtIntArray protoIndices;
    VtVec3fArray positions;
    VtVec3fArray velocities;
    UsdTimeCode velocitiesSampleTime;
    VtVec3fArray scales;
    VtQuathArray orientations;
    VtVec3fArray angularVelocities;
    UsdTimeCode angularVelocitiesSampleTime;
    SdfPathVector protoPaths;
    std::vector<bool> mask;
    float velocityScale;
    if (!_ComputeInstanceTransformsAtTimePreamble(
            baseTime,
            doProtoXforms,
            applyMask,
            &protoIndices,
            &positions,
            &velocities,
            &velocitiesSampleTime,
            &scales,
            &orientations,
            &angularVelocities,
            &angularVelocitiesSampleTime,
            &protoPaths,
            &mask,
            &velocityScale)) {
        return false;
    }

    size_t numInstances = protoIndices.size();
    if (numInstances == 0) {
        xformsArray->clear();
        xformsArray->resize(numSamples);
        return true;
    }

    UsdStageWeakPtr stage = GetPrim().GetStage();

    std::vector<VtArray<GfMatrix4d>> xformsArrayData;
    xformsArrayData.resize(numSamples);
    bool useInterpolated = (velocities.empty() && angularVelocities.empty());
    for (size_t i = 0; i < numSamples; i++) {

        UsdTimeCode time = times[i];
        VtArray<GfMatrix4d>* xforms = &(xformsArrayData[i]);

        // If there are no valid velocities or angular velocities, we fallback to
        // "standard" computation logic (linear interpolation between samples).
        if (useInterpolated) {

            // Try to fetch the positions, scales, and orientations at the sample
            // time. If this fails or the fetched data don't have the correct
            // topology, we fallback to the data from the base time.

            VtVec3fArray interpolatedPositions;
            if (GetPositionsAttr().Get(&interpolatedPositions, time)
                    && interpolatedPositions.size() == numInstances) {
                positions = interpolatedPositions;
            }

            VtVec3fArray interpolatedScales;
            if (GetScalesAttr().Get(&interpolatedScales, time)
                    && interpolatedScales.size() == numInstances) {
                scales = interpolatedScales;
            }

            VtQuathArray interpolatedOrientations;
            if (GetOrientationsAttr().Get(&interpolatedOrientations, time)
                    && interpolatedOrientations.size() == numInstances) {
                orientations = interpolatedOrientations;
            }

        }

        if (!UsdGeomPointInstancer::ComputeInstanceTransformsAtTime(
                xforms,
                stage,
                time,
                protoIndices,
                positions,
                velocities,
                velocitiesSampleTime,
                scales,
                orientations,
                angularVelocities,
                angularVelocitiesSampleTime,
                protoPaths,
                mask,
                velocityScale)) {
            return false;
        }

    }

    *xformsArray = xformsArrayData;
    return true;
}

bool
UsdGeomPointInstancer::ComputeInstanceTransformsAtTime(
    VtArray<GfMatrix4d>* xforms,
    UsdStageWeakPtr& stage,
    UsdTimeCode time,
    const VtIntArray& protoIndices,
    const VtVec3fArray& positions,
    const VtVec3fArray& velocities,
    UsdTimeCode velocitiesSampleTime,
    const VtVec3fArray& scales,
    const VtQuathArray& orientations,
    const VtVec3fArray& angularVelocities,
    UsdTimeCode angularVelocitiesSampleTime,
    const SdfPathVector& protoPaths,
    const std::vector<bool>& mask,
    float velocityScale)
{
    TRACE_FUNCTION();

    size_t numInstances = protoIndices.size();

    const double timeCodesPerSecond = stage->GetTimeCodesPerSecond();
    const float velocityMultiplier =
        velocityScale * static_cast<float>(
            (time.GetValue() - velocitiesSampleTime.GetValue())
            / timeCodesPerSecond);
    const float angularVelocityMultiplier =
        velocityScale * static_cast<float>(
            (time.GetValue() - angularVelocitiesSampleTime.GetValue())
            / timeCodesPerSecond);

    xforms->resize(numInstances);

    const GfMatrix4d identity(1.0);
    std::vector<GfMatrix4d> protoXforms(protoPaths.size(), identity);
    UsdGeomXformCache xformCache(time);
    if (protoPaths.size() != 0) {
        for (size_t protoIndex = 0 ; protoIndex < protoPaths.size() ; 
                ++protoIndex) {
            const SdfPath& protoPath = protoPaths[protoIndex];
            if (const UsdPrim& protoPrim = stage->GetPrimAtPath(protoPath)) {
                // Get the prototype's local transformation.
                bool resetsXformStack;
                protoXforms[protoIndex] = xformCache.GetLocalTransformation(
                    protoPrim, &resetsXformStack);
            }
        }
    }

    const auto computeInstanceXforms = [&mask, &velocityMultiplier, 
        &angularVelocityMultiplier, &scales, &orientations, &positions, 
        &velocities, &angularVelocities, &identity, &protoXforms, &protoIndices, 
        &protoPaths, &xforms] (size_t start, size_t end) {
        for (size_t instanceId = start ; instanceId < end ; ++instanceId) {
            if (!mask.empty() && !mask[instanceId]) {
                continue;
            }

            GfTransform instanceTransform;

            if (!scales.empty()) {
                instanceTransform.SetScale(scales[instanceId]);
            }

            if (!orientations.empty()) {
                GfRotation rotation = GfRotation(orientations[instanceId]);
                if (angularVelocities.size() != 0) {
                    GfVec3f angularVelocity = angularVelocities[instanceId];
                    rotation *= GfRotation(
                        angularVelocity,
                        angularVelocityMultiplier * 
                            angularVelocity.GetLength());
                }
                instanceTransform.SetRotation(rotation);
            }

            GfVec3f translation = positions[instanceId];
            if (velocities.size() != 0) {
                translation += velocityMultiplier * velocities[instanceId];
            }
            instanceTransform.SetTranslation(translation);

            const int protoIndex = protoIndices[instanceId];
            const GfMatrix4d &protoXform = (protoPaths.size() != 0) ? 
                protoXforms[protoIndex] : identity;

            (*xforms)[instanceId] = protoXform * 
                                    instanceTransform.GetMatrix();
        }
    };

    {
        TRACE_SCOPE("UsdGeomPointInstancer::ComputeInstanceTransformsAtTime (Parallel)");
        WorkParallelForN(numInstances, computeInstanceXforms);
    }

    return ApplyMaskToArray(mask, xforms);
}

bool
UsdGeomPointInstancer::_ComputeExtentAtTimePreamble(
    UsdTimeCode baseTime,
    VtIntArray* protoIndices,
    std::vector<bool>* mask,
    UsdRelationship* prototypes,
    SdfPathVector* protoPaths) const
{
    if (!GetProtoIndicesAttr().Get(protoIndices, baseTime)) {
        TF_WARN("%s -- no prototype indices",
                GetPrim().GetPath().GetText());
        return false;
    }

    *mask = ComputeMaskAtTime(baseTime);
    if (!mask->empty() && mask->size() != protoIndices->size()) {
        TF_WARN("%s -- mask.size() [%zu] != protoIndices.size() [%zu]",
                GetPrim().GetPath().GetText(),
                mask->size(),
                protoIndices->size());
        return false;
    }

    *prototypes = GetPrototypesRel();
    if (!prototypes->GetTargets(protoPaths) || protoPaths->empty()) {
        TF_WARN("%s -- no prototypes",
                GetPrim().GetPath().GetText());
        return false;
    }

    // verify that all the protoIndices are in bounds.
    TF_FOR_ALL(iter, *protoIndices) {
        const int protoIndex = *iter;
        if (protoIndex < 0 || 
            static_cast<size_t>(protoIndex) >= protoPaths->size()) {
            TF_WARN("%s -- invalid prototype index: %d. Should be in [0, %zu)",
                    GetPrim().GetPath().GetText(),
                    protoIndex,
                    protoPaths->size());
            return false;
        }
    }

    return true;
}

bool
UsdGeomPointInstancer::_ComputeExtentFromTransforms(
    VtVec3fArray* extent,
    const VtIntArray& protoIndices,
    const std::vector<bool>& mask,
    const UsdRelationship& prototypes,
    const SdfPathVector& protoPaths,
    const VtMatrix4dArray& instanceTransforms,
    UsdTimeCode time,
    const GfMatrix4d* transform) const
{
    TRACE_FUNCTION();

    UsdStageWeakPtr stage = GetPrim().GetStage();
    
    if (protoIndices.size() <= protoPaths.size()) {
        TF_DEBUG(USDGEOM_BBOX).Msg("Number of prototypes (%zu) is >= number"
            "of instances (%zu). May be inefficient.", protoPaths.size(), 
            protoIndices.size());
    }

    // We might want to precompute prototype bounds only when the number of 
    // instances is greater than the number of prototypes.
    std::vector<GfBBox3d> protoUntransformedBounds;
    protoUntransformedBounds.reserve(protoPaths.size());

    UsdGeomBBoxCache bboxCache(time, 
        /*purposes*/ {UsdGeomTokens->default_, 
                      UsdGeomTokens->proxy, 
                      UsdGeomTokens->render });
    for (size_t protoId = 0 ; protoId < protoPaths.size() ; ++protoId) {
        const SdfPath& protoPath = protoPaths[protoId];
        const UsdPrim& protoPrim = stage->GetPrimAtPath(protoPath);
        const GfBBox3d protoBounds = 
                bboxCache.ComputeUntransformedBound(protoPrim);
        protoUntransformedBounds.push_back(protoBounds);
    }

    // Compute all the instance aligned ranges.
    std::vector<GfRange3d> instanceAlignedRanges(protoIndices.size());
    const auto computeInstanceAlignedRange = 
        [&mask, &protoIndices, &transform, &protoUntransformedBounds, 
         &instanceTransforms, &instanceAlignedRanges] 
        (size_t start, size_t end) {
            for (size_t instanceId = start ; instanceId < end ; ++instanceId) {
                if (!mask.empty() && !mask[instanceId]) {
                    continue;
                }

                // Get the prototype bounding box.
                const int protoIndex = protoIndices[instanceId];
                GfBBox3d thisBounds = protoUntransformedBounds[protoIndex];

                // Apply the instance transform.
                thisBounds.Transform(instanceTransforms[instanceId]);

                // Apply the optional transform.
                if (transform) {
                    thisBounds.Transform(*transform);
                }
                instanceAlignedRanges[instanceId] = 
                        thisBounds.ComputeAlignedRange();
            }
        };

    WorkParallelForN(protoIndices.size(), computeInstanceAlignedRange);

    GfRange3d extentRange;
    for (const GfRange3d &instanceRange : instanceAlignedRanges) {
        extentRange.UnionWith(instanceRange);
    }

    const GfVec3d extentMin = extentRange.GetMin();
    const GfVec3d extentMax = extentRange.GetMax();

    *extent = VtVec3fArray(2);
    (*extent)[0] = GfVec3f(extentMin[0], extentMin[1], extentMin[2]);
    (*extent)[1] = GfVec3f(extentMax[0], extentMax[1], extentMax[2]);

    return true;
}

bool
UsdGeomPointInstancer::_ComputeExtentAtTime(
    VtVec3fArray* extent,
    const UsdTimeCode time,
    const UsdTimeCode baseTime,
    const GfMatrix4d* transform) const
{
    if (!extent) {
        TF_CODING_ERROR("%s -- null container passed to ComputeExtentAtTime()",
                GetPrim().GetPath().GetText());
        return false;
    }

    VtIntArray protoIndices;
    std::vector<bool> mask;
    UsdRelationship prototypes;
    SdfPathVector protoPaths;
    if (!_ComputeExtentAtTimePreamble(
            baseTime,
            &protoIndices,
            &mask,
            &prototypes,
            &protoPaths)) {
        return false;
    }

    // Note that we do NOT apply any masking when computing the instance
    // transforms. This is so that for a particular instance we can determine
    // both its transform and its prototype. Otherwise, the instanceTransforms
    // array would have masked instances culled out and we would lose the
    // mapping to the prototypes.
    // Masked instances will be culled before being applied to the extent below.
    VtMatrix4dArray instanceTransforms;
    if (!ComputeInstanceTransformsAtTime(&instanceTransforms,
                                         time,
                                         baseTime,
                                         IncludeProtoXform,
                                         IgnoreMask)) {
        TF_WARN("%s -- could not compute instance transforms",
                GetPrim().GetPath().GetText());
        return false;
    }

    return _ComputeExtentFromTransforms(
        extent,
        protoIndices,
        mask,
        prototypes,
        protoPaths,
        instanceTransforms,
        time,
        transform);
}

bool
UsdGeomPointInstancer::_ComputeExtentAtTimes(
    std::vector<VtVec3fArray>* extents,
    const std::vector<UsdTimeCode>& times,
    const UsdTimeCode baseTime,
    const GfMatrix4d* transform) const
{
    if (!extents) {
        TF_CODING_ERROR("%s -- null container passed to ComputeExtentAtTimes()",
                GetPrim().GetPath().GetText());
        return false;
    }

    VtIntArray protoIndices;
    std::vector<bool> mask;
    UsdRelationship prototypes;
    SdfPathVector protoPaths;
    if (!_ComputeExtentAtTimePreamble(
            baseTime,
            &protoIndices,
            &mask,
            &prototypes,
            &protoPaths)) {
        return false;
    }

    // Note that we do NOT apply any masking when computing the instance
    // transforms. This is so that for a particular instance we can determine
    // both its transform and its prototype. Otherwise, the instanceTransforms
    // array would have masked instances culled out and we would lose the
    // mapping to the prototypes.
    // Masked instances will be culled before being applied to the extent below.
    std::vector<VtMatrix4dArray> instanceTransformsArray;
    if (!ComputeInstanceTransformsAtTimes(
            &instanceTransformsArray,
            times,
            baseTime,
            IncludeProtoXform,
            IgnoreMask)) {
        TF_WARN("%s -- could not compute instance transforms",
                GetPrim().GetPath().GetText());
        return false;
    }

    std::vector<VtVec3fArray> computedExtents;
    computedExtents.resize(times.size());

    for (size_t i = 0; i < times.size(); i++) {

        const UsdTimeCode& time = times[i];
        const VtMatrix4dArray& instanceTransforms = instanceTransformsArray[i];

        if (!_ComputeExtentFromTransforms(
                &(computedExtents[i]),
                protoIndices,
                mask,
                prototypes,
                protoPaths,
                instanceTransforms,
                time,
                transform)) {
            return false;
        }
    }

    extents->swap(computedExtents);
    return true;
}

bool
UsdGeomPointInstancer::ComputeExtentAtTime(
    VtVec3fArray* extent,
    const UsdTimeCode time,
    const UsdTimeCode baseTime) const
{
    return _ComputeExtentAtTime(extent, time, baseTime, nullptr);
}

bool
UsdGeomPointInstancer::ComputeExtentAtTime(
    VtVec3fArray* extent,
    const UsdTimeCode time,
    const UsdTimeCode baseTime,
    const GfMatrix4d& transform) const
{
    return _ComputeExtentAtTime(extent, time, baseTime, &transform);
}

bool
UsdGeomPointInstancer::ComputeExtentAtTimes(
    std::vector<VtVec3fArray>* extents,
    const std::vector<UsdTimeCode>& times,
    const UsdTimeCode baseTime) const
{
    return _ComputeExtentAtTimes(extents, times, baseTime, nullptr);
}

bool
UsdGeomPointInstancer::ComputeExtentAtTimes(
    std::vector<VtVec3fArray>* extents,
    const std::vector<UsdTimeCode>& times,
    const UsdTimeCode baseTime,
    const GfMatrix4d& transform) const
{
    return _ComputeExtentAtTimes(extents, times, baseTime, &transform);
}

static bool
_ComputeExtentForPointInstancer(
    const UsdGeomBoundable& boundable,
    const UsdTimeCode& time,
    const GfMatrix4d* transform,
    VtVec3fArray* extent)
{
    TRACE_FUNCTION();

    const UsdGeomPointInstancer pointInstancerSchema(boundable);
    if (!TF_VERIFY(pointInstancerSchema)) {
        return false;
    }

    // We use the input time as the baseTime because we don't care about
    // velocity or angularVelocity.
    if (transform) {
        return pointInstancerSchema.ComputeExtentAtTime(
            extent, time, time, *transform);
    } else {
        return pointInstancerSchema.ComputeExtentAtTime(extent, time, time);
    }
}

TF_REGISTRY_FUNCTION(UsdGeomBoundable)
{
    UsdGeomRegisterComputeExtentFunction<UsdGeomPointInstancer>(
        _ComputeExtentForPointInstancer);
}

PXR_NAMESPACE_CLOSE_SCOPE
