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
#include "pxr/pxr.h"
#include "pxr/usd/usdGeom/bboxCache.h"

#include "pxr/usd/kind/registry.h"

#include "pxr/usd/usdGeom/boundable.h"
#include "pxr/usd/usdGeom/curves.h"
#include "pxr/usd/usdGeom/debugCodes.h"
#include "pxr/usd/usdGeom/modelAPI.h"
#include "pxr/usd/usdGeom/points.h"
#include "pxr/usd/usdGeom/pointBased.h"
#include "pxr/usd/usdGeom/xform.h"

#include "pxr/usd/usd/treeIterator.h"
#include "pxr/base/tracelite/trace.h"

#include "pxr/base/tf/pyLock.h"
#include "pxr/base/tf/token.h"

#include <tbb/enumerable_thread_specific.h>
#include <tbb/task.h>
#include <algorithm>

PXR_NAMESPACE_OPEN_SCOPE


// Thread-local Xform cache.
// This should be replaced with (TBD) multi-threaded XformCache::Prepopulate
typedef tbb::enumerable_thread_specific<UsdGeomXformCache> _ThreadXformCache;

// -------------------------------------------------------------------------- //
// _BBoxTask
// -------------------------------------------------------------------------- //
class UsdGeomBBoxCache::_BBoxTask: public tbb::task {
    UsdPrim _prim;
    GfMatrix4d _inverseComponentCtm;
    UsdGeomBBoxCache* _owner;
    _ThreadXformCache* _xfCaches;
public:
    _BBoxTask(const UsdPrim& prim, const GfMatrix4d &inverseComponentCtm, 
              UsdGeomBBoxCache* owner, _ThreadXformCache* xfCaches)
        : _prim(prim)
        , _inverseComponentCtm(inverseComponentCtm)
        , _owner(owner)
        , _xfCaches(xfCaches)
    {
    }
    task* execute() {
        // Do not save state here; all state should be accumulated externally.
        _owner->_ResolvePrim(this, _prim, _inverseComponentCtm);
        return NULL;
    }
    _ThreadXformCache* GetXformCaches() { return _xfCaches; }
};

// -------------------------------------------------------------------------- //
// _MasterBBoxResolver
//
// If a master prim has instances nested within it, resolving its bbox
// will depend on the masters for those instances being resolved first.
// These dependencies form an acyclic graph where a given master may depend
// on and be a dependency for one or more masters.
//
// This helper object tracks those dependencies as tasks are dispatched
// and completed.
// -------------------------------------------------------------------------- //
class UsdGeomBBoxCache::_MasterBBoxResolver
{
private:
    UsdGeomBBoxCache* _owner;

    struct _MasterTask
    {
        _MasterTask() : numDependencies(0) { }

        // Number of dependencies -- master prims that must be resolved
        // before this master can be resolved.
        tbb::atomic<size_t> numDependencies;

        // List of master prims that depend on this master.
        std::vector<UsdPrim> dependentMasters;
    };

    typedef TfHashMap<UsdPrim, _MasterTask, boost::hash<UsdPrim> > 
        _MasterTaskMap;

public:
    _MasterBBoxResolver(UsdGeomBBoxCache* bboxCache)
        : _owner(bboxCache)
    {
    }

    void Resolve(const std::vector<UsdPrim>& masterPrims)
    {
        TRACE_FUNCTION();

        _MasterTaskMap masterTasks;
        for (const auto& masterPrim : masterPrims) {
            _PopulateTasksForMaster(masterPrim, &masterTasks);
        }

        // Using the owner's xform cache won't provide a benefit
        // because the masters are separate parts of the scenegraph
        // that won't be traversed when resolving other bounding boxes.
        _ThreadXformCache xfCache;

        WorkDispatcher dispatcher;
        for (const auto& t : masterTasks) {
            if (t.second.numDependencies == 0) {
                dispatcher.Run(
                    &_MasterBBoxResolver::_ExecuteTaskForMaster, 
                    this, t.first, &masterTasks, &xfCache, &dispatcher);
            }
        }
    }

private:
    void _PopulateTasksForMaster(const UsdPrim& masterPrim,
                                 _MasterTaskMap* masterTasks)
    {
        std::pair<_MasterTaskMap::iterator, bool> masterTaskStatus =
            masterTasks->insert(std::make_pair(masterPrim, _MasterTask()));
        if (!masterTaskStatus.second) {
            return;
        }

        std::vector<UsdPrim> requiredMasters;
        _owner->_FindOrCreateEntriesForPrim(masterPrim, &requiredMasters);

        {
            // In order to resolve the bounding box for masterPrim, we need to
            // compute the bounding boxes for all masters for nested instances.
            _MasterTask& masterTaskData = masterTaskStatus.first->second;
            masterTaskData.numDependencies = requiredMasters.size();
        }

        // Recursively populate the task map for the masters needed for
        // nested instances.
        for (const auto& reqMaster : requiredMasters) {
            _PopulateTasksForMaster(reqMaster, masterTasks);
            (*masterTasks)[reqMaster].dependentMasters.push_back(masterPrim);
        }
    }

    void _ExecuteTaskForMaster(const UsdPrim& master,
                               _MasterTaskMap* masterTasks,
                               _ThreadXformCache* xfCaches,
                               WorkDispatcher* dispatcher)
    {
        UsdGeomBBoxCache::_BBoxTask& rootTask = *new(tbb::task::allocate_root()) 
            UsdGeomBBoxCache::_BBoxTask(master, GfMatrix4d(1.0), 
                                        _owner, xfCaches);
        tbb::task::spawn_root_and_wait(rootTask);

        // Update all of the master prims that depended on the completed master
        // and dispatch new tasks for those whose dependencies have been
        // resolved.  We're guaranteed that all the entries were populated by
        // _PopulateTasksForMaster, so we don't check the result of 'find()'.
        const _MasterTask& masterData = masterTasks->find(master)->second;
        for (const auto& dependentMaster : masterData.dependentMasters) {
            _MasterTask& dependentMasterData =
                masterTasks->find(dependentMaster)->second;
            if (dependentMasterData.numDependencies.fetch_and_decrement() == 1){
                dispatcher->Run(
                    &_MasterBBoxResolver::_ExecuteTaskForMaster, 
                    this, dependentMaster, masterTasks, xfCaches, dispatcher);
            }
        }
    }

};

