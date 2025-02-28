//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_IMAGING_HD_CHANGE_TRACKER_H
#define PXR_IMAGING_HD_CHANGE_TRACKER_H

#include "pxr/pxr.h"
#include "pxr/imaging/hd/api.h"
#include "pxr/imaging/hd/version.h"
#include "pxr/imaging/hd/rprimCollection.h"
#include "pxr/imaging/hd/types.h"
#include "pxr/usd/sdf/path.h"
#include "pxr/base/tf/hashmap.h"

#include <tbb/concurrent_hash_map.h>
#include <atomic>

PXR_NAMESPACE_OPEN_SCOPE

class HdRetainedSceneIndex;

/// \class HdChangeTracker
///
/// Tracks changes from the HdSceneDelegate, providing invalidation cues to the
/// render engine.
///
/// Changes flagged here are accumulated until the next time resource associated
/// with the change is required, at which point the resource is updated and the
/// flag is cleared.
///
class HdChangeTracker 
{
public:

    // Common dirty bits for Rprims
    // XXX: Move this to HdRprim
    enum RprimDirtyBits : HdDirtyBits {
        Clean                       = 0,
        InitRepr                    = 1 << 0,
        Varying                     = 1 << 1,
        AllDirty                    = ~Varying,
        DirtyPrimID                 = 1 << 2,
        DirtyExtent                 = 1 << 3,
        DirtyDisplayStyle           = 1 << 4,
        DirtyPoints                 = 1 << 5,
        DirtyPrimvar                = 1 << 6,
        DirtyMaterialId             = 1 << 7,
        DirtyTopology               = 1 << 8,
        DirtyTransform              = 1 << 9,
        DirtyVisibility             = 1 << 10,
        DirtyNormals                = 1 << 11,
        DirtyDoubleSided            = 1 << 12,
        DirtyCullStyle              = 1 << 13,
        DirtySubdivTags             = 1 << 14,
        DirtyWidths                 = 1 << 15,
        DirtyInstancer              = 1 << 16,
        DirtyInstanceIndex          = 1 << 17,
        DirtyRepr                   = 1 << 18,
        DirtyRenderTag              = 1 << 19,
        DirtyComputationPrimvarDesc = 1 << 20,
        DirtyCategories             = 1 << 21,
        DirtyVolumeField            = 1 << 22,
        AllSceneDirtyBits           = ((1<<23) - 1),

        NewRepr                     = 1 << 23,

        CustomBitsBegin             = 1 << 24,
        CustomBitsEnd               = 1 << 30,
        CustomBitsMask              = 0x7f << 24,
    };

    // InstancerDirtybits are a subset of rprim dirty bits right now:
    // DirtyPrimvar, DirtyTransform, DirtyInstanceIndex, DirtyInstancer.

    // Dirty bits for Tasks
    // XXX: Move this to HdTask
    enum TaskDirtyBits : HdDirtyBits {
        DirtyParams           = 1 << 2,
        DirtyCollection       = 1 << 3,
        DirtyRenderTags       = 1 << 4,
    };

    HD_API
    HdChangeTracker();
    HD_API
    virtual ~HdChangeTracker();

    // ---------------------------------------------------------------------- //
    /// \name Rprim Object Tracking
    /// @{
    // ---------------------------------------------------------------------- //

    /// Start tracking Rprim with the given \p id.
    HD_API
    void RprimInserted(SdfPath const& id, HdDirtyBits initialDirtyState);

    /// Stop tracking Rprim with the given \p id.
    HD_API
    void RprimRemoved(SdfPath const& id);

    // ---------------------------------------------------------------------- //
    /// @}
    /// \name Rprim State Tracking
    /// @{
    // ---------------------------------------------------------------------- //

    /// Returns the dirty bits for the rprim with \p id.
    HD_API
    HdDirtyBits GetRprimDirtyBits(SdfPath const& id) const;

    /// Flag the Rprim with the given \p id as being dirty. Multiple calls with
    /// different dirty bits accumulate.
    HD_API
    void MarkRprimDirty(SdfPath const& id, HdDirtyBits bits=AllDirty);

    /// Clear the dirty flags for an HdRprim. if inSync is true, set OutOfSync
    /// flag to notify dirtyList will discover the prim to sync the residual
    /// data for new repr.
    HD_API
    void MarkRprimClean(SdfPath const& id, HdDirtyBits newBits=Clean);

