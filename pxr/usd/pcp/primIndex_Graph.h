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
#ifndef PXR_USD_PCP_PRIM_INDEX_GRAPH_H
#define PXR_USD_PCP_PRIM_INDEX_GRAPH_H

#include "pxr/pxr.h"
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

#include <memory>
#include <utility>
#include <vector>

PXR_NAMESPACE_OPEN_SCOPE

class PcpArc;
class PcpLayerStackSite;

TF_DECLARE_REF_PTRS(PcpPrimIndex_Graph);

/// \class PcpPrimIndex_Graph
///
/// Internal representation of the graph used to represent sources of
/// opinions in the prim index.
///
class PcpPrimIndex_Graph : public TfSimpleRefBase
{
public:
    /// Creates a new graph with a root node for site \p rootSite.
    static PcpPrimIndex_GraphRefPtr New(const PcpLayerStackSite& rootSite,
                                        bool usd);

    /// Creates a new graph that is a clone of \p rhs.
    static PcpPrimIndex_GraphRefPtr New(const PcpPrimIndex_GraphRefPtr& rhs);

    /// Returns true if this graph was created in USD mode.
    bool IsUsd() const {
        return _usd;
    }

    /// Get/set whether this prim index has an authored payload.
    /// Note that it does not necessarily mean that the payload has been
    /// loaded if this is set to true.
    void SetHasPayloads(bool hasPayloads);
    bool HasPayloads() const;

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

    /// Returns the node index of the given \p node in this graph.
    ///
    /// If the node is not in this graph, this returns the end index of the 
    /// graph.
    size_t 
    GetNodeIndexForNode(const PcpNodeRef &node) const;

    /// Returns the indexes of the nodes that encompass the \p subtreeRootNode
    /// and all of its descendants in strong-to-weak order.
    std::pair<size_t, size_t> 
    GetNodeIndexesForSubtreeRange(const PcpNodeRef &subtreeRootNode) const;

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
    /// If the new node would exceeed the graph capacity, an invalid
    /// PcpNodeRef is returned.
    PcpNodeRef InsertChildNode(
        const PcpNodeRef& parentNode,
        const PcpLayerStackSite& site, const PcpArc& arc,
        PcpErrorBasePtr *error);

    /// Inserts \p subgraph as a child of \p parentNode. The root node of 
    /// \p subgraph will be an immediate child of \p parentNode, connected via
    /// \p arc.
    /// Returns the root node of the newly-added subgraph.
    /// If the new nodes would exceeed the graph capacity, an invalid
    /// PcpNodeRef is returned.
    PcpNodeRef InsertChildSubgraph(
        const PcpNodeRef& parentNode,
        const PcpPrimIndex_GraphRefPtr& subgraph, const PcpArc& arc,
        PcpErrorBasePtr *error);

    /// Finalizes the graph. This optimizes internal data structures and
    /// should be called once the graph is fully generated.
    void Finalize();
    
    /// Return true if the graph is in a finalized state.
    bool IsFinalized() const {
        return _finalized;
    }

    /// Get the SdSite from compressed site \p site.
    SdfSite GetSdSite(const Pcp_CompressedSdSite& site) const
    {
        return SdfSite(_GetNode(site.nodeIndex).
                       layerStack->GetLayers()[site.layerIndex],
                       _unshared[site.nodeIndex].sitePath);
    }

    /// Make an uncompressed site reference from compressed site \p site.
    Pcp_SdSiteRef GetSiteRef(const Pcp_CompressedSdSite& site) const
    {
        return Pcp_SdSiteRef(_GetNode(site.nodeIndex).
                             layerStack->GetLayers()[site.layerIndex],
                             _unshared[site.nodeIndex].sitePath);
    }

    /// Get a node from compressed site \p site.
    PcpNodeRef GetNode(const Pcp_CompressedSdSite& site)
    {
        TF_DEV_AXIOM(site.nodeIndex < _GetNumNodes());
        return PcpNodeRef(this, site.nodeIndex);
    }

private:
    // Forward declarations for internal data structures.
    struct _Arc;
    struct _ArcStrengthOrder;
    struct _Node;

