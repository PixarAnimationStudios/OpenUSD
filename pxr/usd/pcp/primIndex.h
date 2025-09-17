//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_USD_PCP_PRIM_INDEX_H
#define PXR_USD_PCP_PRIM_INDEX_H

#include "pxr/pxr.h"
#include "pxr/usd/pcp/api.h"
#include "pxr/usd/pcp/composeSite.h"
#include "pxr/usd/pcp/dependency.h"
#include "pxr/usd/pcp/dynamicFileFormatDependencyData.h"
#include "pxr/usd/pcp/expressionVariablesDependencyData.h"
#include "pxr/usd/pcp/errors.h"
#include "pxr/usd/pcp/iterator.h"
#include "pxr/usd/pcp/node.h"
#include "pxr/usd/pcp/types.h"
#include "pxr/usd/sdf/declareHandles.h"
#include "pxr/usd/sdf/site.h"
#include "pxr/base/tf/declarePtrs.h"
#include "pxr/base/tf/hashmap.h"
#include "pxr/base/tf/hashset.h"

#include <tbb/spin_rw_mutex.h>

#include <functional>
#include <map>
#include <memory>
#include <unordered_set>

PXR_NAMESPACE_OPEN_SCOPE

SDF_DECLARE_HANDLES(SdfLayer);
SDF_DECLARE_HANDLES(SdfPrimSpec);

TF_DECLARE_REF_PTRS(PcpLayerStack);
TF_DECLARE_WEAK_AND_REF_PTRS(PcpPrimIndex_Graph);

class ArResolver;
class PcpCache;
class PcpCacheChanges;
class PcpPrimIndex;
class PcpPrimIndexInputs;
class PcpPrimIndexOutputs;
class SdfPath;

/// \class PcpPrimIndex
///
/// PcpPrimIndex is an index of the all sites of scene description that
/// contribute opinions to a specific prim, under composition
/// semantics.
///
/// PcpComputePrimIndex() builds an index ("indexes") the given prim site.
/// At any site there may be scene description values expressing arcs
/// that represent instructions to pull in further scene description.
/// PcpComputePrimIndex() recursively follows these arcs, building and
/// ordering the results.
///
class PcpPrimIndex
{
public:
    /// Default construct an empty, invalid prim index.
    PCP_API
    PcpPrimIndex();

    /// Copy-construct a prim index.
    PCP_API
    PcpPrimIndex(const PcpPrimIndex& rhs);

    /// Move-construction
    PcpPrimIndex(PcpPrimIndex &&rhs) noexcept = default;

    /// Assignment.
    PcpPrimIndex &operator=(const PcpPrimIndex &rhs) {
        PcpPrimIndex(rhs).Swap(*this);
        return *this;
    }

    // Move-assignment.
    PcpPrimIndex &operator=(PcpPrimIndex &&rhs) noexcept = default;

    /// Swap the contents of this prim index with \p index.
    PCP_API
    void Swap(PcpPrimIndex& rhs);

    /// Same as Swap(), but standard name.
    inline void swap(PcpPrimIndex &rhs) { Swap(rhs); }

    /// Return true if this index is valid.
    /// A default-constructed index is invalid.
    bool IsValid() const { return bool(_graph); }

    void SetGraph(const PcpPrimIndex_GraphRefPtr& graph) {
        _graph = graph;
    }

    /// Add the nodes in \p childPrimIndex to this prim index; \p arcToParent
    /// specifies the node in this prim index where the root node of \p
    /// childPrimIndex will be added, along with other information about the
    /// composition arc connecting the two prim indexes.
    ///
    /// Return the node in this prim index corresponding to the root node
    /// of \p childPrimIndex.
    ///
    PcpNodeRef AddChildPrimIndex(
        const PcpArc &arcToParent, 
        PcpPrimIndex &&childPrimIndex,
        PcpErrorBasePtr *error);

    const PcpPrimIndex_GraphRefPtr &GetGraph() const {
        return _graph;
    }

    /// Returns the root node of the prim index graph.
    PCP_API
    PcpNodeRef GetRootNode() const;