    /// Mark the primvar for the rprim with \p id as being dirty.
    HD_API
    void MarkPrimvarDirty(SdfPath const& id, TfToken const& name);

    /// Flag all the Rprim with the given \p id as being dirty. Multiple calls
    /// with different dirty bits accumulate.
    /// Doesn't touch varying state.
    HD_API
    void MarkAllRprimsDirty(HdDirtyBits bits);

    ///  Clear Varying bit of all prims.
    ///
    /// The idea is that from frame to frame (update iteration), the set of
    /// dirty rprims and their dirty bits do not change; that is, the same
    /// rprims get dirtied with the same dirty bits.. The change tracker can
    /// leverage this help build a stable dirty list and reduce the
    /// overall cost of an update iteration.
    HD_API
    void ResetVaryingState();

    /// Reset the varying state on one Rprim
    /// This is done for Rprims, where we choose not to clean them
    /// (due to state like invisibility).
    HD_API
    void ResetRprimVaryingState(SdfPath const& id);


    // ---------------------------------------------------------------------- //

    /// Returns true if the rprim identified by \p id has any dirty flags set.
    HD_API
    bool IsRprimDirty(SdfPath const& id);
    
    /// Returns true if the rprim identified by \p id has a dirty extent.
    HD_API
    bool IsExtentDirty(SdfPath const& id);

    /// Returns true if the rprim identified by \p id has a dirty display style.
    HD_API
    bool IsDisplayStyleDirty(SdfPath const& id);

    /// Returns true if the rprim identified by \p id with primvar \p name is
    /// dirty.
    HD_API
    bool IsPrimvarDirty(SdfPath const& id, TfToken const& name);

    /// Returns true if the rprim identified by \p id has any dirty primvars.
    HD_API
    bool IsAnyPrimvarDirty(SdfPath const& id);

    /// Returns true if the rprim identified by \p id has a dirty topology.
    HD_API
    bool IsTopologyDirty(SdfPath const& id);

    /// Returns true if the rprim identified by \p id has dirty doubleSided state.
    HD_API
    bool IsDoubleSidedDirty(SdfPath const& id);

    /// Returns true if the rprim identified by \p id has dirty cullstyle.
    HD_API
    bool IsCullStyleDirty(SdfPath const& id);

    /// Returns true if the rprim identified by \p id has a dirty subdiv tags.
    HD_API
    bool IsSubdivTagsDirty(SdfPath const& id);

    /// Returns true if the rprim identified by \p id has a dirty transform.
    HD_API
    bool IsTransformDirty(SdfPath const& id);

    /// Returns true if the rprim identified by \p id has dirty visibility.
    HD_API
    bool IsVisibilityDirty(SdfPath const& id);
    
    /// Returns true if the rprim identified by \p id has a dirty primID.
    HD_API
    bool IsPrimIdDirty(SdfPath const& id);

    /// Returns true if the dirtyBits has any flags set other than the varying
    /// flag.
    static bool IsDirty(HdDirtyBits dirtyBits) {
        return (dirtyBits & AllDirty) != 0;
    }

    /// Returns true if the dirtyBits has no flags set except the varying flag.
    static bool IsClean(HdDirtyBits dirtyBits) {
        return (dirtyBits & AllDirty) == 0;
    }

    /// Returns true if the varying flag is set.
    static bool IsVarying(HdDirtyBits dirtyBits) {
        return (dirtyBits & Varying) != 0;
    }

    /// Returns true if the dirtyBits has a dirty extent. id is for perflog.
    HD_API
    static bool IsExtentDirty(HdDirtyBits dirtyBits, SdfPath const& id);

    /// Returns true if the dirtyBits has a dirty display style. id is for perflog.
    HD_API
    static bool IsDisplayStyleDirty(HdDirtyBits dirtyBits, SdfPath const& id);

    /// Returns true if the dirtyBits has a dirty subdiv tags. id is for perflog.
    HD_API
    static bool IsSubdivTagsDirty(HdDirtyBits dirtyBits, SdfPath const& id);

