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
#ifndef PXR_USD_USD_INSTANCE_CACHE_H
#define PXR_USD_USD_INSTANCE_CACHE_H

#include "pxr/pxr.h"
#include "pxr/usd/usd/instanceKey.h"

#include "pxr/usd/sdf/path.h"
#include "pxr/base/tf/hashmap.h"

#include <tbb/mutex.h>
#include <map>
#include <unordered_map>
#include <vector>

PXR_NAMESPACE_OPEN_SCOPE

/// \class Usd_InstanceChanges
///
/// List of changes to prototype prims due to the discovery of new
/// or destroyed instanceable prim indexes.
///
class Usd_InstanceChanges
{
public:
    void AppendChanges(const Usd_InstanceChanges& c)
    {
        newPrototypePrims.insert(
            newPrototypePrims.end(),
            c.newPrototypePrims.begin(), 
            c.newPrototypePrims.end());

        newPrototypePrimIndexes.insert(
            newPrototypePrimIndexes.end(),
            c.newPrototypePrimIndexes.begin(), 
            c.newPrototypePrimIndexes.end());

        changedPrototypePrims.insert(
            changedPrototypePrims.end(),
            c.changedPrototypePrims.begin(), 
            c.changedPrototypePrims.end());

        changedPrototypePrimIndexes.insert(
            changedPrototypePrimIndexes.end(),
            c.changedPrototypePrimIndexes.begin(), 
            c.changedPrototypePrimIndexes.end());

        deadPrototypePrims.insert(
            deadPrototypePrims.end(), 
            c.deadPrototypePrims.begin(),
            c.deadPrototypePrims.end());
    }

    /// List of new prototype prims and their corresponding source
    /// prim indexes.
    std::vector<SdfPath> newPrototypePrims;
    std::vector<SdfPath> newPrototypePrimIndexes;

    /// List of prototype prims that have been changed to use a new
    /// source prim index.
    std::vector<SdfPath> changedPrototypePrims;
    std::vector<SdfPath> changedPrototypePrimIndexes;

    /// List of prototype prims that no longer have any instances.
    std::vector<SdfPath> deadPrototypePrims;
};

/// \class Usd_InstanceCache
///
/// Private helper object for computing and caching instance information
/// on a UsdStage.  This object is responsible for keeping track of the 
/// instanceable prim indexes and their corresponding prototypes. 
/// This includes:
///
///    - Tracking all instanceable prim indexes and prototype prims 
///      on the stage.
///    - Determining when a new prototype must be created or an
///      old prototype can be reused for a newly-discovered instanceable
///      prim index.
///    - Determining when a prototype can be removed due to it no longer
///      having any instanceable prim indexes.
///
/// During composition, UsdStage will discover instanceable prim indexes
/// which will be registered with this cache. These prim indexes will
/// then be assigned to the appropriate prototype prim. One of these prim
/// indexes will be used as the "source" prim index for the prototype.
/// This object keeps track of the dependencies formed between 
/// prototypes and prim indexes by this process.
///
/// API note: It can be confusing to reason about prototypes and instances,
/// especially with arbitrarily nested instancing.  To help clarify, the API
/// below uses two idioms to describe the two main kinds of relationships
/// involved in instancing: 1) instances to their prototype usd prims, and 2)
/// prototype usd prims to the prim indexes they use.  For #1, we use phrasing
/// like, "prototype for instance". For example 
/// GetPathInPrototypeForInstancePath() finds the corresponding prototype prim
/// for a given instance prim path.  For #2, we use phrasing like, "prototype
/// using prim index".  For example GetPrototypeUsingPrimIndexPath() finds the
/// prototype using the given prim index path as its source, if there is one.
///
class Usd_InstanceCache
{
    Usd_InstanceCache(Usd_InstanceCache const &) = delete;
    Usd_InstanceCache &operator=(Usd_InstanceCache const &) = delete;
public:
    Usd_InstanceCache();

