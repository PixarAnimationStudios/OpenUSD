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
#ifndef USD_INSTANCE_CACHE_H
#define USD_INSTANCE_CACHE_H

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
/// List of changes to master prims due to the discovery of new
/// or destroyed instanceable prim indexes.
///
class Usd_InstanceChanges
{
public:
    void AppendChanges(const Usd_InstanceChanges& c)
    {
        newMasterPrims.insert(
            newMasterPrims.end(),
            c.newMasterPrims.begin(), 
            c.newMasterPrims.end());

        newMasterPrimIndexes.insert(
            newMasterPrimIndexes.end(),
            c.newMasterPrimIndexes.begin(), 
            c.newMasterPrimIndexes.end());

        associatedIndexOld.insert(
            associatedIndexOld.end(),
            c.associatedIndexOld.begin(),
            c.associatedIndexOld.end());

        associatedIndexNew.insert(
            associatedIndexNew.end(),
            c.associatedIndexNew.begin(),
            c.associatedIndexNew.end());

        changedMasterPrims.insert(
            changedMasterPrims.end(),
            c.changedMasterPrims.begin(), 
            c.changedMasterPrims.end());

        changedMasterPrimIndexes.insert(
            changedMasterPrimIndexes.end(),
            c.changedMasterPrimIndexes.begin(), 
            c.changedMasterPrimIndexes.end());

        deadMasterPrims.insert(
            deadMasterPrims.end(), 
            c.deadMasterPrims.begin(), c.deadMasterPrims.end());
    }

    /// List of new master prims and their corresponding source
    /// prim indexes.
    std::vector<SdfPath> newMasterPrims;
    std::vector<SdfPath> newMasterPrimIndexes;

    /// List of index paths that are "associated" in a before/after sense.  For
    /// example, if a prim </foo/bar> previously wasn't instanced, but becomes
    /// instanced and its master uses the prim index at </x/y>, then there will
    /// be an index i such that associatedIndexOld[i] == </foo/bar> and
    /// associatedIndexNew[i] == </x/y>.  Similarly if subsequently </foo/bar>
    /// ceases to be instanced, then we'll see corresponding entries with </x/y>
    /// in Old and </foo/bar> in new.  This will also track changes where an
    /// instancing master changes its source prim index.  This information is
    /// used to propagate payload inclusion across instancing changes.
    std::vector<SdfPath> associatedIndexOld;
    std::vector<SdfPath> associatedIndexNew;
    
    /// List of master prims that have been changed to use a new
    /// source prim index.
    std::vector<SdfPath> changedMasterPrims;
    std::vector<SdfPath> changedMasterPrimIndexes;

    /// List of master prims that no longer have any instances.
    std::vector<SdfPath> deadMasterPrims;
};

/// \class Usd_InstanceCache
///
/// Private helper object for computing and caching instance information
/// on a UsdStage.  This object is responsible for keeping track of the 
/// instanceable prim indexes and their corresponding masters. 
/// This includes:
///
///    - Tracking all instanceable prim indexes and master prims 
///      on the stage.
///    - Determining when a new master must be created or an
///      old master can be reused for a newly-discovered instanceable
///      prim index.
///    - Determining when a master can be removed due to it no longer
///      having any instanceable prim indexes.
///
/// During composition, UsdStage will discover instanceable prim indexes
/// which will be registered with this cache. These prim indexes will
/// then be assigned to the appropriate master prim. One of these prim
/// indexes will be used as the "source" prim index for the master.
/// This object keeps track of the dependencies formed between 
/// masters and prim indexes by this process.
///
/// API note: It can be confusing to reason about masters and instances,
/// especially with arbitrarily nested instancing.  To help clarify, the API
/// below uses two idioms to describe the two main kinds of relationships
/// involved in instancing: 1) instances to their master usd prims, and 2)
/// master usd prims to the prim indexes they use.  For #1, we use phrasing
/// like, "master for instance".  For example GetPathInMasterForInstancePath() finds
/// the corresponding master prim for a given instance prim path.  For #2, we
/// use phrasing like, "master using prim index".  For example
/// GetMasterUsingPrimIndexPath() finds the master using the given prim index
/// path as its source, if there is one.
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
    /// master prim or is the source for an existing master prim, false
    /// otherwise.
    bool RegisterInstancePrimIndex(const PcpPrimIndex& index);

    /// Unregisters all instance prim indexes at or under \p primIndexPath.
    /// The indexes will be added to a list of pending changes and will
    /// not take effect until a subsequent call to ProcessChanges.
    void UnregisterInstancePrimIndexesUnder(const SdfPath& primIndexPath);

    /// Process all instance prim indexes that have been registered or
    /// unregistered since the last call to this function and return the
    /// resulting list of master prim changes via \p changes.
    void ProcessChanges(Usd_InstanceChanges* changes);

    /// Return true if \p path identifies a master or a master descendant.  The
    /// \p path must be either an absolute path or empty.
    static bool IsPathInMaster(const SdfPath& path);

    /// Returns the paths of all master prims for instance prim 
    /// indexes registered with this cache.
    std::vector<SdfPath> GetAllMasters() const;

    /// Returns the number of master prims assigned to instance
    /// prim indexes registered with this cache.
    size_t GetNumMasters() const;

    /// Return the path of the master root prim using the prim index at
    /// \p primIndexPath as its source prim index, or the empty path if no such
    /// master exists.
    ///
    /// Unlike GetMasterForPrimIndexPath, this function will return a
    /// master prim path only if the master prim is using the specified
    /// prim index as its source.
    SdfPath GetMasterUsingPrimIndexPath(const SdfPath& primIndexPath) const;

    /// Return the paths of all prims in masters using the prim index at
    /// \p primIndexPath.
    ///
    /// There are at most two such paths.  Without nested instancing, there is
    /// at most one: the prim in the master corresponding to the instance
    /// identified by \p primIndexPath.  With nested instancing there will be
    /// two if the \p primIndexPath identifies an instanceable prim index
    /// descendant to another instancable prim index, and this \p primIndexPath
    /// was selected for use by that nested instance's master.  In that case
    /// this function will return the path of the nested instance under the
    /// outer master, and also the master path corresponding to that nested
    /// instance.
    std::vector<SdfPath> 
    GetPrimsInMastersUsingPrimIndexPath(const SdfPath& primIndexPath) const;

    /// Return true if a prim in a master uses the prim index at
    /// \p primIndexPath.
    bool MasterUsesPrimIndexPath(const SdfPath& primIndexPath) const;

    /// Return the path of the master prim associated with the instanceable
    /// \p primIndexPath.  If \p primIndexPath is not instanceable, or if it
    /// has no associated master because it lacks composition arcs, return the
    /// empty path.
    SdfPath
    GetMasterForInstanceablePrimIndexPath(const SdfPath& primIndexPath) const;

    /// Returns true if \p primPath is descendent to an instance.  That is,
    /// return true if a strict ancestor path of \p usdPrimPath identifies an
    /// instanceable prim index.
    bool IsPathDescendantToAnInstance(const SdfPath& primPath) const;

    /// Return the corresponding master prim path if \p primPath is descendant
    /// to an instance (see IsPathDescendantToAnInstance()), otherwise the empty
    /// path.
    SdfPath GetPathInMasterForInstancePath(const SdfPath& primPath) const;