    /// Returns true if the dirtyBits has a dirty primvar \p name.
    /// id is for perflog.
    HD_API
    static bool IsPrimvarDirty(HdDirtyBits dirtyBits, SdfPath const& id,
                               TfToken const& name);

    /// Returns true if the dirtyBits has any dirty primvars.
    /// id is for perflog.
    HD_API
    static bool IsAnyPrimvarDirty(HdDirtyBits dirtyBits, SdfPath const& id);

    /// Returns true if the dirtyBits has a dirty topology. id is for perflog.
    HD_API
    static bool IsTopologyDirty(HdDirtyBits dirtyBits, SdfPath const& id);

    /// Returns true if the dirtyBits has dirty doubleSided state. id is for perflog.
    HD_API
    static bool IsDoubleSidedDirty(HdDirtyBits dirtyBits, SdfPath const& id);

    /// Returns true if the dirtyBits has dirty cullstyle. id is for perflog.
    HD_API
    static bool IsCullStyleDirty(HdDirtyBits dirtyBits, SdfPath const& id);

    /// Returns true if the dirtyBits has a dirty transform. id is for perflog.
    HD_API
    static bool IsTransformDirty(HdDirtyBits dirtyBits, SdfPath const& id);

    /// Returns true if the dirtyBits has dirty visibility. id is for perflog.
    HD_API
    static bool IsVisibilityDirty(HdDirtyBits dirtyBits, SdfPath const& id);

    /// Returns true if the dirtyBits has a dirty primID. id is for perflog.
    HD_API
    static bool IsPrimIdDirty(HdDirtyBits dirtyBits, SdfPath const& id);

    /// Returns true if the dirtyBits has a dirty instancer. id is for perflog.
    HD_API
    static bool IsInstancerDirty(HdDirtyBits dirtyBits, SdfPath const& id);

    /// Returns true if the dirtyBits has a dirty instance index. id is for perflog.
    HD_API
    static bool IsInstanceIndexDirty(HdDirtyBits dirtyBits, SdfPath const& id);

    HD_API
    static bool IsReprDirty(HdDirtyBits dirtyBits, SdfPath const &id);

    // ---------------------------------------------------------------------- //

    /// Set the primvar dirty flag to \p dirtyBits.
    HD_API
    static void MarkPrimvarDirty(HdDirtyBits *dirtyBits, TfToken const &name);

    // ---------------------------------------------------------------------- //
    /// @}
    /// \name Task Object Tracking
    /// @{
    // ---------------------------------------------------------------------- //

    /// Start tracking Task with the given \p id.
    HD_API
    void TaskInserted(SdfPath const& id, HdDirtyBits initialDirtyState);

    /// Stop tracking Task with the given \p id.
    HD_API
    void TaskRemoved(SdfPath const& id);

    /// Set the dirty flags to \p bits.
    HD_API
    void MarkTaskDirty(SdfPath const& id, HdDirtyBits bits=AllDirty);

    /// Get the dirty bits for Task with the given \p id.
    HD_API
    HdDirtyBits GetTaskDirtyBits(SdfPath const& id);

    /// Set the dirty flags to \p newBits.
    HD_API
    void MarkTaskClean(SdfPath const& id, HdDirtyBits newBits=Clean);

    /// Retrieve the current version number of the rprim render tag set
    /// XXX Rename to GetRprimRenderTagVersion
    HD_API
    unsigned GetRenderTagVersion() const;

    /// Retrieve the current version number of the task's render tags opinion.
    HD_API
    unsigned GetTaskRenderTagsVersion() const;

    // ---------------------------------------------------------------------- //
    /// @}
    /// \name Instancer State Tracking
    /// @{
    // ---------------------------------------------------------------------- //

    /// Start tracking Instancer with the given \p id.
    HD_API
    void InstancerInserted(SdfPath const& id, HdDirtyBits initialDirtyState);

    /// Stop tracking Instancer with the given \p id.
    HD_API
    void InstancerRemoved(SdfPath const& id);

    /// Returns the dirty bits for the instancer with \p id.
    HD_API
    HdDirtyBits GetInstancerDirtyBits(SdfPath const& id);

    /// Flag the Instancer with the given \p id as being dirty. Multiple calls
    /// with different dirty bits accumulate.
    HD_API
    void MarkInstancerDirty(SdfPath const& id, HdDirtyBits bits=AllDirty);