// -------------------------------------------------------------------------- //
// Helper functions for managing query objects
// -------------------------------------------------------------------------- //

namespace
{
// Enumeration of queries stored for each cached entry that varies
// over time. 
enum _Queries {
    Extent = 0,

    // Note: code in _ResolvePrim relies on ExtentsHint being last.
    ExtentsHint,
    NumQueries
};
}

#define DEFINE_QUERY_ACCESSOR(Name, Schema)                             \
static const UsdAttributeQuery&                                         \
_GetOrCreate##Name##Query(const UsdPrim& prim, UsdAttributeQuery* q)    \
{                                                                       \
    if (!*q) {                                                       \
        if (Schema s = Schema(prim)) {                                  \
            UsdAttribute attr = s.Get##Name##Attr();                    \
            if (TF_VERIFY(attr, "Unable to get attribute '%s' on prim " \
                          "at path <%s>", #Name,                        \
                          prim.GetPath().GetText())) {                  \
                *q = UsdAttributeQuery(attr);                           \
            }                                                           \
        }                                                               \
    }                                                                   \
    return *q;                                                          \
}

DEFINE_QUERY_ACCESSOR(Extent, UsdGeomBoundable);
DEFINE_QUERY_ACCESSOR(Visibility, UsdGeomImageable);

// ExtentsHint is a custom attribute so we need an additional check
// to see if the attribute exists.

static const UsdAttributeQuery&
_GetOrCreateExtentsHintQuery(UsdGeomModelAPI& geomModel, UsdAttributeQuery* q)
{
    if (!*q) {
        UsdAttribute extentsHintAttr = geomModel.GetExtentsHintAttr();
        if (extentsHintAttr) {
            *q = UsdAttributeQuery(extentsHintAttr);
        }
    }
    return *q;
}

// -------------------------------------------------------------------------- //
// UsdGeomBBoxCache Public API
// -------------------------------------------------------------------------- //

UsdGeomBBoxCache::UsdGeomBBoxCache(UsdTimeCode time, TfTokenVector includedPurposes,
                                   bool useExtentsHint)
    : _time(time)
    , _includedPurposes(includedPurposes)
    , _useExtentsHint(useExtentsHint)
    , _ctmCache(time)
{
}

GfBBox3d
UsdGeomBBoxCache::ComputeWorldBound(const UsdPrim& prim)
{
    GfBBox3d bbox;

    if (!prim) {
        TF_CODING_ERROR("Invalid prim: %s", UsdDescribe(prim).c_str());
        return bbox;
    }

    _PurposeToBBoxMap bboxes;
    if (!_Resolve(prim, &bboxes))
        return bbox;

    bbox = _GetCombinedBBoxForIncludedPurposes(bboxes);

    GfMatrix4d ctm = _ctmCache.GetLocalToWorldTransform(prim);
    bbox.Transform(ctm);

    return bbox;
}

GfBBox3d
UsdGeomBBoxCache::ComputeRelativeBound(const UsdPrim& prim, 
                                      const UsdPrim &relativeToAncestorPrim)
{
    GfBBox3d bbox;
    if (!prim) {
        TF_CODING_ERROR("Invalid prim: %s", UsdDescribe(prim).c_str());
        return bbox;
    }

    _PurposeToBBoxMap bboxes;
    if (!_Resolve(prim, &bboxes))
        return bbox;

    bbox = _GetCombinedBBoxForIncludedPurposes(bboxes);

    GfMatrix4d primCtm = _ctmCache.GetLocalToWorldTransform(prim);
    GfMatrix4d ancestorCtm = 
        _ctmCache.GetLocalToWorldTransform(relativeToAncestorPrim);
    GfMatrix4d relativeCtm = ancestorCtm.GetInverse() * primCtm;

    bbox.Transform(relativeCtm);

    return bbox;
}

GfBBox3d
UsdGeomBBoxCache::ComputeLocalBound(const UsdPrim& prim)
{
    GfBBox3d bbox;

    if (!prim) {
        TF_CODING_ERROR("Invalid prim: %s", UsdDescribe(prim).c_str());
        return bbox;
    }

    _PurposeToBBoxMap bboxes;
    if (!_Resolve(prim, &bboxes))
        return bbox;

    bbox = _GetCombinedBBoxForIncludedPurposes(bboxes);

    // The value of resetsXformStack does not affect the local bound.
    bool resetsXformStack = false;
    bbox.Transform(_ctmCache.GetLocalTransformation(prim, &resetsXformStack));
    
    return bbox;
}

GfBBox3d
UsdGeomBBoxCache::ComputeUntransformedBound(const UsdPrim& prim)
{
    GfBBox3d empty;

    if (!prim) {
        TF_CODING_ERROR("Invalid prim: %s", UsdDescribe(prim).c_str());
        return empty;
    }

    _PurposeToBBoxMap bboxes;
    if (!_Resolve(prim, &bboxes))
        return empty;

    return _GetCombinedBBoxForIncludedPurposes(bboxes);
}

