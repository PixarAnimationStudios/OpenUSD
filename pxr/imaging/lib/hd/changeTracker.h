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

#include "pxr/imaging/hd/api.h"
#include "pxr/imaging/hd/version.h"
#include "pxr/imaging/hd/rprimCollection.h"
#include "pxr/usd/sdf/path.h"
#include "pxr/base/tf/hashmap.h"

#include <atomic>
#include <boost/noncopyable.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/unordered_map.hpp>
#include <boost/weak_ptr.hpp>

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
        DirtySurfaceShader    = 1 << 7,   // XXX: surface shader uses this bit
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

    // Dirty bits for Tasks, Textures
    enum NonRprimDirtyBits {
        //Varying               = 1 << 0,
        DirtyType             = 1 << 1,
        DirtyChildren         = 1 << 2,
        DirtyParams           = 1 << 3,
        DirtyCollection       = 1 << 4,
        DirtyTexture          = 1 << 5,
    };

    typedef int DirtyBits;

    HdChangeTracker();
    virtual ~HdChangeTracker();

    // ---------------------------------------------------------------------- //
    /// \name Rprim Object Tracking
    /// @{
    // ---------------------------------------------------------------------- //

    /// Start tracking Rprim with the given \p id.
	HDLIB_API
    void RprimInserted(SdfPath const& id, int initialDirtyState);

    /// Stop tracking Rprim with the given \p id.
	HDLIB_API
    void RprimRemoved(SdfPath const& id);

    // ---------------------------------------------------------------------- //
    /// @}
    /// \name Rprim State Tracking
    /// @{
    // ---------------------------------------------------------------------- //

    /// Returns the dirty bits for the rprim with \p id.
	HDLIB_API
    DirtyBits GetRprimDirtyBits(SdfPath const& id) const;

    /// Flag the Rprim with the given \p id as being dirty. Multiple calls with
    /// different dirty bits accumulate.
	HDLIB_API
    void MarkRprimDirty(SdfPath const& id, DirtyBits bits=AllDirty);

    /// Clear the dirty flags for an HdRprim. if inSync is true, set OutOfSync
    /// flag to notify dirtyList will discover the prim to sync the residual
    /// data for new repr.
	HDLIB_API
    void MarkRprimClean(SdfPath const& id, DirtyBits newBits=Clean);

    /// Mark the primvar for the rprim with \p id as being dirty.
	HDLIB_API
    void MarkPrimVarDirty(SdfPath const& id, TfToken const& name);

    /// Flag all the Rprim with the given \p id as being dirty. Multiple calls
    /// with different dirty bits accumulate.
    /// Doesn't touch varying state.
    void MarkAllRprimsDirty(DirtyBits bits);

    // Clear Varying bit of all prims.
    ///
    /// The idea is that from frame to frame (update iteration), the set of
    /// dirty rprims and their dirty bits do not change; that is, the same
    /// rprims get dirtied with the same dirty bits.. The change tracker can
    /// leverage this and build stable sets of dirty lists and reduce the
    /// overall cost of an update iteration.
	HDLIB_API
    void ResetVaryingState();

    // ---------------------------------------------------------------------- //

    /// Returns true if the rprim identified by \p id has any dirty flags set.
	HDLIB_API
    bool IsRprimDirty(SdfPath const& id);
    
    /// Returns true if the rprim identified by \p id has a dirty extent.
	HDLIB_API
    bool IsExtentDirty(SdfPath const& id);

    /// Returns true if the rprim identified by \p id has a dirty refine level.
	HDLIB_API
    bool IsRefineLevelDirty(SdfPath const& id);

    /// Returns true if the rprim identified by \p id with primvar \p name is
    /// dirty.
	HDLIB_API
    bool IsPrimVarDirty(SdfPath const& id, TfToken const& name);

    /// Returns true if the rprim identified by \p id has any dirty primvars.
	HDLIB_API
    bool IsAnyPrimVarDirty(SdfPath const& id);

    /// Returns true if the rprim identified by \p id has a dirty topology.
	HDLIB_API
    bool IsTopologyDirty(SdfPath const& id);

    /// Returns true if the rprim identified by \p id has dirty doubleSided state.
	HDLIB_API
    bool IsDoubleSidedDirty(SdfPath const& id);

    /// Returns true if the rprim identified by \p id has dirty cullstyle.
	HDLIB_API
    bool IsCullStyleDirty(SdfPath const& id);

    /// Returns true if the rprim identified by \p id has a dirty subdiv tags.
	HDLIB_API
    bool IsSubdivTagsDirty(SdfPath const& id);

    /// Returns true if the rprim identified by \p id has a dirty transform.
	HDLIB_API
    bool IsTransformDirty(SdfPath const& id);

    /// Returns true if the rprim identified by \p id has dirty visibility.
	HDLIB_API
    bool IsVisibilityDirty(SdfPath const& id);
    
    /// Returns true if the rprim identified by \p id has a dirty primID.
	HDLIB_API
    bool IsPrimIdDirty(SdfPath const& id);

    /// Returns true if the dirtyBits has any flags set other than the varying flag.
    static bool IsDirty(DirtyBits dirtyBits) {
        return (dirtyBits & AllDirty) != 0;
    }

    /// Returns true if the dirtyBits has no flags set except the varying flag.
    static bool IsClean(DirtyBits dirtyBits) {
        return (dirtyBits & AllDirty) == 0;
    }

    /// Returns true if the dirtyBits has a dirty extent. id is for perflog.
	HDLIB_API
    static bool IsExtentDirty(DirtyBits dirtyBits, SdfPath const& id);

    /// Returns true if the dirtyBits has a dirty refine level. id is for perflog.
	HDLIB_API
    static bool IsRefineLevelDirty(DirtyBits dirtyBits, SdfPath const& id);

    /// Returns true if the dirtyBits has a dirty subdiv tags. id is for perflog.
	HDLIB_API
    static bool IsSubdivTagsDirty(DirtyBits dirtyBits, SdfPath const& id);

    /// Returns true if the dirtyBits has a dirty primvar \p name.
    /// id is for perflog.
	HDLIB_API
    static bool IsPrimVarDirty(DirtyBits dirtyBits, SdfPath const& id,
                               TfToken const& name);

    /// Returns true if the dirtyBits has any dirty primvars.
    /// id is for perflog.
	HDLIB_API
    static bool IsAnyPrimVarDirty(DirtyBits dirtyBits, SdfPath const& id);

    /// Returns true if the dirtyBits has a dirty topology. id is for perflog.
	HDLIB_API
    static bool IsTopologyDirty(DirtyBits dirtyBits, SdfPath const& id);

    /// Returns true if the dirtyBits has dirty doubleSided state. id is for perflog.
	HDLIB_API
    static bool IsDoubleSidedDirty(DirtyBits dirtyBits, SdfPath const& id);

    /// Returns true if the dirtyBits has dirty cullstyle. id is for perflog.
	HDLIB_API
    static bool IsCullStyleDirty(DirtyBits dirtyBits, SdfPath const& id);

    /// Returns true if the dirtyBits has a dirty transform. id is for perflog.
	HDLIB_API
    static bool IsTransformDirty(DirtyBits dirtyBits, SdfPath const& id);

    /// Returns true if the dirtyBits has dirty visibility. id is for perflog.
	HDLIB_API
    static bool IsVisibilityDirty(DirtyBits dirtyBits, SdfPath const& id);

    /// Returns true if the dirtyBits has a dirty primID. id is for perflog.
	HDLIB_API
    static bool IsPrimIdDirty(DirtyBits dirtyBits, SdfPath const& id);

    /// Returns true if the dirtyBits has a dirty instancer. id is for perflog.
	HDLIB_API
    static bool IsInstancerDirty(DirtyBits dirtyBits, SdfPath const& id);

    /// Returns true if the dirtyBits has a dirty instance index. id is for perflog.
	HDLIB_API
    static bool IsInstanceIndexDirty(DirtyBits dirtyBits, SdfPath const& id);

	HDLIB_API
    static bool IsReprDirty(DirtyBits dirtyBits, SdfPath const &id);

    // ---------------------------------------------------------------------- //

    /// Set the primvar dirty flag to \p dirtyBits.
	HDLIB_API
    static void MarkPrimVarDirty(DirtyBits *dirtyBits, TfToken const &name);

    // ---------------------------------------------------------------------- //
    /// @}
    /// \name Instancer Object Tracking
    /// @{
    // ---------------------------------------------------------------------- //

    /// Start tracking Instancer with the given \p id.
	HDLIB_API
    void InstancerInserted(SdfPath const& id);

    /// Stop tracking Instancer with the given \p id.
	HDLIB_API
    void InstancerRemoved(SdfPath const& id);

    /// Add the gived \p rprimId to the list of rprims associated with the
    /// instancer \p instancerId
    void InstancerRPrimInserted(SdfPath const& instancerId, SdfPath const& rprimId);

    /// Remove the gived \p rprimId to the list of rprims associated with the
    /// instancer \p instancerId
    void InstancerRPrimRemoved(SdfPath const& instancerId, SdfPath const& rprimId);


    // ---------------------------------------------------------------------- //
    /// @}
    /// \name Shader Object Tracking
    /// @{
    // ---------------------------------------------------------------------- //

    /// Start tracking Shader with the given \p id.
	HDLIB_API
    void ShaderInserted(SdfPath const& id);

    /// Stop tracking Shader with the given \p id.
	HDLIB_API
    void ShaderRemoved(SdfPath const& id);

    /// Set the dirty flags to \p bits.
	HDLIB_API
    void MarkShaderDirty(SdfPath const& id, DirtyBits bits=AllDirty);

    /// Get the dirty bits for Shader with the given \p id.
	HDLIB_API
    DirtyBits GetShaderDirtyBits(SdfPath const& id);

    /// Set the dirty flags to \p newBits.
	HDLIB_API
    void MarkShaderClean(SdfPath const& id, DirtyBits newBits=Clean);

    /// Sets all shaders to the given dirty \p bits
    void MarkAllShadersDirty(DirtyBits bits);

    // ---------------------------------------------------------------------- //
    /// @}
    /// \name Task Object Tracking
    /// @{
    // ---------------------------------------------------------------------- //

    /// Start tracking Task with the given \p id.
	HDLIB_API
    void TaskInserted(SdfPath const& id);

    /// Stop tracking Task with the given \p id.
	HDLIB_API
    void TaskRemoved(SdfPath const& id);

    /// Set the dirty flags to \p bits.
	HDLIB_API
    void MarkTaskDirty(SdfPath const& id, DirtyBits bits=AllDirty);

    /// Get the dirty bits for Task with the given \p id.
	HDLIB_API
    DirtyBits GetTaskDirtyBits(SdfPath const& id);

    /// Set the dirty flags to \p newBits.
	HDLIB_API
    void MarkTaskClean(SdfPath const& id, DirtyBits newBits=Clean);

    // ---------------------------------------------------------------------- //
    /// @}
    /// \name Texture Object Tracking
    /// @{
    // ---------------------------------------------------------------------- //

    /// Start tracking Texture with the given \p id.
	HDLIB_API
    void TextureInserted(SdfPath const& id);

    /// Stop tracking Texture with the given \p id.
	HDLIB_API
    void TextureRemoved(SdfPath const& id);

    /// Set the dirty flags to \p bits.
	HDLIB_API
    void MarkTextureDirty(SdfPath const& id, DirtyBits bits=AllDirty);

    /// Get the dirty bits for Texture with the given \p id.
	HDLIB_API
    DirtyBits GetTextureDirtyBits(SdfPath const& id);

    /// Set the dirty flags to \p newBits.
	HDLIB_API
    void MarkTextureClean(SdfPath const& id, DirtyBits newBits=Clean);

    // ---------------------------------------------------------------------- //
    /// @}
    /// \name Instancer State Tracking
    /// @{
    // ---------------------------------------------------------------------- //

    /// Returns the dirty bits for the instancer with \p id.
	HDLIB_API
    DirtyBits GetInstancerDirtyBits(SdfPath const& id);

    /// Flag the Instancer with the given \p id as being dirty. Multiple calls
    /// with different dirty bits accumulate.
	HDLIB_API
    void MarkInstancerDirty(SdfPath const& id, DirtyBits bits=AllDirty);

    /// Clean the specified dirty bits for the instancer with \p id.
	HDLIB_API
    void MarkInstancerClean(SdfPath const& id, DirtyBits newBits=Clean);

    // ---------------------------------------------------------------------- //
    /// @}

    /// \name Sprim (scene state prim: camera, light, ...) state Tracking
    /// @{
    // ---------------------------------------------------------------------- //

    /// Start tracking sprim with the given \p id.
    HDLIB_API
    void SprimInserted(SdfPath const& id, int initialDirtyState);

    /// Stop tracking sprim with the given \p id.
    HDLIB_API
    void SprimRemoved(SdfPath const& id);

    /// Get the dirty bits for sprim with the given \p id.
    HDLIB_API
    DirtyBits GetSprimDirtyBits(SdfPath const& id);

    /// Set the dirty flags to \p bits.
    HDLIB_API
    void MarkSprimDirty(SdfPath const& id, DirtyBits bits);

    /// Set the dirty flags to \p newBits.
    HDLIB_API
    void MarkSprimClean(SdfPath const& id, DirtyBits newBits=Clean);

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
	HDLIB_API
    void AddCollection(TfToken const& collectionName);

    /// Marks a named collection as being dirty, this bumps the version of the
    /// collection.
	HDLIB_API
    void MarkCollectionDirty(TfToken const& collectionName);

    /// Invalidates all collections by bumping a global version number.
	HDLIB_API
    void MarkAllCollectionsDirty();

    /// Returns the current version of the named collection.
	HDLIB_API
    unsigned GetCollectionVersion(TfToken const& collectionName);

    /// Returns the number of changes to visibility. This is intended to be used
    /// to detect when visibility has changed for *any* Rprim.
	HDLIB_API
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
	HDLIB_API
    void MarkShaderBindingsDirty();

    /// Returns the current shader binding version.
	HDLIB_API
    unsigned GetShaderBindingsVersion() const;

    // ---------------------------------------------------------------------- //
    /// @}
    /// \name General state tracking
    /// @{
    // ---------------------------------------------------------------------- //

    /// Adds a named state for tracking.
    HDLIB_API
    void AddState(TfToken const& name);

    /// Marks a named state as being dirty., this bumps the version of the
    /// state.
    HDLIB_API
    void MarkStateDirty(TfToken const& name);

    /// Returns the current version of the named state.
    HDLIB_API
    unsigned GetStateVersion(TfToken const &name) const;

    // ---------------------------------------------------------------------- //
    /// @}
    /// \name Debug
    /// @{
    // ---------------------------------------------------------------------- //
	HDLIB_API
    static std::string StringifyDirtyBits(int dirtyBits);

	HDLIB_API
    static void DumpDirtyBits(int dirtyBits);

    /// @}

private:

    static void _LogCacheAccess(TfToken const& cacheName,
                                SdfPath const& id, bool hit);

    static DirtyBits _PropagateDirtyBits(DirtyBits bits);

    typedef TfHashMap<SdfPath, int, SdfPath::Hash> _IDStateMap;
    typedef TfHashMap<TfToken, int, TfToken::HashFunctor> _CollectionStateMap;
    typedef TfHashMap<SdfPath, SdfPathSet, SdfPath::Hash> _InstancerRprimMap;
    typedef TfHashMap<TfToken, unsigned, TfToken::HashFunctor> _GeneralStateMap;

    // Core dirty state.
    _IDStateMap _rprimState;
    _IDStateMap _instancerState;
    _IDStateMap _shaderState;
    _IDStateMap _taskState;
    _IDStateMap _textureState;
    _IDStateMap _sprimState;
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

#endif //HD_CHANGE_TRACKER_H
