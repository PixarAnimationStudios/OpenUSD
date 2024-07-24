//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/pxr.h"
#include "pxr/usd/pcp/primIndex_Graph.h"
#include "pxr/usd/pcp/arc.h"
#include "pxr/usd/pcp/diagnostic.h"
#include "pxr/usd/pcp/errors.h"
#include "pxr/usd/pcp/node_Iterator.h"
#include "pxr/usd/pcp/strengthOrdering.h"
#include "pxr/usd/pcp/types.h"

#include "pxr/base/trace/trace.h"
#include "pxr/base/tf/mallocTag.h"

PXR_NAMESPACE_OPEN_SCOPE

const size_t PcpPrimIndex_Graph::_Node::_invalidNodeIndex;

////////////////////////////////////////////////////////////

struct PcpPrimIndex_Graph::_ArcStrengthOrder {
    _ArcStrengthOrder(PcpPrimIndex_Graph* graph) : _graph(graph) { }

    bool operator()(size_t aIdx, size_t bIdx) const
    { 
        const PcpNodeRef a(_graph, aIdx);
        const PcpNodeRef b(_graph, bIdx);

        const int result = PcpCompareSiblingNodeStrength(a, b);
        if (!TF_VERIFY(result != 0,
                "Redundant nodes in prim index for <%s>",
                _graph->GetRootNode().GetPath().GetString().c_str())) {
                
            // This should never happen.  It means we have multiple nodes
            // with the same strength information.
            //
            // If this fails, one reason might be that we're processing
            // the same node multiple times, adding redundant arcs.
            // Such arcs will have identical strength, causing us to
            // get into here.  PCP_DIAGNOSTIC_VALIDATION provides
            // a way to detect this.
#ifdef PCP_DIAGNOSTIC_VALIDATION
            printf("\n------------------\n");
            printf("\nEntire graph was:\n");
            PcpDump(a.GetRootNode());
            PcpDumpDotGraph(a.GetRootNode(), "test.dot", true, true);
            printf("\nNode A:\n");
            PcpDump(a, /* recurse */ false);
            printf("\nNode B:\n");
            PcpDump(b, /* recurse */ false);
#endif // PCP_DIAGNOSTIC_VALIDATION

            return a < b;
        }

        return result == -1;
    }

private:
    PcpPrimIndex_Graph* _graph;
};

////////////////////////////////////////////////////////////

void
PcpPrimIndex_Graph::_Node::SetArc(const PcpArc& arc)
{
    TF_VERIFY(static_cast<size_t>(arc.siblingNumAtOrigin) <=
              ((1lu << _nodeIndexSize) - 1));
    TF_VERIFY(static_cast<size_t>(arc.namespaceDepth)     <=
              ((1lu << _depthSize) - 1));
    // Add one because -1 is specifically allowed to mean invalid.
    TF_VERIFY(arc.parent._GetNodeIndex() + 1 <= _invalidNodeIndex);
    TF_VERIFY(arc.origin._GetNodeIndex() + 1 <= _invalidNodeIndex);

    smallInts.arcType               = arc.type;
    smallInts.arcSiblingNumAtOrigin = arc.siblingNumAtOrigin;
    smallInts.arcNamespaceDepth     = arc.namespaceDepth;
    indexes.arcParentIndex          = arc.parent._GetNodeIndex();
    indexes.arcOriginIndex          = arc.origin._GetNodeIndex();

    if (arc.parent) {
        mapToParent = arc.mapToParent;
        mapToRoot   = arc.parent.GetMapToRoot().Compose(mapToParent);
    } else {
        mapToParent = mapToRoot = PcpMapExpression::Identity();
    }
}

PcpPrimIndex_GraphRefPtr
PcpPrimIndex_Graph::New(const PcpLayerStackSite& rootSite, bool usd)
{
    TfAutoMallocTag2 tag("Pcp", "PcpPrimIndex_Graph");

    return TfCreateRefPtr(new PcpPrimIndex_Graph(rootSite, usd));
}

PcpPrimIndex_GraphRefPtr 
PcpPrimIndex_Graph::New(const PcpPrimIndex_GraphRefPtr& copy)
{
    TfAutoMallocTag2 tag("Pcp", "PcpPrimIndex_Graph");

    TRACE_FUNCTION();

    return TfCreateRefPtr(new PcpPrimIndex_Graph(*get_pointer(copy)));
}

