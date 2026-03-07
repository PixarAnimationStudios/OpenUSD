//
// Copyright 2026 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/exec/esfUsd/stageData.h"

#include "pxr/exec/esfUsd/attribute.h"
#include "pxr/exec/esfUsd/object.h"
#include "pxr/exec/esfUsd/prim.h"
#include "pxr/exec/esfUsd/property.h"
#include "pxr/exec/esfUsd/relationship.h"

#include "pxr/base/tf/diagnosticLite.h"
#include "pxr/base/tf/mallocTag.h"
#include "pxr/base/tf/scopeDescription.h"
#include "pxr/base/trace/trace.h"
#include "pxr/base/work/loops.h"
#include "pxr/usd/usd/attribute.h"
#include "pxr/usd/usd/prim.h"
#include "pxr/usd/usd/schemaRegistry.h"
#include "pxr/usd/usd/stage.h"

#include <tbb/concurrent_hash_map.h>

#include <iterator>
#include <set>
#include <utility>

PXR_NAMESPACE_OPEN_SCOPE

namespace {

struct _Hash {
    bool equal(const UsdStageConstPtr &lhs,
               const UsdStageConstPtr &rhs) const {
        return lhs == rhs;
    }

    size_t hash(const UsdStageConstPtr &stage) const {
        return TfHash::Combine(stage);
    }
};

} // anonymous namespace

using _StageDataTable =
    tbb::concurrent_hash_map<
        UsdStageConstPtr, std::weak_ptr<EsfUsdStageData>, _Hash>;

static _StageDataTable stageDataTable;

EsfUsdStageData::~EsfUsdStageData() = default;

EsfUsdStageData::EsfUsdStageData(
    const UsdStageConstPtr &stage)
    : _stage(stage)
{
    _PopulateConnectionTables();
}

std::shared_ptr<EsfUsdStageData>
EsfUsdStageData::RegisterStage(
    const UsdStageConstPtr &stage)
{
    TfAutoMallocTag tag("Exec", __ARCH_PRETTY_FUNCTION__);

    std::shared_ptr<EsfUsdStageData> ptr;

    // If we can get a const accessor to the hash map entry and we can lock the
    // shared_ptr, then return it.
    if (_StageDataTable::const_accessor accessor;
        stageDataTable.find(accessor, stage)) {
        if (ptr = accessor->second.lock()) {
            return ptr;
        }
    }

    // Otherwise, we attempt to emplace a new entry and take a write lock.
    _StageDataTable::accessor accessor; 
    stageDataTable.emplace(accessor, stage, std::weak_ptr<EsfUsdStageData>());

    // If we get a non-null pointer here, there's already a stage data entry,
    // so we just return the shared_ptr.
    if (ptr = accessor->second.lock()) {
        return ptr;
    }

    // If we get here, no stage data has been allocated for this map entry, and
    // we hold a write lock for the entry. Create the stage data and store it
    // in the map.
    ptr.reset(new EsfUsdStageData(stage));
    accessor->second = ptr;
    return ptr;
}

EsfUsdStageData &
EsfUsdStageData::GetStageData(
    const UsdStageConstPtr &stage)
{
    TF_VERIFY(stage);

    _StageDataTable::const_accessor accessor;
    const bool found = stageDataTable.find(accessor, stage);
    if (!TF_VERIFY(found)) {
        static EsfUsdStageData empty(nullptr);
        return empty;
    }

    // TODO: It's unfortunate that we need to perform atomic operations to get
    // to the underlying data, given that we require that it's safe to do so.
    // One idea is that we could store a raw pointer alongside the weak_ptr and
    // simply dereference that, and TF_VERIFY(!accessor->second.expired()).
    const std::shared_ptr<EsfUsdStageData> ptr = accessor->second.lock();
    if (!TF_VERIFY(ptr)) {
        static EsfUsdStageData empty(nullptr);
        return empty;
    }

    // We return a plain reference to the held data. That is safe to do only if
    // the client fulfills the contract that requires that it hold a strong
    // reference to the data while calling this method and while dereferencing
    // the returned reference.
    return *ptr;
}  

