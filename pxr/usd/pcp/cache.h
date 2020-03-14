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
#ifndef PXR_USD_PCP_CACHE_H
#define PXR_USD_PCP_CACHE_H

#include "pxr/pxr.h"
#include "pxr/usd/pcp/api.h"
#include "pxr/usd/pcp/dependency.h"
#include "pxr/usd/pcp/errors.h"
#include "pxr/usd/pcp/mapFunction.h"
#include "pxr/usd/pcp/primIndex.h"
#include "pxr/usd/pcp/propertyIndex.h"
#include "pxr/usd/sdf/declareHandles.h"
#include "pxr/usd/sdf/path.h"
#include "pxr/usd/sdf/pathTable.h"

#include "pxr/usd/ar/resolverContext.h"
#include "pxr/base/tf/declarePtrs.h"
#include "pxr/base/tf/hashset.h"

#include <memory>
#include <string>
#include <unordered_set>
#include <vector>

PXR_NAMESPACE_OPEN_SCOPE

// Forward declarations:
class PcpChanges;
class PcpCacheChanges;
class Pcp_Dependencies;
class PcpLayerStackIdentifier;
class PcpLifeboat;
class PcpNodeRef;
class PcpMapFunction;

TF_DECLARE_WEAK_AND_REF_PTRS(PcpLayerStack);
TF_DECLARE_WEAK_AND_REF_PTRS(Pcp_LayerStackRegistry);
SDF_DECLARE_HANDLES(SdfSpec);

/// \class PcpCache
///
/// PcpCache is the context required to make requests of the Pcp
/// composition algorithm and cache the results.
///
/// Because the algorithms are recursive -- making a request typically
/// makes other internal requests to solve subproblems -- caching
/// subproblem results is required for reasonable performance, and
/// so this cache is the only entrypoint to the algorithms.
///
/// There is a set of parameters that affect the composition results:
///
/// \li variant fallbacks: per named variant set, an ordered list of
///     fallback values to use when composing a prim that defines
///     a variant set but does not specify a selection
/// \li payload inclusion set: an SdfPath set used to identify which
///     prims should have their payloads included during composition;
///     this is the basis for explicit control over the "working set"
///     of composition
/// \li file format target: the file format target that Pcp will request
///     when opening scene description layers
/// \li "USD mode" configures the Pcp composition algorithm to provide
///     only a custom, lighter subset of the full feature set, as needed
///     by the Universal Scene Description system
///
/// There are a number of different computations that can be requested.
/// These include computing a layer stack from a PcpLayerStackIdentifier,
/// computing a prim index or prim stack, and computing a property index.
///
class PcpCache
{
    PcpCache(PcpCache const &) = delete;
    PcpCache &operator=(PcpCache const &) = delete;
public:
    /// Construct a PcpCache to compose results for the layer stack identified
    /// by \a layerStackIdentifier. 
    /// 
    /// If \p fileFormatTarget is given, Pcp will specify \p fileFormatTarget
    /// as the file format target when searching for or opening a layer.
    ///
    /// If \p usd is true, computation of prim indices and composition of prim 
    /// child names are performed without relocates, inherits, permissions, 
    /// symmetry, or payloads, and without populating the prim stack and 
    /// gathering its dependencies.
    PCP_API
    PcpCache(const PcpLayerStackIdentifier & layerStackIdentifier,
             const std::string& fileFormatTarget = std::string(),
             bool usd = false);
    PCP_API ~PcpCache();

    /// \name Parameters
    /// @{

    /// Get the identifier of the layerStack used for composition.
    PCP_API
    PcpLayerStackIdentifier GetLayerStackIdentifier() const;

    /// Get the layer stack for GetLayerStackIdentifier().  Note that
    /// this will neither compute the layer stack nor report errors.
    /// So if the layer stack has not been computed yet this will
    /// return \c NULL.  Use ComputeLayerStack() if you need to
    /// compute the layer stack if it hasn't been computed already
    /// and/or get errors caused by computing the layer stack.
    PCP_API
    PcpLayerStackPtr GetLayerStack() const;

    /// Return true if the cache is configured in Usd mode.
    PCP_API
    bool IsUsd() const;