    /// Clean the specified dirty bits for the instancer with \p id.
    HD_API
    void MarkInstancerClean(SdfPath const& id, HdDirtyBits newBits=Clean);

    /// Insert a dependency between \p rprimId and parent instancer
    /// \p instancerId.  Changes to the latter mark the former with
    /// DirtyInstancer.
    HD_API
    void AddInstancerRprimDependency(SdfPath const& instancerId,
                                     SdfPath const& rprimId);

    /// Remove a dependency between \p rprimId and parent instancer
    /// \p instancerId.
    HD_API
    void RemoveInstancerRprimDependency(SdfPath const& instancerId,
                                        SdfPath const& rprimId);

    /// Insert a dependency between \p instancerId and parent instancer
    /// \p parentInstancerId.  Changes to the latter mark the former with
    /// DirtyInstancer.
    HD_API
    void AddInstancerInstancerDependency(SdfPath const& parentInstancerId,
                                         SdfPath const& instancerId);

    /// Remove a dependency between \p instancerId and parent instancer
    /// \p parentInstancerId.
    HD_API
    void RemoveInstancerInstancerDependency(SdfPath const& parentInstancerId,
                                            SdfPath const& instancerId);

    // ---------------------------------------------------------------------- //
    /// @}
    /// \name Sprim (scene state prim: camera, light, ...) state Tracking
    /// @{
    // ---------------------------------------------------------------------- //

    /// Start tracking sprim with the given \p id.
    HD_API
    void SprimInserted(SdfPath const& id, HdDirtyBits initialDirtyState);

    /// Stop tracking sprim with the given \p id.
    HD_API
    void SprimRemoved(SdfPath const& id);

    /// Get the dirty bits for sprim with the given \p id.
    HD_API
    HdDirtyBits GetSprimDirtyBits(SdfPath const& id);

    /// Set the dirty flags to \p bits.
    HD_API
    void MarkSprimDirty(SdfPath const& id, HdDirtyBits bits);

    /// Set the dirty flags to \p newBits.
    HD_API
    void MarkSprimClean(SdfPath const& id, HdDirtyBits newBits=Clean);

    /// Insert a dependency between \p sprimId and parent instancer
    /// \p instancerId.  Changes to the latter mark the former with
    /// DirtyInstancer.
    HD_API
    void AddInstancerSprimDependency(SdfPath const& instancerId,
                                     SdfPath const& sprimId);

    /// Remove a dependency between \p sprimId and parent instancer
    /// \p instancerId.
    HD_API
    void RemoveInstancerSprimDependency(SdfPath const& instancerId,
                                        SdfPath const& sprimId);

    /// Insert a dependency between \p sprimId and parent sprim
    /// \p parentSprimId.
    HD_API
    void AddSprimSprimDependency(SdfPath const& parentSprimId,
                                 SdfPath const& sprimId);

    /// Remove a dependency between \p sprimId and parent sprim
    /// \p parentSprimId.
    HD_API
    void RemoveSprimSprimDependency(SdfPath const& parentSprimId,
                                    SdfPath const& sprimId);

    /// Remove all dependencies involving \p sprimId as a parent or child.
    HD_API
    void RemoveSprimFromSprimSprimDependencies(SdfPath const& sprimId);

    // ---------------------------------------------------------------------- //
    /// @}
    /// \name Bprim (buffer prim: texture, buffer, ...) state Tracking
    /// @{
    // ---------------------------------------------------------------------- //

    /// Start tracking bprim with the given \p id.
    HD_API
    void BprimInserted(SdfPath const& id, HdDirtyBits initialDirtyState);

    /// Stop tracking bprim with the given \p id.
    HD_API
    void BprimRemoved(SdfPath const& id);

    /// Get the dirty bits for bprim with the given \p id.
    HD_API
    HdDirtyBits GetBprimDirtyBits(SdfPath const& id);

    /// Set the dirty flags to \p bits.
    HD_API
    void MarkBprimDirty(SdfPath const& id, HdDirtyBits bits);

    /// Set the dirty flags to \p newBits.
    HD_API
    void MarkBprimClean(SdfPath const& id, HdDirtyBits newBits=Clean);

    // ---------------------------------------------------------------------- //
    /// @}
    /// \name RprimCollection Tracking
    /// @{
    // ---------------------------------------------------------------------- //