void
EsfUsdStageData::_PopulateConnectionTables()
{
    TRACE_FUNCTION();
    TfAutoMallocTag tag("Exec", __ARCH_PRETTY_FUNCTION__);
    TF_DESCRIBE_SCOPE("Populating exec connection tables");

    if (!TF_VERIFY(_stage)) {
        return;
    }

    const auto range = _stage->GetPseudoRoot().GetDescendants();
    WorkParallelForEach(
        range.begin(), range.end(),
        [&incoming = _incoming, &outgoing = _outgoing]
        (const UsdPrim &prim)
    {
        for (const UsdAttribute &attr : prim.GetAttributes()) {
            SdfPathVector targetPaths;
            if (!attr.GetConnections(&targetPaths)) {
                continue;
            }

            const SdfPath ownerPath = attr.GetPath();
            for (const SdfPath &targetPath : targetPaths) {
                incoming[targetPath].push_back(ownerPath);
            }
            outgoing.emplace(ownerPath, std::move(targetPaths));
        }
    });
}

const SdfPathVector &
EsfUsdStageData::GetOutgoingConnections(
    const UsdStageConstPtr &stage,
    const SdfPath &attrPath)
{
    if (!TF_VERIFY(stage)) {
        static const SdfPathVector empty;
        return empty;
    }

    return GetStageData(stage)._GetOutgoingConnections(attrPath);
}

const SdfPathVector &
EsfUsdStageData::_GetOutgoingConnections(
    const SdfPath &attrPath)
{
    static const SdfPathVector empty;

    const auto it = _outgoing.find(attrPath);
    return it != _outgoing.end() ? it->second : empty;
}

SdfPathVector
EsfUsdStageData::GetIncomingConnections(
    const UsdStageConstPtr &stage,
    const SdfPath &targetPath)
{
    if (!TF_VERIFY(stage)) {
        return SdfPathVector();
    }

    return GetStageData(stage)._GetIncomingConnections(targetPath);
}

SdfPathVector
EsfUsdStageData::_GetIncomingConnections(
    const SdfPath &targetPath)
{
    const auto it = _incoming.find(targetPath);
    if (it == _incoming.end()) {
        return {};
    }

    TRACE_FUNCTION();

    const tbb::concurrent_vector<SdfPath> &connections = it->second;
    SdfPathVector result;
    result.reserve(connections.size());
    for (const SdfPath &path : connections) {
        result.push_back(path);
    }
    return result;
}

void
EsfUsdStageData::UpdateForChangedAttributeConnections(
    const SdfPath &attrPath,
    ChangedPathSet *const incomingConnectionsChanged)
{
    TRACE_FUNCTION();
    TfAutoMallocTag tag("Exec", __ARCH_PRETTY_FUNCTION__);

    if (!TF_VERIFY(_stage)) {
        return;
    }

    // If the attribute is inactive, unloaded, undefined, or abstract, treat it
    // as if it's not in the scene.
    const UsdAttribute attr = [&] {
        const UsdAttribute attr = _stage->GetAttributeAtPath(attrPath);
        return attr && UsdPrimDefaultPredicate(attr.GetPrim())
            ? attr : UsdAttribute();
    }();

    SdfPathVector newConnections, addedTargetPaths, removedTargetPaths;
    _GetChangedConnectionTargets(
        attrPath, attr,
        &newConnections, &addedTargetPaths, &removedTargetPaths);

    // Note that we are careful not to return early here if the connection
    // order may have changed.
    if (newConnections.empty() &&
        addedTargetPaths.empty() &&
        removedTargetPaths.empty()) {
        return;
    }

    // Update the incoming connections table
    for (const SdfPath &targetPath : addedTargetPaths) {
        _incoming[targetPath].push_back(attrPath);
    }
    for (const SdfPath &removedPath : removedTargetPaths) {
        const auto incomingIt = _incoming.find(removedPath);
        if (!TF_VERIFY(incomingIt != _incoming.end())) {
            continue;
        }

        // To remove from the concurrent_vector, move the last element into the
        // position of the removed element and resize.
        tbb::concurrent_vector<SdfPath> &owners = incomingIt->second;
        const auto it = std::find(owners.begin(), owners.end(), attrPath);
        if (!TF_VERIFY(it != owners.end())) {
            continue;
        }

        const auto last = std::prev(owners.end());
        if (it != last) {
            *it = std::move(*last);
        }
        owners.resize(owners.size() - 1);
    }

    // Update the outgoing connections table
    if (newConnections.empty()) {
        _outgoing.unsafe_erase(attrPath);
    } else {
        _outgoing[attrPath] = std::move(newConnections);
    }

    for (SdfPath &path : addedTargetPaths) {
        incomingConnectionsChanged->insert(std::move(path));
    }
    for (SdfPath &path : removedTargetPaths) {
        incomingConnectionsChanged->insert(std::move(path));
    }
}