PcpPrimIndex_Graph::PcpPrimIndex_Graph(const PcpLayerStackSite& rootSite,
                                       bool usd)
    : _nodes(std::make_shared<_NodePool>())
    , _hasPayloads(false)
    , _instanceable(false)
    , _finalized(false)
    , _usd(usd)
{
    PcpArc rootArc;
    rootArc.type = PcpArcTypeRoot;
    rootArc.namespaceDepth = 0;
    rootArc.mapToParent = PcpMapExpression::Identity();
    _CreateNode(rootSite, rootArc);
}

void 
PcpPrimIndex_Graph::SetHasPayloads(bool hasPayloads)
{
    _hasPayloads = hasPayloads;
}

bool
PcpPrimIndex_Graph::HasPayloads() const
{
    return _hasPayloads;
}

void 
PcpPrimIndex_Graph::SetIsInstanceable(bool instanceable)
{
    _instanceable = instanceable;
}

bool
PcpPrimIndex_Graph::IsInstanceable() const
{
    return _instanceable;
}

PcpNodeRef
PcpPrimIndex_Graph::GetRootNode() const
{
    return PcpNodeRef(const_cast<PcpPrimIndex_Graph*>(this), 0);
}

PcpNodeRef 
PcpPrimIndex_Graph::GetNodeUsingSite(const PcpLayerStackSite& site) const
{
    TRACE_FUNCTION();

    for (size_t i = 0, numNodes = _nodes->size(); i != numNodes; ++i) {
        const _Node& node = (*_nodes)[i]; 
        if (!(node.smallInts.inert || _unshared[i].culled)
            && node.layerStack == site.layerStack
            && _unshared[i].sitePath == site.path) {
            return PcpNodeRef(const_cast<PcpPrimIndex_Graph*>(this), i);
        }
    }

    return PcpNodeRef();
}

template <class Predicate>
std::pair<size_t, size_t>
PcpPrimIndex_Graph::_FindRootChildRange(
    const Predicate& pred) const
{
    const _Node& rootNode = _GetNode(0);
    for (size_t startIdx = rootNode.indexes.firstChildIndex;
         startIdx != _Node::_invalidNodeIndex;
         startIdx = _GetNode(startIdx).indexes.nextSiblingIndex) {

        if (!pred(PcpArcType(_GetNode(startIdx).smallInts.arcType))) {
            continue;
        }

        size_t endIdx = _GetNumNodes();
        for (size_t childIdx =_GetNode(startIdx).indexes.nextSiblingIndex;
             childIdx != _Node::_invalidNodeIndex;
             childIdx = _GetNode(childIdx).indexes.nextSiblingIndex) {
            
            if (!pred(PcpArcType(_GetNode(childIdx).smallInts.arcType))) {
                endIdx = childIdx;
                break; 
            }
        }

        return std::make_pair(startIdx, endIdx);
    }

    return std::make_pair(_GetNumNodes(), _GetNumNodes());
}

static PcpArcType
_GetArcTypeForRangeType(const PcpRangeType rangeType)
{
    switch (rangeType) {
    case PcpRangeTypeRoot:
        return PcpArcTypeRoot;
    case PcpRangeTypeInherit:
        return PcpArcTypeInherit;
    case PcpRangeTypeVariant:
        return PcpArcTypeVariant;
    case PcpRangeTypeReference:
        return PcpArcTypeReference;
    case PcpRangeTypePayload:
        return PcpArcTypePayload;
    case PcpRangeTypeSpecialize:
        return PcpArcTypeSpecialize;

    default:
        TF_CODING_ERROR("Unhandled range type");
        return PcpArcTypeRoot;
    }
}

