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
#ifndef HD_CHANGE_TRACKER_H
#define HD_CHANGE_TRACKER_H

#include "pxr/pxr.h"
#include "pxr/imaging/hd/version.h"
#include "pxr/imaging/hd/rprimCollection.h"
#include "pxr/imaging/hd/types.h"
#include "pxr/usd/sdf/path.h"
#include "pxr/base/tf/hashmap.h"

#include <atomic>
#include <boost/noncopyable.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/unordered_map.hpp>
#include <boost/weak_ptr.hpp>

PXR_NAMESPACE_OPEN_SCOPE


class TfToken;
class HdRprimCollection;
class HdRenderIndex;

/// \class HdChangeTracker
///
/// Tracks changes from the HdSceneDelegate, providing invalidation cues to the
/// render engine.
///
/// Changes flagged here are accumulated until the next time resource associated
/// with the change is required, at which point the resource is updated and the
/// flag is cleared.
///
class HdChangeTracker : public boost::noncopyable {
public:

    enum RprimDirtyBits {
        Clean                 = 0,
        ForceSync             = 1 << 0,
        Varying               = 1 << 1,
        AllDirty              = ~Varying,
        DirtyPrimID           = 1 << 2,
        DirtyExtent           = 1 << 3,
        DirtyRefineLevel      = 1 << 4,
        DirtyPoints           = 1 << 5,
        DirtyPrimVar          = 1 << 6,
        DirtySurfaceShader    = 1 << 7,
        DirtyTopology         = 1 << 8,
        DirtyTransform        = 1 << 9,
        DirtyVisibility       = 1 << 10,
        DirtyNormals          = 1 << 11,
        DirtyDoubleSided      = 1 << 12,
        DirtyCullStyle        = 1 << 13,
        DirtySubdivTags       = 1 << 14,
        DirtyWidths           = 1 << 15,
        DirtyInstancer        = 1 << 16,
        DirtyInstanceIndex    = 1 << 17,
        DirtyRepr             = 1 << 18,
        AllSceneDirtyBits     = ((1<<19) - 1),

        CustomBitsBegin       = 1 << 19,
        CustomBitsEnd         = 1 << 30,
    };

    // Dirty bits for Tasks
    enum NonRprimDirtyBits {
        //Varying               = 1 << 0,
        DirtyType             = 1 << 1,
        DirtyChildren         = 1 << 2,
        DirtyParams           = 1 << 3,
        DirtyCollection       = 1 << 4,
    };

    HdChangeTracker();
    virtual ~HdChangeTracker();

    // ---------------------------------------------------------------------- //
    /// \name Rprim Object Tracking
    /// @{
    // ---------------------------------------------------------------------- //

    /// Start tracking Rprim with the given \p id.
    void RprimInserted(SdfPath const& id, HdDirtyBits initialDirtyState);

    /// Stop tracking Rprim with the given \p id.
    void RprimRemoved(SdfPath const& id);

    // ---------------------------------------------------------------------- //
    /// @}
    /// \name Rprim State Tracking
    /// @{
    // ---------------------------------------------------------------------- //

    /// Returns the dirty bits for the rprim with \p id.
    HdDirtyBits GetRprimDirtyBits(SdfPath const& id) const;

    /// Flag the Rprim with the given \p id as being dirty. Multiple calls with
    /// different dirty bits accumulate.
    void MarkRprimDirty(SdfPath const& id, HdDirtyBits bits=AllDirty);

    /// Clear the dirty flags for an HdRprim. if inSync is true, set OutOfSync
    /// flag to notify dirtyList will discover the prim to sync the residual
    /// data for new repr.
    void MarkRprimClean(SdfPath const& id, HdDirtyBits newBits=Clean);

    /// Mark the primvar for the rprim with \p id as being dirty.
    void MarkPrimVarDirty(SdfPath const& id, TfToken const& name);

    /// Flag all the Rprim with the given \p id as being dirty. Multiple calls
    /// with different dirty bits accumulate.
    /// Doesn't touch varying state.
    void MarkAllRprimsDirty(HdDirtyBits bits);

    // Clear Varying bit of all prims.
    ///
    /// The idea is that from frame to frame (update iteration), the set of
    /// dirty rprims and their dirty bits do not change; that is, the same
    /// rprims get dirtied with the same dirty bits.. The change tracker can
    /// leverage this and build stable sets of dirty lists and reduce the
    /// overall cost of an update iteration.
    void ResetVaryingState();