    /// Returns the file format target this cache is configured for.
    PCP_API
    const std::string& GetFileFormatTarget() const;

    /// Get the list of fallbacks to attempt to use when evaluating
    /// variant sets that lack an authored selection.
    PCP_API
    PcpVariantFallbackMap GetVariantFallbacks() const;

    /// Set the list of fallbacks to attempt to use when evaluating
    /// variant sets that lack an authored selection.
    ///
    /// If \p changes is not \c NULL then it's adjusted to reflect the
    /// changes necessary to see the change in standin preferences,
    /// otherwise those changes are applied immediately.
    PCP_API
    void SetVariantFallbacks( const PcpVariantFallbackMap & map,
                              PcpChanges* changes = NULL );

    /// Return true if the payload is included for the given path.
    PCP_API
    bool IsPayloadIncluded(const SdfPath &path) const;

    /// Returns the payloads requested for inclusion.
    using PayloadSet = std::unordered_set<SdfPath, SdfPath::Hash>;
    PCP_API
    PayloadSet const &GetIncludedPayloads() const;

    /// Request payloads to be included or excluded from composition.
    /// \param pathsToInclude is a set of paths to add to the set for
    ///        payload inclusion.
    /// \param pathsToExclude is a set of paths to remove from the set for
    ///        payload inclusion.
    /// \param changes if not \c NULL, is adjusted to reflect the changes
    ///        necessary to see the change in payloads;  otherwise those
    ///        changes are applied immediately.
    /// \note If a path is listed in both pathsToInclude and pathsToExclude,
    /// it will be treated as an inclusion only.
    ///
    PCP_API 
    void RequestPayloads( const SdfPathSet & pathsToInclude,
                          const SdfPathSet & pathsToExclude,
                          PcpChanges* changes = NULL );

    /// Request layers to be muted or unmuted in this cache.  Muted layers
    /// are ignored during composition and do not appear in any layer
    /// stacks.  The root layer of this stage may not be muted; attempting
    /// to do so will generate a coding error.  If the root layer of a
    /// reference or payload layer stack is muted, the behavior is as if 
    /// the muted layer did not exist, which means a composition error will 
    /// be generated.
    ///
    /// A canonical identifier for each layer in \p layersToMute will be
    /// computed using ArResolver::ComputeRepositoryPath.  Any layer 
    /// encountered during composition with the same repository path will
    /// be considered muted and ignored.  Relative paths will be assumed to
    /// be relative to the cache's root layer.  Search paths are immediately 
    /// resolved and the result is used for computing the canonical path.
    ///
    /// Note that muting a layer will cause this cache to release all
    /// references to that layer.  If no other client is holding on to
    /// references to that layer, it will be unloaded.  In this case, if 
    /// there are unsaved edits to the muted layer, those edits are lost.  
    /// Since anonymous layers are not serialized, muting an anonymous
    /// layer will cause that layer and its contents to be lost in this
    /// case.
    ///
    /// If \p changes is not \c nullptr, it is adjusted to reflect the
    /// changes necessary to see the change in muted layers.  Otherwise,
    /// those changes are applied immediately.
    /// 
    PCP_API 
    void RequestLayerMuting(const std::vector<std::string>& layersToMute,
                            const std::vector<std::string>& layersToUnmute,
                            PcpChanges* changes = nullptr);

    /// Returns the list of canonical identifiers for muted layers
    /// in this cache.  See documentation on RequestLayerMuting for
    /// more details.
    PCP_API 
    const std::vector<std::string>& GetMutedLayers() const;

    /// Returns true if the layer specified by \p layerIdentifier is
    /// muted in this cache, false otherwise.  If \p layerIdentifier
    /// is relative, it is assumed to be relative to this cache's
    /// root layer.  See documentation on RequestLayerMuting for
    /// more details.
    PCP_API 
    bool IsLayerMuted(const std::string& layerIdentifier) const;