std::pair<size_t, size_t> 
PcpPrimIndex_Graph::GetNodeIndexesForRange(PcpRangeType rangeType) const
{
    // This function essentially returns indexes that point into
    // this graph's node pool. That pool will not necessarily be sorted
    // in strength order unless this graph has been finalized. So, verify
    // that that's the case.
    TF_VERIFY(_finalized);

    std::pair<size_t, size_t> nodeRange(_GetNumNodes(), _GetNumNodes());

    switch (rangeType) {
    case PcpRangeTypeInvalid:
        TF_CODING_ERROR("Invalid range type specified");
        break;

    case PcpRangeTypeAll:
        nodeRange = std::make_pair(0, _GetNumNodes());
        break;
    case PcpRangeTypeWeakerThanRoot:
        nodeRange = std::make_pair(1, _GetNumNodes());
        break;
    case PcpRangeTypeStrongerThanPayload:
        nodeRange = _FindRootChildRange(
            [](PcpArcType arcType) { return arcType == PcpArcTypePayload; });
        nodeRange = std::make_pair(0, nodeRange.first);
        break;

    case PcpRangeTypeRoot:
        nodeRange = std::make_pair(0, 1);
        break;
    default:
        nodeRange = _FindRootChildRange(
            [rangeType](PcpArcType arcType) {
                return arcType == _GetArcTypeForRangeType(rangeType);
            });
        break;
    };

    return nodeRange;
}

size_t 
PcpPrimIndex_Graph::GetNodeIndexForNode(const PcpNodeRef &node) const
{
    return node.GetOwningGraph() == this
        ? node._GetNodeIndex()
        : _GetNumNodes();
}

std::pair<size_t, size_t> 
PcpPrimIndex_Graph::GetNodeIndexesForSubtreeRange(
    const PcpNodeRef &subtreeRootNode) const
{
    if (subtreeRootNode.GetOwningGraph() != this) {
        return std::make_pair(_GetNumNodes(), _GetNumNodes());
    }

    // Range always starts at subtree root node index.
    const size_t subtreeRootIndex = subtreeRootNode._GetNodeIndex();

    // Find the index of the last node in the subtree.
    size_t lastSubtreeIndex = subtreeRootIndex;
    while (true) {
        const _Node &node = _GetNode(lastSubtreeIndex);
        // This node is the last node in the subtree if it has no children, 
        // otherwise the last node in subtree is or is under this node's last 
        // child.
        if (node.indexes.lastChildIndex == _Node::_invalidNodeIndex) {
            break;
        } else {
            lastSubtreeIndex = node.indexes.lastChildIndex;
        }
    }

    return std::make_pair(subtreeRootIndex, lastSubtreeIndex + 1);
}

void
PcpPrimIndex_Graph::Finalize()
{
    TRACE_FUNCTION();

    if (_finalized) {
        return;
    }

    // We want to store the nodes in the node pool in strong-to-weak order.
    // In particular, this allows strength-order iteration over the nodes in 
    // the graph to be a simple traversal of the pool. So, we compute the
    // strength ordering of our nodes and reorder the pool if needed.
    std::vector<size_t> nodeIndexToStrengthOrder;
    const bool nodeOrderMatchesStrengthOrder = 
        _ComputeStrengthOrderIndexMapping(&nodeIndexToStrengthOrder);
    if (!nodeOrderMatchesStrengthOrder) {
        _ApplyNodeIndexMapping(nodeIndexToStrengthOrder);
    }

    // There may be nodes in the pool that have been marked for culling that
    // can be erased from the node pool. Compute and apply the necessary
    // transformation.
    std::vector<size_t> culledNodeMapping;
    const bool hasNodesToCull =
        _ComputeEraseCulledNodeIndexMapping(&culledNodeMapping);
    if (hasNodesToCull) {
        _ApplyNodeIndexMapping(culledNodeMapping);
    }

    _finalized = true;
}

// Several helper macros to make it easier to access indexes for other
// nodes.
#define PARENT(node) node.indexes.arcParentIndex
#define ORIGIN(node) node.indexes.arcOriginIndex
#define FIRST_CHILD(node) node.indexes.firstChildIndex
#define LAST_CHILD(node) node.indexes.lastChildIndex
#define NEXT_SIBLING(node) node.indexes.nextSiblingIndex
#define PREV_SIBLING(node) node.indexes.prevSiblingIndex

