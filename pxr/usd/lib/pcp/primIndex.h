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
#ifndef PCP_PRIM_INDEX_H
#define PCP_PRIM_INDEX_H

#include "pxr/usd/pcp/composeSite.h"
#include "pxr/usd/pcp/errors.h"
#include "pxr/usd/pcp/iterator.h"
#include "pxr/usd/pcp/node.h"
#include "pxr/usd/pcp/types.h"
#include "pxr/usd/sdf/declareHandles.h"
#include "pxr/usd/sdf/site.h"
#include "pxr/base/tf/declarePtrs.h"
#include "pxr/base/tf/hashmap.h"
#include "pxr/base/tf/hashset.h"
#include <boost/shared_ptr.hpp>
#include <boost/unordered_map.hpp>
#include <map>

SDF_DECLARE_HANDLES(SdfLayer);
SDF_DECLARE_HANDLES(SdfPrimSpec);

TF_DECLARE_REF_PTRS(PcpLayerStack);
TF_DECLARE_WEAK_AND_REF_PTRS(PcpPrimIndex_Graph);

class ArResolver;
class PcpCache;
class PcpPrimIndex;
class PcpPrimIndexInputs;
class PcpPrimIndexOutputs;
class PcpPayloadDecorator;
class SdfPath;

// A set of sites that a given site depends on.  Also notes if the type of
// dependency.  The sites use a layer stack ref ptr so PcpLayerStackSite
// isn't used.
class PcpPrimIndexDependencies {
public:
    typedef std::pair<PcpLayerStackRefPtr, SdfPath> Site;
    struct Hash {
        size_t operator()(const Site& site) const;
    };

    /// Swap contents with \p r.
    inline void swap(PcpPrimIndexDependencies &r) { sites.swap(r.sites); }

    typedef boost::unordered_map<Site, PcpDependencyType, Hash> Map;
    Map sites;
};