private:
    typedef std::vector<SdfPath> _PrimIndexPaths;

    void _CreateOrUpdateMasterForInstances(
        const Usd_InstanceKey& instanceKey,
        _PrimIndexPaths* primIndexPaths,
        Usd_InstanceChanges* changes,
        std::unordered_map<SdfPath, SdfPath, SdfPath::Hash> const &
        masterToOldSourceIndexPath);

    void _RemoveInstances(
        const Usd_InstanceKey& instanceKey,
        const _PrimIndexPaths& primIndexPaths,
        Usd_InstanceChanges* changes,
        std::unordered_map<SdfPath, SdfPath, SdfPath::Hash> *
        masterToOldSourceIndexPath);

    void _RemoveMasterIfNoInstances(
        const Usd_InstanceKey& instanceKey,
        Usd_InstanceChanges* changes);

    bool _MasterUsesPrimIndexPath(
        const SdfPath& primIndexPath,
        std::vector<SdfPath>* masterPaths = NULL) const;

    SdfPath _GetNextMasterPath(const Usd_InstanceKey& key);
    
private:
    tbb::spin_mutex _mutex;

    // Mapping from instance key <-> master prim path.
    // This stores the path of the master prim that should be used
    // for all instanceable prim indexes with the given instance key.
    typedef TfHashMap<Usd_InstanceKey, SdfPath, boost::hash<Usd_InstanceKey> >
        _InstanceKeyToMasterMap;
    typedef TfHashMap<SdfPath, Usd_InstanceKey, SdfPath::Hash>
        _MasterToInstanceKeyMap;
    _InstanceKeyToMasterMap _instanceKeyToMasterMap;
    _MasterToInstanceKeyMap _masterToInstanceKeyMap;

    // Mapping from instance prim index path <-> master prim path.
    // This map stores which prim index serves as the source index
    // for a given master prim.
    typedef std::map<SdfPath, SdfPath> _SourcePrimIndexToMasterMap;
    typedef std::map<SdfPath, SdfPath> _MasterToSourcePrimIndexMap;
    _SourcePrimIndexToMasterMap _sourcePrimIndexToMasterMap;
    _MasterToSourcePrimIndexMap _masterToSourcePrimIndexMap;

    // Mapping from master prim path <-> list of instanceable prim indexes
    // This map stores which instanceable prim indexes have been assigned to
    // a master prim.
    typedef std::map<SdfPath, _PrimIndexPaths> _MasterToPrimIndexesMap;
    typedef std::map<SdfPath, SdfPath> _PrimIndexToMasterMap;
    _MasterToPrimIndexesMap _masterToPrimIndexesMap;
    _PrimIndexToMasterMap _primIndexToMasterMap;

    // Map from instance key -> list of prim index paths
    // These maps contain lists of pending changes and are the only containers 
    // that should be modified during registration and unregistration.
    typedef TfHashMap<
        Usd_InstanceKey, _PrimIndexPaths, boost::hash<Usd_InstanceKey> > 
        _InstanceKeyToPrimIndexesMap;
    _InstanceKeyToPrimIndexesMap _pendingAddedPrimIndexes;
    _InstanceKeyToPrimIndexesMap _pendingRemovedPrimIndexes;

    // Index of last master prim created. Used to create
    // master prim names.
    size_t _lastMasterIndex;
};


PXR_NAMESPACE_CLOSE_SCOPE

#endif // USD_INSTANCE_CACHE_H