GfBBox3d 
UsdGeomBBoxCache::ComputeUntransformedBound(
    const UsdPrim &prim, 
    const SdfPathSet &pathsToSkip,
    const TfHashMap<SdfPath, GfMatrix4d, SdfPath::Hash> &ctmOverrides)
{
    GfBBox3d empty;

    if (!prim) {
        TF_CODING_ERROR("Invalid prim: %s", UsdDescribe(prim).c_str());
        return empty;
    }

    // Use a path table to populate a hash map containing all ancestors of the 
    // paths in pathsToSkip.
    SdfPathTable<bool> ancestorsOfPathsToSkip;
    for (const SdfPath &p : pathsToSkip) {
        ancestorsOfPathsToSkip[p.GetParentPath()] = true;
    }

    // Use a path table to populate a hash map containing all ancestors of the 
    // paths in ctmOverrides.
    SdfPathTable<bool> ancestorsOfOverrides;
    for (const auto &override : ctmOverrides) {
        ancestorsOfOverrides[override.first.GetParentPath()] = true;
    }

    GfBBox3d result;
    for (UsdTreeIterator it(prim); it ; ++it) {
        const UsdPrim &p = *it;
        const SdfPath &primPath = p.GetPath();

        // If this is one of the paths to be skipped, then prune subtree and 
        // continue traversal.
        if (pathsToSkip.count(primPath)) {
            it.PruneChildren();
            continue;
        }

        // If this is an ancestor of a path that's skipped, then we must 
        // continue the travesal down to find prims whose bounds can be 
        // included.
        if (ancestorsOfPathsToSkip.find(primPath) != 
                ancestorsOfPathsToSkip.end()) {            
            continue;
        }

        // Check if any of the descendants of the prim have transform overrides.
        // If yes, we need to continue the travesal down to find prims whose 
        // bounds can be included.
        if (ancestorsOfOverrides.find(primPath) != ancestorsOfOverrides.end()) {
            continue;
        }

        // Check to see if any of the ancestors of the prim or the prim itself 
        // has an xform override. 
        SdfPath pathWithOverride = primPath;
        bool foundAncestorWithOverride = false;
        TfHashMap<SdfPath, GfMatrix4d, SdfPath::Hash>::const_iterator 
            overrideIter;
        while (pathWithOverride != prim.GetPath()) {
            overrideIter = ctmOverrides.find(pathWithOverride);
            if (overrideIter != ctmOverrides.end()) {
                // We're only interested in the nearest override since we 
                // have the override CTMs in the given prim's space.
                foundAncestorWithOverride = true;
                break;
            }
            pathWithOverride = pathWithOverride.GetParentPath();
        }
        
        GfBBox3d bbox;
        if (!foundAncestorWithOverride) {
            bbox = ComputeRelativeBound(p, prim);
        } else {
            // Compute bound relative to the path for which we know the 
            // corrected prim-relative CTM.
            bbox = ComputeRelativeBound(p, 
                prim.GetStage()->GetPrimAtPath(overrideIter->first));

            // The override CTM is already relative to the given prim.
            const GfMatrix4d &overrideXform = overrideIter->second;
            bbox.Transform(overrideXform);
        }

        result = GfBBox3d::Combine(result, bbox);
        it.PruneChildren();
    }

    return result;
}

void
UsdGeomBBoxCache::Clear()
{
    TF_DEBUG(USDGEOM_BBOX).Msg("[BBox Cache] CLEARED\n");
    _ctmCache.Clear();
    _bboxCache.clear();
}

void
UsdGeomBBoxCache::SetIncludedPurposes(const TfTokenVector& includedPurposes)
{
    _includedPurposes = includedPurposes;
}

GfBBox3d
UsdGeomBBoxCache::_GetCombinedBBoxForIncludedPurposes(
    const _PurposeToBBoxMap &bboxes)
{
    GfBBox3d combinedBound;
    TF_FOR_ALL(purposeIt, _includedPurposes) {
        _PurposeToBBoxMap::const_iterator it = bboxes.find(*purposeIt);
        if (it != bboxes.end()) {
            const GfBBox3d &bboxForPurpose = it->second;
            if (!bboxForPurpose.GetRange().IsEmpty())
                combinedBound = GfBBox3d::Combine(combinedBound, bboxForPurpose);
        }
    }
    return combinedBound;
}

void
UsdGeomBBoxCache::SetTime(UsdTimeCode time)
{
    if (time == _time)
        return;

    // If we're switching time into or out of default, then clear all the 
    // entries in the cache.
    // 
    // This is done because the _IsVarying() check (below) returns false for an
    // attribute when
    // * it has a default value,
    // * it has a single time sample and
    // * its default value is different from the varying time sample.
    //
    // This is an optimization that works well when playing through a shot and 
    // computing bboxes sequentially.
    // 
    // It should not common to compute bboxes at the default frame. Hence, 
    // clearing all values here should not cause any performance issues.
    // 
    bool clearUnvarying = false;
    if (_time == UsdTimeCode::Default() || time == UsdTimeCode::Default())
        clearUnvarying = true;
    TF_DEBUG(USDGEOM_BBOX).Msg("[BBox Cache] Setting time: %f "
                               " clearUnvarying: %s\n",
                               time.GetValue(),
                               clearUnvarying ? "true": "false");

    TF_FOR_ALL(it, _bboxCache) {
        if (clearUnvarying || it->second.isVarying) {
            it->second.isComplete = false;
            // Clear cached bboxes.
            it->second.bboxes.clear();
            TF_DEBUG(USDGEOM_BBOX).Msg("[BBox Cache] invalidating %s "
                                       "for time change\n",
                                       it->first.GetPath().GetText());
        }
    }
    _time = time;
    _ctmCache.SetTime(_time);
}

// -------------------------------------------------------------------------- //
// UsdGeomBBoxCache Private API
// -------------------------------------------------------------------------- //