/// Free function version for generic code and ADL.
inline void
swap(PcpPrimIndexDependencies &l, PcpPrimIndexDependencies &r) { l.swap(r); } 

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
    PcpPrimIndex();

    /// Copy-construct a prim index.
    PcpPrimIndex(const PcpPrimIndex& rhs);

    /// Assignment.
    PcpPrimIndex &operator=(const PcpPrimIndex &rhs) {
        PcpPrimIndex(rhs).Swap(*this);
        return *this;
    }

    /// Swap the contents of this prim index with \p index.
    void Swap(PcpPrimIndex& rhs);

    /// Same as Swap(), but standard name.
    inline void swap(PcpPrimIndex &rhs) { Swap(rhs); }

    void SetGraph(const PcpPrimIndex_GraphRefPtr& graph);
    PcpPrimIndex_GraphPtr GetGraph() const;

    /// Returns the root node of the prim index graph.
    PcpNodeRef GetRootNode() const;

    /// Returns the path of the prim whose opinions are represented by this 
    /// prim index.
    const SdfPath& GetPath() const;

    /// Returns true if this prim index contains any scene description
    /// opinions.
    bool HasSpecs() const;

    /// Returns true if the prim has an authored payload arc.
    /// The payload contents are only resolved and included
    /// if this prim's path is in the payload inclusion set
    /// provided in PcpPrimIndexInputs.
    bool HasPayload() const;

    /// Returns true if this prim index was composed in USD mode.
    /// \see PcpCache::IsUsd().
    bool IsUsd() const;

    /// Returns true if this prim index is instanceable.
    /// Instanceable prim indexes with the same instance key are
    /// guaranteed to have the same set of opinions, but may not have
    /// local opinions about name children.
    /// \see PcpInstanceKey
    bool IsInstanceable() const;

    /// Get the set of asset paths used by direct arcs in this prim.
    /// This set does not include asset paths used by ancestral arcs
    /// (from namespace ancestors), which may also contribute opinions
    /// to this prim.  It also includes any asset paths that were
    /// requested by arcs, but could not be resolved.
    std::vector<std::string> GetUsedAssetPaths() const;

    /// Record a used asset path.
    /// Only meant for internal use while constructing a PcpPrimIndex.
    void AddUsedAssetPath(const std::string& assetPath);
    void AddUsedAssetPaths(const std::vector<std::string>& assetPaths);

    /// \name Iteration
    /// @{

    /// Returns range of iterators that encompass all direct children
    /// with the given arc type as well as their descendants, in 
    /// strong-to-weak order.
    /// 
    /// By default, this returns a range encompassing the entire index.
    PcpNodeRange GetNodeRange(PcpRangeType rangeType = PcpRangeTypeAll) const;

    /// Returns range of iterators that encompasses all prims, in
    /// strong-to-weak order.
    PcpPrimRange GetPrimRange(PcpRangeType rangeType = PcpRangeTypeAll) const;

    /// Returns range of iterators that encompasses all prims from the
    /// site of \p node. \p node must belong to this prim index.
    PcpPrimRange GetPrimRangeForNode(const PcpNodeRef& node) const;

    /// @}

    /// \name Lookup
    /// @{

    /// Returns the node that brings opinions from \p primSpec into
    /// this prim index. If no such node exists, returns an invalid PcpNodeRef.
    PcpNodeRef GetNodeProvidingSpec(const SdfPrimSpecHandle& primSpec) const;

    /// Returns the node that brings opinions from the Sd prim spec at \p layer
    /// and \p path into this prim index. If no such node exists, returns an
    /// invalid PcpNodeRef.
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
    void PrintStatistics() const;

    /// Dump the prim index contents to a string.
    ///
    /// If \p includeInheritOriginInfo is \c true, output for implied inherit
    /// nodes will include information about the originating inherit node.
    /// If \p includeMaps is \c true, output for each node will include the
    /// mappings to the parent and root node.
    std::string DumpToString(
        bool includeInheritOriginInfo = true,
        bool includeMaps = true) const;

    /// Dump the prim index in dot format to the file named \p filename.
    /// See Dump(...) for information regarding arguments.
    void DumpToDotGraph(
        const std::string& filename,
        bool includeInheritOriginInfo = true,
        bool includeMaps = false) const;

    /// Verify that this is index is well-formed.
    void Validate();

    /// @}


    /// \name Derived computations
    /// @{

    /// Compute the prim child names for the given path. \p errors will 
    /// contain any errors encountered while performing this operation.
    void ComputePrimChildNames(TfTokenVector *nameOrder,
                               PcpTokenSet *prohibitedNameSet) const;

    /// Compute the prim property names for the given path. \p errors will
    /// contain any errors encountered while performing this operation.  The
    /// \p nameOrder vector must not contain any duplicate entries.
    void ComputePrimPropertyNames(TfTokenVector *nameOrder) const;

    /// Compose the authored prim variant selections.
    ///
    /// These are the variant selections expressed in scene description.
    /// Note that these selections may not have actually been applied,
    /// if they are invalid.
    ///
    /// \note This result is not cached, but computed each time.
    SdfVariantSelectionMap ComposeAuthoredVariantSelections() const;

    /// Return the variant selecion applied for the named variant set.
    /// If none was applied, this returns an empty string.
    /// This can be different from the authored variant selection;
    /// for example, if the authored selection is invalid.
    std::string GetSelectionAppliedForVariantSet(
        const std::string &variantSet) const;

    /// @}

private:
    friend class PcpPrimIterator;
    friend struct Pcp_PrimIndexer;
    friend void Pcp_BuildPrimStack(
        PcpPrimIndex*, SdfSiteVector*, PcpNodeRefVector*);

    // The node graph representing the compositional structure of this prim.
    PcpPrimIndex_GraphRefPtr _graph;

    // The prim stack.  This is just a derived structure representing
    // a cached strong-to-weak traversal of the graph collecting specs.
    Pcp_CompressedSdSiteVector _primStack;

    // List of errors local to this prim, encountered during computation.
    // NULL if no errors were found (the expected common case).
    boost::scoped_ptr<PcpErrorVector> _localErrors;

    // List of asset paths directly used by this prim.  
    // This data cannot be derived purely from the
    // graph since it includes asset paths that failed to resolve,
    // and consequently did not contribute any nodes to the graph.
    // NULL if this list is empty (the expected common case).
    boost::scoped_ptr<std::vector<std::string> > _usedAssetPaths;
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
    /// Prim index describing the composition structure for the associated
    /// prim.
    PcpPrimIndex primIndex;

    /// Dependencies found during prim indexing. Note that these dependencies
    /// may be keeping structures (e.g., layer stacks) in the above prim index 
    /// alive and must live on at least as long as the prim index.
    PcpPrimIndexDependencies dependencies;
    SdfSiteVector dependencySites;
    PcpNodeRefVector dependencyNodes;

    /// Spooky dependency specs are those that were consulted in the process
    /// of building this prim index, but which are not a namespace ancestor
    /// of this prim due to relocations.
    PcpPrimIndexDependencies spookyDependencies;
    SdfSiteVector spookyDependencySites;
    PcpNodeRefVector spookyDependencyNodes;

    /// List of all errors encountered during indexing.
    PcpErrorVector allErrors;
    
    /// Swap content with \p r.
    inline void swap(PcpPrimIndexOutputs &r) {
        primIndex.swap(r.primIndex);
        dependencies.swap(r.dependencies);
        dependencySites.swap(r.dependencySites);
        dependencyNodes.swap(r.dependencyNodes);
        spookyDependencies.swap(r.spookyDependencies);
        spookyDependencySites.swap(r.spookyDependencySites);
        spookyDependencyNodes.swap(r.spookyDependencyNodes);
        allErrors.swap(r.allErrors);
    }
};