    // Private constructors -- use New instead.
    PcpPrimIndex_Graph(const PcpLayerStackSite& rootSite, bool usd);
    PcpPrimIndex_Graph(const PcpPrimIndex_Graph& rhs) = default;

    size_t _CreateNode(
        const PcpLayerStackSite& site, const PcpArc& arc);
    size_t _CreateNodesForSubgraph(
        const PcpPrimIndex_Graph& subgraph, const PcpArc& arc);

    PcpNodeRef _InsertChildInStrengthOrder(
        size_t parentNodeIdx, size_t childNodeIdx);

    void _DetachSharedNodePool();
    void _DetachSharedNodePoolForNewNodes(size_t numAddedNodes = -1);

    // Iterates through the immediate children of the root node looking
    // for the first node for which p(node) is true and the first subsequent
    // node where p(node) is false. Returns the indexes of the resulting 
    // nodes.
    template <class Predicate>
    std::pair<size_t, size_t> _FindRootChildRange(const Predicate& p) const;

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
        return _nodes->size();
    }

    const _Node& _GetNode(size_t idx) const
    {
        TF_DEV_AXIOM(idx < _GetNumNodes());
        return (*_nodes)[idx];
    }
    const _Node& _GetNode(const PcpNodeRef& node) const
    {
        return _GetNode(node._GetNodeIndex());
    }

