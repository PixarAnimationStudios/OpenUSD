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
#ifndef HD_RENDER_INDEX_H
#define HD_RENDER_INDEX_H

#include "pxr/pxr.h"
#include "pxr/imaging/hd/version.h"
#include "pxr/imaging/hd/changeTracker.h"
#include "pxr/imaging/hd/perfLog.h"
#include "pxr/imaging/hd/primTypeIndex.h"
#include "pxr/imaging/hd/tokens.h"

#include "pxr/imaging/hf/perfLog.h"

#include "pxr/base/gf/vec4i.h"

#include "pxr/usd/sdf/path.h"

#include <boost/noncopyable.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/smart_ptr.hpp>
#include "pxr/base/tf/hashmap.h"

#include <vector>
#include <unordered_map>

PXR_NAMESPACE_OPEN_SCOPE


class HdRprim;
class HdSprim;
class HdBprim;
class HdDrawItem;
class HdRprimCollection;
class HdSceneDelegate;
class HdRenderDelegate;
class VtValue;


typedef boost::shared_ptr<class HdDirtyList> HdDirtyListSharedPtr;
typedef boost::shared_ptr<class HdInstancer> HdInstancerSharedPtr;
typedef boost::shared_ptr<class HdTask> HdTaskSharedPtr;
typedef std::vector<HdTaskSharedPtr> HdTaskSharedPtrVector;
typedef std::unordered_map<TfToken,
                           VtValue,
                           TfToken::HashFunctor,
                           TfToken::TokensEqualFunctor> HdTaskContext;

/// \class HdRenderIndex
///
/// The mapping from client scenegraph to the render engine's scene.
///
/// The HdRenderIndex only tracks primitives that result in draw calls and
/// relies on the HdSceneDelegate to provide any hierarchical or other
/// computed values.
///
class HdRenderIndex : public boost::noncopyable {
public:

    typedef std::unordered_map<TfToken, std::vector<HdDrawItem const*>,
                               boost::hash<TfToken> > HdDrawItemView;

    HdRenderIndex();
    ~HdRenderIndex();

    /// Clear all r (render), s (state) and b (buffer) prims.
    void Clear();

    /// Given a prim id and instance id, returns the prim path of the owner
    SdfPath GetPrimPathFromPrimIdColor(GfVec4i const& primIdColor,
                                       GfVec4i const& instanceIdColor,
                                       int* instanceIndexOut = NULL) const;
 
    // ---------------------------------------------------------------------- //
    /// \name Synchronization
    // ---------------------------------------------------------------------- //

    HdDrawItemView GetDrawItems(HdRprimCollection const& collection);

    /// Synchronize all objects in the DirtyList
    void Sync(HdDirtyListSharedPtr const &dirtyList);

    /// Processes all pending dirty lists 
    void SyncAll(HdTaskSharedPtrVector const &tasks, HdTaskContext *taskContext);

    /// Returns a vector of Rprim IDs that are bound to the given DelegateID.
    SdfPathVector const& GetDelegateRprimIDs(SdfPath const& delegateID) const;

    /// For each delegate that has at least one child with dirty bits matching
    /// the given dirtyMask, pushes the delegate ID into the given IDs vector.
    /// The resulting vector is a list of all delegate IDs who have at least one
    /// child that matches the mask.
    void GetDelegateIDsWithDirtyRprims(int dirtyMask, SdfPathVector* IDs) const;

    // ---------------------------------------------------------------------- //
    /// \name Change Tracker
    // ---------------------------------------------------------------------- //

    HdChangeTracker& GetChangeTracker() { return _tracker; }
    HdChangeTracker const& GetChangeTracker() const { return _tracker; }

    // ---------------------------------------------------------------------- //
    /// \name Rprim Support
    // ---------------------------------------------------------------------- //

    /// Insert a rprim into index
    void InsertRprim(TfToken const& typeId,
                     HdSceneDelegate* sceneDelegate,
                     SdfPath const& rprimId,
                     SdfPath const& instancerId = SdfPath());

    /// \deprecated {
    ///   Old templated mathod of inserting a rprim into the index.
    ///   This API has been superseeded by passing the typeId token.
    ///   XXX: This method still exists to aid transition but may be
    ///   removed at any time.
    /// }
    template <typename T>
    void InsertRprim(HdSceneDelegate* delegate,
                     SdfPath const& id,
                     SdfPath const&,   // Unused
                     SdfPath const& instancerId = SdfPath());

    /// Remove a rprim from index
    void RemoveRprim(SdfPath const& id);

    /// Returns true if rprim \p id exists in index.
    bool HasRprim(SdfPath const& id) { 
        return _rprimMap.find(id) != _rprimMap.end();
    }

    /// Returns the rprim of id
    HdRprim const *GetRprim(SdfPath const &id) const;

    /// Returns the scene delegate for the given rprim
    HdSceneDelegate *GetSceneDelegateForRprim(SdfPath const &id) const;