void
EsfUsdStageData::UpdateForResync(
    const SdfPath &resyncedPath,
    ChangedPathSet *const incomingConnectionsChanged)
{
    TRACE_FUNCTION();
    TfAutoMallocTag tag("Exec", __ARCH_PRETTY_FUNCTION__);
    TF_DESCRIBE_SCOPE("Updating exec connection tables in response to resync");

    if (!TF_VERIFY(_stage)) {
        return;
    }

    if (resyncedPath.IsPropertyPath()) {
        UpdateForChangedAttributeConnections(
            resyncedPath, incomingConnectionsChanged);
        return;
    }

    // We only expect resyncs for prims or properties.
    if (!TF_VERIFY(resyncedPath.IsPrimPath())) {
        return;
    }

    // Queue up paths of owner attributes to remove in series.
    tbb::concurrent_vector<SdfPath> ownerAttrsToRemove;

    // Queue up incoming connections to be removed, since we can't remove these
    // elements in parallel. This is a map from owning attribute to vector of
    // connection target paths that have gone away.
    _IncomingPathTable incomingToRemove;

    WorkDispatcher dispatcher;

    const UsdPrim resyncedPrim = _stage->GetPrimAtPath(resyncedPath);

    // If there's no prim at the resynced path, or not one that is active,
    // loaded, defined, and non-abstract, then we remove entries from the
    // outgoing connection table for all paths under the given prim path. We
    // queue up incoming connections to be removed later.
    // 
    // Note that as long as an owning attribute has a given connection, the
    // incoming connection map entry remains populated, regardless of whether
    // the targeted object exists in the scene.
    if (!resyncedPrim || !UsdPrimDefaultPredicate(resyncedPrim)) {
        TRACE_FUNCTION_SCOPE("Update for expired resynced prim");

        for (auto it = _outgoing.lower_bound(resyncedPath);
             it != _outgoing.end() && it->first.HasPrefix(resyncedPath);) {
            const SdfPath &ownerPath = it->first;
            for (SdfPath &targetPath : it->second) {
                incomingToRemove[targetPath].push_back(ownerPath);
                incomingConnectionsChanged->insert(std::move(targetPath));
            }
            it = _outgoing.unsafe_erase(it);
        }
    }

    // Otherwise, the resynced prim still exists, so make updates for that prim
    // and all descendants of that prim (including descendants that are present
    // in the connection tables, but no longer present in the scene).
    //
    // We do this in stages, first starting from the resynced prim and
    // traversing its namespace descendants in parallel. New outgoing and
    // incoming connections can be added immediately, but removed incoming
    // connections, and owning attributes whose connections should be removed
    // from the outgoing table, must be queued up to be processed in the second
    // stage. Once the parallel traversal is complete, we apply the queued
    // updates to the incoming and outgoing tables.
    else {
        TRACE_FUNCTION_SCOPE("Update for valid prim");

        // Update for the resynced prim.
        dispatcher.Run(
            [this, &resyncedPrim, &incomingToRemove, incomingConnectionsChanged]
        {
            TRACE_SCOPE("Update for resynced prim");

            _UpdateForChangedPrim(
                resyncedPrim, &incomingToRemove, incomingConnectionsChanged);
        });

        // Update for all descendants of the resynced prim.
        dispatcher.Run(
            [this, &resyncedPrim, &incomingToRemove, incomingConnectionsChanged]
        {
            TRACE_SCOPE("Update for resynced prim descendants");

            const auto range = resyncedPrim.GetDescendants();
            WorkParallelForEach(
                range.begin(), range.end(),
                [this, &incomingToRemove, incomingConnectionsChanged]
                (const UsdPrim &prim)
            {
                _UpdateForChangedPrim(
                    prim, &incomingToRemove, incomingConnectionsChanged);
            });
        });

        // Iterate over the outgoing map in parallel to find entries for
        // attributes on the resynced prim and its descendants that no longer
        // exist in the scene.
        dispatcher.Run(
            [this, &resyncedPath, &ownerAttrsToRemove, &incomingToRemove]
        {
            _UpdateForRemovedAttributes(
                resyncedPath, &incomingToRemove, &ownerAttrsToRemove);
        });

        // Wait until we've collected all deferred updates.
        dispatcher.Wait();
    }

    // Remove outgoing connections for the queued owner paths.
    if (!ownerAttrsToRemove.empty()) {
        dispatcher.Run(
            [&outgoing = _outgoing,
             &ownerAttrsToRemove = std::as_const(ownerAttrsToRemove)]
        {
            TRACE_SCOPE("Apply deferred outgoing connections updates");

            for (const SdfPath &path : ownerAttrsToRemove) {
                outgoing.unsafe_erase(path);
            }
        });
    }

    // Remove entries from the incoming table.
    if (!incomingToRemove.empty()) {
        dispatcher.Run(
            [this, &incomingToRemove = std::as_const(incomingToRemove)]
        {
            _RemoveIncomingTableEntries(incomingToRemove);
        });
    }

    dispatcher.Wait();
}