    /// Returns the path of the prim whose opinions are represented by this 
    /// prim index.
    PCP_API
    const SdfPath& GetPath() const;

    /// Returns true if this prim index contains any scene description
    /// opinions.
    PCP_API
    bool HasSpecs() const;

    /// Returns true if the prim has any authored payload arcs.
    /// The payload contents are only resolved and included
    /// if this prim's path is in the payload inclusion set
    /// provided in PcpPrimIndexInputs. 
    PCP_API
    bool HasAnyPayloads() const;

    /// Returns true if this prim index was composed in USD mode.
    /// \see PcpCache::IsUsd().
    PCP_API
    bool IsUsd() const;

    /// Returns true if this prim index is instanceable.
    /// Instanceable prim indexes with the same instance key are
    /// guaranteed to have the same set of opinions, but may not have
    /// local opinions about name children.
    /// \see PcpInstanceKey
    PCP_API
    bool IsInstanceable() const;

    /// \name Iteration
    /// @{

    /// Returns range of iterators that encompass all children of the root node
    /// with the given arc type as well as their descendants, in 
    /// strong-to-weak order.
    /// 
    /// By default, this returns a range encompassing the entire index.
    PCP_API
    PcpNodeRange GetNodeRange(PcpRangeType rangeType = PcpRangeTypeAll) const;

    /// Returns the node iterator that points to the given \p node if the
    /// node is in the prim index graph.
    /// Returns the end of the node range if the node is not contained in this
    /// prim index.
    PCP_API
    PcpNodeIterator GetNodeIteratorAtNode(const PcpNodeRef &node) const;

    /// Returns range of iterators that encompass the given \p node and all of
    /// its descendants in strong-to-weak order.
    PCP_API
    PcpNodeRange GetNodeSubtreeRange(const PcpNodeRef &node) const;

    /// Returns range of iterators that encompasses all prims, in
    /// strong-to-weak order.
    PCP_API
    PcpPrimRange GetPrimRange(PcpRangeType rangeType = PcpRangeTypeAll) const;

    /// Returns range of iterators that encompasses all prims from the
    /// site of \p node. \p node must belong to this prim index.
    PCP_API
    PcpPrimRange GetPrimRangeForNode(const PcpNodeRef& node) const;

    /// @}

    /// \name Lookup
    /// @{

    /// Returns the node that brings opinions from \p primSpec into
    /// this prim index. If no such node exists, returns an invalid PcpNodeRef.
    PCP_API
    PcpNodeRef GetNodeProvidingSpec(const SdfPrimSpecHandle& primSpec) const;

    /// Returns the node that brings opinions from the Sd prim spec at \p layer
    /// and \p path into this prim index. If no such node exists, returns an
    /// invalid PcpNodeRef.
    PCP_API
    PcpNodeRef GetNodeProvidingSpec(
        const SdfLayerHandle& layer, const SdfPath& path) const;

    /// @}

    /// \name Diagnostics
    /// @{

    /// Return the list of errors local to this prim.
    PcpErrorVector GetLocalErrors() const {
        return _localErrors ? *_localErrors.get() : PcpErrorVector();
    }

    /// Prints various statistics about this prim index.
    PCP_API
    void PrintStatistics() const;

    /// Dump the prim index contents to a string.
    ///
    /// If \p includeInheritOriginInfo is \c true, output for implied inherit
    /// nodes will include information about the originating inherit node.
    /// If \p includeMaps is \c true, output for each node will include the
    /// mappings to the parent and root node.
    PCP_API
    std::string DumpToString(
        bool includeInheritOriginInfo = true,
        bool includeMaps = true) const;

    /// Dump the prim index in dot format to the file named \p filename.
    /// See Dump(...) for information regarding arguments.
    PCP_API
    void DumpToDotGraph(
        const std::string& filename,
        bool includeInheritOriginInfo = true,
        bool includeMaps = false) const;

    /// @}


    /// \name Derived computations
    /// @{

    /// Compute the prim child names for the given path. \p errors will 
    /// contain any errors encountered while performing this operation.
    PCP_API
    void ComputePrimChildNames(TfTokenVector *nameOrder,
                               PcpTokenSet *prohibitedNameSet) const;

