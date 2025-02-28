//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_IMAGING_HD_RENDER_INDEX_H
#define PXR_IMAGING_HD_RENDER_INDEX_H

#include "pxr/pxr.h"
#include "pxr/imaging/hd/api.h"
#include "pxr/imaging/hd/version.h"
#include "pxr/imaging/hd/changeTracker.h"
#include "pxr/imaging/hd/dirtyList.h"
#include "pxr/imaging/hd/perfLog.h"
#include "pxr/imaging/hd/primTypeIndex.h"
#include "pxr/imaging/hd/resourceRegistry.h"
#include "pxr/imaging/hd/sortedIds.h"
#include "pxr/imaging/hd/tokens.h"

#include "pxr/imaging/hd/sceneIndex.h"
#include "pxr/imaging/hd/mergingSceneIndex.h"
#include "pxr/imaging/hd/legacyPrimSceneIndex.h"
#include "pxr/imaging/hd/legacyTaskFactory.h"
#include "pxr/imaging/hd/noticeBatchingSceneIndex.h"

#include "pxr/imaging/hf/perfLog.h"

#include "pxr/usd/sdf/path.h"

#include "pxr/base/gf/vec4i.h"
#include "pxr/base/tf/hashmap.h"

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
class HdDriver;

using HdDriverVector = std::vector<HdDriver*>;
using HdRprimCollectionVector = std::vector<HdRprimCollection>;
using HdTaskSharedPtr = std::shared_ptr<class HdTask>;
using HdResourceRegistrySharedPtr = std::shared_ptr<class HdResourceRegistry>;
using HdTaskSharedPtrVector = std::vector<HdTaskSharedPtr>;
using HdTaskContext = std::unordered_map<TfToken,
                           VtValue,
                           TfToken::HashFunctor>;

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
class HdRenderIndex final 
{
public:
    typedef std::vector<HdDrawItem const*> HdDrawItemPtrVector;

    /// Create a render index with the given render delegate.
    /// Returns null if renderDelegate is null.
    /// The render delegate and render tasks may require access to a renderer's
    /// device provided by the application. The objects can be
    /// passed in as 'drivers'. Hgi is an example of a HdDriver.
    //    hgi = Hgi::CreatePlatformDefaultHgi()
    //    hgiDriver = new HdDriver<Hgi*>(HgiTokens→renderDriver, hgi)
    //    HdRenderIndex::New(_renderDelegate, {_hgiDriver})
    // 
    /// "instanceName" is an optional identifier useful for applications to
    /// associate this render index with related resources (such as the scene
    /// index instances).
    ///
    /// "appName" is an application name that is used to load scene index
    /// plugins that are enabled on a per-app basis, when scene index emulation
    /// is enabled. See HdSceneIndexPluginRegistry for more information.
    ///
    HD_API
    static HdRenderIndex* New(
        HdRenderDelegate *renderDelegate,
        HdDriverVector const& drivers,
        const std::string &instanceName=std::string(),
        const std::string &appName=std::string());

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
    /// Given a prim id, returns the path of the corresponding rprim
    /// or an empty path if none is found.
    HD_API
    SdfPath GetRprimPathFromPrimId(int primId) const;
 
    // ---------------------------------------------------------------------- //
    /// \name Synchronization
    // ---------------------------------------------------------------------- //

    /// Hydra's core currently needs to know the collections used by tasks
    /// to aggregate the reprs that need to be synced for the dirty Rprims.
    /// 
    HD_API
    void EnqueueCollectionToSync(HdRprimCollection const &collection);

    /// Syncs input tasks, B & S prims, (external) computations and updates the
    /// Rprim dirty list to then sync the Rprims.
    /// At the end of this step, all the resources that need to be updated have
    /// handles to their data sources.
    /// This is the first phase in Hydra's execution. See HdEngine::Execute
    HD_API
    void SyncAll(HdTaskSharedPtrVector *tasks, HdTaskContext *taskContext);

    // ---------------------------------------------------------------------- //
    /// \name Execution
    // ---------------------------------------------------------------------- //

