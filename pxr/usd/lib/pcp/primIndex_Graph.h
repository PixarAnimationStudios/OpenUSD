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
#ifndef PCP_PRIM_INDEX_GRAPH_H
#define PCP_PRIM_INDEX_GRAPH_H

#include "pxr/usd/pcp/iterator.h"
#include "pxr/usd/pcp/layerStack.h"
#include "pxr/usd/pcp/node.h"
#include "pxr/usd/pcp/mapExpression.h"
#include "pxr/usd/pcp/mapFunction.h"
#include "pxr/usd/pcp/types.h"
#include "pxr/usd/sdf/types.h"

#include "pxr/base/arch/attributes.h"
#include "pxr/base/tf/declarePtrs.h"
#include "pxr/base/tf/refBase.h"
#include "pxr/base/tf/weakBase.h"

#include <utility>
#include <vector>

class PcpArc;
class PcpLayerStackSite;

TF_DECLARE_WEAK_AND_REF_PTRS(PcpPrimIndex_Graph);

/// \class PcpPrimIndex_Graph
///
/// Internal representation of the graph used to represent sources of
/// opinions in the prim index.
///
class PcpPrimIndex_Graph 
    : public TfRefBase
    , public TfWeakBase
{
public:
    /// Creates a new graph with a root node for site \p rootSite.
    static PcpPrimIndex_GraphRefPtr New(const PcpLayerStackSite& rootSite,
                                        bool usd);

    /// Creates a new graph that is a clone of \p rhs.
    static PcpPrimIndex_GraphRefPtr New(const PcpPrimIndex_GraphPtr& rhs);

    /// Returns true if this graph was created in USD mode.
    bool IsUsd() const;

    /// Get/set whether this prim index has an authored payload.
    /// Note that it does not necessarily mean that the payload has been
    /// loaded if this is set to true.
    void SetHasPayload(bool hasPayload);
    bool HasPayload() const;

    /// Get/set whether this prim index is instanceable.
    void SetIsInstanceable(bool isInstanceable);
    bool IsInstanceable() const;

    /// Returns this graph's root node. This should always return a valid
    /// node.
    PcpNodeRef GetRootNode() const;

    /// Returns the indexes of the nodes that encompass all direct child
    /// nodes in the specified range as well as their descendants, in
    /// strong-to-weak order.
    ///
    /// By default, this returns a range encompassing the entire graph.
    std::pair<size_t, size_t> 
    GetNodeIndexesForRange(PcpRangeType rangeType = PcpRangeTypeAll) const;

    /// Returns a node from the graph that uses the given site and can
    /// contribute specs, if one exists. If multiple nodes in the graph 
    /// use the same site, the one that will be returned by this function 
    /// is undefined.
    PcpNodeRef GetNodeUsingSite(const PcpLayerStackSite& site) const;

    /// Appends the final element of \p childPath to each node's site path.
    /// This takes the entire childPath as an optimization -- it's often the
    /// case that the site paths are the parent path of childPath, in which case
    /// we can just reuse childPath instead of reassembling a new matching path.
    void AppendChildNameToAllSites(const SdfPath& childPath);

    /// Inserts a new node with site \p site as a child of \p parentNode,
    /// connected via \p arc.
    /// Returns the newly-added child node.
    PcpNodeRef InsertChildNode(
        const PcpNodeRef& parentNode,
        const PcpLayerStackSite& site, const PcpArc& arc);

    /// Inserts \p subgraph as a child of \p parentNode. The root node of 
    /// \p subgraph will be an immediate child of \p parentNode, connected via
    /// \p arc.
    /// Returns the root node of the newly-added subgraph.
    PcpNodeRef InsertChildSubgraph(
        const PcpNodeRef& parentNode,
        const PcpPrimIndex_GraphPtr& subgraph, const PcpArc& arc);

    /// Finalizes the graph. This optimizes internal data structures and
    /// should be called once the graph is fully generated.
    void Finalize();
    
    /// Return true if the graph is in a finalized state.
    bool IsFinalized() const;

    /// Get the SdSite from compressed site \p site.
    SdfSite GetSdSite(const Pcp_CompressedSdSite& site) const
    {
        return SdfSite(_GetNode(site.nodeIndex).
                                 layerStack->GetLayers()[site.layerIndex],
                             _nodeSitePaths[site.nodeIndex]);
    }

    /// Make an uncompressed site reference from compressed site \p site.
    Pcp_SdSiteRef GetSiteRef(const Pcp_CompressedSdSite& site) const
    {
        return Pcp_SdSiteRef(_GetNode(site.nodeIndex).
                                 layerStack->GetLayers()[site.layerIndex],
                             _nodeSitePaths[site.nodeIndex]);
    }

    /// Get a node from compressed site \p site.
    PcpNodeRef GetNode(const Pcp_CompressedSdSite& site)
    {
        TF_VERIFY(site.nodeIndex < _GetNumNodes());
        return PcpNodeRef(this, site.nodeIndex);
    }

private:
    // Forward declarations for internal data structures.
    struct _Arc;
    struct _ArcStrengthOrder;
    struct _Node;

    // Private constructors -- use New instead.
    PcpPrimIndex_Graph(const PcpLayerStackSite& rootSite, bool usd);
    PcpPrimIndex_Graph(const PcpPrimIndex_Graph& rhs);

    size_t _CreateNode(
        const PcpLayerStackSite& site, const PcpArc& arc);
    size_t _CreateNodesForSubgraph(
        const PcpPrimIndex_Graph& subgraph, const PcpArc& arc);

    PcpNodeRef _InsertChildInStrengthOrder(
        size_t parentNodeIdx, size_t childNodeIdx);

    void _DetachSharedNodePool();

    // Iterates through the immediate children of the root node looking
    // for the first node for which p(node) is true and the first subsequent
    // node where p(node) is false. Returns the indexes of the resulting 
    // nodes.
    template <class Predicate>
    std::pair<size_t, size_t> _FindDirectChildRange(const Predicate& p) const;

    // Helper functions to compute a mapping between node indexes and 
    // the strength order of the corresponding node. 
    //
    // Returns: 
    // True if the order of nodes in the node pool is the same as strength
    // ordering, false otherwise.
    // 
    // nodeIndexToStrengthOrder[i] => strength order of node at index i.
    bool _ComputeStrengthOrderIndexMapping(
        std::vector<size_t>* nodeIndexToStrengthOrder) const;
    bool _ComputeStrengthOrderIndexMappingRecursively(
        size_t nodeIdx, size_t* strengthIdx,
        std::vector<size_t>* nodeIndexToStrengthOrder) const;

    // Helper function to compute a node index mapping that erases nodes
    // that have been marked for culling.
    //
    // Returns:
    // True if any nodes marked for culling can be erased, false otherwise.
    // culledNodeMapping[i] => index of node i after culled nodes are erased.
    bool _ComputeEraseCulledNodeIndexMapping(
        std::vector<size_t>* culledNodeMapping) const;

    // Transforms the node pool by applying the given node index mapping.
    // References to to other nodes in the pool are fixed up appropriately.
    //
    // \p nodeIndexMap is a vector of the same size as the node pool, where
    // \p nodeIndexMap[i] => new position of node i. 
    // If \p nodeIndexMap[i] == _invalidNodeIndex, that node will be erased.
    void _ApplyNodeIndexMapping(const std::vector<size_t>& nodeIndexMap);

private:
    // PcpNodeRef is allowed to reach directly into the node pool to get/set
    // data.
    friend class PcpNodeRef;
    friend class PcpNodeRef_ChildrenIterator;
    friend class PcpNodeRef_ChildrenReverseIterator;
    friend class PcpNodeRef_PrivateChildrenConstIterator;
    friend class PcpNodeRef_PrivateChildrenConstReverseIterator;

    // NOTE: These accessors assume the consumer will be changing the node
    //       and may cause shared node data to be copied locally.
    _Node& _GetWriteableNode(size_t idx);
    _Node& _GetWriteableNode(const PcpNodeRef& node);

    size_t _GetNumNodes() const
    {
        return _data->nodes.size();
    }

    const _Node& _GetNode(size_t idx) const
    {
        TF_VERIFY(idx < _GetNumNodes());
        return _data->nodes[idx];
    }
    const _Node& _GetNode(const PcpNodeRef& node) const
    {
        return _GetNode(node._GetNodeIndex());
    }

private:
    // Allow Pcp_Statistics access to internal data for diagnostics.
    friend class Pcp_Statistics;

    struct _Node {
        static const size_t _nodeIndexSize = 15;
        static const size_t _childrenSize  = 10;
        static const size_t _depthSize     = 10;
        // These types should be just large enough to hold the above sizes.
        // This allows this structure to be packed into less space.
        typedef unsigned short _NodeIndexType;
        typedef unsigned short _ChildrenSizeType;
        typedef unsigned short _DepthSizeType;

        // Index used to represent an invalid node.
        static const size_t _invalidNodeIndex = (1lu << _nodeIndexSize) - 1lu;

        _Node()
            /* The equivalent initializations to the memset().
            , permission(SdfPermissionPublic)
            , hasSymmetry(false)
            , hasVariantSelections(false)
            , inert(false)
            , culled(false)
            , permissionDenied(false)
            , shouldContributeDependencies(false)
            , arcType(PcpArcTypeRoot)
            , arcSiblingNumAtOrigin(0)
            , arcNamespaceDepth(0)
            , arcParentIndex(0)
            , arcOriginIndex(0)
            */
        {
            memset(&smallInts, 0, sizeof(smallInts));
            smallInts.arcParentIndex   = _invalidNodeIndex;
            smallInts.arcOriginIndex   = _invalidNodeIndex;
            smallInts.firstChildIndex  = _invalidNodeIndex;
            smallInts.lastChildIndex   = _invalidNodeIndex;
            smallInts.prevSiblingIndex = _invalidNodeIndex;
            smallInts.nextSiblingIndex = _invalidNodeIndex;
        }

        void Swap(_Node& rhs)
        {
            std::swap(layerStack, rhs.layerStack);
            mapToRoot.Swap(rhs.mapToRoot);
            mapToParent.Swap(rhs.mapToParent);
            std::swap(smallInts, rhs.smallInts);
        }

        void SetArc(const PcpArc& arc);

        // NOTE: We pack all info into _Node, including stuff that
        //       would reasonably encapsulate in other types like
        //       info about the arc to the parent, so we can lay
        //       out the data in memory as tightly as possible.

        // The layer stack for this node.
        PcpLayerStackPtr layerStack;
        // Mapping function used to translate from this node directly
        // to the root node. This is essentially the composition of the 
        // mapToParent for every arc between this node and the root.
        PcpMapExpression mapToRoot;
        // The value-mapping function used to map values from this arc's source
        // node to its parent node.
        PcpMapExpression mapToParent;
        // Pack the non-byte sized integers into an unnamed structure.
        // This allows us to initialize them all at once.  g++ will,
        // suprisingly, initialize each individually in the default
        // copy constructor if they're direct members of _Node.
        struct _SmallInts {
            // The permissions for this node (whether specs on this node 
            // can be accessed from other nodes).
            SdfPermission permission:2;
            // Whether this node contributes symmetry information to
            // composition. This implies that prims at this node's site 
            // or at any of its namespace ancestors contain symmetry 
            // information.
            bool hasSymmetry:1;
            // Whether this node contains variant selections. This implies
            // that prims at this node's site or at any of its namespace
            // ancestors contain variant selections.
            bool hasVariantSelections:1;
            // Whether this node is inert. This is set to true in cases
            // where a node is needed to represent a structural dependency
            // but no opinions are allowed to be added.
            bool inert:1;
            // Whether this node was culled. This implies that no opinions
            // exist at this node and all child nodes. Because of this,
            // prim indexing does not need to expand this node to look for
            // other arcs.
            bool culled:1;
            // Whether this node is in violation of permission settings.
            // This is set to true when: we arrive at this node from a
            // node that was marked \c SdfPermissionPrivate, or we arrive
            // at this node from  another node that was denied permission.
            bool permissionDenied:1;
            // Whether this node should contribute specs for dependency
            // tracking. This is set to true in cases where this node is
            // not allowed to contribute opinions, but we still need to
            // know about specs for  dependency tracking.
            bool shouldContributeDependencies:1;
            // The type of the arc to the parent node.
            PcpArcType arcType:4;
            // Index among sibling arcs at origin; lower is stronger
            _ChildrenSizeType arcSiblingNumAtOrigin:_childrenSize;
            // Absolute depth in namespace of node that introduced this
            // node.  Note that this does *not* count any variant
            // selections.
            _DepthSizeType arcNamespaceDepth:_depthSize;

            // The following are padded to word size to avoid needing to
            // bit shift for read/write access and having to access two
            // words to read a value that straddles two machine words.
            // Note that bitfield layout should be examined when any
            // field is added, removed, or resized.

            // The index of the parent (or target) node of this arc.
            _NodeIndexType:0;
            _NodeIndexType arcParentIndex:_nodeIndexSize;
            // The index of the origin node of this arc.
            _NodeIndexType:0;
            _NodeIndexType arcOriginIndex:_nodeIndexSize;

            // The indexes of the first/last child, previous/next sibling.
            // The previous sibling index of a first child and the next
            // sibling index of a last child are _invalidNodeIndex (i.e.
            // they form a list, not a ring).
            _NodeIndexType:0;
            _NodeIndexType firstChildIndex:_nodeIndexSize;
            _NodeIndexType:0;
            _NodeIndexType lastChildIndex:_nodeIndexSize;
            _NodeIndexType:0;
            _NodeIndexType prevSiblingIndex:_nodeIndexSize;
            _NodeIndexType:0;
            _NodeIndexType nextSiblingIndex:_nodeIndexSize;
        }
        // Make this structure as small as possible.
#if defined(ARCH_COMPILER_GCC) || defined(ARCH_COMPILER_CLANG)
        __attribute__((packed))
#endif
        ;
        _SmallInts smallInts;
    };

    typedef std::vector<_Node> _NodePool;

    struct _SharedData {
        _SharedData(bool usd_) 
            : finalized(false)
            , usd(usd_)
            , hasPayload(false)
            , instanceable(false)
        { }

        // Pool of nodes for this graph. 
        _NodePool nodes;

        // Whether this node pool has been finalized.
        bool finalized:1;
        // Whether this prim index is composed in USD mode.
        bool usd:1;
        // Whether this prim index has an authored payload.
        bool hasPayload:1;
        // Whether this prim index is instanceable.
        bool instanceable:1;
    };

    // Container of graph data. PcpPrimIndex_Graph implements a 
    // copy-on-write scheme, so this data may be shared among multiple graph
    // instances.
    boost::shared_ptr<_SharedData> _data;

    // The following data is not included in the shared data object above
    // because they will typically differ between graph instances. Including
    // them in the shared data object would cause more graph instances to
    // be created.

    // Site paths for each node. Elements in this vector correspond to nodes
    // in the shared node pool. Together, _data->nodes[i].layerStack and 
    // _nodeSitePaths[i] form a node's site.
    std::vector<SdfPath> _nodeSitePaths;

    // Flags indicating whether a particular node has any specs to contribute
    // to the composed prim. Elements in this vector correspond to nodes in
    // the shared node pool.
    std::vector<bool> _nodeHasSpecs;
};

#endif // PCP_PRIM_INDEX_GRAPH_H
