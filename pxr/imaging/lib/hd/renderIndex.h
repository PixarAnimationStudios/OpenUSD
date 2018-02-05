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
#include "pxr/imaging/hd/api.h"
#include "pxr/imaging/hd/version.h"
#include "pxr/imaging/hd/changeTracker.h"
#include "pxr/imaging/hd/perfLog.h"
#include "pxr/imaging/hd/primTypeIndex.h"
#include "pxr/imaging/hd/sortedIds.h"
#include "pxr/imaging/hd/tokens.h"

#include "pxr/imaging/hf/perfLog.h"

#include "pxr/usd/sdf/path.h"

#include "pxr/base/gf/vec4i.h"
#include "pxr/base/tf/hashmap.h"

#include <boost/noncopyable.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/smart_ptr.hpp>

#include <tbb/enumerable_thread_specific.h>

#include <vector>
#include <unordered_map>
#include <memory>

PXR_NAMESPACE_OPEN_SCOPE


class HdRprim;
class HdSprim;
class HdBprim;
class HdDrawItem;
class HdRprimCollection;
class HdSceneDelegate;
class HdRenderDelegate;
class HdExtComputation;
class VtValue;
class HdInstancer;


typedef boost::shared_ptr<class HdDirtyList> HdDirtyListSharedPtr;
typedef boost::shared_ptr<class HdTask> HdTaskSharedPtr;
typedef boost::shared_ptr<class HdResourceRegistry> HdResourceRegistrySharedPtr;
typedef std::vector<HdTaskSharedPtr> HdTaskSharedPtrVector;
typedef std::unordered_map<TfToken,
                           VtValue,
                           TfToken::HashFunctor> HdTaskContext;

/// \class HdRenderIndex
///
/// The Hydra render index is a flattened representation of the client scene 
/// graph, which may be composed of several self-contained scene graphs, each of
/// which provides a HdSceneDelegate adapter for data access.
/// 
/// Thus, multiple HdSceneDelegate's may be tied to the same HdRenderIndex.
/// 
/// The render index, however, is tied to a single HdRenderDelegate, which
/// handles the actual creation and deletion of Hydra scene primitives. These
/// include geometry and non-drawable objects (such as the camera and texture
/// buffers). The render index simply holds a handle to these primitives, and 
/// tracks any changes to them via the HdChangeTracker. 
/// It also tracks computations and tasks that may update resources and render a
/// subset of the renderable primitives.
/// 
/// The render index orchestrates the "syncing" of scene primitives, by
/// providing the relevant scene delegate for data access, and leaves resource
/// management to the rendering backend (via HdResourceRegistry).
/// 
/// It also provides "execution" functionality for application facing Hydra
/// concepts (such as HdTask/HdRenderPass) in computing the set of HdDrawItems
/// for a given HdRprimCollection, for rendering.
/// 
/// \sa
/// HdChangeTracker
/// HdDrawItem
/// HdRenderDelegate
/// HdRprimCollection
/// HdSceneDelegate
/// 
/// \note
/// The current design ties a HdRenderIndex to a HdRenderDelegate.
/// However, the HdRenderIndex isn't tied to a viewer (viewport). 
/// It is common to have multiple viewers image the composed scene (for example,
/// with different cameras), in which case the HdRenderIndex and
/// HdRenderDelegate are shared by the viewers.
/// 
/// If two viewers use different HdRenderDelegate's, then it may unfortunately 
/// require populating two HdRenderIndex's.
///
class HdRenderIndex final : public boost::noncopyable {
public:
    typedef std::vector<HdDrawItem const*> HdDrawItemPtrVector;
    typedef std::unordered_map<TfToken, HdDrawItemPtrVector,
                               boost::hash<TfToken> > HdDrawItemView;

    /// Create a render index with the given render delegate.
    /// Returns null if renderDelegate is null.
    HD_API
    static HdRenderIndex* New(HdRenderDelegate *renderDelegate) {
        if (renderDelegate == nullptr) {
            TF_CODING_ERROR(
                "Null Render Delegate provided to create render index");
            return nullptr;
        }
        return new HdRenderIndex(renderDelegate);
    }

    HD_API
    ~HdRenderIndex();

    /// Clear all r (render), s (state) and b (buffer) prims.
    HD_API
    void Clear();