void 
PcpPrimIndex_Graph::_ApplyNodeIndexMapping(
    const std::vector<size_t>& nodeIndexMap)
{
    // Ensure this node pool is unshared first.
    _DetachSharedNodePool();
    
    _NodePool& oldNodes = *_nodes;
    std::vector<_UnsharedData> &oldUnshared = _unshared;

    TF_VERIFY(oldNodes.size() == oldUnshared.size());
    TF_VERIFY(nodeIndexMap.size() == oldNodes.size());

    const size_t numNodesToErase = 
        std::count(nodeIndexMap.begin(), nodeIndexMap.end(), 
                   _Node::_invalidNodeIndex);

    const size_t oldNumNodes = oldNodes.size();
    const size_t newNumNodes = oldNumNodes - numNodesToErase;
    TF_VERIFY(newNumNodes <= oldNumNodes);

    struct _ConvertOldToNewIndex {
        _ConvertOldToNewIndex(const std::vector<size_t>& table,
                              size_t numNewNodes) : _table(table)
        {
            for (size_t i = 0, n = _table.size(); i != n; ++i) {
                TF_VERIFY(_table[i] < numNewNodes || 
                          _table[i] == _Node::_invalidNodeIndex);
            }
        }

        size_t operator()(size_t oldIndex) const
        {
            if (oldIndex != _Node::_invalidNodeIndex) {
                return _table[oldIndex];
            }
            else {
                return oldIndex;
            }
        }
        const std::vector<size_t>& _table;

    };

    const _ConvertOldToNewIndex convertToNewIndex(nodeIndexMap, newNumNodes);

    // If this mapping causes nodes to be erased, it's much more convenient
    // to fix up node indices to accommodate those erasures in the old node
    // pool before moving nodes to their new position. 
    if (numNodesToErase > 0) {
        _NodePool &nodes = *_nodes;
        for (size_t i = 0; i < oldNumNodes; ++i) {
            const size_t oldNodeIndex = i;
            const size_t newNodeIndex = convertToNewIndex(oldNodeIndex);

            _Node& node = nodes[oldNodeIndex];

            // Sanity-check: If this node isn't going to be erased, its parent
            // can't be erased either.
            const bool nodeWillBeErased = 
                (newNodeIndex == _Node::_invalidNodeIndex);
            if (!nodeWillBeErased) {
                const bool parentWillBeErased = 
                    PARENT(node) != _Node::_invalidNodeIndex &&
                    convertToNewIndex(PARENT(node)) == _Node::_invalidNodeIndex;
                TF_VERIFY(!parentWillBeErased);
                continue;
            }

            if (PREV_SIBLING(node) != _Node::_invalidNodeIndex) {
                _Node& prevNode = nodes[PREV_SIBLING(node)];
                NEXT_SIBLING(prevNode) = NEXT_SIBLING(node);
            }
            if (NEXT_SIBLING(node) != _Node::_invalidNodeIndex) {
                _Node& nextNode = nodes[NEXT_SIBLING(node)];
                PREV_SIBLING(nextNode) = PREV_SIBLING(node);
            }

            _Node& parentNode = nodes[PARENT(node)];
            if (FIRST_CHILD(parentNode) == oldNodeIndex) {
                FIRST_CHILD(parentNode) = NEXT_SIBLING(node);
            }
            if (LAST_CHILD(parentNode) == oldNodeIndex) {
                LAST_CHILD(parentNode) = PREV_SIBLING(node);
            }
        }
    }

    // Swap nodes into their new position.
    _NodePool nodesAfterMapping(newNumNodes);
    std::vector<_UnsharedData> unsharedAfterMapping(newNumNodes);

    for (size_t i = 0; i < oldNumNodes; ++i) {
        const size_t oldNodeIndex = i;
        const size_t newNodeIndex = convertToNewIndex(oldNodeIndex);
        if (newNodeIndex == _Node::_invalidNodeIndex) {
            continue;
        }

        // Swap the node from the old node pool into the new node pool at
        // the desired location.
        _Node& oldNode = oldNodes[oldNodeIndex];
        _Node& newNode = nodesAfterMapping[newNodeIndex];
        newNode.Swap(oldNode);

        PARENT(newNode)       = convertToNewIndex(PARENT(newNode));
        ORIGIN(newNode)       = convertToNewIndex(ORIGIN(newNode));
        FIRST_CHILD(newNode)  = convertToNewIndex(FIRST_CHILD(newNode));
        LAST_CHILD(newNode)   = convertToNewIndex(LAST_CHILD(newNode));
        PREV_SIBLING(newNode) = convertToNewIndex(PREV_SIBLING(newNode));
        NEXT_SIBLING(newNode) = convertToNewIndex(NEXT_SIBLING(newNode));

        // Copy the corresponding unshared data.
        unsharedAfterMapping[newNodeIndex] = oldUnshared[oldNodeIndex];
    }

    _nodes->swap(nodesAfterMapping);
    _unshared.swap(unsharedAfterMapping);
}
    