    /// Query function to return the id's of the scene delegate and instancer
    /// associated with the Rprim at the given path.
    bool GetSceneDelegateAndInstancerIds(SdfPath const &id,
                                         SdfPath* sceneDelegateId,
                                         SdfPath* instancerId) const;

    /// Returns true if the given RprimID is a member of the collection.
    bool IsInCollection(SdfPath const& id, TfToken const& collectionName) const;

    /// Returns the subtree rooted under the given path.
    SdfPathVector GetRprimSubtree(SdfPath const& root) const;


    // ---------------------------------------------------------------------- //
    /// \name Instancer Support
    // ---------------------------------------------------------------------- //

    /// Insert an instancer into index
    void InsertInstancer(HdSceneDelegate* delegate,
                         SdfPath const &id,
                         SdfPath const &parentId = SdfPath());

    /// Remove an instancer from index
    void RemoveInstancer(SdfPath const& id);

    /// Returns true if instancer \p id exists in index.
    bool HasInstancer(SdfPath const& id) {
        return _instancerMap.find(id) != _instancerMap.end();
    }

    /// Returns the instancer of id
    HdInstancerSharedPtr GetInstancer(SdfPath const &id) const;

    // ---------------------------------------------------------------------- //
    /// \name Task Support
    // ---------------------------------------------------------------------- //

    /// Inserts a new task into the render index with an identifer of \p id.
    template <typename T>
    void InsertTask(HdSceneDelegate* delegate, SdfPath const& id);

    /// Removes the given task from the RenderIndex.
    void RemoveTask(SdfPath const& id);

    /// Returns true if a task exists in the index with the given \p id.
    bool HasTask(SdfPath const& id) {
        return _taskMap.find(id) != _taskMap.end();
    }

    /// Returns the task for the given \p id.
    HdTaskSharedPtr const& GetTask(SdfPath const& id) const;

    // ---------------------------------------------------------------------- //
    /// \name Scene state prims (e.g. camera, light)
    // ---------------------------------------------------------------------- //

    /// Insert a sprim into index
    void InsertSprim(TfToken const& typeId,
                     HdSceneDelegate* delegate,
                     SdfPath const& sprimId);

    /// \deprecated {
    ///   Old templated mathod of inserting a sprim into the index.
    ///   This API has been superseeded by passing the typeId token.
    ///   XXX: This method still exists to aid transition but may be
    ///   removed at any time.
    /// }
    template <typename T>
    void
    InsertSprim(HdSceneDelegate* delegate, SdfPath const &id);

    void RemoveSprim(TfToken const& typeId, SdfPath const &id);

    HdSprim const *GetSprim(TfToken const& typeId, SdfPath const &id) const;

    /// Returns the subtree rooted under the given path for the given sprim
    /// type.
    SdfPathVector GetSprimSubtree(TfToken const& typeId,
                                  SdfPath const& root) const;

    /// Returns the fullback prim for the Sprim of the given type.
    HdSprim *GetFallbackSprim(TfToken const& typeId) const;


    // ---------------------------------------------------------------------- //
    /// \name Buffer prims (e.g. textures, buffers)
    // ---------------------------------------------------------------------- //

    /// Insert a bprim into index
    void InsertBprim(TfToken const& typeId,
                     HdSceneDelegate* delegate,
                     SdfPath const& bprimId);

    void RemoveBprim(TfToken const& typeId, SdfPath const &id);

    HdBprim const *GetBprim(TfToken const& typeId, SdfPath const &id) const;

    /// Returns the subtree rooted under the given path for the given bprim
    /// type.
    SdfPathVector GetBprimSubtree(TfToken const& typeId,
                                  SdfPath const& root) const;

    /// Returns the fallback prim for the Bprim of the given type.
    HdBprim *GetFallbackBprim(TfToken const& typeId) const;


    // ---------------------------------------------------------------------- //
    /// \name Render Delegate
    // ---------------------------------------------------------------------- //
    /// Currently, a render index only supports connection to one type of
    /// render delegate.  Due to the inserted information and change tracking
    /// being specific to that delegate type.
    void SetRenderDelegate(HdRenderDelegate *renderDelegate);
    TfToken GetRenderDelegateType() const;

    /// Creates fallback prims for each supported prim type
    bool CreateFallbackPrims();

    /// Release the fallback prims
    void DestroyFallbackPrims();

private:
    // ---------------------------------------------------------------------- //
    // Private Helper methods 
    // ---------------------------------------------------------------------- //

    // Go through all RPrims and reallocate their instance ids
    // This is called once we have exhausted all all 24bit instance ids.
    void _CompactPrimIds();

    // Allocate the next available instance id to the prim
    void _AllocatePrimId(HdRprim* prim);

    // Inserts the task into the index and updates tracking state.
    void _TrackDelegateTask(HdSceneDelegate* delegate, 
                            SdfPath const& taskId,
                            HdTaskSharedPtr const& task);