    /// Returns true if the layer specified by \p layerIdentifier is
    /// muted in this cache, false otherwise.  If \p layerIdentifier
    /// is relative, it is assumed to be relative to \p anchorLayer.
    /// If \p canonicalMutedLayerIdentifier is supplied, it will be
    /// populated with the canonical identifier of the muted layer if this
    /// function returns true.  See documentation on RequestLayerMuting
    /// for more details.  
    PCP_API 
    bool IsLayerMuted(const SdfLayerHandle& anchorLayer,
                      const std::string& layerIdentifier,
                      std::string* canonicalMutedLayerIdentifier 
                          = nullptr) const;

    /// Returns parameter object containing all inputs for the prim index
    /// computation used by this cache. 
    PCP_API
    PcpPrimIndexInputs GetPrimIndexInputs();

    /// @}

    /// \name Computations
    /// @{

    /// Returns the layer stack for \p identifier if it exists, otherwise
    /// creates a new layer stack for \p identifier.  This returns \c NULL
    /// if \p identifier is invalid (i.e. its root layer is \c NULL). 
    /// \p allErrors will contain any errors encountered while creating a 
    /// new layer stack.  It'll be unchanged if the layer stack already existed.
    PCP_API
    PcpLayerStackRefPtr
    ComputeLayerStack(const PcpLayerStackIdentifier &identifier, 
                      PcpErrorVector *allErrors);

    /// Returns the layer stack for \p identifier if it has been computed
    /// and cached, otherwise returns \c NULL.
    PCP_API
    PcpLayerStackPtr
    FindLayerStack(const PcpLayerStackIdentifier &identifier) const;

    /// Compute and return a reference to the cached result for the
    /// prim index for the given path. \p allErrors will contain any errors 
    /// encountered while performing this operation.
    PCP_API
    const PcpPrimIndex &
    ComputePrimIndex(const SdfPath &primPath, PcpErrorVector *allErrors);

    /// Compute PcpPrimIndexes in the subtree rooted at path in parallel,
    /// recursing to children based on the supplied \p childrenPred.  Also
    /// include payloads not already in this cache's included payloads (see
    /// GetIncludedPayloads()) according to \p payloadPred.
    ///
    /// This is similar to ComputePrimIndex(), except it computes an entire
    /// subtree of indexes in parallel so it can be much more efficient.  This
    /// function invokes both \p childrenPred and \p payloadPred concurrently,
    /// so it must be safe to do so.  
    ///
    /// When a PcpPrimIndex computation completes invoke \p childrenPred, 
    /// passing it the PcpPrimIndex.  If \p childrenPred returns true, continue 
    /// indexing children prim indexes.  In this case, \p childrenPred may 
    /// provide a list of names of the children prim indexes to compute.
    /// If it does not, all children prim indexes will be computed.
    /// If \p childrenPred returns false, stop indexing in that subtree.  
    ///
    /// If payloads discovered during indexing do not already appear in this
    /// cache's set of included payloads, invoke \p payloadPred, passing it the
    /// path for the prim with the payload.  If \p payloadPred returns true,
    /// include its payload and add it to the cache's set of included payloads
    /// upon completion.
    template <class ChildrenPredicate, class PayloadPredicate>
    void ComputePrimIndexesInParallel(const SdfPath &path,
                                      PcpErrorVector *allErrors,
                                      const ChildrenPredicate &childrenPred,
                                      const PayloadPredicate &payloadPred) {
        ComputePrimIndexesInParallel(SdfPathVector(1, path), allErrors,
                                     childrenPred, payloadPred,
                                     "Pcp", "ComputePrimIndexesInParallel");
    }

    /// \overload
    /// XXX Do not add new callers of this method.  It is needed as a workaround
    /// for bug #132031, which we hope to tackle soon (as of 6/2016)
    template <class ChildrenPredicate, class PayloadPredicate>
    void ComputePrimIndexesInParallel(const SdfPath &path,
                                      PcpErrorVector *allErrors,
                                      const ChildrenPredicate &childrenPred,
                                      const PayloadPredicate &payloadPred,
                                      const char *mallocTag1,
                                      const char *mallocTag2) {
        ComputePrimIndexesInParallel(SdfPathVector(1, path), allErrors,
                                     childrenPred, payloadPred,
                                     mallocTag1, mallocTag2);
    }