    /// Clear all entries in the render index under
    /// the given root and belong to a specified delegate.
    ///
    /// Used for example to unload a delegate.
    HD_API
    void RemoveSubtree(const SdfPath &root, HdSceneDelegate* sceneDelegate);

    // ---------------------------------------------------------------------- //
    /// Given a prim id, returns the path of the correspoding rprim
    /// or an empty path if none is found.
    HD_API
    SdfPath GetRprimPathFromPrimId(int primId) const;
 
    // ---------------------------------------------------------------------- //
    /// \name Synchronization
    // ---------------------------------------------------------------------- //

    /// Adds the dirty list to the sync queue. The actual processing of the
    /// dirty list happens later in SyncAll().
    /// 
    /// This is typically called from HdRenderPass::Sync. However, the current
    /// call chain ties it to SyncAll, i.e.
    /// HdRenderIndex::SyncAll > .... > HdRenderPass::Sync > HdRenderIndex::Sync
    HD_API
    void Sync(HdDirtyListSharedPtr const &dirtyList);

    /// Syncs input tasks, B & S prims, (external) computations and processes 
    /// all pending dirty lists (which syncs the R prims). At the end of this
    /// step, all the resources that need to be updated have handles to their
    /// data sources.
    /// This is the first phase in Hydra's execution. See HdEngine::Execute
    HD_API
    void SyncAll(HdTaskSharedPtrVector const &tasks, HdTaskContext *taskContext);

    // ---------------------------------------------------------------------- //
    /// \name Execution
    // ---------------------------------------------------------------------- //

    /// Returns a tag based grouping of the list of relevant draw items for the 
    /// collection.
    /// The is typically called during render pass execution, which is the 
    /// final phase in the Hydra's execution. See HdRenderPass::Execute
    HD_API
    HdDrawItemView GetDrawItems(HdRprimCollection const& collection);

    // ---------------------------------------------------------------------- //
    /// \name Change Tracker
    // ---------------------------------------------------------------------- //

    HdChangeTracker& GetChangeTracker() { return _tracker; }
    HdChangeTracker const& GetChangeTracker() const { return _tracker; }

    // ---------------------------------------------------------------------- //
    /// \name Renderable prims (e.g. meshes, basis curves)
    // ---------------------------------------------------------------------- //

    /// Returns whether the rprim type is supported by this render index.
    HD_API
    bool IsRprimTypeSupported(TfToken const& typeId) const;

    /// Insert a rprim into index
    HD_API
    void InsertRprim(TfToken const& typeId,
                     HdSceneDelegate* sceneDelegate,
                     SdfPath const& rprimId,
                     SdfPath const& instancerId = SdfPath());

    /// Remove a rprim from index
    HD_API
    void RemoveRprim(SdfPath const& id);

    /// Returns true if rprim \p id exists in index.
    bool HasRprim(SdfPath const& id) { 
        return _rprimMap.find(id) != _rprimMap.end();
    }

    /// Returns the rprim of id
    HD_API
    HdRprim const *GetRprim(SdfPath const &id) const;

    /// Returns the scene delegate for the given rprim
    HD_API
    HdSceneDelegate *GetSceneDelegateForRprim(SdfPath const &id) const;

    /// Query function to return the id's of the scene delegate and instancer
    /// associated with the Rprim at the given path.
    HD_API
    bool GetSceneDelegateAndInstancerIds(SdfPath const &id,
                                         SdfPath* sceneDelegateId,
                                         SdfPath* instancerId) const;

    /// Returns the render tag for the given rprim
    HD_API
    TfToken GetRenderTag(SdfPath const& id, TfToken const& reprName) const;

    /// Returns a sorted list of all Rprims in the render index.
    /// The list is sorted by std::less<SdfPath>
    HD_API
    const SdfPathVector &GetRprimIds() { return _rprimIds.GetIds(); }


    /// Returns the subtree rooted under the given path.
    HD_API
    SdfPathVector GetRprimSubtree(SdfPath const& root);


    // ---------------------------------------------------------------------- //
    /// \name Instancer Support
    // ---------------------------------------------------------------------- //

    /// Insert an instancer into index
    HD_API
    void InsertInstancer(HdSceneDelegate* delegate,
                         SdfPath const &id,
                         SdfPath const &parentId = SdfPath());

    /// Remove an instancer from index
    HD_API
    void RemoveInstancer(SdfPath const& id);