    /// Returns a list of relevant draw items that match the criteria specified
    //  by renderTags and collection.
    /// The is typically called during render pass execution, which is the 
    /// final phase in the Hydra's execution. See HdRenderPass::Execute
    HD_API
    HdDrawItemPtrVector GetDrawItems(HdRprimCollection const& collection,
                                     TfTokenVector const& renderTags);

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
                     SdfPath const& rprimId);

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
    TfToken GetRenderTag(SdfPath const& id) const;

    /// Like GetRenderTag, but updates the render tag if dirty.
    TfToken UpdateRenderTag(SdfPath const& id,
                            HdDirtyBits bits);

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
                         SdfPath const &id);

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

    /// Inserts a new task into the render index with an identifier of \p id.
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
    HdSprim *GetSprim(TfToken const& typeId, SdfPath const &id) const;

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
    HdBprim *GetBprim(TfToken const& typeId, SdfPath const &id) const;

    /// Returns the subtree rooted under the given path for the given bprim
    /// type.
    HD_API
    SdfPathVector GetBprimSubtree(TfToken const& typeId,
                                  SdfPath const& root);

    /// Returns the fallback prim for the Bprim of the given type.
    HD_API
    HdBprim *GetFallbackBprim(TfToken const& typeId) const;

    // ---------------------------------------------------------------------- //
    /// \name Scene indices
    // ---------------------------------------------------------------------- //
    HD_API
    void InsertSceneIndex(
            const HdSceneIndexBaseRefPtr &inputScene,
            SdfPath const& scenePathPrefix,
            bool needsPrefixing = true);

    HD_API
    void RemoveSceneIndex(
            const HdSceneIndexBaseRefPtr &inputScene);

    /// The terminal scene index that is driving what is in the render index
    /// through emulation.
    HD_API
    HdSceneIndexBaseRefPtr GetTerminalSceneIndex() const;

    // ---------------------------------------------------------------------- //
    /// \name Render Delegate
    // ---------------------------------------------------------------------- //
    /// Currently, a render index only supports connection to one type of
    /// render delegate, due to the inserted information and change tracking
    /// being specific to that delegate type.
    HD_API
    HdRenderDelegate *GetRenderDelegate() const;

    // The render delegate may require access to a render context / device 
    // that is provided by the application.
    HD_API
    HdDriverVector const& GetDrivers() const;

    /// Returns a shared ptr to the resource registry of the current render
    /// delegate.
    HD_API
    HdResourceRegistrySharedPtr GetResourceRegistry() const;

    /// Returns true if scene index features are available
    /// This is true by default but can be controlled via an
    /// HD_ENABLE_SCENE_INDEX_EMULATION environment variable.
    HD_API
    static bool IsSceneIndexEmulationEnabled();

    /// An application or legacy scene delegate may prefer for the scene 
    /// index observer notices generated from its prim insertions, removals, or
    /// invalidations to be consolidated into vectorized batches. Calling this
    /// will cause subsequent notices to be queued.
    /// 
    /// NOTE: This tracks depth internally and is safe to call in nested 
    ///       contexts. It is not safe to call from multiple threads, though.
    HD_API
    void SceneIndexEmulationNoticeBatchBegin();

    /// Flushes any queued scene index observer notices and disables further
    /// queueing.
    ///
    /// NOTE: This tracks depth internally and is safe to call in nested 
    ///       contexts. It is not safe to call from multiple threads, though.
    HD_API
    void SceneIndexEmulationNoticeBatchEnd();

    /// Consolidate notices generated by the merging scene index to vectorized
    /// batches. Calling this will cause subsequent notices to be queued.
    /// 
    /// NOTE: This tracks depth internally and is safe to call in nested 
    ///       contexts. It is not safe to call from multiple threads, though.
    HD_API
    void MergingSceneIndexNoticeBatchBegin();

    /// Flushes any queued scene index observer notices from the merging scene
    /// index and disables further queueing.
    ///
    /// NOTE: This tracks depth internally and is safe to call in nested 
    ///       contexts. It is not safe to call from multiple threads, though.
    HD_API
    void MergingSceneIndexNoticeBatchEnd();

    HD_API
    std::string GetInstanceName() const;