    /// Compute the prim child names for this prim when composed from only the
    /// subtree starting at \p subtreeRootNode.
    PCP_API
    void ComputePrimChildNamesInSubtree(
        const PcpNodeRef &subtreeRootNode,
        TfTokenVector *nameOrder,
        PcpTokenSet *prohibitedNameSet) const;

    /// Compute the prim property names for the given path. \p errors will
    /// contain any errors encountered while performing this operation.  The
    /// \p nameOrder vector must not contain any duplicate entries.
    PCP_API
    void ComputePrimPropertyNames(TfTokenVector *nameOrder) const;

    /// Compose the authored prim variant selections.
    ///
    /// These are the variant selections expressed in scene description.
    /// Note that these selections may not have actually been applied,
    /// if they are invalid.
    ///
    /// \note This result is not cached, but computed each time.
    PCP_API
    SdfVariantSelectionMap ComposeAuthoredVariantSelections() const;

    /// Return the variant selection applied for the named variant set.
    /// If none was applied, this returns an empty string.
    /// This can be different from the authored variant selection;
    /// for example, if the authored selection is invalid.
    PCP_API
    std::string GetSelectionAppliedForVariantSet(
        const std::string &variantSet) const;

    /// @}

private:
    friend class PcpPrimIterator;
    friend struct Pcp_PrimIndexer;
    friend void Pcp_RescanForSpecs(
                    PcpPrimIndex*, bool usd,
                    bool updateHasSpecs,
                    const PcpCacheChanges *cacheChanges);

    // The node graph representing the compositional structure of this prim.
    PcpPrimIndex_GraphRefPtr _graph;

    // The prim stack.  This is just a derived structure representing
    // a cached strong-to-weak traversal of the graph collecting specs.
    Pcp_CompressedSdSiteVector _primStack;

    // List of errors local to this prim, encountered during computation.
    // NULL if no errors were found (the expected common case).
    std::unique_ptr<PcpErrorVector> _localErrors;
};

/// Free function version for generic code and ADL.
inline void swap(PcpPrimIndex &l, PcpPrimIndex &r) { l.swap(r); }

/// \class PcpPrimIndexOutputs
///
/// Outputs of the prim indexing procedure.
///
class PcpPrimIndexOutputs 
{
public:
    /// Enumerator whose enumerants describe the payload state of this prim
    /// index.  NoPayload if the index has no payload arcs, otherwise whether
    /// payloads were included or excluded, and if done so by consulting either
    /// the cache's payload include set, or determined by a payload predicate.
    enum PayloadState { NoPayload,
                        IncludedByIncludeSet, ExcludedByIncludeSet,
                        IncludedByPredicate, ExcludedByPredicate };

    /// Prim index describing the composition structure for the associated
    /// prim.
    PcpPrimIndex primIndex;

    /// List of all errors encountered during indexing.
    PcpErrorVector allErrors;

    /// Indicates the payload state of this index.  See documentation for
    /// PayloadState enum for more information.
    PayloadState payloadState = NoPayload;
    
    /// A list of names of fields that were composed to generate dynamic file 
    /// format arguments for a node in primIndex. These are not necessarily 
    /// fields that had values, but is the list of all fields that a  composed 
    /// value was requested for. 
    PcpDynamicFileFormatDependencyData dynamicFileFormatDependency;

    /// Dependencies on expression variables from composition arcs in this
    /// prim index.
    PcpExpressionVariablesDependencyData expressionVariablesDependency;

    /// Site dependencies from nodes in the prim index that have been culled.
    std::vector<PcpCulledDependency> culledDependencies;

    /// Appends the outputs from \p childOutputs to this object, using 
    /// \p arcToParent to connect \p childOutputs' prim index to this object's
    /// prim index. 
    /// 
    /// Returns the node in this object's prim index corresponding to the root
    /// node of \p childOutputs' prim index.
    PcpNodeRef Append(PcpPrimIndexOutputs&& childOutputs,
                      const PcpArc& arcToParent,
                      PcpErrorBasePtr *error);
};

