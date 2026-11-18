//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_USD_USD_CLIP_CACHE_H
#define PXR_USD_USD_CLIP_CACHE_H

#include "pxr/pxr.h"

#include "pxr/usd/usd/clip.h"
#include "pxr/usd/usd/clipSet.h"
#include "pxr/usd/pcp/layerStack.h"
#include "pxr/usd/sdf/changeList.h"
#include "pxr/usd/sdf/pathTable.h"
#include "pxr/base/tf/hash.h"
#include "pxr/base/tf/notice.h"

#include <functional>
#include <mutex>
#include <unordered_map>
#include <vector>

PXR_NAMESPACE_OPEN_SCOPE

class PcpPrimIndex;

/// \class Usd_ClipCache
///
/// Private helper object for computing and caching clip information for 
/// a prim on a UsdStage.
///
class Usd_ClipCache
{
    Usd_ClipCache(Usd_ClipCache const &) = delete;
    Usd_ClipCache &operator=(Usd_ClipCache const &) = delete;
public:
    /// Function called to start listening for changes to a clip layer.  The
    /// returned key is revoked when no clip set in this cache uses the layer
    /// any longer.
    using RegisterLayerFn =
        std::function<TfNotice::Key (const SdfLayerHandle &)>;

    /// Construct a cache that calls \p registerLayerFn the first time any of
    /// its clips opens a given layer. If \p registerLayerFn is empty, the cache
    /// does not track clip layers.
    explicit Usd_ClipCache(RegisterLayerFn registerLayerFn = {});
    
    ~Usd_ClipCache();

    /// \struct ConcurrentPopulationContext
    ///
    /// Structure for enabling cache population via concurrent calls to
    /// PopulateClipsForPrim.  Protects member data reads/writes with a mutex
    /// during its lifetime.
    /// \sa PopulateClipsForPrim.
    struct ConcurrentPopulationContext
    {
        explicit ConcurrentPopulationContext(Usd_ClipCache &cache);
        ~ConcurrentPopulationContext();
        Usd_ClipCache &_cache;
        std::mutex _mutex;
    };

    /// Populate the cache with clips for \p prim. Returns true if clips
    /// that may contribute opinions to attributes on \p prim are found,
    /// false otherwise.
    ///
    /// This function assumes that clips for ancestors of \p prim have 
    /// already been populated.
    bool PopulateClipsForPrim(
        const SdfPath& path, const PcpPrimIndex& primIndex);

    /// Get all the layers that have been opened because we needed to extract
    /// data from their corresponding clips.  USD tries to be as lazy as 
    /// possible about opening clip layers to avoid unnecessary latency and
    /// memory bloat; however, once a layer is open, it will generally be
    /// kept open for the life of the stage.
    SdfLayerHandleSet GetUsedLayers() const;

    /// Reload all clip layers that have been opened. Generated clip manifests
    /// will also be regenerated.
    void Reload();

    /// A site whose composed attribute values may have been changed by edits to
    /// a value clip or manifest layer. \c path is in the namespace of \c
    /// layerStack, the source layer stack of the clip set that uses the edited
    /// layer. If \c path is a property path, the values of that attribute may
    /// have changed. Otherwise, the values of any attributes at and below the
    /// prim at \c path may have changed in ways the change list does not
    /// itemize.
    struct ChangedSite {
        PcpLayerStackPtr layerStack;
        SdfPath path;
    };
    using ChangedSites = std::vector<ChangedSite>;

    /// Add to \p sites the sites whose values may have been changed by \p
    /// changeList, the changes made to \p layer. Nothing is added unless \p
    /// layer is a value clip or manifest layer opened by a clip set in this
    /// cache.
    void ComputeChangedSites(
        const SdfLayerHandle &layer,
        const SdfChangeList &changeList,
        ChangedSites *sites) const;

    /// Return a change list entry describing a change to the values an
    /// attribute takes from value clips. Clips only provide time samples and
    /// splines, so the entry indicates that time samples changed and has an
    /// info change for the spline field, with empty old and new values. Both
    /// fields are reported regardless of which kind of value the attribute
    /// takes from clips, so consumers of the changed fields see a value change
    /// either way.
    static const SdfChangeList::Entry &GetClipValuesChangedEntry();

    /// Get all clips that may contribute opinions to attributes on the
    /// prim at \p path, including clips that were authored on ancestral prims.
    ///
    /// The returned vector contains all clips that affect the prim at \p path
    /// in strength order. Each individual list of value clips will be ordered
    /// by start time.
    const std::vector<Usd_ClipSetRefPtr>& 
    GetClipsForPrim(const SdfPath& path) const;

    /// \struct Lifeboat
    ///
    /// Structure for keeping invalidated clip data alive.
    /// \sa InvalidateClipsForPrim.
    ///
    struct Lifeboat
    {
        explicit Lifeboat(Usd_ClipCache& cache);
        ~Lifeboat();
        Usd_ClipCache& _cache;

        class Data;
        std::unique_ptr<Data> _data;
    };

    /// Invalidates the clip cache for prims at and below \p path. 
    /// A Lifeboat object must be active for this cache before calling
    /// this function. This potentially allows the underlying clip layers
    /// to be reused if the clip cache is repopulated while the lifeboat
    /// is still active.
    ///
    /// NOTE: This function must not be invoked concurrently with any other
    /// function on this object.
    void InvalidateClipsForPrim(const SdfPath& path);

private:
    friend struct Usd_Clip;

    // Start listening for changes to layer if not already doing so. Called when
    // a clip in this cache opens layer, which may happen concurrently on many
    // threads.
    void _AddClipLayer(const SdfLayerHandle &layer);

    // Associate the clips in a newly created clip set with this cache and
    // record the layers they have already opened.
    void _AdoptClipSet(const Usd_ClipSetRefPtr &clipSet);

    // Stop listening for changes to layers that no clip set in _table has open.
    void _RemoveUnusedClipLayers();

    // Add to sites the sites in clipSet whose values may have been changed by
    // changeList, the changes to a layer clipSet uses as its manifest if
    // isManifest is true, and as one of its value clips if isValueClip is true.
    // manifest is clipSet's manifest layer, which must be open.
    static void _AddChangedSites(
        const Usd_ClipSet &clipSet, const SdfLayerHandle &manifest,
        bool isManifest, bool isValueClip,
        const SdfChangeList &changeList, ChangedSites *sites);

    void _ComputeClipsFromPrimIndex(
        const SdfPath& usdPrimPath,
        const PcpPrimIndex& primIndex,
        std::vector<Usd_ClipSetRefPtr>* clips);

    inline const std::vector<Usd_ClipSetRefPtr>& 
    _GetClipsForPrim_NoLock(const SdfPath& path) const;

    // Map from prim path to all clips that apply to that prim, including
    // ancestral clips. This map is sparse; only prims where clips are
    // authored will have entries.
    typedef SdfPathTable<std::vector<Usd_ClipSetRefPtr> >_ClipTable;
    _ClipTable _table;

    ConcurrentPopulationContext *_concurrentPopulationContext;
    Lifeboat *_lifeboat;

    // Map from each clip or manifest layer opened by a clip set in this cache
    // to the key for listening to changes to it. Entries for layers that clip
    // sets in _table no longer have open are removed when a Lifeboat is
    // destroyed.
    RegisterLayerFn _registerLayerFn;
    std::unordered_map<SdfLayerHandle, TfNotice::Key, TfHash> _clipLayers;
    mutable std::mutex _clipLayersMutex;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_USD_USD_CLIP_CACHE_H
