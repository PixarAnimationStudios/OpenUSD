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

#include "pxr/pxr.h"
#include "pxr/usd/pcp/primIndex_Graph.h"
#include "pxr/usd/pcp/arc.h"
#include "pxr/usd/pcp/diagnostic.h"
#include "pxr/usd/pcp/node_Iterator.h"
#include "pxr/usd/pcp/strengthOrdering.h"
#include "pxr/usd/pcp/types.h"

#include "pxr/base/tracelite/trace.h"
#include "pxr/base/tf/mallocTag.h"

#include <boost/bind.hpp>

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
    TF_VERIFY(static_cast<size_t>(arc.siblingNumAtOrigin) <= ((1lu << _childrenSize)  - 1));
    TF_VERIFY(static_cast<size_t>(arc.namespaceDepth)     <= ((1lu << _depthSize) - 1));
    // Add one because -1 is specifically allowed to mean invalid.
    TF_VERIFY(arc.parent._GetNodeIndex() + 1 <= _invalidNodeIndex);
    TF_VERIFY(arc.origin._GetNodeIndex() + 1 <= _invalidNodeIndex);

    smallInts.arcType               = arc.type;
    smallInts.arcSiblingNumAtOrigin = arc.siblingNumAtOrigin;
    smallInts.arcNamespaceDepth     = arc.namespaceDepth;
    smallInts.arcParentIndex        = arc.parent._GetNodeIndex();
    smallInts.arcOriginIndex        = arc.origin._GetNodeIndex();

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
PcpPrimIndex_Graph::New(const PcpPrimIndex_GraphPtr& copy)
{
    TfAutoMallocTag2 tag("Pcp", "PcpPrimIndex_Graph");

    TRACE_FUNCTION();

    return TfCreateRefPtr(
        new PcpPrimIndex_Graph(*boost::get_pointer(copy)));
}

PcpPrimIndex_Graph::PcpPrimIndex_Graph(const PcpLayerStackSite& rootSite,
                                       bool usd)
    : _data(new _SharedData(usd))
{
    PcpArc rootArc;
    rootArc.type = PcpArcTypeRoot;
    rootArc.namespaceDepth = 0;
    rootArc.mapToParent = PcpMapExpression::Identity();
    _CreateNode(rootSite, rootArc);
}

PcpPrimIndex_Graph::PcpPrimIndex_Graph(const PcpPrimIndex_Graph& rhs)
    : _data(rhs._data)
    , _nodeSitePaths(rhs._nodeSitePaths)
    , _nodeHasSpecs(rhs._nodeHasSpecs)
{
    // There are no internal references to rhs in the nodes that we've
    // copied, so we don't need to do anything here.
}

bool 
PcpPrimIndex_Graph::IsUsd() const
{
    return _data->usd;
}

void 
PcpPrimIndex_Graph::SetHasPayload(bool hasPayload)
{
    if (_data->hasPayload != hasPayload) {
        _DetachSharedNodePool();
        _data->hasPayload = hasPayload;
    }
}

bool
PcpPrimIndex_Graph::HasPayload() const
{
    return _data->hasPayload;
}

void 
PcpPrimIndex_Graph::SetIsInstanceable(bool instanceable)
{
    if (_data->instanceable != instanceable) {
        _DetachSharedNodePool();
        _data->instanceable = instanceable;
    }
}

bool
PcpPrimIndex_Graph::IsInstanceable() const
{
    return _data->instanceable;
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

    for (size_t i = 0, numNodes = _data->nodes.size(); i != numNodes; ++i) {
        const _Node& node = _data->nodes[i]; 
        if (!(node.smallInts.inert || node.smallInts.culled)
            && node.layerStack == site.layerStack
            && _nodeSitePaths[i] == site.path) {
            return PcpNodeRef(const_cast<PcpPrimIndex_Graph*>(this), i);
        }
    }

    return PcpNodeRef();
}