bool
UsdGeomBBoxCache::_ShouldIncludePrim(const UsdPrim& prim)
{
    TRACE_FUNCTION();
    // Only imageable prims participate in child bounds accumulation.
    if (!prim.IsA<UsdGeomImageable>()) {
        TF_DEBUG(USDGEOM_BBOX).Msg("[BBox Cache] excluded, not IMAGEABLE type. "
                                   "prim: %s, primType: %s\n",
                                   prim.GetPath().GetText(),
                                   prim.GetTypeName().GetText());

        return false;
    }

    UsdGeomImageable img(prim);
    TfToken vis;
    if (img.GetVisibilityAttr().Get(&vis, _time)
        && vis == UsdGeomTokens->invisible) {
        TF_DEBUG(USDGEOM_BBOX).Msg("[BBox Cache] excluded for VISIBILITY. "
                                   "prim: %s visibility: %s\n",
                                   prim.GetPath().GetText(),
                                   vis.GetText());
        return false;
    }

    return true;
}

template <class AttributeOrQuery>
static bool
_IsVaryingImpl(const UsdTimeCode time, const AttributeOrQuery& attr) 
{
    // XXX: Copied from UsdImagingDelegate::_TrackVariability.
    // XXX: This logic is highly sensitive to the underlying quantization of
    //      time. Also, the epsilon value (.000001) may become zero for large
    //      time values.
    double lower, upper, queryTime;
    bool hasSamples;
    queryTime = time.IsDefault() ? 1.000001 : time.GetValue() + 0.000001;
    // TODO: migrate this logic into UsdAttribute.
    if (attr.GetBracketingTimeSamples(queryTime, &lower, &upper, &hasSamples)
        && hasSamples)
    {
        // The potential results are:
        //    * Requested time was between two time samples
        //    * Requested time was out of the range of time samples (lesser)
        //    * Requested time was out of the range of time samples (greater)
        //    * There was a time sample exactly at the requested time or
        //      there was exactly one time sample.
        // The following logic determines which of these states we are in.

        // Between samples?
        if (lower != upper) {
            return true;
        } 

        // Out of range (lower) or exactly on a time sample?
        attr.GetBracketingTimeSamples(lower+.000001, 
                                      &lower, &upper, &hasSamples);
        if (lower != upper) {
            return true;
        }

        // Out of range (greater)?
        attr.GetBracketingTimeSamples(lower-.000001, 
                                      &lower, &upper, &hasSamples);
        if (lower != upper) {
            return true;
        }
        // Really only one time sample --> not varying for our purposes
    } 
    return false;
}

bool
UsdGeomBBoxCache::_IsVarying(const UsdAttribute& attr)
{
    return _IsVaryingImpl(_time, attr);
}

bool 
UsdGeomBBoxCache::_IsVarying(const UsdAttributeQuery& query)
{
    return _IsVaryingImpl(_time, query);
}

// Returns true if the given prim is a component or a subcomponent.
static 
bool 
_IsComponentOrSubComponent(const UsdPrim &prim) 
{
    UsdModelAPI model(prim);
    TfToken kind;
    if (!model.GetKind(&kind))
        return false;


    return KindRegistry::IsA(kind, KindTokens->component) || 
        KindRegistry::IsA(kind, KindTokens->subcomponent);
}

// Returns the nearest ancestor prim that's a component or a subcomponent, or 
// the stage's pseudoRoot if none are found. For the purpose of computing 
// bounding boxes, subcomponents as treated similar to components, i.e. child 
// bounds are accumulated in subcomponent-space for prims that are underneath
// a subcomponent.
// 
static 
UsdPrim
_GetNearestComponent(const UsdPrim &prim) 
{
    UsdPrim modelPrim = prim;
    while (modelPrim) {
        if (_IsComponentOrSubComponent(modelPrim))
            return modelPrim;

        modelPrim = modelPrim.GetParent();        
    }

    // If we get here, it means we did not find a model or a subcomponent at or
    // above the given prim. Hence, return the stage's pseudoRoot.
    return prim.GetStage()->GetPseudoRoot();
}

TfToken 
UsdGeomBBoxCache::_ComputePurpose(const UsdPrim &prim) 
{
    TfToken purpose;

    UsdGeomImageable img(prim);

    UsdPrim parentPrim = prim.GetParent();
    if (parentPrim && parentPrim.GetPath() != SdfPath::AbsoluteRootPath()) {
        // Try and get the parent prim's purpose first. If we find it in the 
        // cache, we can compute this prim's purpose efficiently by avoiding the 
        // n^2 recursion which results from using the 
        // UsdGeomImageable::ComputePurpose() API directly.
        // 
        _PrimBBoxHashMap::iterator parentEntryIter = 
            _bboxCache.find(parentPrim);
        if (parentEntryIter != _bboxCache.end()) {
            const TfToken &parentPurpose = parentEntryIter->second.purpose;
            // parentPurpose could be empty when "prim" is the root prim of the 
            // subgraph for which bounds are being computed. In this case, we 
            // fallback to using UsdGeomImageable::ComputePurpose().
            if (!parentPurpose.IsEmpty()) {
                if (parentPurpose == UsdGeomTokens->default_) {
                    if (img) {
                        img.GetPurposeAttr().Get(&purpose);
                    } else {
                        purpose = UsdGeomTokens->default_;
                    }
                } else {
                    purpose = parentPurpose;
                }
            }
        }
    }

    if (purpose.IsEmpty()) {
        purpose = img ? img.ComputePurpose() 
                      : UsdGeomTokens->default_;
    }

    return purpose;
}

bool
UsdGeomBBoxCache::_ShouldPruneChildren(const UsdPrim &prim, 
                                       UsdGeomBBoxCache::_Entry *entry)
{
   // If the entry is already complete, we don't need to try to initialize it.
    if (entry->isComplete) {
        return true;
    }

    if (prim.GetPath() != SdfPath::AbsoluteRootPath() && 
        _useExtentsHint && prim.IsModel()) {

        UsdAttribute extentsHintAttr 
            = UsdGeomModelAPI(prim).GetExtentsHintAttr();
        VtVec3fArray extentsHint;
        if (extentsHintAttr 
            && extentsHintAttr.Get(&extentsHint, _time)
            && extentsHint.size() >= 2) {
            return true;
        }
    }

    return false;
}