    /// Vectorized form of ComputePrimIndexesInParallel().  Equivalent to
    /// invoking that method for each path in \p paths, but more efficient.
    template <class ChildrenPredicate, class PayloadPredicate>
    void ComputePrimIndexesInParallel(const SdfPathVector &paths,
                                      PcpErrorVector *allErrors,
                                      const ChildrenPredicate &childrenPred,
                                      const PayloadPredicate &payloadPred) {
        _UntypedIndexingChildrenPredicate cp(&childrenPred);
        _UntypedIndexingPayloadPredicate pp(&payloadPred);
        _ComputePrimIndexesInParallel(paths, allErrors, cp, pp,
                                      "Pcp", "ComputePrimIndexesInParallel");
    }

    /// \overload
    /// XXX Do not add new callers of this method.  It is needed as a workaround
    /// for bug #132031, which we hope to tackle soon (as of 6/2016)
    template <class ChildrenPredicate, class PayloadPredicate>
    void ComputePrimIndexesInParallel(const SdfPathVector &paths,
                                      PcpErrorVector *allErrors,
                                      const ChildrenPredicate &childrenPred,
                                      const PayloadPredicate &payloadPred,
                                      const char *mallocTag1,
                                      const char *mallocTag2) {
        _UntypedIndexingChildrenPredicate cp(&childrenPred);
        _UntypedIndexingPayloadPredicate pp(&payloadPred);
        _ComputePrimIndexesInParallel(paths, allErrors, cp, pp,
                                      mallocTag1, mallocTag2);
    }

    /// Returns a pointer to the cached computed prim index for the given
    /// path, or NULL if it has not been computed.
    PCP_API
    const PcpPrimIndex *
    FindPrimIndex(const SdfPath &primPath) const;

    /// Compute and return a reference to the cached result for the
    /// property index for the given path. \p allErrors will contain any
    /// errors encountered while performing this operation.
    PCP_API
    const PcpPropertyIndex &
    ComputePropertyIndex(const SdfPath &propPath, PcpErrorVector *allErrors);

    /// Returns a pointer to the cached computed property index for the given
    /// path, or NULL if it has not been computed.
    PCP_API
    const PcpPropertyIndex *
    FindPropertyIndex(const SdfPath &propPath) const;

    /// Compute the relationship target paths for the relationship at
    /// \p relationshipPath into \p paths.  If \p localOnly is \c true then
    /// this will compose relationship targets from local nodes only.  If
    /// \p stopProperty is not \c NULL then this will stop composing
    /// relationship targets at \p stopProperty, including \p stopProperty
    /// iff \p includeStopProperty is \c true.  \p allErrors will contain any
    /// errors encountered while performing this operation.
    PCP_API
    void
    ComputeRelationshipTargetPaths(const SdfPath &relationshipPath, 
                                   SdfPathVector *paths,
                                   bool localOnly,
                                   const SdfSpecHandle &stopProperty,
                                   bool includeStopProperty,
                                   PcpErrorVector *allErrors);

    /// Compute the attribute connection paths for the attribute at
    /// \p attributePath into \p paths.  If \p localOnly is \c true then
    /// this will compose attribute connections from local nodes only.  If
    /// \p stopProperty is not \c NULL then this will stop composing
    /// attribute connections at \p stopProperty, including \p stopProperty
    /// iff \p includeStopProperty is \c true.  \p allErrors will contain any
    /// errors encountered while performing this operation.
    PCP_API
    void
    ComputeAttributeConnectionPaths(const SdfPath &attributePath,
                                    SdfPathVector *paths,
                                    bool localOnly,
                                    const SdfSpecHandle &stopProperty,
                                    bool includeStopProperty,
                                    PcpErrorVector *allErrors);

    /// @}
    /// \name Dependencies
    /// @{

    /// Returns set of all layers used by this cache. 
    PCP_API
    SdfLayerHandleSet GetUsedLayers() const;

    /// Returns set of all root layers used by this cache.
    PCP_API
    SdfLayerHandleSet GetUsedRootLayers() const;

    /// Returns every computed & cached layer stack that includes \p layer.
    PCP_API
    const PcpLayerStackPtrVector&
    FindAllLayerStacksUsingLayer(const SdfLayerHandle& layer) const;

