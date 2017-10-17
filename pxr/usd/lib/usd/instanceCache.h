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
    /// master prim, false otherwise.
    bool RegisterInstancePrimIndex(const PcpPrimIndex& index);

    /// Unregisters all instance prim indexes at or under \p primIndexPath.
    /// The indexes will be added to a list of pending changes and will
    /// not take effect until a subsequent call to ProcessChanges.
    void UnregisterInstancePrimIndexesUnder(const SdfPath& primIndexPath);

    /// Process all instance prim indexes that have been registered or
    /// unregistered since the last call to this function and return the
    /// resulting list of master prim changes via \p changes.
    void ProcessChanges(Usd_InstanceChanges* changes);

    /// Returns true if an object at \p path is a master or in a
    /// master.  \p path must be either an absolute path or empty.
    static bool IsPathMasterOrInMaster(const SdfPath& path);

    /// Returns the paths of all master prims for instance prim 
    /// indexes registered with this cache.
    std::vector<SdfPath> GetAllMasters() const;

    /// Returns the number of master prims assigned to instance
    /// prim indexes registered with this cache.
    size_t GetNumMasters() const;

    /// Returns the path of the master prim using the prim index
    /// at \p primIndexPath as its source prim index, or an empty path
    /// if no such master exists.
    ///
    /// Unlike GetMasterForPrimIndexAtPath, this function will return a
    /// master prim path only if the master prim is using the specified
    /// prim index as its source.
    SdfPath GetMasterUsingPrimIndexAtPath(const SdfPath& primIndexPath) const;

    /// Returns the paths of all prims in masters using the prim index
    /// at \p primIndexPath. There may be more than one such prim in
    /// the case of nested instances.
    std::vector<SdfPath> 
    GetPrimsInMastersUsingPrimIndexAtPath(const SdfPath& primIndexPath) const;

    /// Returns true if the prim index at \p primIndexPath is being used
    /// by any prim in a master.
    bool IsPrimInMasterUsingPrimIndexAtPath(const SdfPath& primIndexPath) const;

    /// If \p primIndexPath is an instanceable prim index that has been
    /// assigned to a master prim, returns the path of that master prim,
    /// otherwise returns an empty path.
    ///
    /// Unlike GetMasterUsingPrimIndexAtPath, this function will return a
    /// master prim path even if that master prim is not using the specified
    /// prim index as its source.
    SdfPath GetMasterForPrimIndexAtPath(const SdfPath& primIndexPath) const;

    /// If \p primIndexPath is a descendent of an instanceable prim index
    /// that has been assigned to a master prim, returns the path of the
    /// corresponding descendent of that master prim.
    SdfPath 
    GetPrimInMasterForPrimIndexAtPath(const SdfPath& primIndexPath) const;

    /// Returns true if \p primIndexPath is a descendent of an instanceable
    /// prim index that has been assigned to a master prim.
    bool IsPrimInMasterForPrimIndexAtPath(const SdfPath& primIndexPath) const;

    /// If the given \p primPath specifies a prim beneath an instance, 
    /// returns the path of the corresponding prim in that instance's 
    /// master.
    SdfPath GetPrimInMasterForPath(const SdfPath& primPath) const;

private:
    typedef std::vector<SdfPath> _PrimIndexPaths;

    void _CreateOrUpdateMasterForInstances(
        const Usd_InstanceKey& instanceKey,
        _PrimIndexPaths* primIndexPaths,
        Usd_InstanceChanges* changes);

    void _RemoveInstances(
        const Usd_InstanceKey& instanceKey,
        const _PrimIndexPaths& primIndexPaths,
        Usd_InstanceChanges* changes);

    void _RemoveMasterIfNoInstances(
        const Usd_InstanceKey& instanceKey,
        Usd_InstanceChanges* changes);

    bool _IsPrimInMasterUsingPrimIndexAtPath(
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
