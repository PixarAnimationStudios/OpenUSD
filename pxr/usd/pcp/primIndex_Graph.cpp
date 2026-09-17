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
#include "pxr/base/tf/smallVector.h"

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
    , _hasNewNodes(false)
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
PcpPrimIndex_Graph::SetHasNewNodes(bool hasNewNodes)
{
    _hasNewNodes = hasNewNodes;
}

bool
PcpPrimIndex_Graph::HasNewNodes() const
{
    return _hasNewNodes;
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

    // We want to arrange the nodes in the node pool in strong-to-weak order, so
    // that strength-order iteration is a simple forward traversal.  We also
    // erase nodes that culling determined contribute nothing.  Both are just
    // permutations/removals over the node pool, so we build a single old-index
    // -> new-index mapping that performs both and apply it once.
    // _ApplyNodeIndexMapping then materializes only the surviving nodes, so we
    // never reference-count nodes that culling is about to discard.
    //
    // The eventual mapping is the only working space: one array of node
    // indexes, built in two passes over the same storage.  First we mark which
    // nodes are erasable, then the strength-order traversal overwrites each
    // mark with that node's new index.  Local capacity covers typical graph
    // sizes so we can avoid a heap allocation in the common case.
    //
    // DefaultInit leaves the elements uninitialized.
    // _ComputeEraseableNodeMarks seeds every element before reading any.  The
    // value-initializing constructor zero-fills first and gcc 11 for one
    // doesn't dead-store-eliminate it.
    using _NodeIndexData = TfSmallVector<_NodeIndexType, 64>;

    const size_t numNodes = _GetNumNodes();
    _NodeIndexData nodeIndexMapData(numNodes, _NodeIndexData::DefaultInit);
    const TfSpan<_NodeIndexType> nodeIndexMap(nodeIndexMapData);

    // Culling: nodeIndexMap[i] == _erasedIndex marks node i erasable.  The
    // erasable set is structural (origin/parent chains), independent of the
    // node ordering, so it is valid to compute it here on the pre-reorder pool.
    _ComputeErasableNodeMarks(nodeIndexMap);

    // Assign each surviving node its new index in strength order, rewriting the
    // erasure marks in place.
    size_t newNumNodes = 0;
    const bool isIdentity = _ComputeNewNodeIndexes(nodeIndexMap, &newNumNodes);

    // If there's mapping to do, apply it now.
    if (!isIdentity) {
        _ApplyNodeIndexMapping(nodeIndexMap, newNumNodes);
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
    TfSpan<const _NodeIndexType> nodeIndexMap, size_t newNumNodes)
{
    // Note: this builds a fresh node pool containing only the surviving nodes
    // and then reassigns _nodes to it (rather than calling
    // _DetachSharedNodePool()).  The source pool is only read (never mutated),
    // so a pool shared with another graph (e.g. the ancestral prim index this
    // one was cloned from) is left untouched.  This lets us reference-count
    // only the survivors instead of copy-constructing and bumping counts on the
    // whole pool and then dropping counts destroying the culled nodes after.

    const size_t InvalidIdx = _Node::_invalidNodeIndex;

    _NodePool const            &oldNodes    = *_nodes;
    std::vector<_UnsharedData> &oldUnshared = _unshared;

    const size_t oldNumNodes = oldNodes.size();

    TF_VERIFY(oldNumNodes == oldUnshared.size());
    TF_VERIFY(nodeIndexMap.size() == oldNumNodes);
    TF_VERIFY(newNumNodes <= oldNumNodes);

    // Get the new index for oldIndex -- nodeIndexMap[oldIndex] or InvalidIdx if
    // erased.  This helper passes InvalidIdx through unchanged so it can be
    // applied to a node's parent/origin links directly.
    auto getNewIndex = [&nodeIndexMap](size_t oldIndex) -> _NodeIndexType {
        return (oldIndex == InvalidIdx) ? InvalidIdx : nodeIndexMap[oldIndex];
    };

    // Build the new node pool with only the surviving nodes.  If the source
    // pool is uniquely owned we move each survivor's handles (no reference-
    // count traffic).  If it is shared we must copy, which reference-counts
    // only the surviors.  Note that _nodes cannot _gain_ use-counts
    // concurrently here; that can only happen after the graph is finalized and
    // published.  So if we start unique, we'll stay unique throughout.  It's
    // possible that we could _lose_ use-counts concurrently and become unique.
    // That's fine -- we just take the copy path in that case.
    const bool sharedPool = (_nodes.use_count() != 1);

    auto newNodesPtr = std::make_shared<_NodePool>(newNumNodes);
    _NodePool &newNodes = *newNodesPtr;
    std::vector<_UnsharedData> newUnshared(newNumNodes);

    for (size_t oldNodeIndex = 0; oldNodeIndex < oldNumNodes; ++oldNodeIndex) {
        const _NodeIndexType newNodeIndex = nodeIndexMap[oldNodeIndex];
        if (newNodeIndex == InvalidIdx) {
            continue;
        }

        _Node const &oldNode = oldNodes[oldNodeIndex];
        _Node       &newNode = newNodes[newNodeIndex];

        // Copy this node's links out first.  The Swap() below exchanges indexes
        // with the new node, so after oldNode.indexes holds the new node's
        // (default) links rather than the original ones.  These are six small
        // integers, so taking them by value is cheap.
        const _Node::_Indexes oldLinks = oldNode.indexes;

        if (sharedPool) {
            newNode.layerStack  = oldNode.layerStack;
            newNode.mapToRoot   = oldNode.mapToRoot;
            newNode.mapToParent = oldNode.mapToParent;
            newNode.smallInts   = oldNode.smallInts;
        }
        else {
            // Cast away constness just here to move the references.
            _Node &mutOldNode = const_cast<_Node &>(oldNode);
            newNode.Swap(mutOldNode);
        }

        // Remap the parent and origin links.  The child and sibling lists are
        // rebuilt below rather than remapped, so clear them here -- the Swap
        // above carries over the old node's links.
        _Node::_Indexes &newLinks = newNode.indexes;
        newLinks.arcParentIndex   = getNewIndex(oldLinks.arcParentIndex);
        newLinks.arcOriginIndex   = getNewIndex(oldLinks.arcOriginIndex);
        newLinks.firstChildIndex  = InvalidIdx;
        newLinks.lastChildIndex   = InvalidIdx;
        newLinks.prevSiblingIndex = InvalidIdx;
        newLinks.nextSiblingIndex = InvalidIdx;

        // A surviving node's parent must itself survive, so remapping the
        // parent only yields InvalidIdx for the root.  An origin link may point
        // to an erased node only if this node is erased too, which
        // _ComputeEraseCulledNodeMarks guarantees, so the same holds there.
        TF_VERIFY(newLinks.arcParentIndex != InvalidIdx || newNodeIndex == 0);
        TF_VERIFY(newLinks.arcOriginIndex != InvalidIdx ||
                  oldLinks.arcOriginIndex == InvalidIdx);

        newUnshared[newNodeIndex] = std::move(oldUnshared[oldNodeIndex]);
    }

    // Rebuild the child and sibling lists.  The new pool is in strength order,
    // which is a preorder traversal of those lists, so a node's children appear
    // after it in ascending order, and appending each node to its parent's list
    // in ascending index order reproduces the original sibling order.  Erased
    // nodes are simply never appended, splicing them out implicitly.
    for (size_t i = 1; i < newNumNodes; ++i) {
        _Node::_Indexes &
            childLinks = newNodes[i].indexes;
        _Node::_Indexes &
            parentLinks = newNodes[childLinks.arcParentIndex].indexes;

        if (parentLinks.firstChildIndex == InvalidIdx) {
            parentLinks.firstChildIndex = static_cast<_NodeIndexType>(i);
        }
        else {
            childLinks.prevSiblingIndex = parentLinks.lastChildIndex;
            newNodes[parentLinks.lastChildIndex]
                .indexes.nextSiblingIndex = static_cast<_NodeIndexType>(i);
        }
        parentLinks.lastChildIndex = static_cast<_NodeIndexType>(i);
    }

    _nodes    = std::move(newNodesPtr);
    _unshared = std::move(newUnshared);
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
    if (_nodes.use_count() != 1) {
        TRACE_FUNCTION();
        TfAutoMallocTag tag("_DetachSharedNodePool");
        _nodes = std::make_shared<_NodePool>(*_nodes);
    }
}

void 
PcpPrimIndex_Graph::_DetachSharedNodePoolForNewNodes(size_t numAddedNodes)
{
    if (_nodes.use_count() != 1) {
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
PcpPrimIndex_Graph::_ComputeNewNodeIndexes(
    TfSpan<_NodeIndexType> nodeIndexMap, size_t *numSurvivors) const
{
    TRACE_FUNCTION();

    // Strength order is a preorder traversal of the child lists starting at the
    // root, so walking the graph that way and handing out ascending indexes
    // assigns each surviving node its strength-ordered position.
    //
    // We can traverse without recursion or a stack by relying on the existing
    // parent links.  The walk descends the first child chain, and on reaching a
    // node with no children takes its next sibling, climbing parent links until
    // it finds a node that has one.
    _NodeIndexType nodeIdx = 0, nextIndex = 0;
    bool isIdentity = true;

    while (true) {
        // Write the new index for every kept node.  Reading and then
        // overwriting the node's own slot is safe because each node is visited
        // exactly once and no other node's slot is consulted here.
        if (nodeIndexMap[nodeIdx] != _erasedIndex) {
            isIdentity &= (nodeIdx == nextIndex);
            nodeIndexMap[nodeIdx] = nextIndex++;
        }

        _Node::_Indexes const *indexes = &_GetNode(nodeIdx).indexes;

        // Descend.
        if (indexes->firstChildIndex != _Node::_invalidNodeIndex) {
            nodeIdx = indexes->firstChildIndex;
            continue;
        }

        // This node has no children so its subtree is done.  Climb until a node
        // with an unvisited sibling turns up and go there.  Running out of
        // parents at the root means the whole graph is done.
        while (indexes->nextSiblingIndex == _Node::_invalidNodeIndex) {
            if (nodeIdx == 0) {  // reached the root.
                *numSurvivors = nextIndex;

                // The mapping is only the identity if it also erases nothing;
                // otherwise the pool has to shrink even when the surviving
                // nodes keep their indexes.
                return isIdentity && nextIndex == nodeIndexMap.size();
            }
            nodeIdx = indexes->arcParentIndex;
            indexes = &_GetNode(nodeIdx).indexes;
        }
        nodeIdx = indexes->nextSiblingIndex;
    }
}

void
PcpPrimIndex_Graph
::_ComputeErasableNodeMarks(TfSpan<_NodeIndexType> cullMarks) const
{
    TRACE_FUNCTION();

    // A surviving node's parent and origin must themselves survive: Pcp relies
    // on the chain of origins for strength ordering, so the keep-set is the
    // closure of the non-culled nodes under the two rules:
    //
    //     X kept -> parent(X) kept
    //     X kept -> origin(X) kept
    //
    // This function computes that closure, which is the smallest set satisfying
    // them -- every node in it is forced by some non-culled node.
    //
    // Parent links always point to a smaller index, so a descending pass
    // propagates the parent rule completely.  Origin links do not: they point
    // forward for a quarter to a half of all nodes in measured workloads, and a
    // forward write lands at an index the current sweep has already passed.  We
    // detect that case and sweep again, which keeps the pass free of any
    // assumption about link direction.  Nearly all graphs settle in one sweep.
    //
    // The repeated sweep is quadratic in the worst case -- as was the
    // origin-chain walk it replaced.  If a workload ever shows high sweep
    // counts, the cheap fix is to alternate the sweep direction once the first
    // few sweeps have not converged: an ascending sweep propagates forward
    // origin links in one pass, and only the already-passed test changes (idx >
    // i becomes idx < i).

    // Initialize every element before any reads -- callers may pass
    // uninitialized memory.  _erasedIndex = erasable, 0 = non-erasable.
    const size_t numNodes = _GetNumNodes();

    bool anyCulled = false;
    for (size_t i = 0; i < numNodes; ++i) {
        const bool culled = _unshared[i].culled;
        anyCulled = anyCulled || culled;
        cullMarks[i] = culled ? _erasedIndex : 0;
    }

    // With nothing culled there is nothing to erase and nothing to propagate.
    if (!anyCulled) {
        return;
    }

    // Keep a node's parent and origin, reporting whether that flipped a mark at
    // an index this sweep has already passed.
    auto keep = [&cullMarks](size_t idx, size_t curIdx) {
        if (idx == _erasedIndex || cullMarks[idx] != _erasedIndex) {
            return false;
        }
        cullMarks[idx] = 0;
        return idx > curIdx;
    };

    bool wroteVisited;
    do {
        wroteVisited = false;
        for (size_t i = numNodes; i-- > 0; ) {
            if (cullMarks[i] == _erasedIndex) {
                continue;
            }
            _Node::_Indexes const &indexes = _GetNode(i).indexes;
            // Don't replace with ||.  A short-circuit skips the second call.
            wroteVisited |= keep(indexes.arcParentIndex, i);
            wroteVisited |= keep(indexes.arcOriginIndex, i);
        }
    } while (wroteVisited);
}

PXR_NAMESPACE_CLOSE_SCOPE