    /// Returns dependencies on the given site of scene description,
    /// as discovered by the cached index computations.
    ///
    /// \param depMask specifies what classes of dependency to include;
    ///        see PcpDependencyFlags for details
    /// \param recurseOnSite includes incoming dependencies on
    ///        children of sitePath
    /// \param recurseOnIndex extends the result to include all PcpCache
    ///        child indexes below discovered results
    /// \param filterForExistingCachesOnly filters the results to only
    ///        paths representing computed prim and property index caches;
    ///        otherwise a recursively-expanded result can include
    ///        un-computed paths that are expected to depend on the site
    PCP_API
    PcpDependencyVector
    FindSiteDependencies(const PcpLayerStackPtr& siteLayerStack,
                         const SdfPath& sitePath,
                         PcpDependencyFlags depMask,
                         bool recurseOnSite,
                         bool recurseOnIndex,
                         bool filterForExistingCachesOnly) const;

    /// Returns dependencies on the given site of scene description,
    /// as discovered by the cached index computations.
    ///
    /// This method overload takes a site layer rather than a layer
    /// stack.  It will check every layer stack using that layer, and
    /// apply any relevant sublayer offsets to the map functions in the
    /// returned PcpDependencyVector.
    ///
    /// See the other method for parameter details.
    PCP_API
    PcpDependencyVector
    FindSiteDependencies(const SdfLayerHandle& siteLayer,
                         const SdfPath& sitePath,
                         PcpDependencyFlags depMask,
                         bool recurseOnSite,
                         bool recurseOnIndex,
                         bool filterForExistingCachesOnly) const;

    /// Returns \c true if an opinion for the site at \p localPcpSitePath
    /// in the cache's layer stack can be provided by an opinion in \p layer,
    /// \c false otherwise.  If \c true and \p allowedPathInLayer is not
    /// \c NULL then it's set to a path in \p layer that would provide an
    /// opinion.
    ///
    /// This returns \c false if no prim index has yet been computed for
    /// \p localPcpSitePath.
    PCP_API 
    bool CanHaveOpinionForSite(const SdfPath& localPcpSitePath,
                               const SdfLayerHandle& layer,
                               SdfPath* allowedPathInLayer) const;

    /// Returns a vector of sublayer asset paths used in the layer stack
    /// that didn't resolve to valid assets.
    PCP_API 
    std::vector<std::string> GetInvalidSublayerIdentifiers() const;

    /// Returns true if \p identifier was used as a sublayer path in a 
    /// layer stack but did not identify a valid layer. This is functionally 
    /// equivalent to examining the values in the vector returned by
    /// GetInvalidSublayerIdentifiers, but more efficient.
    PCP_API 
    bool IsInvalidSublayerIdentifier(const std::string& identifier) const;

    /// Returns a map of prim paths to asset paths used by that prim
    /// (e.g. in a reference) that didn't resolve to valid assets.
    PCP_API 
    std::map<SdfPath, std::vector<std::string>, SdfPath::FastLessThan>
    GetInvalidAssetPaths() const;

    /// Returns true if \p resolvedAssetPath was used by a prim (e.g. in
    /// a reference) but did not resolve to a valid asset. This is
    /// functionally equivalent to examining the values in the map returned
    /// by GetInvalidAssetPaths, but more efficient.
    PCP_API 
    bool IsInvalidAssetPath(const std::string& resolvedAssetPath) const;

    /// Returns true if any prim index in this cache has a dependency on a 
    /// dynamic file format argument field. 
    PCP_API
    bool HasAnyDynamicFileFormatArgumentDependencies() const;

    /// Returns true if the given \p field is the name of a field that 
    /// was composed while generating dynamic file format arguments for any prim
    /// index in this cache. 
    PCP_API
    bool IsPossibleDynamicFileFormatArgumentField(const TfToken &field) const;

    /// Returns the dynamic file format dependency data object for the prim
    /// index with the given \p primIndexPath. This will return an empty 
    /// dependency data if either there is no cache prim index for the path or 
    /// if the prim index has no dynamic file formats that it depends on.
    PCP_API
    const PcpDynamicFileFormatDependencyData &
    GetDynamicFileFormatArgumentDependencyData(
        const SdfPath &primIndexPath) const;

    /// @}

    /// \name Change handling
    /// @{