UsdGeomBBoxCache::_Entry* 
UsdGeomBBoxCache::_FindOrCreateEntriesForPrim(
    const UsdPrim& prim,
    std::vector<UsdPrim>* masterPrims)
{
    // If the bound is in the cache, return it.
    _Entry* entry = TfMapLookupPtr(_bboxCache, prim);
    if (entry && entry->isComplete) {
        const _PurposeToBBoxMap& bboxes = entry->bboxes;
        TF_DEBUG(USDGEOM_BBOX).Msg("[BBox Cache] hit: %s %s\n",
            prim.GetPath().GetText(),
            TfStringify(_GetCombinedBBoxForIncludedPurposes(bboxes)).c_str());
        return entry;
    }
    TF_DEBUG(USDGEOM_BBOX).Msg("[BBox Cache] miss: %s\n",
                                prim.GetPath().GetText());

    // Pre-populate all cache entries, note that some entries may already exist.
    // Note also we do not exclude unloaded prims - we want them because they
    // may have authored extentsHints we can use; thus we can have bboxes in
    // model-hierarchy-only.

    TfHashSet<UsdPrim, _UsdPrimHash> seenMasterPrims;

    for (UsdTreeIterator it(
            prim, (UsdPrimIsActive && UsdPrimIsDefined 
                   && !UsdPrimIsAbstract)); it; ++it) {

        _PrimBBoxHashMap::iterator cacheIt = _bboxCache.insert(
            std::make_pair(*it, _Entry())).first;
        if (_ShouldPruneChildren(*it, &cacheIt->second)) {
            // The entry already exists and is complete, we don't need 
            // the child entries for this query.
            it.PruneChildren();
        }

        if (it->IsInstance()) {
            // This prim is an instance, so we need to compute 
            // bounding boxes for the master prims.
            const UsdPrim master = it->GetMaster();
            if (seenMasterPrims.insert(master).second) {
                masterPrims->push_back(master);
            }
            it.PruneChildren();
        }
    }

    // isIncluded only gets cached in the multi-threaded path for child prims,
    // make sure the prim we're querying has the correct flag cached also. We
    // can't do this in _ResolvePrim because we need to the flag for children
    // before recursing upon them.
    //
    // Note that this means we always have an entry for the given prim,
    // even if that prim does not pass the predicate given to the tree
    // iterator above (e.g., the prim is a class).
    entry = &(_bboxCache[prim]);
    entry->isIncluded = _ShouldIncludePrim(prim);

    return entry;
}

bool
UsdGeomBBoxCache::_Resolve(
    const UsdPrim& prim, 
    UsdGeomBBoxCache::_PurposeToBBoxMap *bboxes)
{
    TRACE_FUNCTION();
    // NOTE: Bounds are cached in local space, but computed in world space.

    // Drop the GIL here if we have it before we spawn parallel tasks, since
    // resolving properties on prims in worker threads may invoke plugin code
    // that needs the GIL.
    TF_PY_ALLOW_THREADS_IN_SCOPE();

    // If the bound is in the cache, return it.
    std::vector<UsdPrim> masterPrims;
    _Entry* entry = _FindOrCreateEntriesForPrim(prim, &masterPrims);
    if (entry && entry->isComplete) {
        *bboxes = entry->bboxes;
        return (!bboxes->empty());
    }

    // Resolve all master prims first to avoid having to synchronize
    // tasks that depend on the same master.
    if (!masterPrims.empty()) {
        _MasterBBoxResolver bboxesForMasters(this);
        bboxesForMasters.Resolve(masterPrims);
    }

    // XXX: This swapping out is dubious... see XXX below.
    _ThreadXformCache xfCaches;
    xfCaches.local().Swap(_ctmCache);

    // Find the nearest ancestor prim that's a model or a subcomponent. 
    UsdPrim modelPrim = _GetNearestComponent(prim);
    GfMatrix4d inverseComponentCtm = _ctmCache.GetLocalToWorldTransform(
        modelPrim).GetInverse();

    _BBoxTask& rootTask = *new(tbb::task::allocate_root()) 
                               _BBoxTask(prim, inverseComponentCtm, 
                                         this, &xfCaches);
    tbb::task::spawn_root_and_wait(rootTask);

    // We save the result of one of the caches, but it might be interesting to
    // merge them all here at some point.
    // XXX: Is this valid?  This only makes sense if we're *100% certain* that
    // rootTask above runs in this thread.  If it's picked up by another worker
    // it won't populate the local xfCaches we're swapping with.
    xfCaches.local().Swap(_ctmCache);

    // Note: the map may contain unresolved entries, but future queries will 
    // populate them.

    // If the bound is in the cache, return it.
    entry = TfMapLookupPtr(_bboxCache, prim);
    *bboxes = entry->bboxes;
    return (!bboxes->empty());
}

