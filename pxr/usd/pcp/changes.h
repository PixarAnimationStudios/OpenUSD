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
#ifndef PXR_USD_PCP_CHANGES_H
#define PXR_USD_PCP_CHANGES_H

/// \file pcp/changes.h

#include "pxr/pxr.h"
#include "pxr/usd/pcp/api.h"
#include "pxr/usd/sdf/changeList.h"
#include "pxr/usd/sdf/declareHandles.h"
#include "pxr/usd/sdf/path.h"
#include "pxr/usd/sdf/types.h"
#include "pxr/base/tf/declarePtrs.h"
#include "pxr/base/tf/span.h"

#include <map>
#include <set>

PXR_NAMESPACE_OPEN_SCOPE

SDF_DECLARE_HANDLES(SdfLayer);
TF_DECLARE_WEAK_AND_REF_PTRS(PcpLayerStack);

class PcpCache;
class PcpSite;

/// \class PcpLayerStackChanges
///
/// Types of changes per layer stack.
///
class PcpLayerStackChanges {
public:
    /// Must rebuild the layer tree.  Implies didChangeLayerOffsets.
    bool didChangeLayers;

    /// Must rebuild the layer offsets.
    bool didChangeLayerOffsets;

    /// Must rebuild the relocation tables.
    bool didChangeRelocates;

    /// A significant layer stack change means the composed opinions of
    /// the layer stack may have changed in arbitrary ways.  This
    /// represents a coarse invalidation. By way of contrast, an example
    /// of an insignificant change is adding or removing a layer empty
    /// of opinions.
    bool didChangeSignificantly;

    /// New relocation maps for this layer stack.
    /// If didChangeRelocates is true, these fields will be populated
    /// as part of determining the changes to this layer stack.
    /// However, we do not immediately apply those changes to the
    /// layer stack; we store them here and commit them in Apply().
    SdfRelocatesMap newRelocatesTargetToSource;
    SdfRelocatesMap newRelocatesSourceToTarget;
    SdfRelocatesMap newIncrementalRelocatesSourceToTarget;
    SdfRelocatesMap newIncrementalRelocatesTargetToSource;
    SdfPathVector newRelocatesPrimPaths;

    /// Paths that are affected by the above relocation changes.
    SdfPathSet pathsAffectedByRelocationChanges;

    PcpLayerStackChanges() :
        didChangeLayers(false),
        didChangeLayerOffsets(false),
        didChangeRelocates(false),
        didChangeSignificantly(false)
    {}
};

/// \class PcpCacheChanges
///
/// Types of changes per cache.
///
class PcpCacheChanges {
public:
    enum TargetType {
        TargetTypeConnection         = 1 << 0,
        TargetTypeRelationshipTarget = 1 << 1
    };

    /// Must rebuild the indexes at and below each path.  This
    /// implies rebuilding the prim/property stacks at
    /// and below each path.
    SdfPathSet didChangeSignificantly;

    /// Must rebuild the prim/property stacks at each path.
    SdfPathSet didChangeSpecs;

    /// Must rebuild the prim indexes at each path.  This implies rebuilding
    /// the prim stack at each path.
    SdfPathSet didChangePrims;

    /// Must rebuild the connections/targets at each path.
    std::map<SdfPath, int, SdfPath::FastLessThan> didChangeTargets;

    /// Must update the path on every namespace object at and below each
    /// given path. The first path is the old path to the object and the
    /// second path is the new path. The order of the vector matters and 
    /// indicates the order in which the namespace edits occur.
    std::vector<std::pair<SdfPath, SdfPath>> didChangePath;

    /// Layers used in the composition may have changed.
    bool didMaybeChangeLayers = false;

private:
    friend class PcpCache;
    friend class PcpChanges;

    // Must rebuild the prim/property stacks at each path due to a change
    // that only affects the internal representation of the stack and
    // not its contents.  Because this causes no externally-observable
    // changes in state, clients do not need to be aware of these changes.
    SdfPathSet _didChangeSpecsInternal;
};

/// Structure used to temporarily retain layers and layerStacks within
/// a code block.  Analogous to the autorelease pool in obj-c.
class PcpLifeboat {
public:
    PcpLifeboat();
    ~PcpLifeboat();

    /// Ensure that \p layer exists until this object is destroyed.
    void Retain(const SdfLayerRefPtr& layer);

    /// Ensure that \p layerStack exists until this object is destroyed.
    void Retain(const PcpLayerStackRefPtr& layerStack);

    /// Returns reference to the set of layer stacks currently being held
    /// in the lifeboat.
    const std::set<PcpLayerStackRefPtr>& GetLayerStacks() const;

    /// Swap the contents of this and \p other.
    void Swap(PcpLifeboat& other);

private:
    std::set<SdfLayerRefPtr> _layers;
    std::set<PcpLayerStackRefPtr> _layerStacks;
};