    /// Adds a named collection for tracking.
    HD_API
    void AddCollection(TfToken const& collectionName);

    /// Marks a named collection as being dirty, this bumps the version of the
    /// collection.
    HD_API
    void MarkCollectionDirty(TfToken const& collectionName);

    /// Returns the current version of the named collection.
    HD_API
    unsigned GetCollectionVersion(TfToken const& collectionName) const;

    /// Returns the number of changes to visibility. This is intended to be used
    /// to detect when visibility has changed for *any* Rprim.
    HD_API
    unsigned GetVisibilityChangeCount() const;

    /// Returns the number of changes to instance index. This is intended to be used
    /// to detect when instance indices changed for *any* Rprim. Use in with
    /// GetInstancerIndexVersion() to detect all changes to instance indices.
    HD_API
    unsigned GetInstanceIndicesChangeCount() const;

    /// Returns the current version of varying state. This is used to refresh
    /// cached DirtyLists
    unsigned GetVaryingStateVersion() const {
        return _varyingStateVersion;
    }

    // ---------------------------------------------------------------------- //
    /// @}
    /// \name Render Index Versioning
    /// @{
    // ---------------------------------------------------------------------- //

    /// Returns the current version of the Render Index's RPrim set.
    /// This version number changes when Rprims are inserted or removed
    /// from the render index.  Invalidating any cached gather operations.
    unsigned GetRprimIndexVersion() const {
        return _rprimIndexVersion;
    }

    /// Returns the current version of the Render Index's SPrim set.
    /// This version number changes when Sprims are inserted or removed
    /// from the render index.  Invalidating any cached gather operations.
    unsigned GetSprimIndexVersion() const {
        return _sprimIndexVersion;
    }

    /// Returns the current version of the Render Index's BPrim set.
    /// This version number changes when Bprims are inserted or removed
    /// from the render index.  Invalidating any cached gather operations.
    unsigned GetBprimIndexVersion() const {
        return _bprimIndexVersion;
    }

    /// Returns the current version of the Render Index's Instancer set.
    /// This version number changes when Instancers are inserted or removed
    /// from the render index.  Invalidating any cached gather operations.
    unsigned GetInstancerIndexVersion() const {
        return _instancerIndexVersion;
    }


    /// Returns the current version of the scene state.
    /// This version number changes whenever any prims are inserted, removed
    /// or marked dirty.
    /// The use case is to detect that nothing has changed, so the Sync
    /// phase can be avoided.
    unsigned GetSceneStateVersion() const {
        return _sceneStateVersion;
    }

    // ---------------------------------------------------------------------- //
    /// @}
    /// \name General state tracking
    /// @{
    // ---------------------------------------------------------------------- //

    /// Adds a named state for tracking.
    HD_API
    void AddState(TfToken const& name);

    /// Marks a named state as being dirty., this bumps the version of the
    /// state.
    HD_API
    void MarkStateDirty(TfToken const& name);

    /// Returns the current version of the named state.
    HD_API
    unsigned GetStateVersion(TfToken const &name) const;

    // ---------------------------------------------------------------------- //
    /// @}
    /// \name Debug
    /// @{
    // ---------------------------------------------------------------------- //
    HD_API
    static std::string StringifyDirtyBits(HdDirtyBits dirtyBits);

    HD_API
    static void DumpDirtyBits(HdDirtyBits dirtyBits);

    /// @}

private:

    // Don't allow copies
    HdChangeTracker(const HdChangeTracker &) = delete;
    HdChangeTracker &operator=(const HdChangeTracker &) = delete;


    static void _LogCacheAccess(TfToken const& cacheName,
                                SdfPath const& id, bool hit);

    typedef TfHashMap<SdfPath, HdDirtyBits, SdfPath::Hash> _IDStateMap;
    typedef TfHashMap<TfToken, int, TfToken::HashFunctor> _CollectionStateMap;
    typedef TfHashMap<TfToken, unsigned, TfToken::HashFunctor> _GeneralStateMap;

    struct _PathHashCompare {
        static bool     equal(const SdfPath& a, const SdfPath& b)
                        { return a == b; }