    /// Returns true if instancer \p id exists in index.
    bool HasInstancer(SdfPath const& id) {
        return _instancerMap.find(id) != _instancerMap.end();
    }

    /// Returns the instancer of id
    HD_API
    HdInstancer *GetInstancer(SdfPath const &id) const;

    // ---------------------------------------------------------------------- //
    /// \name Task Support
    // ---------------------------------------------------------------------- //

    /// Inserts a new task into the render index with an identifer of \p id.
    template <typename T>
    void InsertTask(HdSceneDelegate* delegate, SdfPath const& id);

    /// Removes the given task from the RenderIndex.
    HD_API
    void RemoveTask(SdfPath const& id);

    /// Returns true if a task exists in the index with the given \p id.
    bool HasTask(SdfPath const& id) {
        return _taskMap.find(id) != _taskMap.end();
    }

    /// Returns the task for the given \p id.
    HD_API
    HdTaskSharedPtr const& GetTask(SdfPath const& id) const;

    // ---------------------------------------------------------------------- //
    /// \name Scene state prims (e.g. camera, light)
    // ---------------------------------------------------------------------- //

    /// Returns whether the sprim type is supported by this render index.
    HD_API
    bool IsSprimTypeSupported(TfToken const& typeId) const;

    /// Insert a sprim into index
    HD_API
    void InsertSprim(TfToken const& typeId,
                     HdSceneDelegate* delegate,
                     SdfPath const& sprimId);

    HD_API
    void RemoveSprim(TfToken const& typeId, SdfPath const &id);

    HD_API
    HdSprim const *GetSprim(TfToken const& typeId, SdfPath const &id) const;

    /// Returns the subtree rooted under the given path for the given sprim
    /// type.
    HD_API
    SdfPathVector GetSprimSubtree(TfToken const& typeId,
                                  SdfPath const& root);

    /// Returns the fullback prim for the Sprim of the given type.
    HD_API
    HdSprim *GetFallbackSprim(TfToken const& typeId) const;


    // ---------------------------------------------------------------------- //
    /// \name Buffer prims (e.g. textures, buffers)
    // ---------------------------------------------------------------------- //

    /// Returns whether the bprim type is supported by this render index.
    HD_API
    bool IsBprimTypeSupported(TfToken const& typeId) const;

    /// Insert a bprim into index
    HD_API
    void InsertBprim(TfToken const& typeId,
                     HdSceneDelegate* delegate,
                     SdfPath const& bprimId);

    HD_API
    void RemoveBprim(TfToken const& typeId, SdfPath const &id);

    HD_API
    HdBprim const *GetBprim(TfToken const& typeId, SdfPath const &id) const;

    /// Returns the subtree rooted under the given path for the given bprim
    /// type.
    HD_API
    SdfPathVector GetBprimSubtree(TfToken const& typeId,
                                  SdfPath const& root);

    /// Returns the fallback prim for the Bprim of the given type.
    HD_API
    HdBprim *GetFallbackBprim(TfToken const& typeId) const;

    // ---------------------------------------------------------------------- //
    /// \name ExtComputation Support
    // ---------------------------------------------------------------------- //

    /// Insert an ExtComputation into index
    HD_API
    void InsertExtComputation(HdSceneDelegate* delegate,
                              SdfPath const &id);

    /// Remove an ExtComputation from index
    HD_API
    void RemoveExtComputation(SdfPath const& id);

    /// Returns true if ExtComputation \p id exists in index.
    bool HasExtComputation(SdfPath const& id) {
        return _extComputationMap.find(id) != _extComputationMap.end();
    }

    /// Returns the ExtComputation of id
    HD_API
    HdExtComputation const *GetExtComputation(SdfPath const &id) const;

    /// Query function to look up a computation and its delegate
    HD_API
    void GetExtComputationInfo(SdfPath const &id,
                               HdExtComputation **computation,
                               HdSceneDelegate **sceneDelegate);


    // ---------------------------------------------------------------------- //
    /// \name Render Delegate
    // ---------------------------------------------------------------------- //
    /// Currently, a render index only supports connection to one type of
    /// render delegate, due to the inserted information and change tracking
    /// being specific to that delegate type.
    HD_API
    HdRenderDelegate *GetRenderDelegate() const;