/// \class PcpPrimIndexInputs
///
/// Inputs for the prim indexing procedure.
///
class PcpPrimIndexInputs {
public:
    PcpPrimIndexInputs() 
        : cache(nullptr)
        , variantFallbacks(nullptr)
        , includedPayloads(nullptr)
        , includedPayloadsMutex(nullptr)
        , parentIndex(nullptr)
        , cull(true)
        , usd(false) 
    { }

    /// Returns true if prim index computations using this parameters object
    /// would be equivalent to computations using \p params.
    bool IsEquivalentTo(const PcpPrimIndexInputs& params) const;

    /// If supplied, the given PcpCache will be used where possible to compute
    /// needed intermediate results.
    PcpPrimIndexInputs& Cache(PcpCache* cache_)
    { cache = cache_; return *this; }

    /// Ordered list of variant names to use for the "standin" variant set
    /// if there is no authored opinion in scene description.
    PcpPrimIndexInputs& VariantFallbacks(const PcpVariantFallbackMap* map)
    { variantFallbacks = map; return *this; }

    /// Set of paths to prims that should have their payloads included
    /// during composition.
    using PayloadSet = std::unordered_set<SdfPath, SdfPath::Hash>;
    PcpPrimIndexInputs& IncludedPayloads(const PayloadSet* payloadSet)
    { includedPayloads = payloadSet; return *this; }

    /// Optional mutex for accessing includedPayloads.
    PcpPrimIndexInputs &IncludedPayloadsMutex(tbb::spin_rw_mutex *mutex)
    { includedPayloadsMutex = mutex; return *this; }

    /// Optional predicate evaluated when a not-yet-included payload is
    /// discovered while indexing.  If the predicate returns true, indexing
    /// includes the payload and sets the includedDiscoveredPayload bit in the
    /// outputs.
    PcpPrimIndexInputs &IncludePayloadPredicate(
        std::function<bool (const SdfPath &)> predicate)
    { includePayloadPredicate = predicate; return *this; }
    
    /// Whether subtrees that contribute no opinions should be culled
    /// from the index.
    PcpPrimIndexInputs& Cull(bool doCulling = true)
    { cull = doCulling; return *this; }

    /// Whether the prim stack should be computed, and
    /// whether relocates, inherits, permissions, symmetry, or payloads should
    /// be considered during prim index computation,
    PcpPrimIndexInputs& USD(bool doUSD = true)
    { usd = doUSD; return *this; }

    /// The file format target for scene description layers encountered during
    /// prim index computation.
    PcpPrimIndexInputs& FileFormatTarget(const std::string& target)
    { fileFormatTarget = target; return *this; }

// private:
    PcpCache* cache;
    const PcpVariantFallbackMap* variantFallbacks;
    const PayloadSet* includedPayloads;
    tbb::spin_rw_mutex *includedPayloadsMutex;
    std::function<bool (const SdfPath &)> includePayloadPredicate;
    const PcpPrimIndex *parentIndex;
    std::string fileFormatTarget;
    bool cull;
    bool usd;
};

/// Compute an index for the given path. \p errors will contain any errors
/// encountered while performing this operation.
PCP_API
void
PcpComputePrimIndex(
    const SdfPath& primPath,
    const PcpLayerStackPtr& layerStack,
    const PcpPrimIndexInputs& inputs,
    PcpPrimIndexOutputs* outputs,
    ArResolver* pathResolver = NULL);

/// Computes the list of prim specs that contribute opinions for the given
/// \p primIndex in order from strongest to weakest. This should only be used
/// when it is needed to be known what all the specs are that contribute to the
/// prim index and it makes sense to potentially cache the result. This should
/// never be used for value resolution as it is ineffecient for that purpose.
PCP_API
SdfPrimSpecHandleVector
PcpComputePrimStackForPrimIndex(const PcpPrimIndex &primIndex);

// Returns true if \p index should be recomputed due to changes to
// any computed asset paths that were used to find or open layers
// when originally composing \p index. This may be due to scene
// description changes or external changes to asset resolution that
// may affect the computation of those asset paths.
bool
Pcp_NeedToRecomputeDueToAssetPathChange(const PcpPrimIndex& index);

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_USD_PCP_PRIM_INDEX_H