private:
    // The render index constructor is private so we can check
    // renderDelegate before construction: see HdRenderIndex::New(...).
    HdRenderIndex(
        HdRenderDelegate *renderDelegate, 
        HdDriverVector const& drivers,
        const std::string &instanceName,
        const std::string &appName);

    // ---------------------------------------------------------------------- //
    // Private Helper methods 
    // ---------------------------------------------------------------------- //

    // Go through all RPrims and reallocate their instance ids
    // This is called once we have exhausted all 24bit instance ids.
    void _CompactPrimIds();

    // Allocate the next available instance id to the prim
    void _AllocatePrimId(HdRprim* prim);

    using HdTaskCreateFnc =
            std::function<HdTaskSharedPtr(HdSceneDelegate*, SdfPath const&)>;

    // Implements InsertTask<T>.
    // Inserts task going through emulation if enabled.
    HD_API
    void _InsertSceneDelegateTask(
        HdSceneDelegate* delegate, 
        SdfPath const& taskId,
        HdLegacyTaskFactorySharedPtr factory);

    template <typename T>
    static inline const TfToken & _GetTypeId();


    // Private versions of equivalent public methods which insert and remove
    // from this render index.
    // 
    // The public versions check to see if scene delegate emulation is active.
    // If not, they call through to these. Otherwise, they forward to the
    // the HdLegacyPrimSceneIndex member. If a legacy render delegate is also
    // in use, the scene index chain will terminate with a
    // HdSceneIndexAdapterSceneDelegate. That will call the private versions
    // directly so that the internal render index tables are updated.
    // 
    // This prevents cyclic insertion/removals while allowing a single 
    // HdRenderIndex to be used for both front and back-end emulation.
    //
    // Note: all render index users should call the public APIs; only
    // sceneIndexAdapterSceneDelegate.cpp should call these versions, to keep
    // state synchronized.  Note, for example, that _RemoveSubtree and _Clear
    // don't affect the task map, since tasks aren't part of emulation, whereas
    // RemoveSubtree and Clear do affect the task map...
    friend class HdSceneIndexAdapterSceneDelegate;
    void _InsertRprim(TfToken const& typeId,
                      HdSceneDelegate* sceneDelegate,
                      SdfPath const& rprimId);
    void _InsertSprim(TfToken const& typeId,
                      HdSceneDelegate* delegate,
                      SdfPath const& sprimId);
    void _InsertBprim(TfToken const& typeId,
                      HdSceneDelegate* delegate,
                      SdfPath const& bprimId);
    void _InsertInstancer(HdSceneDelegate* delegate,
                          SdfPath const &id);
    void _InsertTask(HdSceneDelegate* delegate,
                     SdfPath const &id,
                     HdTaskSharedPtr const &task);

    void _RemoveRprim(SdfPath const& id);
    void _RemoveSprim(TfToken const& typeId, SdfPath const& id);
    void _RemoveBprim(TfToken const& typeId, SdfPath const& id);
    void _RemoveInstancer(SdfPath const& id);
    void _RemoveSubtree(SdfPath const& id, HdSceneDelegate* sceneDelegate);
    void _RemoveRprimSubtree(const SdfPath &root,
                             HdSceneDelegate* sceneDelegate);
    void _RemoveInstancerSubtree(const SdfPath &root,
                                 HdSceneDelegate* sceneDelegate);
    void _RemoveExtComputationSubtree(const SdfPath &root,
                                      HdSceneDelegate* sceneDelegate);
    void _RemoveTaskSubtree(const SdfPath &root,
                            HdSceneDelegate* sceneDelegate);
    void _RemoveTask(SdfPath const &id);
    void _Clear();

    // ---------------------------------------------------------------------- //
    // Index State
    // ---------------------------------------------------------------------- //
    struct _RprimInfo {
        HdSceneDelegate *sceneDelegate;
        HdRprim *rprim;
    };

    class _NoticeBatchingContext;

    HdLegacyPrimSceneIndexRefPtr _emulationSceneIndex;
    std::unique_ptr<_NoticeBatchingContext> _emulationBatchingCtx;

    std::unique_ptr<class HdSceneIndexAdapterSceneDelegate> _siSd;

    HdMergingSceneIndexRefPtr _mergingSceneIndex;
    std::unique_ptr<_NoticeBatchingContext> _mergingBatchingCtx;

    HdSceneIndexBaseRefPtr _terminalSceneIndex;

    struct _TaskInfo {
        HdSceneDelegate *sceneDelegate;
        HdTaskSharedPtr task;
    };

    typedef std::unordered_map<SdfPath, _TaskInfo, SdfPath::Hash> _TaskMap;
    typedef TfHashMap<SdfPath, _RprimInfo, SdfPath::Hash> _RprimMap;
    typedef std::vector<SdfPath> _RprimPrimIDVector;

    typedef Hd_PrimTypeIndex<HdSprim> _SprimIndex;
    typedef Hd_PrimTypeIndex<HdBprim> _BprimIndex;

    _RprimMap _rprimMap;
    Hd_SortedIds _rprimIds;

    _RprimPrimIDVector _rprimPrimIdMap;

    _TaskMap _taskMap;

    _SprimIndex _sprimIndex;
    _BprimIndex _bprimIndex;

    HdChangeTracker _tracker;

    typedef TfHashMap<SdfPath, HdInstancer*, SdfPath::Hash> _InstancerMap;
    _InstancerMap _instancerMap;

    HdRenderDelegate *_renderDelegate;
    HdDriverVector _drivers;


    std::string _instanceName;

    // ---------------------------------------------------------------------- //
    // Sync State
    // ---------------------------------------------------------------------- //
    HdRprimCollectionVector _collectionsToSync;
    HdDirtyList _rprimDirtyList;

    // ---------------------------------------------------------------------- //

    /// Register the render delegate's list of supported prim types.
    void _InitPrimTypes();

    /// Creates fallback prims for each supported prim type.
    bool _CreateFallbackPrims();

    /// Release the fallback prims.
    void _DestroyFallbackPrims();

    typedef tbb::enumerable_thread_specific<HdDrawItemPtrVector>
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

    // Don't allow copies
    HdRenderIndex(const HdRenderIndex &) = delete;
    HdRenderIndex &operator=(const HdRenderIndex &) = delete; 

};

template <typename T>
void
HdRenderIndex::InsertTask(HdSceneDelegate* delegate, SdfPath const& id)
{
    _InsertSceneDelegateTask(
        delegate, id, HdMakeLegacyTaskFactory<T>());
}

PXR_NAMESPACE_CLOSE_SCOPE

#endif //PXR_IMAGING_HD_RENDER_INDEX_H