void 
PcpPrimIndex_Graph::AppendChildNameToAllSites(const SdfPath& childPath)
{
    const SdfPath &parentPath = childPath.GetParentPath();
    for (_UnsharedData &unshared: _unshared) {
        if (unshared.sitePath == parentPath) {
            unshared.sitePath = childPath;
        } else {
            unshared.sitePath =
                unshared.sitePath.AppendChild(childPath.GetNameToken());
        }
    }

    // Note that appending a child name doesn't require finalization
    // of the graph because doing so doesn't affect the strength ordering of 
    // nodes.
}

PcpNodeRef
PcpPrimIndex_Graph::InsertChildNode(
    const PcpNodeRef& parent, 
    const PcpLayerStackSite& site, const PcpArc& arc,
    PcpErrorBasePtr *error)
{
    TfAutoMallocTag2 tag("Pcp", "PcpPrimIndex_Graph");

    TF_VERIFY(arc.type != PcpArcTypeRoot);
    TF_VERIFY(arc.parent == parent);

    // Node capacity is limited by both NodeIndexBits and reservation
    // of the _invalidNodeIndex value.  Other fields are limited by
    // the number of bits allocated to represent them.
    if (_GetNumNodes() >= _Node::_invalidNodeIndex) {
        if (error) {
            *error = PcpErrorCapacityExceeded::New(
                PcpErrorType_IndexCapacityExceeded);
        }
        return PcpNodeRef();
    }
    if (arc.namespaceDepth >= (1<<_Node::_depthSize)) {
        if (error) {
            *error = PcpErrorCapacityExceeded::New(
                PcpErrorType_ArcNamespaceDepthCapacityExceeded);
        }
        return PcpNodeRef();
    }

    _DetachSharedNodePoolForNewNodes();

    const size_t parentNodeIdx = parent._GetNodeIndex();
    const size_t childNodeIdx = _CreateNode(site, arc);

    return _InsertChildInStrengthOrder(parentNodeIdx, childNodeIdx);
}

PcpNodeRef
PcpPrimIndex_Graph::InsertChildSubgraph(
    const PcpNodeRef& parent,
    const PcpPrimIndex_GraphRefPtr& subgraph, const PcpArc& arc,
    PcpErrorBasePtr *error)
{
    TfAutoMallocTag2 tag("Pcp", "PcpPrimIndex_Graph");

    TF_VERIFY(arc.type != PcpArcTypeRoot);
    TF_VERIFY(arc.parent == parent);

    // Node capacity is limited by NodeIndexBits and reservation
    // of _invalidNodeIndex.
    // Other capacity-limited fields were validated when
    // the nodes were added to the subgraph.
    if (_GetNumNodes() + subgraph->_GetNumNodes() >= _Node::_invalidNodeIndex) {
        if (error) {
            *error = PcpErrorCapacityExceeded::New(
                PcpErrorType_IndexCapacityExceeded);
        }
        return PcpNodeRef();
    }

    PcpPrimIndex_Graph const &subgraphRef = *get_pointer(subgraph);
    _DetachSharedNodePoolForNewNodes(subgraphRef._GetNumNodes());

    const size_t parentNodeIdx = parent._GetNodeIndex();
    const size_t childNodeIdx = _CreateNodesForSubgraph(subgraphRef, arc);

    return _InsertChildInStrengthOrder(parentNodeIdx, childNodeIdx);
}