    /// Returns a shared ptr to the resource registry of the current render
    /// delegate.
    HD_API
    HdResourceRegistrySharedPtr GetResourceRegistry() const;

private:
    // The render index constructor is private so we can check
    // renderDelegate before construction: see HdRenderIndex::New(...).
    HdRenderIndex(HdRenderDelegate *renderDelegate);

    // ---------------------------------------------------------------------- //
    // Private Helper methods 
    // ---------------------------------------------------------------------- //

    // Go through all RPrims and reallocate their instance ids
    // This is called once we have exhausted all all 24bit instance ids.
    void _CompactPrimIds();

    // Allocate the next available instance id to the prim
    void _AllocatePrimId(HdRprim* prim);

    // Inserts the task into the index and updates tracking state.
    // _TrackDelegateTask is called by the inlined InsertTask<T>, so it needs
    // to be marked HD_API.
    HD_API
    void _TrackDelegateTask(HdSceneDelegate* delegate, 
                            SdfPath const& taskId,
                            HdTaskSharedPtr const& task);

    template <typename T>
    static inline const TfToken & _GetTypeId();


    void _RemoveRprimSubtree(const SdfPath &root,
                             HdSceneDelegate* sceneDelegate);
    void _RemoveInstancerSubtree(const SdfPath &root,
                                 HdSceneDelegate* sceneDelegate);
    void _RemoveExtComputationSubtree(const SdfPath &root,
                                      HdSceneDelegate* sceneDelegate);
    void _RemoveTaskSubtree(const SdfPath &root,
                            HdSceneDelegate* sceneDelegate);


    // ---------------------------------------------------------------------- //
    // Index State
    // ---------------------------------------------------------------------- //
    struct _RprimInfo {
        HdSceneDelegate *sceneDelegate;
        HdRprim         *rprim;
    };

    typedef std::unique_ptr<HdExtComputation> HdExtComputationPtr;
    struct _ExtComputationInfo {
        HdSceneDelegate     *sceneDelegate;
        HdExtComputationPtr  extComputation;
    };


    typedef TfHashMap<SdfPath, HdTaskSharedPtr, SdfPath::Hash> _TaskMap;
    typedef TfHashMap<SdfPath, _RprimInfo, SdfPath::Hash> _RprimMap;
    typedef std::map<uint32_t, SdfPath> _RprimPrimIDMap;

    typedef Hd_PrimTypeIndex<HdSprim> _SprimIndex;
    typedef Hd_PrimTypeIndex<HdBprim> _BprimIndex;

    _RprimMap     _rprimMap;
    Hd_SortedIds  _rprimIds;

    _RprimPrimIDMap _rprimPrimIdMap;

    _TaskMap _taskMap;

    _SprimIndex     _sprimIndex;
    _BprimIndex     _bprimIndex;

    HdChangeTracker _tracker;
    int32_t _nextPrimId; 

    typedef TfHashMap<SdfPath, HdInstancer*, SdfPath::Hash> _InstancerMap;
    _InstancerMap _instancerMap;


    typedef std::unordered_map<SdfPath, _ExtComputationInfo, SdfPath::Hash>
                                                             _ExtComputationMap;
    _ExtComputationMap _extComputationMap;


    // XXX: TO FIX Move
    typedef std::vector<HdDirtyListSharedPtr> _DirtyListVector;
    _DirtyListVector _syncQueue;

    HdRenderDelegate *_renderDelegate;

    /// Register the render delegate's list of supported prim types.
    void _InitPrimTypes();

    /// Creates fallback prims for each supported prim type.
    bool _CreateFallbackPrims();

    /// Release the fallback prims.
    void _DestroyFallbackPrims();

    typedef tbb::enumerable_thread_specific<HdRenderIndex::HdDrawItemView>
                                                           _ConcurrentDrawItems;

    void _AppendDrawItems(const SdfPathVector &rprimIds,
                          size_t begin,
                          size_t end,
                          HdRprimCollection const& collection,
                          _ConcurrentDrawItems* result);

    /// Register core hydra reprs. Only ever called once, the first time
    /// a render index is created.
    /// XXX: This code should move to the application layer.
    static void _ConfigureReprs();

    // Remove default constructor
    HdRenderIndex() = delete;
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

PXR_NAMESPACE_CLOSE_SCOPE

#endif //HD_RENDER_INDEX_H