    /// Registers the given instance prim index \p index with the cache.
    /// The index will be added to a list of pending changes and will
    /// not take effect until a subsequent call to ProcessChanges.
    ///
    /// It is safe to call this function concurrently from multiple 
    /// threads.
    ///
    /// Returns true if the given instance prim index requires a new 
    /// prototype prim or is the source for an existing prototype prim, false
    /// otherwise.
    bool RegisterInstancePrimIndex(const PcpPrimIndex& index,
                                   const UsdStagePopulationMask *mask,
                                   const UsdStageLoadRules &loadRules);

    /// Unregisters all instance prim indexes at or under \p primIndexPath.
    /// The indexes will be added to a list of pending changes and will
    /// not take effect until a subsequent call to ProcessChanges.
    void UnregisterInstancePrimIndexesUnder(const SdfPath& primIndexPath);

    /// Process all instance prim indexes that have been registered or
    /// unregistered since the last call to this function and return the
    /// resulting list of prototype prim changes via \p changes.
    void ProcessChanges(Usd_InstanceChanges* changes);

    /// Return true if \p path identifies a prototype or a prototype descendant.
    /// The \p path must be either an absolute path or empty.
    static bool IsPathInPrototype(const SdfPath& path);

    /// Return true if \p path identifies a prototype.  
    static bool IsPrototypePath(const SdfPath& path);

    /// Return instance prim indexes registered for \p prototypePath, an empty
    /// vector otherwise
    std::vector<SdfPath> GetInstancePrimIndexesForPrototype(
            const SdfPath& prototype) const;

    /// Returns the paths of all prototype prims for instance prim 
    /// indexes registered with this cache.
    std::vector<SdfPath> GetAllPrototypes() const;

    /// Returns the number of prototype prims assigned to instance
    /// prim indexes registered with this cache.
    size_t GetNumPrototypes() const;

    /// Return the path of the prototype root prim using the prim index at
    /// \p primIndexPath as its source prim index, or the empty path if no such
    /// prototype exists.
    ///
    /// Unlike GetPrototypeForPrimIndexPath, this function will return a
    /// prototype prim path only if the prototype prim is using the specified
    /// prim index as its source.
    SdfPath GetPrototypeUsingPrimIndexPath(const SdfPath& primIndexPath) const;

    /// Return the paths of all prims in prototypes using the prim index at
    /// \p primIndexPath.
    ///
    /// There are at most two such paths.  Without nested instancing, there is
    /// at most one: the prim in the prototype corresponding to the instance
    /// identified by \p primIndexPath.  With nested instancing there will be
    /// two if the \p primIndexPath identifies an instanceable prim index
    /// descendant to another instancable prim index, and this \p primIndexPath
    /// was selected for use by that nested instance's prototype.  In that case
    /// this function will return the path of the nested instance under the
    /// outer prototype, and also the prototype path corresponding to that
    /// nested instance.
    std::vector<SdfPath> 
    GetPrimsInPrototypesUsingPrimIndexPath(const SdfPath& primIndexPath) const;

    /// Return a vector of pair of prototype and respective source prim index
    /// path for all prototypes using the prim index at \p primIndexPath or as 
    /// descendent of \p primIndexPath.
    std::vector<std::pair<SdfPath, SdfPath>> 
    GetPrototypesUsingPrimIndexPathOrDescendents(
        const SdfPath& primIndexPath) const;

    /// Return true if a prim in a prototype uses the prim index at
    /// \p primIndexPath.
    bool PrototypeUsesPrimIndexPath(const SdfPath& primIndexPath) const;

    /// Return the path of the prototype prim associated with the instanceable
    /// \p primIndexPath.  If \p primIndexPath is not instanceable, or if it
    /// has no associated prototype because it lacks composition arcs, return
    /// the empty path.
    SdfPath
    GetPrototypeForInstanceablePrimIndexPath(
        const SdfPath& primIndexPath) const;

    /// Returns true if \p primPath is descendent to an instance.  That is,
    /// return true if a strict ancestor path of \p usdPrimPath identifies an
    /// instanceable prim index.
    bool IsPathDescendantToAnInstance(const SdfPath& primPath) const;