bool
UsdGeomBBoxCache::_GetBBoxFromExtentsHint(
    const UsdGeomModelAPI &geomModel,
    const UsdAttributeQuery &extentsHintQuery,
    _PurposeToBBoxMap *bboxes)
{
    VtVec3fArray extents;

    if (!extentsHintQuery || !extentsHintQuery.Get(&extents, _time)){
        if (TfDebug::IsEnabled(USDGEOM_BBOX) &&
            !geomModel.GetPrim().IsLoaded()){
            TF_DEBUG(USDGEOM_BBOX).Msg("[BBox Cache] MISSING extentsHint for "
                                       "UNLOADED model %s.\n", 
                                       geomModel.GetPrim().GetPath()
                                       .GetString().c_str()); 
        }

        return false;
    }

    TF_DEBUG(USDGEOM_BBOX).Msg("[BBox Cache] Found cached extentsHint for "
        "model %s.\n", geomModel.GetPrim().GetPath().GetString().c_str()); 

    const TfTokenVector &purposeTokens = 
        UsdGeomImageable::GetOrderedPurposeTokens();

    for(size_t i = 0; i < purposeTokens.size(); ++i) {
        size_t idx = i*2;
        // If extents are not available for the value of purpose, it 
        // implies that the rest of the bounds are empty. 
        // Hence, we can break.
        if ((idx + 2) > extents.size())
            break;

        (*bboxes)[purposeTokens[i]] = 
            GfBBox3d( GfRange3d(extents[idx], extents[idx+1]) );
    }

    return true;
}

bool
UsdGeomBBoxCache::_ComputeMissingExtent(
    const UsdGeomPointBased &pointBasedObj, 
    const VtVec3fArray &points,
    VtVec3fArray* extent)
{
    //  We provide this method to compute extent for PointBased prims.
    //  Specifically, if a pointbased prim does not have a valid authored 
    //  extent we try to compute it here.
    //  See Bugzilla #s 97111, 115735.

    // Calculate Extent Based on Prim Type
    if (UsdGeomPoints pointsObj = UsdGeomPoints(pointBasedObj.GetPrim())) {
        
        // Extract any width data
        VtFloatArray widths;
        bool hasWidth = pointsObj.GetWidthsAttr().Get(&widths);

        if (hasWidth) {
            return UsdGeomPoints::ComputeExtent(points, widths, extent);
        }

    } else if (UsdGeomCurves curvesObj = 
                    UsdGeomCurves(pointBasedObj.GetPrim())) {
        // Calculate Extent for a Curve;

        // XXX: All curves can be bounded by their control points, excluding
        //      catmull rom and hermite. For now, we treat hermite and catmull
        //      rom curves like their convex-hull counterparts. While there are
        //      some bounds approximations we could perform, hermite's
        //      implementation is not fully supported and catmull rom splines
        //      are very rare. For simplicity, we ignore these odd corner cases
        //      and provide a still reasonable approximation, but we also 
        //      recognize there could be some out-of-bounds error. 
        //      For the purposes of BBox-Cache extent fallback, some small 
        //      chance of error is probably OK.

        // Extract any width data; if no width, create 0 width array
        VtFloatArray widths;
        if (!curvesObj.GetWidthsAttr().Get(&widths)) {
            widths.push_back(0);
        }

        return UsdGeomCurves::ComputeExtent(points, widths, extent);
    }

    // The prim should be calculated as a PointBased;
    return UsdGeomPointBased::ComputeExtent(points, extent);
}