    // ---------------------------------------------------------------------- //

    /// Returns true if the rprim identified by \p id has any dirty flags set.
    bool IsRprimDirty(SdfPath const& id);
    
    /// Returns true if the rprim identified by \p id has a dirty extent.
    bool IsExtentDirty(SdfPath const& id);

    /// Returns true if the rprim identified by \p id has a dirty refine level.
    bool IsRefineLevelDirty(SdfPath const& id);

    /// Returns true if the rprim identified by \p id with primvar \p name is
    /// dirty.
    bool IsPrimVarDirty(SdfPath const& id, TfToken const& name);

    /// Returns true if the rprim identified by \p id has any dirty primvars.
    bool IsAnyPrimVarDirty(SdfPath const& id);

    /// Returns true if the rprim identified by \p id has a dirty topology.
    bool IsTopologyDirty(SdfPath const& id);

    /// Returns true if the rprim identified by \p id has dirty doubleSided state.
    bool IsDoubleSidedDirty(SdfPath const& id);

    /// Returns true if the rprim identified by \p id has dirty cullstyle.
    bool IsCullStyleDirty(SdfPath const& id);

    /// Returns true if the rprim identified by \p id has a dirty subdiv tags.
    bool IsSubdivTagsDirty(SdfPath const& id);

    /// Returns true if the rprim identified by \p id has a dirty transform.
    bool IsTransformDirty(SdfPath const& id);

    /// Returns true if the rprim identified by \p id has dirty visibility.
    bool IsVisibilityDirty(SdfPath const& id);
    
    /// Returns true if the rprim identified by \p id has a dirty primID.
    bool IsPrimIdDirty(SdfPath const& id);

    /// Returns true if the dirtyBits has any flags set other than the varying flag.
    static bool IsDirty(HdDirtyBits dirtyBits) {
        return (dirtyBits & AllDirty) != 0;
    }

    /// Returns true if the dirtyBits has no flags set except the varying flag.
    static bool IsClean(HdDirtyBits dirtyBits) {
        return (dirtyBits & AllDirty) == 0;
    }

    /// Returns true if the dirtyBits has a dirty extent. id is for perflog.
    static bool IsExtentDirty(HdDirtyBits dirtyBits, SdfPath const& id);

    /// Returns true if the dirtyBits has a dirty refine level. id is for perflog.
    static bool IsRefineLevelDirty(HdDirtyBits dirtyBits, SdfPath const& id);

    /// Returns true if the dirtyBits has a dirty subdiv tags. id is for perflog.
    static bool IsSubdivTagsDirty(HdDirtyBits dirtyBits, SdfPath const& id);

    /// Returns true if the dirtyBits has a dirty primvar \p name.
    /// id is for perflog.
    static bool IsPrimVarDirty(HdDirtyBits dirtyBits, SdfPath const& id,
                               TfToken const& name);

    /// Returns true if the dirtyBits has any dirty primvars.
    /// id is for perflog.
    static bool IsAnyPrimVarDirty(HdDirtyBits dirtyBits, SdfPath const& id);

    /// Returns true if the dirtyBits has a dirty topology. id is for perflog.
    static bool IsTopologyDirty(HdDirtyBits dirtyBits, SdfPath const& id);

    /// Returns true if the dirtyBits has dirty doubleSided state. id is for perflog.
    static bool IsDoubleSidedDirty(HdDirtyBits dirtyBits, SdfPath const& id);

    /// Returns true if the dirtyBits has dirty cullstyle. id is for perflog.
    static bool IsCullStyleDirty(HdDirtyBits dirtyBits, SdfPath const& id);

    /// Returns true if the dirtyBits has a dirty transform. id is for perflog.
    static bool IsTransformDirty(HdDirtyBits dirtyBits, SdfPath const& id);

    /// Returns true if the dirtyBits has dirty visibility. id is for perflog.
    static bool IsVisibilityDirty(HdDirtyBits dirtyBits, SdfPath const& id);

    /// Returns true if the dirtyBits has a dirty primID. id is for perflog.
    static bool IsPrimIdDirty(HdDirtyBits dirtyBits, SdfPath const& id);

    /// Returns true if the dirtyBits has a dirty instancer. id is for perflog.
    static bool IsInstancerDirty(HdDirtyBits dirtyBits, SdfPath const& id);