PcpNodeRef
PcpPrimIndex_Graph::_InsertChildInStrengthOrder(
    size_t parentNodeIdx, size_t childNodeIdx)
{
    TF_VERIFY(parentNodeIdx < _GetNumNodes());
    TF_VERIFY(childNodeIdx < _GetNumNodes());

    // Insert the child in the list of children, maintaining
    // the relative strength order.
    _NodePool &nodes = *_nodes;
    _Node& parentNode = nodes[parentNodeIdx];
    _Node& childNode  = nodes[childNodeIdx];
    _ArcStrengthOrder comp(this);
    if (FIRST_CHILD(parentNode) == _Node::_invalidNodeIndex) {
        // No children yet so this is the first child.
        TF_VERIFY(LAST_CHILD(parentNode) == _Node::_invalidNodeIndex);

        FIRST_CHILD(parentNode) =
        LAST_CHILD(parentNode)  = childNodeIdx;
    }
    else if (comp(childNodeIdx, FIRST_CHILD(parentNode))) {
        // New first child.
        TF_VERIFY(LAST_CHILD(parentNode) != _Node::_invalidNodeIndex);

        _Node& nextNode = nodes[FIRST_CHILD(parentNode)];
        NEXT_SIBLING(childNode) = FIRST_CHILD(parentNode);
        PREV_SIBLING(nextNode)  = childNodeIdx;
        FIRST_CHILD(parentNode) = childNodeIdx;
    }
    else if (!comp(childNodeIdx, LAST_CHILD(parentNode))) {
        // New last child.
        _Node& prevNode = nodes[LAST_CHILD(parentNode)];
        PREV_SIBLING(childNode) = LAST_CHILD(parentNode);
        NEXT_SIBLING(prevNode)  = childNodeIdx;
        LAST_CHILD(parentNode)  = childNodeIdx;
    }
    else {
        // Child goes somewhere internal to the sibling linked list.
        for (size_t index = FIRST_CHILD(parentNode);
                index != _Node::_invalidNodeIndex;
                index = NEXT_SIBLING(nodes[index])) {
            if (comp(childNodeIdx, index)) {
                _Node& nextNode = nodes[index];
                TF_VERIFY(PREV_SIBLING(nextNode) != _Node::_invalidNodeIndex);
                _Node& prevNode =nodes[PREV_SIBLING(nextNode)];
                PREV_SIBLING(childNode) = PREV_SIBLING(nextNode);
                NEXT_SIBLING(childNode) = index;
                PREV_SIBLING(nextNode)  = childNodeIdx;
                NEXT_SIBLING(prevNode)  = childNodeIdx;
                break;
            }
        }
    }

    return PcpNodeRef(this, childNodeIdx);
}

void 
PcpPrimIndex_Graph::_DetachSharedNodePool()
{
    if (!_nodes.unique()) {
        TRACE_FUNCTION();
        TfAutoMallocTag tag("_DetachSharedNodePool");
        _nodes = std::make_shared<_NodePool>(*_nodes);
    }
}

void 
PcpPrimIndex_Graph::_DetachSharedNodePoolForNewNodes(size_t numAddedNodes)
{
    if (!_nodes.unique()) {
        TRACE_FUNCTION();
        TfAutoMallocTag tag("_DetachSharedNodePoolForNewNodes");
        // Create a new copy, but with some extra capacity since we are adding
        // new nodes.  If we just created a copy, its capacity will be the same
        // as its size, so when we add a new node, the vector will have to
        // reallocate and copy everything again anyway.  This way we can avoid
        // that.
        size_t nodesSize = _nodes->size();
        auto newNodes = std::make_shared<_NodePool>();

        // If numAddedNodes is -1, that means the caller doesn't know how many
        // nodes will be added -- just increase the size by 25% in that case.
        if (numAddedNodes == size_t(-1)) {
            numAddedNodes = std::max(size_t(1), nodesSize / 4);
        }        
        newNodes->reserve(nodesSize + numAddedNodes);
        newNodes->insert(newNodes->begin(), _nodes->begin(), _nodes->end());
        _nodes = newNodes;
    }
}

size_t
PcpPrimIndex_Graph::_CreateNode(
    const PcpLayerStackSite& site, const PcpArc& arc)
{
    _unshared.emplace_back(site.path);
    _nodes->emplace_back();
    _finalized = false;

    _Node& node = _nodes->back();
    node.layerStack = site.layerStack;
    node.SetArc(arc);

    return _nodes->size() - 1;
}