private:
    // Allow Pcp_Statistics access to internal data for diagnostics.
    friend class Pcp_Statistics;

    struct _Node {
        static const size_t _nodeIndexSize = 16;
        static const size_t _depthSize     = 16;
        // These types should be just large enough to hold the above sizes.
        // This allows this structure to be packed into less space.
        using _NodeIndexType = uint16_t;
        using _DepthSizeType = uint16_t;

        // Index used to represent an invalid node.
        static const size_t _invalidNodeIndex =
            static_cast<size_t>(_NodeIndexType(~0));

        _Node() noexcept
        /* The equivalent initializations to the ctor body -- gcc emits much
         * better code for the memset calls.
            , arcSiblingNumAtOrigin(0)
            , arcNamespaceDepth(0)
            , arcParentIndex(_invalidNodeIndex)
            , arcOriginIndex(_invalidNodeIndex)
            , firstChildIndex(_invalidNodeIndex)
            , lastChildIndex(_invalidNodeIndex)
            , prevSiblingIndex(_invalidNodeIndex)
            , nextSiblingIndex(_invalidNodeIndex)
            , arcType(PcpArcTypeRoot)
            , permission(SdfPermissionPublic)
            , hasSymmetry(false)
            , inert(false)
            , permissionDenied(false)
        */
        {
            memset(&smallInts, 0, sizeof(smallInts));
            memset(&indexes, ~0, sizeof(indexes));
        }

        void Swap(_Node& rhs) noexcept
        {
            std::swap(layerStack, rhs.layerStack);
            mapToRoot.Swap(rhs.mapToRoot);
            mapToParent.Swap(rhs.mapToParent);
            std::swap(indexes, rhs.indexes);
            std::swap(smallInts, rhs.smallInts);
        }

        void SetArc(const PcpArc& arc);

        // NOTE: We pack all info into _Node, including stuff that
        //       would reasonably encapsulate in other types like
        //       info about the arc to the parent, so we can lay
        //       out the data in memory as tightly as possible.

        // The layer stack for this node.
        PcpLayerStackRefPtr layerStack;
        // Mapping function used to translate from this node directly
        // to the root node. This is essentially the composition of the 
        // mapToParent for every arc between this node and the root.
        PcpMapExpression mapToRoot;
        // The value-mapping function used to map values from this arc's source
        // node to its parent node.
        PcpMapExpression mapToParent;

        struct _Indexes {
            // The index of the parent (or target) node of this arc.
            _NodeIndexType arcParentIndex;
            // The index of the origin node of this arc.
            _NodeIndexType arcOriginIndex;
            
            // The indexes of the first/last child, previous/next sibling.
            // The previous sibling index of a first child and the next
            // sibling index of a last child are _invalidNodeIndex (i.e.
            // they form a list, not a ring).
            _NodeIndexType firstChildIndex;
            _NodeIndexType lastChildIndex;
            _NodeIndexType prevSiblingIndex;
            _NodeIndexType nextSiblingIndex;
        };
        _Indexes indexes;

        // Pack the non-byte sized integers into an unnamed structure.
        // This allows us to initialize them all at once.  g++ will,
        // surprisingly, initialize each individually in the default
        // copy constructor if they're direct members of _Node.
        struct _SmallInts {
            // Index among sibling arcs at origin; lower is stronger
            _NodeIndexType arcSiblingNumAtOrigin;
            // Absolute depth in namespace of node that introduced this
            // node.  Note that this does *not* count any variant
            // selections.
            _DepthSizeType arcNamespaceDepth;

            // Some of the following fields use 8 bits when they could use fewer
            // because the compiler can often avoid shift & mask operations when
            // working with whole bytes.
            
            // The type of the arc to the parent node.
            PcpArcType arcType:8;
            // The permissions for this node (whether specs on this node 
            // can be accessed from other nodes).
            SdfPermission permission:8;
            // Whether this node contributes symmetry information to
            // composition. This implies that prims at this node's site 
            // or at any of its namespace ancestors contain symmetry 
            // information.
            bool hasSymmetry:1;
            // Whether this node is inert. This is set to true in cases
            // where a node is needed to represent a structural dependency
            // but no opinions are allowed to be added.
            bool inert:1;
            // Whether this node is in violation of permission settings.
            // This is set to true when: we arrive at this node from a
            // node that was marked \c SdfPermissionPrivate, or we arrive
            // at this node from  another node that was denied permission.
            bool permissionDenied:1;
        };
        _SmallInts smallInts;
    };

    // Pool of nodes for this graph. 
    using _NodePool = std::vector<_Node>;

    // Container of graph data. PcpPrimIndex_Graph implements a 
    // copy-on-write scheme, so this data may be shared among multiple graph
    // instances.
    std::shared_ptr<_NodePool> _nodes;

    // The following unshared data are not included in the shared data object
    // above because they will typically differ between graph
    // instances. Including them in the shared data object would cause more
    // graph instances to be created.
    struct _UnsharedData {
        _UnsharedData()
            : hasSpecs(false), culled(false), isDueToAncestor(false) {}
        explicit _UnsharedData(SdfPath const &p)
            : sitePath(p)
            , hasSpecs(false)
            , culled(false)
            , isDueToAncestor(false) {}
        explicit _UnsharedData(SdfPath &&p)
            : sitePath(std::move(p))
            , hasSpecs(false)
            , culled(false)
            , isDueToAncestor(false) {}

        // The site path for a particular node.
        SdfPath sitePath;

        // Whether or not a particular node has any specs to contribute to the
        // composed prim.
        bool hasSpecs:1;

        // Whether this node was culled. This implies that no opinions
        // exist at this node and all child nodes. Because of this,
        // prim indexing does not need to expand this node to look for
        // other arcs.
        bool culled:1;

        // Whether this node is copied from the namespace ancestor prim
        // index (true) or introduced here due to a direct arc (false)
        bool isDueToAncestor:1;
    };
    
    // Elements in this vector correspond to nodes in the shared node
    // pool. Together, (*_nodes)[i].layerStack and _unshared[i].sitePath form a
    // node's site.
    std::vector<_UnsharedData> _unshared;

    // Whether or not this graph reached any specs with authored payloads.
    bool _hasPayloads:1;

    // Whether or not this graph is considered 'instanceable'.
    bool _instanceable:1;

    // Whether or not this graph's node pool has been finalized.
    bool _finalized:1;

    // Whether or not this graph was composed in 'usd'-mode, which disables
    // certain features such as permissions, symmetry, etc.
    bool _usd:1;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_USD_PCP_PRIM_INDEX_GRAPH_H