    /// Returns the shortest ancestor of \p primPath that identifies an
    /// instanceable prim.  If there is no such ancestor, return the empty path.
    SdfPath GetMostAncestralInstancePath(const SdfPath &primPath) const;

    /// Return the corresponding prototype prim path if \p primPath is
    /// descendant to an instance (see IsPathDescendantToAnInstance()), 
    /// otherwise the empty path.
    SdfPath GetPathInPrototypeForInstancePath(const SdfPath& primPath) const;

private:
    typedef std::vector<SdfPath> _PrimIndexPaths;

    void _CreateOrUpdatePrototypeForInstances(
        const Usd_InstanceKey& instanceKey,
        _PrimIndexPaths* primIndexPaths,
        Usd_InstanceChanges* changes,
        std::unordered_map<SdfPath, SdfPath, SdfPath::Hash> const &
        prototypeToOldSourceIndexPath);

    void _RemoveInstances(
        const Usd_InstanceKey& instanceKey,
        const _PrimIndexPaths& primIndexPaths,
        Usd_InstanceChanges* changes,
        std::unordered_map<SdfPath, SdfPath, SdfPath::Hash> *
        prototypeToOldSourceIndexPath);

    void _RemovePrototypeIfNoInstances(
        const Usd_InstanceKey& instanceKey,
        Usd_InstanceChanges* changes);

    bool _PrototypeUsesPrimIndexPath(
        const SdfPath& primIndexPath,
        std::vector<SdfPath>* prototypePaths = nullptr) const;

    SdfPath _GetNextPrototypePath(const Usd_InstanceKey& key);
    
private:
    tbb::spin_mutex _mutex;

    // Mapping from instance key <-> prototype prim path.
    // This stores the path of the prototype prim that should be used
    // for all instanceable prim indexes with the given instance key.
    typedef TfHashMap<Usd_InstanceKey, SdfPath, boost::hash<Usd_InstanceKey> >
        _InstanceKeyToPrototypeMap;
    typedef TfHashMap<SdfPath, Usd_InstanceKey, SdfPath::Hash>
        _PrototypeToInstanceKeyMap;
    _InstanceKeyToPrototypeMap _instanceKeyToPrototypeMap;
    _PrototypeToInstanceKeyMap _prototypeToInstanceKeyMap;

    // Mapping from instance prim index path <-> prototype prim path.
    // This map stores which prim index serves as the source index
    // for a given prototype prim.
    typedef std::map<SdfPath, SdfPath> _SourcePrimIndexToPrototypeMap;
    typedef std::map<SdfPath, SdfPath> _PrototypeToSourcePrimIndexMap;
    _SourcePrimIndexToPrototypeMap _sourcePrimIndexToPrototypeMap;
    _PrototypeToSourcePrimIndexMap _prototypeToSourcePrimIndexMap;

    // Mapping from prototype prim path <-> list of instanceable prim indexes
    // This map stores which instanceable prim indexes have been assigned to
    // a prototype prim.
    typedef std::map<SdfPath, _PrimIndexPaths> _PrototypeToPrimIndexesMap;
    typedef std::map<SdfPath, SdfPath> _PrimIndexToPrototypeMap;
    _PrototypeToPrimIndexesMap _prototypeToPrimIndexesMap;
    _PrimIndexToPrototypeMap _primIndexToPrototypeMap;

    // Map from instance key -> list of prim index paths
    // These maps contain lists of pending changes and are the only containers 
    // that should be modified during registration and unregistration.
    typedef TfHashMap<
        Usd_InstanceKey, _PrimIndexPaths, boost::hash<Usd_InstanceKey> > 
        _InstanceKeyToPrimIndexesMap;
    _InstanceKeyToPrimIndexesMap _pendingAddedPrimIndexes;
    _InstanceKeyToPrimIndexesMap _pendingRemovedPrimIndexes;

    // Index of last prototype prim created. Used to create
    // prototype prim names.
    size_t _lastPrototypeIndex;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_USD_USD_INSTANCE_CACHE_H