/// \class PcpChanges
///
/// Describes Pcp changes.
///
/// Collects changes to Pcp necessary to reflect changes in Sd.  It does
/// not cause any changes to any Pcp caches, layer stacks, etc;  it only
/// computes what changes would be necessary to Pcp to reflect the Sd
/// changes.
///
class PcpChanges {
public:
    PCP_API PcpChanges();
    PCP_API ~PcpChanges();

    /// Breaks down \p changes into individual changes on the caches in
    /// \p caches.  This simply translates data in \p changes into other
    /// Did...() calls on this object.
    ///
    /// Clients will typically call this method once then call \c Apply() or
    /// get the changes using \c GetLayerStackChanges() and
    /// \c GetCacheChanges().
    PCP_API 
    void DidChange(const TfSpan<const PcpCache*> &caches,
                   const SdfLayerChangeListVec& changes);

    /// Tries to load the sublayer of \p layer at \p sublayerPath.  If
    /// successful, any layer stack using \p layer is marked as having changed
    /// and all prims in \p cache using any prim in any of those layer stacks
    /// are marked as changed.
    PCP_API 
    void DidMaybeFixSublayer(const PcpCache* cache,
                             const SdfLayerHandle& layer,
                             const std::string& assetPath);

    /// Tries to load the asset at \p assetPath.  If successful, any prim
    /// in \p cache using the site \p site is marked as changed.
    PCP_API 
    void DidMaybeFixAsset(const PcpCache* cache,
                          const PcpSite& site,
                          const SdfLayerHandle& srcLayer,
                          const std::string& assetPath);

    /// The layer identified by \p layerId was muted in \p cache.
    PCP_API 
    void DidMuteLayer(const PcpCache* cache, const std::string& layerId);

    /// The layer identified by \p layerId was unmuted in \p cache.
    PCP_API 
    void DidUnmuteLayer(const PcpCache* cache, const std::string& layerId);

    /// The sublayer tree changed.  This often, but doesn't always, imply that
    /// anything and everything may have changed.  If clients want to indicate
    /// that anything and everything may have changed they should call this
    /// method and \c DidChangePrimGraph() with the absolute root path.
    PCP_API
    void DidChangeLayers(const PcpCache* cache);

    /// The sublayer offsets changed.
    PCP_API
    void DidChangeLayerOffsets(const PcpCache* cache);

    /// The object at \p path changed significantly enough to require
    /// recomputing the entire prim or property index.  A significant change
    /// implies changes to every namespace descendant's index, specs, and
    /// dependencies.
    PCP_API 
    void DidChangeSignificantly(const PcpCache* cache, const SdfPath& path);

    /// The spec stack for the prim or property has changed, due to the
    /// addition or removal of the spec in \p changedLayer at \p changedPath.
    /// This is used when inert prims/properties are added or removed or when 
    /// any change requires rebuilding the property stack.  It implies that 
    /// dependencies on those specs has changed.
    PCP_API 
    void DidChangeSpecs(const PcpCache* cache, const SdfPath& path,
                        const SdfLayerHandle& changedLayer,
                        const SdfPath& changedPath);

    /// The spec stack for the prim or property at \p path in \p cache has
    /// changed.
    PCP_API 
    void DidChangeSpecStack(const PcpCache* cache, const SdfPath& path);

    /// The connections on the attribute or targets on the relationship have
    /// changed.
    PCP_API 
    void DidChangeTargets(const PcpCache* cache, const SdfPath& path,
                          PcpCacheChanges::TargetType targetType);

    /// The relocates that affect prims and properties at and below
    /// the given cache path have changed.
    PCP_API 
    void DidChangeRelocates(const PcpCache* cache, const SdfPath& path);

    /// The composed object at \p oldPath was moved to \p newPath.  This
    /// implies every corresponding Sd change.  This object will subsume
    /// those Sd changes under this higher-level move.  Sd path changes
    /// that are not so subsumed will be converted to DidChangePrimGraph()
    /// and/or DidChangeSpecs() changes.
    PCP_API 
    void DidChangePaths(const PcpCache* cache,
                        const SdfPath& oldPath, const SdfPath& newPath);

    /// Remove any changes for \p cache.
    PCP_API
    void DidDestroyCache(const PcpCache* cache);

    /// The asset resolver has changed, invalidating previously-resolved
    /// asset paths. This function will check all prim indexes in \p cache
    /// for composition arcs that may now refer to a different asset and
    /// mark them as needing significant resyncs.
    PCP_API
    void DidChangeAssetResolver(const PcpCache* cache);

    /// Swap the contents of this and \p other.
    PCP_API
    void Swap(PcpChanges& other);

    /// Returns \c true iff there are no changes.
    PCP_API
    bool IsEmpty() const;

    typedef std::map<PcpLayerStackPtr, PcpLayerStackChanges> LayerStackChanges;
    typedef std::map<PcpCache*, PcpCacheChanges> CacheChanges;