void
EsfUsdStageData::_GetChangedConnectionTargets(
    const SdfPath &attrPath,
    const UsdAttribute &attribute,
    SdfPathVector *const newConnections,
    SdfPathVector *const addedTargetPaths,
    SdfPathVector *const removedTargetPaths) const
{
    TRACE_FUNCTION();

    // If we have no attribute and no record of connections for an attribute at
    // this path, there's nothing to do. At least one way this can happen is if
    // the object at attrPath is a relationship.
    const auto it = _outgoing.find(attrPath);
    if (it == _outgoing.end() && !attribute) {
        return;
    }

    static const SdfPathVector empty;
    const SdfPathVector &oldConnections =
        it == _outgoing.end() ? empty : it->second;

    if (attribute) {
        attribute.GetConnections(newConnections);
    } else {
        newConnections->clear();
    }

    const std::set<SdfPath> oldSet(
        oldConnections.begin(), oldConnections.end());
    const std::set<SdfPath> newSet(
        newConnections->begin(), newConnections->end());

    std::set_difference(
        newSet.begin(), newSet.end(),
        oldSet.begin(), oldSet.end(),
        std::inserter(*addedTargetPaths, addedTargetPaths->begin()));

    std::set_difference(
        oldSet.begin(), oldSet.end(),
        newSet.begin(), newSet.end(),
        std::inserter(*removedTargetPaths, removedTargetPaths->begin()));
}

void
EsfUsdStageData::_UpdateForChangedPrim(
    const UsdPrim &prim,
    _IncomingPathTable *const incomingToRemove,
    ChangedPathSet *const incomingConnectionsChanged)
{
    TRACE_FUNCTION();

    // Visit all of the attributes on the prim and get the changed connections.
    for (const UsdAttribute &attribute : prim.GetAttributes()) {
        const SdfPath attrPath = attribute.GetPath();

        SdfPathVector newConnections, addedTargetPaths, removedTargetPaths;
        _GetChangedConnectionTargets(
            attrPath, attribute,
            &newConnections, &addedTargetPaths, &removedTargetPaths);

        // Incoming connections can be added immediately.
        for (const SdfPath &targetPath : addedTargetPaths) {
            _incoming[targetPath].push_back(attrPath);
        }

        // Add new connections to the outgoing table.
        _outgoing[attribute.GetPath()] = std::move(newConnections);

        // Queue up incoming connections to be removed.
        for (const SdfPath &targetPath : removedTargetPaths) {
            (*incomingToRemove)[targetPath].push_back(attrPath);
        }

        for (SdfPath &path : removedTargetPaths) {
            incomingConnectionsChanged->insert(std::move(path));
        }
        for (SdfPath &path : addedTargetPaths) {
            incomingConnectionsChanged->insert(std::move(path));
        }
    }
}