    /// Returns true if the dirtyBits has a dirty instance index. id is for perflog.
    static bool IsInstanceIndexDirty(HdDirtyBits dirtyBits, SdfPath const& id);

    static bool IsReprDirty(HdDirtyBits dirtyBits, SdfPath const &id);

    // ---------------------------------------------------------------------- //

    /// Set the primvar dirty flag to \p dirtyBits.
    static void MarkPrimVarDirty(HdDirtyBits *dirtyBits, TfToken const &name);

    // ---------------------------------------------------------------------- //
    /// @}
    /// \name Instancer Object Tracking
    /// @{
    // ---------------------------------------------------------------------- //

    /// Start tracking Instancer with the given \p id.
    void InstancerInserted(SdfPath const& id);

    /// Stop tracking Instancer with the given \p id.
    void InstancerRemoved(SdfPath const& id);

    /// Add the gived \p rprimId to the list of rprims associated with the
    /// instancer \p instancerId
    void InstancerRPrimInserted(SdfPath const& instancerId, SdfPath const& rprimId);

    /// Remove the gived \p rprimId to the list of rprims associated with the
    /// instancer \p instancerId
    void InstancerRPrimRemoved(SdfPath const& instancerId, SdfPath const& rprimId);

    // ---------------------------------------------------------------------- //
    /// @}
    /// \name Task Object Tracking
    /// @{
    // ---------------------------------------------------------------------- //

    /// Start tracking Task with the given \p id.
    void TaskInserted(SdfPath const& id);

    /// Stop tracking Task with the given \p id.
    void TaskRemoved(SdfPath const& id);

    /// Set the dirty flags to \p bits.
    void MarkTaskDirty(SdfPath const& id, HdDirtyBits bits=AllDirty);

    /// Get the dirty bits for Task with the given \p id.
    HdDirtyBits GetTaskDirtyBits(SdfPath const& id);

    /// Set the dirty flags to \p newBits.
    void MarkTaskClean(SdfPath const& id, HdDirtyBits newBits=Clean);

    // ---------------------------------------------------------------------- //
    /// @}
    /// \name Instancer State Tracking
    /// @{
    // ---------------------------------------------------------------------- //

    /// Returns the dirty bits for the instancer with \p id.
    HdDirtyBits GetInstancerDirtyBits(SdfPath const& id);

    /// Flag the Instancer with the given \p id as being dirty. Multiple calls
    /// with different dirty bits accumulate.
    void MarkInstancerDirty(SdfPath const& id, HdDirtyBits bits=AllDirty);

    /// Clean the specified dirty bits for the instancer with \p id.
    void MarkInstancerClean(SdfPath const& id, HdDirtyBits newBits=Clean);

    // ---------------------------------------------------------------------- //
    /// @}
    /// \name Sprim (scene state prim: camera, light, ...) state Tracking
    /// @{
    // ---------------------------------------------------------------------- //

    /// Start tracking sprim with the given \p id.
    void SprimInserted(SdfPath const& id, HdDirtyBits initialDirtyState);

    /// Stop tracking sprim with the given \p id.
    void SprimRemoved(SdfPath const& id);

    /// Get the dirty bits for sprim with the given \p id.
    HdDirtyBits GetSprimDirtyBits(SdfPath const& id);

    /// Set the dirty flags to \p bits.
    void MarkSprimDirty(SdfPath const& id, HdDirtyBits bits);

    /// Set the dirty flags to \p newBits.
    void MarkSprimClean(SdfPath const& id, HdDirtyBits newBits=Clean);

    // ---------------------------------------------------------------------- //
    /// @}
    /// \name Bprim (buffer prim: texture, buffer, ...) state Tracking
    /// @{
    // ---------------------------------------------------------------------- //

    /// Start tracking bprim with the given \p id.
    void BprimInserted(SdfPath const& id, HdDirtyBits initialDirtyState);

    /// Stop tracking bprim with the given \p id.
    void BprimRemoved(SdfPath const& id);

    /// Get the dirty bits for bprim with the given \p id.
    HdDirtyBits GetBprimDirtyBits(SdfPath const& id);

    /// Set the dirty flags to \p bits.
    void MarkBprimDirty(SdfPath const& id, HdDirtyBits bits);

    /// Set the dirty flags to \p newBits.
    void MarkBprimClean(SdfPath const& id, HdDirtyBits newBits=Clean);


    // ---------------------------------------------------------------------- //
    /// @}
    /// \name GarbageCollection Tracking
    /// @{
    // ---------------------------------------------------------------------- //