template <class Predicate>
std::pair<size_t, size_t>
PcpPrimIndex_Graph::_FindDirectChildRange(
    const Predicate& pred) const
{
    const _Node& rootNode = _GetNode(0);
    for (size_t startIdx = rootNode.smallInts.firstChildIndex;
         startIdx != _Node::_invalidNodeIndex;
         startIdx = _GetNode(startIdx).smallInts.nextSiblingIndex) {

        if (!pred(PcpArcType(_GetNode(startIdx).smallInts.arcType))) {
            continue;
        }

        size_t endIdx = _GetNumNodes();
        for (size_t childIdx =_GetNode(startIdx).smallInts.nextSiblingIndex;
             childIdx != _Node::_invalidNodeIndex;
             childIdx = _GetNode(childIdx).smallInts.nextSiblingIndex) {
            
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
    case PcpRangeTypeLocalInherit:
        return PcpArcTypeLocalInherit;
    case PcpRangeTypeGlobalInherit:
        return PcpArcTypeGlobalInherit;
    case PcpRangeTypeVariant:
        return PcpArcTypeVariant;
    case PcpRangeTypeReference:
        return PcpArcTypeReference;
    case PcpRangeTypePayload:
        return PcpArcTypePayload;
    case PcpRangeTypeLocalSpecializes:
        return PcpArcTypeLocalSpecializes;
    case PcpRangeTypeGlobalSpecializes:
        return PcpArcTypeGlobalSpecializes;

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
    TF_VERIFY(_data->finalized);

    std::pair<size_t, size_t> nodeRange(_GetNumNodes(), _GetNumNodes());

    switch (rangeType) {
    case PcpRangeTypeInvalid:
        TF_CODING_ERROR("Invalid range type specified");
        break;

    case PcpRangeTypeAll:
        nodeRange = std::make_pair(0, _GetNumNodes());
        break;
    case PcpRangeTypeAllInherits:
        nodeRange = _FindDirectChildRange(PcpIsInheritArc);
        break;
    case PcpRangeTypeWeakerThanRoot:
        nodeRange = std::make_pair(1, _GetNumNodes());
        break;
    case PcpRangeTypeStrongerThanPayload:
        nodeRange = _FindDirectChildRange(
            [](PcpArcType arcType) { return arcType == PcpArcTypePayload; });
        nodeRange = std::make_pair(0, nodeRange.first);
        break;

    case PcpRangeTypeRoot:
        nodeRange = std::make_pair(0, 1);
        break;
    default:
        nodeRange = _FindDirectChildRange(
            [rangeType](PcpArcType arcType) {
                return arcType == _GetArcTypeForRangeType(rangeType);
            });
        break;
    };

    return nodeRange;
}

void
PcpPrimIndex_Graph::Finalize()
{
    TRACE_FUNCTION();

    if (_data->finalized) {
        return;
    }

    // We assume that the node pool being finalized is not being shared.
    // We'd have problems if the pool was being shared with other graphs at 
    // this point because we wouldn't be able to fix up the _nodeSitePaths 
    // member in those other graphs. That data is aligned with the node pool,
    // but is *not* shared.
    TF_VERIFY(_data.unique());

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

    _data->finalized = true;
}

// Several helper macros to make it easier to access indexes for other
// nodes.
#define PARENT(node) node.smallInts.arcParentIndex
#define ORIGIN(node) node.smallInts.arcOriginIndex
#define FIRST_CHILD(node) node.smallInts.firstChildIndex
#define LAST_CHILD(node) node.smallInts.lastChildIndex
#define NEXT_SIBLING(node) node.smallInts.nextSiblingIndex
#define PREV_SIBLING(node) node.smallInts.prevSiblingIndex

void 
PcpPrimIndex_Graph::_ApplyNodeIndexMapping(
    const std::vector<size_t>& nodeIndexMap)
{
    _NodePool& oldNodes = _data->nodes;
    SdfPathVector& oldSitePaths = _nodeSitePaths;
    std::vector<bool>& oldHasSpecs = _nodeHasSpecs;
    TF_VERIFY(oldNodes.size() == oldSitePaths.size() &&
              oldNodes.size() == oldHasSpecs.size());
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
        for (size_t i = 0; i < oldNumNodes; ++i) {
            const size_t oldNodeIndex = i;
            const size_t newNodeIndex = convertToNewIndex(oldNodeIndex);

            _Node& node = _data->nodes[oldNodeIndex];

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
                _Node& prevNode = _data->nodes[PREV_SIBLING(node)];
                NEXT_SIBLING(prevNode) = NEXT_SIBLING(node);
            }
            if (NEXT_SIBLING(node) != _Node::_invalidNodeIndex) {
                _Node& nextNode = _data->nodes[NEXT_SIBLING(node)];
                PREV_SIBLING(nextNode) = PREV_SIBLING(node);
            }

            _Node& parentNode = _data->nodes[PARENT(node)];
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
    SdfPathVector nodeSitePathsAfterMapping(newNumNodes);
    std::vector<bool> nodeHasSpecsAfterMapping(newNumNodes);

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

        // Copy the corresponding node site path.
        nodeSitePathsAfterMapping[newNodeIndex] = oldSitePaths[oldNodeIndex];
        nodeHasSpecsAfterMapping[newNodeIndex] = oldHasSpecs[oldNodeIndex];
    }

    _data->nodes.swap(nodesAfterMapping);
    _nodeSitePaths.swap(nodeSitePathsAfterMapping);
    _nodeHasSpecs.swap(nodeHasSpecsAfterMapping);
}
    
bool
PcpPrimIndex_Graph::IsFinalized() const
{
    return _data->finalized;
}

void 
PcpPrimIndex_Graph::AppendChildNameToAllSites(const SdfPath& childPath)
{
    const SdfPath &parentPath = childPath.GetParentPath();
    TF_FOR_ALL(it, _nodeSitePaths) {
        if (*it == parentPath) {
            *it = childPath;
        } else {
            *it = it->AppendChild(childPath.GetNameToken());
        }
    }

    // Note that appending a child name doesn't require finalization
    // of the graph because doing so doesn't affect the strength ordering of 
    // nodes.
}

PcpNodeRef
PcpPrimIndex_Graph::InsertChildNode(
    const PcpNodeRef& parent, 
    const PcpLayerStackSite& site, const PcpArc& arc)
{
    TfAutoMallocTag2 tag("Pcp", "PcpPrimIndex_Graph");

    TF_VERIFY(arc.type != PcpArcTypeRoot);
    TF_VERIFY(arc.parent == parent);

    _DetachSharedNodePool();

    const size_t parentNodeIdx = parent._GetNodeIndex();
    const size_t childNodeIdx = _CreateNode(site, arc);

    return _InsertChildInStrengthOrder(parentNodeIdx, childNodeIdx);
}

PcpNodeRef
PcpPrimIndex_Graph::InsertChildSubgraph(
    const PcpNodeRef& parent,
    const PcpPrimIndex_GraphPtr& subgraph, const PcpArc& arc)
{
    TfAutoMallocTag2 tag("Pcp", "PcpPrimIndex_Graph");

    TF_VERIFY(arc.type != PcpArcTypeRoot);
    TF_VERIFY(arc.parent == parent);

    _DetachSharedNodePool();

    const size_t parentNodeIdx = parent._GetNodeIndex();
    const size_t childNodeIdx = 
        _CreateNodesForSubgraph(*boost::get_pointer(subgraph), arc);

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
    _Node& parentNode = _data->nodes[parentNodeIdx];
    _Node& childNode  = _data->nodes[childNodeIdx];
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

        _Node& nextNode = _data->nodes[FIRST_CHILD(parentNode)];
        NEXT_SIBLING(childNode) = FIRST_CHILD(parentNode);
        PREV_SIBLING(nextNode)  = childNodeIdx;
        FIRST_CHILD(parentNode) = childNodeIdx;
    }
    else if (!comp(childNodeIdx, LAST_CHILD(parentNode))) {
        // New last child.
        _Node& prevNode = _data->nodes[LAST_CHILD(parentNode)];
        PREV_SIBLING(childNode) = LAST_CHILD(parentNode);
        NEXT_SIBLING(prevNode)  = childNodeIdx;
        LAST_CHILD(parentNode)  = childNodeIdx;
    }
    else {
        // Child goes somewhere internal to the sibling linked list.
        for (size_t index = FIRST_CHILD(parentNode);
                index != _Node::_invalidNodeIndex;
                index = NEXT_SIBLING(_data->nodes[index])) {
            if (comp(childNodeIdx, index)) {
                _Node& nextNode = _data->nodes[index];
                TF_VERIFY(PREV_SIBLING(nextNode) != _Node::_invalidNodeIndex);
                _Node& prevNode =_data->nodes[PREV_SIBLING(nextNode)];
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
    if (!_data.unique()) {
        TRACE_FUNCTION();
        _data.reset(new _SharedData(*_data));

        // XXX: This probably causes more finalization than necessary. Only
        //      need to finalize if (a) nodes are added (b) nodes are culled.
        _data->finalized = false;
    }
}

size_t
PcpPrimIndex_Graph::_CreateNode(
    const PcpLayerStackSite& site, const PcpArc& arc)
{
    _nodeSitePaths.push_back(site.path);
    _nodeHasSpecs.push_back(false);
    _data->nodes.push_back(_Node());
    _data->finalized = false;

    _Node& node = _data->nodes.back();
    node.layerStack = site.layerStack;
    node.SetArc(arc);

    return _data->nodes.size() - 1;
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
    _data->finalized = false;
    _data->nodes.insert(
        _data->nodes.end(), 
        subgraph._data->nodes.begin(), subgraph._data->nodes.end());
    _nodeSitePaths.insert(
        _nodeSitePaths.end(), 
        subgraph._nodeSitePaths.begin(), subgraph._nodeSitePaths.end());
    _nodeHasSpecs.insert(
        _nodeHasSpecs.end(),
        subgraph._nodeHasSpecs.begin(), subgraph._nodeHasSpecs.end());        
    const size_t newNumNodes = _GetNumNodes();
    const size_t subgraphRootNodeIndex = oldNumNodes;

    // Set the arc connecting the root of the subgraph to the rest of the
    // graph.
    _Node& subgraphRoot = _data->nodes[subgraphRootNodeIndex];
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
        _Node& newNode = _data->nodes[i];

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
    return _data->nodes[idx];
}

PcpPrimIndex_Graph::_Node& 
PcpPrimIndex_Graph::_GetWriteableNode(const PcpNodeRef& node)
{
    const size_t idx = node._GetNodeIndex();
    TF_VERIFY(idx < _GetNumNodes());
    _DetachSharedNodePool();
    return _data->nodes[idx];
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
    const _Node::_SmallInts& smallInts = _GetNode(nodeIdx).smallInts;
    size_t index = smallInts.firstChildIndex;
    if (index != _Node::_invalidNodeIndex) {
        (*strengthIdx)++;

        const bool nodeOrderMatchesStrengthOrderInSubtree =
            _ComputeStrengthOrderIndexMappingRecursively(
                index, strengthIdx, nodeIndexToStrengthOrder);

        nodeOrderMatchesStrengthOrder &= nodeOrderMatchesStrengthOrderInSubtree;
    }

    // Recurse across.
    index = smallInts.nextSiblingIndex;
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
    std::vector<bool> nodeCanBeErased(numNodes);
    for (size_t i = 0; i < numNodes; ++i) {
        nodeCanBeErased[i] = _GetNode(i).smallInts.culled;
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