size_t
PcpPrimIndex_Graph::_CreateNodesForSubgraph(
    const PcpPrimIndex_Graph& subgraph, const PcpArc& arc)
{
    // The subgraph's root should never have a parent or origin node; we
    // rely on this invariant below.
    TF_VERIFY(!subgraph.GetRootNode().GetParentNode() &&
              !subgraph.GetRootNode().GetOriginNode());

    // Insert a copy of all of the node data in the given subgraph into our
    // node pool.
    const size_t oldNumNodes = _GetNumNodes();
    _finalized = false;
    _nodes->insert(_nodes->end(),
                   subgraph._nodes->begin(), subgraph._nodes->end());
    _unshared.insert(_unshared.end(),
                     subgraph._unshared.begin(), subgraph._unshared.end());
        
    const size_t newNumNodes = _GetNumNodes();
    const size_t subgraphRootNodeIndex = oldNumNodes;

    // Set the arc connecting the root of the subgraph to the rest of the
    // graph.
    _NodePool &nodes = *_nodes;
    _Node& subgraphRoot = nodes[subgraphRootNodeIndex];
    subgraphRoot.SetArc(arc);

    // XXX: This is very similar to code in _ApplyNodeIndexMapping that
    //      adjust node references. There must be a good way to factor
    //      all of that out...

    // Iterate over all of the newly-copied nodes and adjust references to
    // other nodes in the node pool.
    struct _ConvertOldToNewIndex {
        _ConvertOldToNewIndex(size_t base, size_t numNewNodes) :
            _base(base), _numNewNodes(numNewNodes) { }
        size_t operator()(size_t oldIndex) const
        {
            if (oldIndex != _Node::_invalidNodeIndex) {
                TF_VERIFY(oldIndex + _base < _numNewNodes);
                return oldIndex + _base;
            }
            else {
                return oldIndex;
            }
        }
        size_t _base;
        size_t _numNewNodes;
    };
    const _ConvertOldToNewIndex convertToNewIndex(subgraphRootNodeIndex,
                                                  newNumNodes);

    for (size_t i = oldNumNodes; i < newNumNodes; ++i) {
        _Node& newNode = nodes[i];

        // Update the node's mapToRoot since it is now part of a new graph.
        if (i != subgraphRootNodeIndex) {
            newNode.mapToRoot =
                subgraphRoot.mapToRoot.Compose(newNode.mapToRoot);
        }

        // The parent and origin of the root of the newly-inserted subgraph 
        // don't need to be fixed up because it doesn't point to a node 
        // within the subgraph.
        if (i != subgraphRootNodeIndex) {
            PARENT(newNode) = convertToNewIndex(PARENT(newNode));
            ORIGIN(newNode) = convertToNewIndex(ORIGIN(newNode));
        }

        FIRST_CHILD(newNode)  = convertToNewIndex(FIRST_CHILD(newNode));
        LAST_CHILD(newNode)   = convertToNewIndex(LAST_CHILD(newNode));
        PREV_SIBLING(newNode) = convertToNewIndex(PREV_SIBLING(newNode));
        NEXT_SIBLING(newNode) = convertToNewIndex(NEXT_SIBLING(newNode));
    }

    return subgraphRootNodeIndex;
}

PcpPrimIndex_Graph::_Node& 
PcpPrimIndex_Graph::_GetWriteableNode(size_t idx)
{
    TF_VERIFY(idx < _GetNumNodes());
    _DetachSharedNodePool();
    return (*_nodes)[idx];
}

PcpPrimIndex_Graph::_Node& 
PcpPrimIndex_Graph::_GetWriteableNode(const PcpNodeRef& node)
{
    const size_t idx = node._GetNodeIndex();
    TF_VERIFY(idx < _GetNumNodes());
    _DetachSharedNodePool();
    return (*_nodes)[idx];
}

bool
PcpPrimIndex_Graph::_ComputeStrengthOrderIndexMapping(
    std::vector<size_t>* nodeIndexToStrengthOrder) const
{
    TRACE_FUNCTION();

    nodeIndexToStrengthOrder->resize(_GetNumNodes());

    const size_t rootNodeIdx = 0;
    size_t strengthIdx = 0;
    return _ComputeStrengthOrderIndexMappingRecursively(
        rootNodeIdx, &strengthIdx, nodeIndexToStrengthOrder);
}