        static size_t   hash(const SdfPath& path)
                        { return hash_value(path); }
    };
    typedef tbb::concurrent_hash_map<SdfPath, SdfPathSet, _PathHashCompare>
        _DependencyMap;

    // Core dirty state.
    _IDStateMap _rprimState;
    _IDStateMap _instancerState;
    _IDStateMap _taskState;
    _IDStateMap _sprimState;
    _IDStateMap _bprimState;
    _GeneralStateMap _generalState;

    // Collection versions / state.
    _CollectionStateMap _collectionState;

    // Provides reverse-association between instancers and the child
    // instancers/prims that use them.
    _DependencyMap _instancerRprimDependencies;
    _DependencyMap _instancerSprimDependencies;
    _DependencyMap _instancerInstancerDependencies;

    // Provides forward and reverse-association between sprims and the child
    // sprims that reference them. For example, a light prim (child) who needs 
    // to know when its light filter (parent) is modified.
    // Maps parent sprim to child sprim.
    _DependencyMap _sprimSprimTargetDependencies;
    // Maps child sprim to parent sprim.
    _DependencyMap _sprimSprimSourceDependencies;
    
    // Dependency map helpers
    void _AddDependency(_DependencyMap &depMap,
        SdfPath const& parent, SdfPath const& child);
    void _RemoveDependency(_DependencyMap &depMap,
        SdfPath const& parent, SdfPath const& child);

    // Typically the Rprims that get marked dirty per update iteration end up
    // being a stable set of objects; to leverage this fact, we require the
    // delegate notify the change tracker when that state changes, which bumps
    // the varyingStateVersion, which triggers downstream invalidation.
    unsigned _varyingStateVersion;
 
    // Tracks changes (insertions/removals) of prims in the render index.
    // This is used to indicating that cached gather operations need to be
    // re-evaluated, such as dirty lists or batch building.
    unsigned _rprimIndexVersion;
    unsigned _sprimIndexVersion;
    unsigned _bprimIndexVersion;
    unsigned _instancerIndexVersion;

    // The following tracks any changes of state.  As a result it is very broad.
    // The use case to detect, when no changes have been made, as to
    // avoid the need to sync or reset progressive renderers.
    unsigned _sceneStateVersion;

    // Used to detect that visibility changed somewhere in the render index.
    unsigned _visChangeCount;

    // Used to detect that instance indices changed somewhere in the render index.
    unsigned _instanceIndicesChangeCount;

    // Used to detect changes to the render tag opinion of rprims.
    unsigned _rprimRenderTagVersion;

    // Used to detect changes to the render tags opinion of tasks.
    unsigned _taskRenderTagsVersion;

    // Allow HdRenderIndex to provide a scene index to forward dirty
    // information. This is necessary to accommodate legacy HdSceneDelegate
    // based applications that rely on the HdChangeTracker for invalidating
    // state on Hydra prims.
    friend class HdRenderIndex;
    // Does not take ownership. The HdRenderIndex manages the lifetime of this
    // scene index.
    HdRetainedSceneIndex * _emulationSceneIndex;
    void _SetTargetSceneIndex(HdRetainedSceneIndex *emulationSceneIndex);

    // Private methods which implement the behaviors of their public
    // equivalents. The public versions check to see if legacy emulation is
    // active. If so, they dirty the HdRetainedSceneIndex member instead
    // of directly acting. If legacy render delegate emulation is active, these
    // will eventually make their way back to the private methods via
    // HdSceneIndexAdapterSceneDelegate. This prevents dirtying cycles while
    // allowing single HdRenderIndex/HdChangeTracker instances to be used for
    // both ends of emulation.
    friend class HdSceneIndexAdapterSceneDelegate;
    void _MarkRprimDirty(SdfPath const& id, HdDirtyBits bits=AllDirty);
    void _MarkSprimDirty(SdfPath const& id, HdDirtyBits bits=AllDirty);
    void _MarkBprimDirty(SdfPath const& id, HdDirtyBits bits=AllDirty);
    void _MarkInstancerDirty(SdfPath const& id, HdDirtyBits bits=AllDirty);
    void _MarkTaskDirty(SdfPath const& id, HdDirtyBits bits=AllDirty);
};


PXR_NAMESPACE_CLOSE_SCOPE

#endif //PXR_IMAGING_HD_CHANGE_TRACKER_H