void 
EsfUsdStageData::_UpdateForRemovedAttributes(
    const SdfPath &resyncedPath,
    _IncomingPathTable *const incomingToRemove,
    tbb::concurrent_vector<SdfPath> *const ownerAttrsToRemove) const
{
    TRACE_FUNCTION();

    const auto begin = _outgoing.lower_bound(resyncedPath);
    const auto end = _outgoing.lower_bound(_PathPrefix{resyncedPath});
    if (begin == end) {
        return;
    }

    // Iterate over the outgoing map in parallel to find entries for attributes
    // on the resynced prim and its descendants that no longer exist in the
    // scene.
    WorkParallelForEach(
        begin, end,
        [stage = _stage, ownerAttrsToRemove, incomingToRemove]
        (const auto &entry)
    {
        const SdfPath &ownerPath = entry.first;
        if (const UsdAttribute attr = stage->GetAttributeAtPath(ownerPath);
            attr && UsdPrimDefaultPredicate(attr.GetPrim())) {
            return;
        }

        ownerAttrsToRemove->push_back(ownerPath);
        for (const SdfPath &targetPath : entry.second) {
            (*incomingToRemove)[targetPath].push_back(ownerPath);
        }
    });
}

void
EsfUsdStageData::_RemoveIncomingTableEntries(
    const _IncomingPathTable &incomingToRemove)
{
    TRACE_FUNCTION();

    // Iterate over the incomingToRemove map in parallel, updating changed owner
    // paths in the incoming connections table. Entries that no longer have
    // incoming connections are queued up to be removed in series below.
    tbb::concurrent_vector<SdfPath> targetsToRemove;

    WorkParallelForN(
        incomingToRemove.unsafe_bucket_count(),
        [&incoming = _incoming, &incomingToRemove, &targetsToRemove]
        (const size_t b, const size_t e)
    {
        for (size_t bucketIdx = b; bucketIdx != e; ++bucketIdx) {
            auto bucketIt = incomingToRemove.unsafe_cbegin(bucketIdx);
            const auto bucketEnd = incomingToRemove.unsafe_cend(bucketIdx);
            for (; bucketIt != bucketEnd; ++bucketIt) {
                const SdfPath &targetPath = bucketIt->first;
                const tbb::concurrent_vector<SdfPath> &toRemove =
                    bucketIt->second;

                const auto incomingIt = incoming.find(targetPath);
                if (!TF_VERIFY(incomingIt != incoming.end())) {
                    continue;
                }

                tbb::concurrent_vector<SdfPath> &owners = incomingIt->second;
                if (!TF_VERIFY(owners.size() >= toRemove.size())) {
                    continue;
                }

                // If we're removing all of the incoming owners, queue up for
                // removal below.
                const size_t numRemainingOwners =
                    owners.size() - toRemove.size();
                if (numRemainingOwners == 0) {
                    targetsToRemove.push_back(targetPath);
                    continue;
                }

                // To remove from the concurrent_vector, move elements from the
                // end and resize.
                auto last = std::prev(owners.end());
                for (auto it = owners.begin(); it != last;) {
                    if (std::find(toRemove.begin(), toRemove.end(), *it) !=
                        toRemove.end()) {
                        *it = std::move(*last);
                        --last;
                    } else {
                        ++it;
                    }
                }
                owners.resize(numRemainingOwners);
            }
        }
    });

    {
        TRACE_SCOPE("Clean up empty incoming connections");

        for (const SdfPath &targetPath : targetsToRemove) {
            _incoming.unsafe_erase(targetPath);
        }
    }
}

PXR_NAMESPACE_CLOSE_SCOPE