    /// Apply the changes in \p changes.  This blows caches.  It's up to
    /// the client to pull on those caches again as needed.
    ///
    /// Objects that are no longer needed and would be destroyed are
    /// retained in \p lifeboat and won't be destroyed until \p lifeboat is
    /// itself destroyed.  This gives the client control over the timing
    /// of the destruction of those objects.  Clients may choose to pull
    /// on the caches before destroying \p lifeboat.  That may cause the
    /// caches to again retain the objects, meaning they won't be destroyed
    /// when \p lifeboat is destroyed.
    ///
    /// For example, if blowing a cache means an SdfLayer is no longer
    /// needed then \p lifeboat will hold an SdfLayerRefPtr to that layer. 
    /// The client can then pull on that cache, which could cause the
    /// cache to hold an SdfLayerRefPtr to the layer again.  If so then
    /// destroying \p changes will not destroy the layer.  In any case,
    /// we don't destroy the layer and then read it again.  However, if
    /// the client destroys \p lifeboat before pulling on the cache then
    /// we would destroy the layer then read it again.
    PCP_API 
    void Apply(const PcpCacheChanges& changes, PcpLifeboat* lifeboat);

    /// Reload the layers of the layer stack, except session layers
    /// and sublayers of session layers.  This will also try to load
    /// sublayers in this cache's layer stack that could not be loaded
    /// previously.  It will also try to load any referenced or payloaded
    /// layer that could not be loaded previously.  Clients should
    /// subsequently \c Apply() \p changes to use any now-valid layers.
    PCP_API
    void Reload(PcpChanges* changes);

    /// Reload every layer used by the prim at \p primPath that's across
    /// a reference or payload.  Clients should subsequently apply the
    /// changes to use any now valid layers.
    ///
    /// Note:  If a reference or payload was to an invalid asset and this
    /// asset is valid upon reloading then this call will not necessarily
    /// reload every layer accessible across the reference or payload.
    /// For example, say prim R has an invalid reference and prim Q has a
    /// valid reference to layer X with sublayer Y.  If on reload R now
    /// has a valid reference to layer Z with sublayer Y, we will load Z
    /// but we will not reload Y.
    PCP_API 
    void ReloadReferences(PcpChanges* changes, const SdfPath& primPath);

    /// @}

    /// \name Diagnostics
    /// @{

    /// Prints various statistics about the data stored in this cache.
    PCP_API
    void PrintStatistics() const;

    /// @}

private:
    friend class PcpChanges;
    friend class Pcp_Statistics;

    struct _ParallelIndexer;

    // Helper struct to type-erase a children predicate for the duration of
    // ComputePrimIndexesInParallel.
    //
    // This lets us achieve two goals.  First, clients may pass any arbitrary
    // type as a predicate (e.g. they do not have to derive some base class).
    // Second, it lets us keep the parallel indexing implementation in the .cpp
    // file, avoiding any large template code instantiation.
    //
    // The cost we pay is this very thin indirect call.  We instantiate a
    // function template with the client's predicate type that simply does a
    // typecast and predicate invocation, and pass that function pointer into
    // the implementation.  There is no heap allocation, no predicate copy, no
    // argument marshalling, etc.
    struct _UntypedIndexingChildrenPredicate {
        _UntypedIndexingChildrenPredicate() : pred(nullptr), invoke(nullptr) {}
        template <class Pred>
        explicit _UntypedIndexingChildrenPredicate(const Pred *pred)
            : pred(pred), invoke(_Invoke<Pred>) {}

        inline bool operator()(const PcpPrimIndex &index, 
                               TfTokenVector *childNamesToCompose) const {
            return invoke(pred, index, childNamesToCompose);
        }
    private:
        template <class Pred>
        static bool _Invoke(const void *pred, const PcpPrimIndex &index,
                            TfTokenVector *namesToCompose) {
            return (*static_cast<const Pred *>(pred))(index, namesToCompose);
        }
        const void *pred;
        bool (*invoke)(const void *, const PcpPrimIndex &, TfTokenVector *);
    };