void 
UsdGeomBBoxCache::_ResolvePrim(_BBoxTask* task, 
                               const UsdPrim& prim, 
                               const GfMatrix4d &inverseComponentCtm)
{
    TRACE_FUNCTION();
    // NOTE: Bounds are cached in local space, but computed in world space.

    // If the bound is in the cache, return it.
    _Entry* entry = TfMapLookupPtr(_bboxCache, prim);
    if (!TF_VERIFY(entry != NULL))
        return;

    _PurposeToBBoxMap *bboxes = &entry->bboxes;
    if (entry->isComplete) {
        TF_DEBUG(USDGEOM_BBOX).Msg("[BBox Cache] Dependent cache hit: "
            "%s %s\n", prim.GetPath().GetText(),
            TfStringify(_GetCombinedBBoxForIncludedPurposes(*bboxes)).c_str());
        return;
    }
    TF_DEBUG(USDGEOM_BBOX).Msg("[BBox Cache] Dependent cache miss: %s\n",
                                prim.GetPath().GetText());

    // Initially the bboxes hash map is empty, which implies empty bounds.

    UsdGeomXformCache &xfCache = task->GetXformCaches()->local();

    // Setting the time redundantly will be a no-op;
    xfCache.SetTime(_time);

    // Compute the purpose for the entry.
    TfToken &purpose = entry->purpose;
    if (purpose.IsEmpty()) {
        purpose = _ComputePurpose(prim);
    }

    // Check if the prim is a model and has extentsHint
    const bool useExtentsHintForPrim = 
        (_useExtentsHint && prim.IsModel() &&
            prim.GetPath() != SdfPath::AbsoluteRootPath());

    boost::shared_array<UsdAttributeQuery> &queries = entry->queries;
    if (!queries) {
        // If this cache doesn't use extents hints, we don't need the
        // corresponding query. 
        const size_t numQueries = 
            (useExtentsHintForPrim ? NumQueries : NumQueries - 1);
        queries.reset(new UsdAttributeQuery[numQueries]);
    }

    if (useExtentsHintForPrim) {
        UsdGeomModelAPI geomModel(prim);
        const UsdAttributeQuery& extentsHintQuery = 
            _GetOrCreateExtentsHintQuery(geomModel, &queries[ExtentsHint]);

        if (_GetBBoxFromExtentsHint(geomModel, extentsHintQuery, bboxes)) {
            entry->isComplete = true;

            // XXX: Do we only need to be doing the following in
            //      the non-varying case, similar to below?
            entry->isVarying = _IsVarying(extentsHintQuery);
            entry->isIncluded = _ShouldIncludePrim(prim);
            if (entry->isVarying) {
                entry->queries = queries;
            }
            return;
        }
    }

    // We only check when isVarying is false, since when an entry doesn't
    // vary over time, this code will only be executed once. If an entry has
    // been marked as varying, we need not check if it's varying again.
    // This relies on entries being initialized with isVarying=false.
    if (!entry->isVarying) {
        // Note that child variability is also accumulated into
        // entry->isVarying (below).

        UsdAttributeQuery visQuery;
        _GetOrCreateVisibilityQuery(prim, &visQuery);
        const UsdAttributeQuery& extentQuery =
            _GetOrCreateExtentQuery(prim, &queries[Extent]);

        UsdGeomXformable xformable(prim);
        entry->isVarying = 
            (xformable && xformable.TransformMightBeTimeVarying())
            || (extentQuery && _IsVarying(extentQuery))
            || (visQuery && _IsVarying(visQuery));
    }

    // Leaf gprims and boundable intermediate prims.
    //
    // When boundable prims have an authored extent, it is expected to
    // incorporate the extent of all children, which are pruned from further
    // traversal.
    GfRange3d myRange;
    bool pruneChildren = false;

    // Attempt to resolve a boundable prim's extent. If no extent is authored,
    // we attempt to create it for usdGeomPointBased and child classes. If
    // it cannot be created or found, the user is notified of an incorrect prim.
    if (prim.IsA<UsdGeomBoundable>()) {
        VtVec3fArray extent;
        // Read the extent of the geometry, an axis-aligned bounding box in
        // local space.
        const UsdAttributeQuery& extentQuery = 
            _GetOrCreateExtentQuery(prim, &queries[Extent]);

        // If some extent is authored, check validity
        bool successGettingExtent = false;
        if (extentQuery.Get(&extent, _time)) {

            successGettingExtent = extent.size() == 2;
            if (!successGettingExtent) {
                TF_WARN("[BBox Cache] Extent for <%s> is of size %zu "
                        "instead of 2.", prim.GetPath().GetString().c_str(),
                        extent.size());
            }
        }

        // If we failed to get extent, try to create it.
        if (!successGettingExtent) {

            // If the prim is a PointBased, try to calculate the extent
            if (UsdGeomPointBased pointBasedObj = UsdGeomPointBased(prim)) {

                // XXX: We check if the points attribute is authored on the given
                // prim. All we require from clients is that IF they author 
                // points, they MUST also author extent.
                // 
                // If no extent is authored, but points has some value, we 
                // compute the extent and display a debug message.
                // 
                // Otherwise, the client is consistent with our demands;
                // no warning is issued, and no extent is computed.
                // 
                // For more information, see bugzilla #115735

                bool primHasAuthoredPoints = 
                    pointBasedObj.GetPointsAttr().HasAuthoredValueOpinion();

                if (primHasAuthoredPoints) {
                    TF_DEBUG(USDGEOM_BBOX).Msg(
                        "[BBox Cache] WARNING: No valid extent authored for "
                        "<%s>. Computing a fallback value.",
                        prim.GetPath().GetString().c_str());

                    // Create extent
                    VtVec3fArray points;
                    if (pointBasedObj.GetPointsAttr().Get(&points)) {
                        successGettingExtent = _ComputeMissingExtent(
                            pointBasedObj, points, &extent);

                        if (!successGettingExtent) {
                            TF_DEBUG(USDGEOM_BBOX).Msg(
                                "[BBox Cache] WARNING: Unable to compute extent for "
                                "<%s>.", prim.GetPath().GetString().c_str());
                        }
                    }
                }

            } else { 
                // Skip non-PointsBased prims without extent. Display a message 
                // if the debug flag is enabled.
                TF_DEBUG(USDGEOM_BBOX).Msg("[BBox Cache] WARNING: No valid "
                    "extent authored for <%s>.", 
                    prim.GetPath().GetString().c_str());

            }
        }

        // On Successful extent, create BBox for purpose.
        if (successGettingExtent) {
            pruneChildren = true;
            GfBBox3d &bboxForPurpose = (*bboxes)[purpose];
            bboxForPurpose.SetRange(GfRange3d(extent[0], extent[1]));
        }
    }

    // --
    // NOTE: bbox is currently in its local space, the space in which
    // we want to cache it.  If we need to merge in child bounds below,
    // though, we will need to temporarily transform it into component space.
    // --
    bool bboxInComponentSpace = false;

    // This will be computed below if the prim has children with bounds.
    GfMatrix4d localToComponentXform(1.0);

    // Accumulate child bounds:
    // 
    //  1) Filter and queue up the children to be processed.
    //  2) Spawn new child tasks and wait for them to complete.
    //  3) Accumulate the results into this cache entry.
    //
    int refCount = 1;

    // Filter childen and queue children.
    if (!pruneChildren) {
        // Compute the enclosing model's (or subcomponent's) inverse CTM. 
        // This will be used to compute the child bounds in model-space.
        const GfMatrix4d &inverseEnclosingComponentCtm = 
                _IsComponentOrSubComponent(prim) ?
                xfCache.GetLocalToWorldTransform(prim).GetInverse() :
                inverseComponentCtm; 

        std::vector<std::pair<UsdPrim, _BBoxTask*> > included;
        // See comment in _Resolve about unloaded prims
        UsdPrimSiblingRange children;

        const bool primIsInstance = prim.IsInstance();
        if (primIsInstance) {
            const UsdPrim master = prim.GetMaster();
            children = master.GetFilteredChildren(
                UsdPrimIsActive && UsdPrimIsDefined && !UsdPrimIsAbstract);
        }
        else {
            children = prim.GetFilteredChildren(
                UsdPrimIsActive && UsdPrimIsDefined && !UsdPrimIsAbstract);
        }

        TF_FOR_ALL(childIt, children) {
            const UsdPrim &childPrim = *childIt;
            // Skip creating bbox tasks for excluded children.
            //
            // We must do this check here on the children, because when an
            // invisible prim is queried explicitly, we want to return the bound
            // to the client, even if that prim's bbox is not included in the
            // parent bound.
            _PrimBBoxHashMap::iterator it = _bboxCache.find(childPrim);
            if (!TF_VERIFY(it != _bboxCache.end(), "Could not find prim <%s>"
                "in the bboxCache.", childPrim.GetPath().GetText())) {
                continue;
            }

            _Entry* childEntry = &it->second;

            // If we're about to process the child for the first time, we must
            // populate isIncluded.
            if (!childEntry->isComplete)
                childEntry->isIncluded = _ShouldIncludePrim(childPrim);

            // We're now confident that the cached flag is correct.
            if (!childEntry->isIncluded) {
                // If the child prim is excluded, mark the parent as varying 
                // if the child is imageable and its visibility is varying.
                // This will ensure that the parent entry gets dirtied when 
                // the child becomes visible.
                UsdGeomImageable img (childPrim);
                if (img)
                    entry->isVarying |= _IsVarying(img.GetVisibilityAttr());
                continue;
            }

            // Queue up the child to be processed.
            if (primIsInstance) {
                // If the prim we're processing is an instance, all of its
                // child prims will come from its master prim. The bboxes
                // for these prims should already have been computed in
                // _Resolve, so we don't need to schedule an additional task.
                included.push_back(std::make_pair(*childIt, (_BBoxTask*)0));
            }
            else {
                included.push_back(std::make_pair(*childIt,
                    new(task->allocate_child())
                        _BBoxTask(*childIt,
                                  inverseEnclosingComponentCtm,
                                  this, 
                                  task->GetXformCaches())));
                ++refCount;
            }
        }

        // Spawn and wait.
        //
        // Warning: calling spawn() before set_ref_count results in undefined
        // behavior. 
        // 
        // All the child bboxTasks will be NULL if the prim is an instance.
        // 
        if (!primIsInstance) {
            task->set_ref_count(refCount);
            TF_FOR_ALL(childIt, included) {
                if (childIt->second) {
                    task->spawn(*childIt->second);
                }
            }
            task->wait_for_all();

            // We may have switched threads, grab the thread-local xfCache again.
            xfCache = task->GetXformCaches()->local();
            xfCache.SetTime(_time);
        }

        // Accumulate child results.
        // Process the child bounding boxes, accumulating their variability and
        // volume into this cache entry.
        TF_FOR_ALL(childIt, included) {
            // The child's bbox is returned in local space, so we must convert
            // it to model space to be compatible with the current bbox.
            UsdPrim const& childPrim = childIt->first;
            const _Entry* childEntry = TfMapLookupPtr(_bboxCache, childPrim);
            if (!TF_VERIFY(childEntry->isComplete))
                continue;

            // Accumulate child variability.
            entry->isVarying |= childEntry->isVarying;

            // Accumulate child bounds.
            if (!childEntry->bboxes.empty()) {
                if (!bboxInComponentSpace){
                    // Put the local extent into "baked" component space, i.e.
                    // a bbox with identity transform
                    localToComponentXform = 
                        xfCache.GetLocalToWorldTransform(prim) *
                        inverseEnclosingComponentCtm;

                    TF_FOR_ALL(bboxIt, *bboxes) {
                        GfBBox3d &bbox = bboxIt->second;
                        bbox.SetMatrix(localToComponentXform);
                        bbox = GfBBox3d(bbox.ComputeAlignedRange());
                    }

                    bboxInComponentSpace = true;
                }

                _PurposeToBBoxMap childBBoxes = childEntry->bboxes;

                GfMatrix4d childLocalToComponentXform;
                if (primIsInstance) {
                    bool resetsXf = false;
                    childLocalToComponentXform = 
                        xfCache.GetLocalTransformation(childPrim, &resetsXf) *
                        localToComponentXform;
                }
                else {
                    childLocalToComponentXform = 
                        xfCache.GetLocalToWorldTransform(childPrim) * 
                        inverseEnclosingComponentCtm;
                }

                // Convert the resolved BBox to component space.
                TF_FOR_ALL(childBBoxIt, childBBoxes) {
                    const TfToken purposeToken = childBBoxIt->first;

                    GfBBox3d &childBBox = childBBoxIt->second;
                    childBBox.Transform(childLocalToComponentXform);

                    // Since the range is in component space and the matrix is
                    // identity, we can union in component space.
                    GfBBox3d &bbox = (*bboxes)[purposeToken];
                    bbox.SetRange(GfRange3d(bbox.GetRange()).UnionWith(
                                  childBBox.ComputeAlignedRange()));
                }
            }
        }
    }

    // All prims must be cached in local space: convert bbox from component to 
    // local space.
    if (bboxInComponentSpace) {
        // When children are accumulated, the bbox range is in component space, 
        // so we must apply the inverse component-space transform 
        // (component-to-local) to move it to local space.
        GfMatrix4d componentToLocalXform = localToComponentXform.GetInverse();
        TF_FOR_ALL(bboxIt, *bboxes) {
            GfBBox3d &bbox = bboxIt->second;
            bbox.SetMatrix(componentToLocalXform);
        }
    } 

    // --
    // NOTE: bbox is now in local space, either via the matrix or range.
    // --

    // Performance note: we could leverage the fact that the bound is initially
    // computed in world space and avoid an extra transformation for recursive
    // calls, however that optimization was not significant in early tests.
    
    // Stash away queries for varying entries so they can be reused
    // for computations at other times.
    if (entry->isVarying) {
        entry->queries = queries;
    }
    
    // Mark as cached and return.
    entry->isComplete = true;
    TF_DEBUG(USDGEOM_BBOX).Msg("[BBox Cache] resolved value: %s %s "
        "(varying: %s)\n",
        prim.GetPath().GetText(),
        TfStringify(_GetCombinedBBoxForIncludedPurposes(*bboxes)).c_str(),
        entry->isVarying ? "true" : "false");
}

PXR_NAMESPACE_CLOSE_SCOPE