    template <typename T>
    static inline const TfToken & _GetTypeId();


    // ---------------------------------------------------------------------- //
    // Index State
    // ---------------------------------------------------------------------- //
    struct _RprimInfo {
        HdSceneDelegate *sceneDelegate;
        size_t           childIndex;
        HdRprim         *rprim;
    };


    typedef TfHashMap<SdfPath, HdTaskSharedPtr, SdfPath::Hash> _TaskMap;
    typedef TfHashMap<SdfPath, _RprimInfo, SdfPath::Hash> _RprimMap;
    typedef TfHashMap<SdfPath, SdfPathVector, SdfPath::Hash> _DelegateRprimMap;


    typedef std::set<SdfPath> _RprimIDSet;
    typedef std::map<uint32_t, SdfPath> _RprimPrimIDMap;

    typedef Hd_PrimTypeIndex<HdSprim> _SprimIndex;
    typedef Hd_PrimTypeIndex<HdBprim> _BprimIndex;

    _DelegateRprimMap _delegateRprimMap;
    _RprimMap _rprimMap;

    _RprimIDSet _rprimIDSet;
    _RprimPrimIDMap _rprimPrimIdMap;

    _TaskMap _taskMap;

    _SprimIndex     _sprimIndex;
    _BprimIndex     _bprimIndex;

    HdChangeTracker _tracker;
    int32_t _nextPrimId; 

    typedef TfHashMap<SdfPath, HdInstancerSharedPtr, SdfPath::Hash> _InstancerMap;
    _InstancerMap _instancerMap;

    // XXX: TO FIX Move
    typedef std::vector<HdDirtyListSharedPtr> _DirtyListVector;
    _DirtyListVector _syncQueue;

    HdRenderDelegate *_renderDelegate;

    // XXX: This is a temporary variable to aid in transition to the new
    // context api.  Under the new API, the render delegate is owned by
    // the context.  However, as clients are not creating the delegate
    // yet, the render index will create one on their behalf.
    //
    // It was preferred to add this variable than use the reference counting
    // mechanism.  As that impacted the new code path, rather than explicitly
    // calling out the transitional elements.
    bool _ownsDelegateXXX;

    void _InitPrimTypes();
};

template <typename T>
void
HdRenderIndex::InsertTask(HdSceneDelegate* delegate, SdfPath const& id)
{
    HD_TRACE_FUNCTION();
    HF_MALLOC_TAG_FUNCTION();

    HdTaskSharedPtr task = boost::make_shared<T>(delegate, id);
    _TrackDelegateTask(delegate, id, task);
}

///////////////////////////////////////////////////////////////////////////////
//
// Transitional support routines to convert from the templated InsertRprim()
// to the one that takes a typeId token.
//
// XXX: To be removed with the InsertRprim<> API.
//
///////////////////////////////////////////////////////////////////////////////
class HdMesh;
class HdBasisCurves;
class HdPoints;
class HdxCamera;
class HdxDrawTarget;
class HdxLight;

namespace HdRenderIndexInternal
{  
    template <typename T>
    inline const TfToken & _GetTypeId();

    template <>
    inline
    const TfToken &
    _GetTypeId<HdMesh>()
    {
        return HdPrimTypeTokens->mesh;
    }
    
    template <>
    inline
    const TfToken &
    _GetTypeId<HdBasisCurves>()
    {
        return HdPrimTypeTokens->basisCurves;
    }
    
    template <>
    inline
    const TfToken &
    _GetTypeId<HdPoints>()
    {
        return HdPrimTypeTokens->points;
    }

    template <>
    inline
    const TfToken &
    _GetTypeId<HdxCamera>()
    {
        return HdPrimTypeTokens->camera;
    }

    template <>
    inline
    const TfToken &
    _GetTypeId<HdxDrawTarget>()
    {
        return HdPrimTypeTokens->drawTarget;
    }

    template <>
    inline
    const TfToken &
    _GetTypeId<HdxLight>()
    {
        return HdPrimTypeTokens->light;
    }

} 

template <typename T>
void
HdRenderIndex::InsertRprim(HdSceneDelegate* delegate, SdfPath const& id,
                           SdfPath const&,
                           SdfPath const& instancerId)
{
    InsertRprim(HdRenderIndexInternal::_GetTypeId<T>(), delegate, id, instancerId);
}


template <typename T>
void
HdRenderIndex::InsertSprim(HdSceneDelegate* delegate, SdfPath const& id)
{
    InsertSprim(HdRenderIndexInternal::_GetTypeId<T>(), delegate, id);
}

///////////////////////////////////////////////////////////////////////////////
//
// XXX: End Transitional support routines
//
///////////////////////////////////////////////////////////////////////////////



PXR_NAMESPACE_CLOSE_SCOPE

#endif //HD_RENDER_INDEX_H