    // See doc for _UntypedIndexingChildrenPredicate above.  This does the same
    // for the payload inclusion predicate.
    struct _UntypedIndexingPayloadPredicate {
        template <class Pred>
        explicit _UntypedIndexingPayloadPredicate(const Pred *pred)
            : pred(pred), invoke(_Invoke<Pred>) {}

        inline bool operator()(const SdfPath &path) const {
            return invoke(pred, path);
        }
    private:
        template <class Pred>
        static bool _Invoke(const void *pred, const SdfPath &path) {
            return (*static_cast<const Pred *>(pred))(path);
        }
        const void *pred;
        bool (*invoke)(const void *, const SdfPath &);
    };

    // Internal helper for recursive indexing.
    const PcpPrimIndex &
    _ComputePrimIndexWithCompatibleInputs(
        const SdfPath & path, const PcpPrimIndexInputs &inputs,
        PcpErrorVector *allErrors);

    // Friend to allow low-level indexing code access to the above.
    friend const PcpPrimIndex &
    Pcp_ComputePrimIndexWithCompatibleInputs(
        PcpCache &cache,
        const SdfPath & path, const PcpPrimIndexInputs &inputs,
        PcpErrorVector *allErrors);

    // Parallel indexing implementation.
    PCP_API
    void _ComputePrimIndexesInParallel(
        const SdfPathVector &paths,
        PcpErrorVector *allErrors,
        _UntypedIndexingChildrenPredicate childrenPred,
        _UntypedIndexingPayloadPredicate payloadPred,
        const char *mallocTag1,
        const char *mallocTag2);

    void _RemovePrimCache(const SdfPath& primPath, PcpLifeboat* lifeboat);
    void _RemovePrimAndPropertyCaches(const SdfPath& root,
                                      PcpLifeboat* lifeboat);
    void _RemovePropertyCache(const SdfPath& root, PcpLifeboat* lifeboat);
    void _RemovePropertyCaches(const SdfPath& root, PcpLifeboat* lifeboat);

    // Returns the prim index for \p path if it exists, NULL otherwise.
    PcpPrimIndex* _GetPrimIndex(const SdfPath& path);
    const PcpPrimIndex* _GetPrimIndex(const SdfPath& path) const;

    // Returns the property index for \p path if it exists, NULL otherwise.
    PcpPropertyIndex* _GetPropertyIndex(const SdfPath& path);
    const PcpPropertyIndex* _GetPropertyIndex(const SdfPath& path) const;

private:
    // Fixed evaluation parameters, set when the cache is created.  Note that
    // _rootLayer and _sessionLayer are not const because we want to mutate them
    // to enable parallel teardown in the destructor.
    SdfLayerRefPtr _rootLayer;
    SdfLayerRefPtr _sessionLayer;
    const ArResolverContext _pathResolverContext;

    // Flag that configures PcpCache to use the restricted set of USD features.
    // Currently it governs whether relocates, inherits, permissions,
    // symmetry, or payloads are considered, and whether the prim stack
    // is populated and its depdencies gathered during computation of
    // prim indices and composition of prim child names.
    const bool _usd;

    // File format target for all scene description layers this cache will
    // find or open during prim index computation.
    const std::string _fileFormatTarget;

    // The layer stack for this cache.  Holding this by ref ptr means we
    // hold all of our local layers by ref ptr (including the root and
    // session layers, again).
    PcpLayerStackRefPtr _layerStack;

    // Modifiable evaluation parameters.
    // Anything that changes these should also yield a PcpChanges
    // value describing the necessary cache invalidation.
    PayloadSet _includedPayloads;
    PcpVariantFallbackMap _variantFallbackMap;

    // Cached computation types.
    typedef Pcp_LayerStackRegistryRefPtr _LayerStackCache;
    typedef SdfPathTable<PcpPrimIndex> _PrimIndexCache;
    typedef SdfPathTable<PcpPropertyIndex> _PropertyIndexCache;

    // Cached computations.
    _LayerStackCache _layerStackCache;
    _PrimIndexCache  _primIndexCache;
    _PropertyIndexCache  _propertyIndexCache;
    std::unique_ptr<Pcp_Dependencies> _primDependencies;

    // Parallel indexer state.
    std::unique_ptr<_ParallelIndexer> _parallelIndexer;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_USD_PCP_CACHE_H