bool
PcpPrimIndex_Graph::_ComputeStrengthOrderIndexMappingRecursively(
    size_t nodeIdx, 
    size_t* strengthIdx,
    std::vector<size_t>* nodeIndexToStrengthOrder) const
{
    bool nodeOrderMatchesStrengthOrder = true;

    (*nodeIndexToStrengthOrder)[nodeIdx] = *strengthIdx;
    nodeOrderMatchesStrengthOrder &= (nodeIdx == *strengthIdx);

    // Recurse down.
    _Node::_Indexes const &indexes = _GetNode(nodeIdx).indexes;
    size_t index = indexes.firstChildIndex;
    if (index != _Node::_invalidNodeIndex) {
        (*strengthIdx)++;

        const bool nodeOrderMatchesStrengthOrderInSubtree =
            _ComputeStrengthOrderIndexMappingRecursively(
                index, strengthIdx, nodeIndexToStrengthOrder);

        nodeOrderMatchesStrengthOrder &= nodeOrderMatchesStrengthOrderInSubtree;
    }

    // Recurse across.
    index = indexes.nextSiblingIndex;
    if (index != _Node::_invalidNodeIndex) {
        (*strengthIdx)++;

        const bool nodeOrderMatchesStrengthOrderInSubtree =
            _ComputeStrengthOrderIndexMappingRecursively(
                index, strengthIdx, nodeIndexToStrengthOrder);

        nodeOrderMatchesStrengthOrder &= nodeOrderMatchesStrengthOrderInSubtree;
    }

    return nodeOrderMatchesStrengthOrder;
}

bool
PcpPrimIndex_Graph::_ComputeEraseCulledNodeIndexMapping(
    std::vector<size_t>* erasedIndexMapping) const
{
    TRACE_FUNCTION();

    // Figure out which of the nodes that are marked for culling can
    // actually be erased from the node pool.
    const size_t numNodes = _GetNumNodes();
    std::vector<bool> nodeCanBeErased(_unshared.size());
    for (size_t i = 0; i != _unshared.size(); ++i) {
        nodeCanBeErased[i] = _unshared[i].culled;
    }

    // If a node is marked for culling, but serves as the origin node for a 
    // node that is *not* culled, we can't erase it from the graph. Doing so
    // would break the chain of origins Pcp relies on for strength ordering.
    // So, we iterate through the nodes to detect this situation and mark
    // the appropriate nodes as un-erasable.
    //
    // XXX: This has some O(N^2) behavior, as we wind up visiting the nodes
    //      in the chain of origins multiple times. We could keep track of
    //      nodes we've already visited to avoid re-processing them.
    for (size_t i = 0; i < numNodes; ++i) {
        if (ORIGIN(_GetNode(i)) == _Node::_invalidNodeIndex) {
            continue;
        }

        // Follow origin chain until we find the first non-culled node.
        // All subsequent nodes in the chain cannot be erased. This also
        // means that the parents of those nodes cannot be erased.
        bool subsequentOriginsCannotBeCulled = false;
        for (size_t nIdx = i; ; nIdx = ORIGIN(_GetNode(nIdx))) {
            const bool nodeIsCulled = nodeCanBeErased[nIdx];
            if (!nodeIsCulled) {
                subsequentOriginsCannotBeCulled = true;
            }
            else if (nodeIsCulled && subsequentOriginsCannotBeCulled) {
                for (size_t pIdx = nIdx; 
                     pIdx != _Node::_invalidNodeIndex &&
                         nodeCanBeErased[pIdx];
                     pIdx = PARENT(_GetNode(pIdx))) {
                    nodeCanBeErased[pIdx] = false;
                }
            }

            if (ORIGIN(_GetNode(nIdx)) == PARENT(_GetNode(nIdx))) {
                break;
            }
        }
    }

    // Now that we've determined which nodes can and can't be erased,
    // create the node index mapping.
    const size_t numNodesToCull = 
        std::count(nodeCanBeErased.begin(), nodeCanBeErased.end(), true);
    if (numNodesToCull == 0) {
        return false;
    }

    size_t numCulledNodes = 0;
    erasedIndexMapping->resize(numNodes);
    for (size_t i = 0; i < numNodes; ++i) {
        if (nodeCanBeErased[i]) {
            (*erasedIndexMapping)[i] = _Node::_invalidNodeIndex;
            ++numCulledNodes;
        }
        else {
            (*erasedIndexMapping)[i] = i - numCulledNodes;
        }
    }

    return true;
}

PXR_NAMESPACE_CLOSE_SCOPE