    /// Clears the garbageCollectionNeeded flag.
    void ClearGarbageCollectionNeeded() {
        _needsGarbageCollection = false;
    }

    /// Sets the garbageCollectionNeeded flag.
    void SetGarbageCollectionNeeded() {
        _needsGarbageCollection = true;
    }

    /// Returns true if garbage collection was flagged to be run.
    /// Currently, this flag only gets set internally when Rprims are removed.
    bool IsGarbageCollectionNeeded() const {
        return _needsGarbageCollection;
    }

    // ---------------------------------------------------------------------- //
    /// @}
    /// \name RprimCollection Tracking
    /// @{
    // ---------------------------------------------------------------------- //

    /// Adds a named collection for tracking.
    void AddCollection(TfToken const& collectionName);

    /// Marks a named collection as being dirty, this bumps the version of the
    /// collection.
    void MarkCollectionDirty(TfToken const& collectionName);

    /// Invalidates all collections by bumping a global version number.
    void MarkAllCollectionsDirty();

    /// Returns the current version of the named collection.
    unsigned GetCollectionVersion(TfToken const& collectionName);

    /// Returns the number of changes to visibility. This is intended to be used
    /// to detect when visibility has changed for *any* Rprim.
    unsigned GetVisibilityChangeCount();

    /// Returns the current version of varying state. This is used to refresh
    /// cached DirtyLists
    unsigned GetVaryingStateVersion() const {
        return _varyingStateVersion;
    }

    /// Returns the change count 
    unsigned GetChangeCount() const {
        return _changeCount;
    }

    /// Marks all shader bindings dirty (draw batches need to be validated).
    void MarkShaderBindingsDirty();

    /// Returns the current shader binding version.
    unsigned GetShaderBindingsVersion() const;

    // ---------------------------------------------------------------------- //
    /// @}
    /// \name General state tracking
    /// @{
    // ---------------------------------------------------------------------- //

    /// Adds a named state for tracking.
    void AddState(TfToken const& name);

    /// Marks a named state as being dirty., this bumps the version of the
    /// state.
    void MarkStateDirty(TfToken const& name);

    /// Returns the current version of the named state.
    unsigned GetStateVersion(TfToken const &name) const;

    // ---------------------------------------------------------------------- //
    /// @}
    /// \name Debug
    /// @{
    // ---------------------------------------------------------------------- //
    static std::string StringifyDirtyBits(HdDirtyBits dirtyBits);

    static void DumpDirtyBits(HdDirtyBits dirtyBits);

    /// @}

private:

    static void _LogCacheAccess(TfToken const& cacheName,
                                SdfPath const& id, bool hit);

    typedef TfHashMap<SdfPath, HdDirtyBits, SdfPath::Hash> _IDStateMap;
    typedef TfHashMap<TfToken, int, TfToken::HashFunctor> _CollectionStateMap;
    typedef TfHashMap<SdfPath, SdfPathSet, SdfPath::Hash> _InstancerRprimMap;
    typedef TfHashMap<TfToken, unsigned, TfToken::HashFunctor> _GeneralStateMap;

    // Core dirty state.
    _IDStateMap _rprimState;
    _IDStateMap _instancerState;
    _IDStateMap _taskState;
    _IDStateMap _sprimState;
    _IDStateMap _bprimState;
    _GeneralStateMap _generalState;

    // Collection versions / state.
    _CollectionStateMap _collectionState;
    bool _needsGarbageCollection;

    // Provides reverse-assosiation between instancers and the rprims that use
    // them.
    _InstancerRprimMap _instancerRprimMap;

    // Typically the Rprims that get marked dirty per update iteration end up
    // being a stable set of objects; to leverage this fact, we require the
    // delegate notify the change tracker when that state changes, which bumps
    // the varyingStateVersion, which triggers downstream invalidation.
    unsigned _varyingStateVersion;
 
    // Used for coarse grain invalidation of all RprimCollections.
    unsigned _indexVersion;

    // Used to detect that no changes have occured when building dirty lists.
    unsigned _changeCount;

    // Used to detect that visibility changed somewhere in the render index.
    unsigned _visChangeCount;

    // Used to validate shader bindings (to validate draw batches)
    std::atomic_uint _shaderBindingsVersion;
};


PXR_NAMESPACE_CLOSE_SCOPE

#endif //HD_CHANGE_TRACKER_H