    /// Returns a map of all of the layer stack changes.  Note that some
    /// keys may be to expired layer stacks.
    PCP_API
    const LayerStackChanges& GetLayerStackChanges() const;

    /// Returns a map of all of the cache changes.
    PCP_API
    const CacheChanges& GetCacheChanges() const;

    /// Returns the lifeboat responsible for maintaining the lifetime of
    /// layers and layer stacks during change processing. Consumers may
    /// inspect this object to determine which of these objects, if any,
    /// had their lifetimes affected during change processing.
    PCP_API
    const PcpLifeboat& GetLifeboat() const;

    /// Applies the changes to the layer stacks and caches.
    PCP_API
    void Apply() const;

private:
    // Internal data types for namespace edits from Sd.
    typedef std::map<SdfPath, SdfPath> _PathEditMap;
    typedef std::map<PcpCache*, _PathEditMap> _RenameChanges;

    // Returns the PcpLayerStackChanges for the given cache's layer stack.
    PcpLayerStackChanges& _GetLayerStackChanges(const PcpCache* cache);

    // Returns the PcpLayerStackChanges for the given layer stack.
    PcpLayerStackChanges& _GetLayerStackChanges(const PcpLayerStackPtr&);

    // Returns the PcpCacheChanges for the given cache.
    PcpCacheChanges& _GetCacheChanges(const PcpCache* cache);

    // Returns the _PathEditMap for the given cache.
    _PathEditMap& _GetRenameChanges(const PcpCache* cache);


    // Optimize the changes.
    void _Optimize() const;

    // Optimize the changes.
    void _Optimize();

    // Optimize the changes for a given cache.
    void _Optimize(PcpCacheChanges*);

    // Optimize path changes.
    void _OptimizePathChanges(const PcpCache* cache, PcpCacheChanges* changes,
                              const _PathEditMap* pathChanges);

    // Sublayer change type for _DidChangeSublayer.
    enum _SublayerChangeType {
        _SublayerAdded,
        _SublayerRemoved
    };

    // Helper function for loading a sublayer of \p layer at \p sublayerPath
    // for processing changes described by \p sublayerChange.
    SdfLayerRefPtr _LoadSublayerForChange(const PcpCache* cache,
                                          const SdfLayerHandle& layer,
                                          const std::string& sublayerPath,
                                          _SublayerChangeType changeType) const;

    // Helper function for loading a sublayer at \p sublayerPath
    // for processing changes described by \p sublayerChange.
    SdfLayerRefPtr _LoadSublayerForChange(const PcpCache* cache,
                                          const std::string& sublayerPath,
                                          _SublayerChangeType changeType) const;

    // Propagates changes to \p sublayer specified by \p sublayerChange to 
    // the dependents of that sublayer.  This includes all layer stacks
    // that include the sublayer.
    void _DidChangeSublayerAndLayerStacks(const PcpCache* cache,
                                          const PcpLayerStackPtrVector& stacks,
                                          const std::string& sublayerPath,
                                          const SdfLayerHandle& sublayer,
                                          _SublayerChangeType sublayerChange,
                                          std::string* debugSummary);

    // Propagates changes to \p sublayer specified by \p sublayerChange to 
    // the dependents of that sublayer.
    void _DidChangeSublayer(const PcpCache* cache,
                            const PcpLayerStackPtrVector& layerStacks,
                            const std::string& sublayerPath,
                            const SdfLayerHandle& sublayer,
                            _SublayerChangeType sublayerChange,
                            std::string* debugSummary,
                            bool *significant);

    // Mark the layer stack as having changed.
    void _DidChangeLayerStack(
        const TfSpan<const PcpCache*>& caches,
        const PcpLayerStackPtr& layerStack,
        bool requiresLayerStackChange,
        bool requiresLayerStackOffsetsChange,
        bool requiresSignificantChange);

    // Mark the layer stack's relocations as having changed.
    // Recompute the new relocations, storing the result in the Changes,
    // so that change-processing can determine which other caches it
    // needs to invalidate.
    void _DidChangeLayerStackRelocations(
        const TfSpan<const PcpCache*>& caches,
        const PcpLayerStackPtr & layerStack,
        std::string* debugSummary);

    // Register changes to any prim indexes in \p caches that are affected
    // by a change to a layer's resolved path used by \p layerStack.
    void _DidChangeLayerStackResolvedPath(
        const TfSpan<const PcpCache*>& caches,
        const PcpLayerStackPtr& layerStack,
        bool requiresLayerStackChange,
        std::string* debugSummary);

    // The spec stack for the prim or property index at \p path must be
    // recomputed due to a change that affects only the internal representation
    // of the stack and not its contents.
    void _DidChangeSpecStackInternal(
        const PcpCache* cache, const SdfPath& path);

private:
    LayerStackChanges _layerStackChanges;
    CacheChanges _cacheChanges;
    _RenameChanges _renameChanges;
    mutable PcpLifeboat _lifeboat;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_USD_PCP_CHANGES_H
