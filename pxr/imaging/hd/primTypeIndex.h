//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_IMAGING_HD_PRIM_TYPE_INDEX_H
#define PXR_IMAGING_HD_PRIM_TYPE_INDEX_H

#include "pxr/pxr.h"
#include "pxr/imaging/hd/types.h"
#include "pxr/imaging/hd/sortedIds.h"
#include "pxr/base/tf/token.h"
#include "pxr/usd/sdf/path.h"

#include <set>
#include <vector>
#include <unordered_map>

PXR_NAMESPACE_OPEN_SCOPE

class HdChangeTracker;
class HdRenderDelegate;
class HdRenderParam;
class HdSceneDelegate;
class SdfPath;
using HdSceneDelegatePtrVector = std::vector<HdSceneDelegate*>;

/// This class is only used by the render index.
/// It provides functionality to manage and store one class of prim
/// such as a Sprim or Bprim.
template <class PrimType>
class Hd_PrimTypeIndex {
public:
    Hd_PrimTypeIndex();
    ~Hd_PrimTypeIndex();

    ///
    /// Initialize this prim index, specifying the typeId tokens
    /// that should be supported by this index.
    ///
    void InitPrimTypes(const TfTokenVector &primTypes);

    ///
    /// Removes and frees all prims in this index.
    /// The render delegate is responsible for freeing the actual memory
    /// allocated to the prim.
    /// The prim is also removed from the change tracker.
    ///
    void Clear(HdChangeTracker &tracker, HdRenderDelegate *renderDelegate);

    ///
    /// Add a new a prim to the render index identified by the globally unique
    /// identifier, primId.
    /// typeId is the type of the prim to create, which is allocated using
    /// the provided render delegate.  The Scene delegate provided is
    /// associated with the prim and is the one used to pull the data for the
    /// prim during sync processing.
    /// As well as being inserted into this index, the prim is added to the
    /// change tracker, with the initial dirty state provided by the prim itself.
    ///
    void InsertPrim(const TfToken &typeId,
                    HdSceneDelegate *sceneDelegate,
                    const SdfPath &primId,
                    HdChangeTracker &tracker,
                    HdRenderDelegate *renderDelegate);

    ///
    /// Removes the prim identifier by primId.  TypeId is the type of that
    /// prim.  Memory for the prim is deallocated using the render delegate.
    /// The prim is also removed from the change tracker.
    ///
    void RemovePrim(const TfToken &typeId,
                    const SdfPath &primId,
                    HdChangeTracker &tracker,
                    HdRenderDelegate *renderDelegate);

    ///
    /// Removes the subtree of prims identifier by root that are owned
    /// by the given scene delegate.
    /// This function affects all prim types.
    /// Memory for the prim is deallocated using the render delegate.
    /// The prim is also removed from the change tracker.
    ///
    void RemoveSubtree(const SdfPath &root,
                       HdSceneDelegate* sceneDelegate,
                       HdChangeTracker &tracker,
                       HdRenderDelegate *renderDelegate);

    /// Obtains a modifiable pointer the prim with the given type and id.
    /// If no prim with the given id is in the index or the type id is
    /// incorrect, then nullptr is returned.
    PrimType *GetPrim(const TfToken &typeId,
                      const SdfPath &primId) const;

    ///
    /// Obtain a prim, that implements the schema given by type id, that
    /// can be used as a substitute for any prim of that type in the event of
    /// an error.
    ///
    /// Hydra guarantees that the prim is not null for any type that
    /// is supported by the back-end.
    ///
    PrimType *GetFallbackPrim(TfToken const &typeId) const;

    ///
    /// Returns a list of Prim Ids in outPaths of prims that type match
    /// typeId who are namespace children of rootPath.
    /// rootPath does not need to match any prim in the index or
    /// it may point to a prim of a different type.
    ///
    void GetPrimSubtree(const TfToken &typeId,
                        const SdfPath &rootPath,
                        SdfPathVector *outPaths);

    ///
    /// Uses the provided render delegate to create the fallback prims
    /// for use by the index.  The prim types created are based on those
    /// specified by InitPrimTypes.
    ///
    /// If the render delegate fails to create a prim, this function returns
    /// false and the index is remain uninitialized and shouldn't be used.
    ///
    bool CreateFallbackPrims(HdRenderDelegate *renderDelegate);

    ///
    /// Clean-up function for the index.  Uses the delegate to deallocate
    /// the memory used by the fallback prims.  The index is returned to
    /// an uninitialized state and shouldn't be used, unless reinitialized.
    ///
    void DestroyFallbackPrims(HdRenderDelegate *renderDelegate);

    ///
    /// Main Sync Processing function.
    ///
    /// Will call the Sync function on all prims in the index that
    /// are marked dirty in the specified change tracker.
    /// Also updates an internal list of scene delegates for the dirty prims.
    ///
    void SyncPrims(HdChangeTracker &tracker,
                   HdRenderParam *renderParam,
                   HdRenderDelegate *renderDelegate);
    
    /// Returns a vector of unique scene delegates corresponding to the dirty
    /// prims that were sync'd in SyncPrims.
   const HdSceneDelegatePtrVector& GetSceneDelegatesForDirtyPrims();

private:
    struct _PrimInfo {
        HdSceneDelegate *sceneDelegate;
        PrimType        *prim;
    };

    typedef std::unordered_map<SdfPath, _PrimInfo, SdfPath::Hash> _PrimMap;

    struct _PrimTypeEntry
   {
        _PrimMap         primMap;
        Hd_SortedIds     primIds;   // Primarily for sub-tree searching
        PrimType        *fallbackPrim;

        _PrimTypeEntry()
         : primMap()
         , primIds()
         , fallbackPrim(nullptr)
        {
        }
    };

    typedef std::unordered_map<TfToken, size_t, TfToken::HashFunctor> _TypeIndex;

    typedef std::vector<_PrimTypeEntry> _PrimTypeList;

    _PrimTypeList _entries;
    _TypeIndex    _index;
    HdSceneDelegatePtrVector _dirtyPrimDelegates;
    TfTokenVector _primTypeNames;


    // Template methods that are expected to be specialized on PrimType.
    // These are to handle prim type specific function names on called objects.
    static void _TrackerInsertPrim(HdChangeTracker &tracker,
                                   const SdfPath &path,
                                   HdDirtyBits initialDirtyState);

    static void _TrackerRemovePrim(HdChangeTracker &tracker,
                                   const SdfPath &path);

    static HdDirtyBits _TrackerGetPrimDirtyBits(HdChangeTracker &tracker,
                                                const SdfPath &path);

    static void _TrackerMarkPrimClean(HdChangeTracker &tracker,
                                      const SdfPath &path,
                                      HdDirtyBits dirtyBits);

    static PrimType *_RenderDelegateCreatePrim(HdRenderDelegate *renderDelegate,
                                               const TfToken &typeId,
                                               const SdfPath &primId);
    static PrimType *_RenderDelegateCreateFallbackPrim(
            HdRenderDelegate *renderDelegate,
            const TfToken &typeId);

    static void _RenderDelegateDestroyPrim(HdRenderDelegate *renderDelegate,
                                           PrimType *prim);

    // No copying
    Hd_PrimTypeIndex(const Hd_PrimTypeIndex &) = delete;
    Hd_PrimTypeIndex &operator =(const Hd_PrimTypeIndex &) = delete;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_IMAGING_HD_PRIM_TYPE_INDEX_H