/// Free function version for generic code and ADL.
inline void swap(PcpPrimIndexOutputs &l, PcpPrimIndexOutputs &r) { l.swap(r); }

/// \class PcpPrimIndexInputs
///
/// Inputs for the prim indexing procedure.
///
class PcpPrimIndexInputs {
public:
    PcpPrimIndexInputs() 
        : cache(NULL)
        , variantFallbacks(NULL)
        , includedPayloads(NULL)
        , parentIndex(NULL)
        , cull(true)
        , usd(false) 
        , payloadDecorator(NULL)
    { }

    /// Returns true if prim index computations using this parameters object
    /// would be equivalent to computations using \p params.
    bool IsEquivalentTo(const PcpPrimIndexInputs& params) const;

    /// If supplied, the given PcpCache will be used where possible to compute
    /// needed intermediate results.
    PcpPrimIndexInputs& Cache(PcpCache* cache_)
    { cache = cache_; return *this; }

    /// If supplied, the given PcpPayloadDecorator will be invoked when
    /// processing a payload arc.
    PcpPrimIndexInputs& PayloadDecorator(PcpPayloadDecorator* decorator)
    { payloadDecorator = decorator; return *this; }

    /// Ordered list of variant names to use for the "standin" variant set
    /// if there is no authored opinion in scene description.
    PcpPrimIndexInputs& VariantFallbacks(const PcpVariantFallbackMap* map)
    { variantFallbacks = map; return *this; }

    /// Set of paths to prims that should have their payloads included
    /// during composition.
    typedef TfHashSet<SdfPath, SdfPath::Hash> PayloadSet;
    PcpPrimIndexInputs& IncludedPayloads(const PayloadSet* payloadSet)
    { includedPayloads = payloadSet; return *this; }

    /// Whether subtrees that contribute no opinions should be culled
    /// from the index.
    PcpPrimIndexInputs& Cull(bool doCulling = true)
    { cull = doCulling; return *this; }

    /// Whether the prim stack and its dependencies should be computed, and
    /// whether relocates, inherits, permissions, symmetry, or payloads should
    /// be considered during prim index computation,
    PcpPrimIndexInputs& USD(bool doUSD = true)
    { usd = doUSD; return *this; }

    /// The target schema for scene description layers encountered during
    /// prim index computation.
    PcpPrimIndexInputs& TargetSchema(const std::string& schema)
    { targetSchema = schema; return *this; }

// private:
    PcpCache* cache;
    const PcpVariantFallbackMap* variantFallbacks;
    const PayloadSet* includedPayloads;
    const PcpPrimIndex *parentIndex;
    bool cull;
    bool usd;
    std::string targetSchema;
    PcpPayloadDecorator* payloadDecorator;
};

/// Compute an index for the given path. \p errors will contain any errors
/// encountered while performing this operation. Any encountered dependencies
/// on remote layer stacks will be appended to \p dependencies;  it must not 
/// be \c NULL because it's what keeps the layer stacks alive.
void
PcpComputePrimIndex(
    const SdfPath& primPath,
    const PcpLayerStackPtr& layerStack,
    const PcpPrimIndexInputs& inputs,
    PcpPrimIndexOutputs* outputs,
    ArResolver* pathResolver = NULL);

/// Returns true if the 'new' default standin behavior is enabled.
bool
PcpIsNewDefaultStandinBehaviorEnabled();

// Sets the prim stack in \p index and returns the sites of prims that
// \p index depends on in \p dependencySites, as well as the nodes from which 
// these prims originated in \p dependencyNodes.
void
Pcp_BuildPrimStack(
    PcpPrimIndex* index,
    SdfSiteVector* dependencySites,
    PcpNodeRefVector* dependencyNodes);

// Updates the prim stack and related flags in \p index,  and returns the sites 
// of prims that \p index depends on in \p dependencySites, as well as the 
// nodes from which these prims originated in \p dependencyNodes.
void
Pcp_UpdatePrimStack(
    PcpPrimIndex* index,
    SdfSiteVector* dependencySites,
    PcpNodeRefVector* dependencyNodes);

#endif
