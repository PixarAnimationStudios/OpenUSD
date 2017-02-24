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
#include "pxr/usd/pcp/node.h"
#include "pxr/usd/pcp/primIndex_Graph.h"

PXR_NAMESPACE_OPEN_SCOPE

static inline int _GetNonVariantPathElementCount(const SdfPath &path);

bool 
PcpNodeRef::operator<(const PcpNodeRef& rhs) const
{
    if (_nodeIdx < rhs._nodeIdx)
        return true;
    if (_nodeIdx > rhs._nodeIdx)
        return false;

    return _graph < rhs._graph;
}

PcpPrimIndex_GraphPtr 
PcpNodeRef::GetOwningGraph() const
{
    return TfCreateWeakPtr(_graph);
}

void* 
PcpNodeRef::GetUniqueIdentifier() const
{
    return _graph + _nodeIdx;
}

PcpNodeRef
PcpNodeRef::InsertChildSubgraph(
    const PcpPrimIndex_GraphPtr& subgraph, const PcpArc& arc)
{
    return _graph->InsertChildSubgraph(*this, subgraph, arc);
}

PcpNodeRef 
PcpNodeRef::InsertChild(const PcpLayerStackSite& site, const PcpArc& arc)
{
    return _graph->InsertChildNode(*this, site, arc);
}

PcpNodeRef
PcpNodeRef::GetRootNode() const
{
    return _graph->GetRootNode();
}

PcpNodeRef
PcpNodeRef::GetOriginRootNode() const
{
    PcpNodeRef root(*this);
    while (root.GetOriginNode() &&
           root.GetOriginNode() != root.GetParentNode())
        root = root.GetOriginNode();

    return root;
}

#define PCP_DEFINE_GET_API(typeName, getter, varName)           \
    typeName PcpNodeRef::getter() const {                       \
        const PcpPrimIndex_Graph::_Node& graphNode =            \
            _graph->_GetNode(_nodeIdx);                         \
        return graphNode.varName;                               \
    }

// Same as PCP_DEFINE_GET_API() except specifically for retrieving
// node indices. Consumers expect this to return -1 for an invalid
// node, so we need to specifically check for the _invalidNodeIndex
// value.
#define PCP_DEFINE_GET_NODE_API(typeName, getter, varName)      \
    typeName PcpNodeRef::getter() const {                       \
        const PcpPrimIndex_Graph::_Node& graphNode =            \
            _graph->_GetNode(_nodeIdx);                         \
        if (graphNode.varName ==                                \
                PcpPrimIndex_Graph::_Node::_invalidNodeIndex)   \
            return (typeName)-1;                                \
        return graphNode.varName;                               \
    }

#define PCP_DEFINE_SET_API(typeName, setter, varName)           \
    void PcpNodeRef::setter(typeName val) {                     \
        const PcpPrimIndex_Graph::_Node& graphNode =            \
            _graph->_GetNode(_nodeIdx);                         \
        if (graphNode.varName != val) {                         \
            PcpPrimIndex_Graph::_Node& writeableGraphNode =     \
                _graph->_GetWriteableNode(_nodeIdx);            \
            writeableGraphNode.varName = val;                   \
        }                                                       \
    }

#define PCP_DEFINE_API(typeName, getter, setter, varName)       \
    PCP_DEFINE_GET_API(typeName, getter, varName)               \
    PCP_DEFINE_SET_API(typeName, setter, varName)

PCP_DEFINE_GET_API(PcpArcType, GetArcType, smallInts.arcType);
PCP_DEFINE_GET_API(int, GetNamespaceDepth, smallInts.arcNamespaceDepth);
PCP_DEFINE_GET_API(int, GetSiblingNumAtOrigin, smallInts.arcSiblingNumAtOrigin);
PCP_DEFINE_GET_API(const PcpMapExpression&, GetMapToParent, mapToParent);
PCP_DEFINE_GET_API(const PcpMapExpression&, GetMapToRoot, mapToRoot);

PCP_DEFINE_API(bool, HasSymmetry, SetHasSymmetry, smallInts.hasSymmetry);
PCP_DEFINE_API(SdfPermission, GetPermission, SetPermission, smallInts.permission);
PCP_DEFINE_API(bool, IsCulled, SetCulled, smallInts.culled);
PCP_DEFINE_API(bool, IsRestricted, SetRestricted, smallInts.permissionDenied);

PCP_DEFINE_SET_API(bool, SetInert, smallInts.inert);

PCP_DEFINE_GET_NODE_API(size_t, _GetParentIndex, smallInts.arcParentIndex);
PCP_DEFINE_GET_NODE_API(size_t, _GetOriginIndex, smallInts.arcOriginIndex);
PCP_DEFINE_GET_API(const PcpLayerStackRefPtr&, GetLayerStack, layerStack);

bool
PcpNodeRef::HasSpecs() const
{
    TF_VERIFY(_nodeIdx < _graph->_nodeHasSpecs.size());
    return _graph->_nodeHasSpecs[_nodeIdx];
}

void
PcpNodeRef::SetHasSpecs(bool hasSpecs)
{
    TF_VERIFY(_nodeIdx < _graph->_nodeHasSpecs.size());
    _graph->_nodeHasSpecs[_nodeIdx] = hasSpecs;
}

const SdfPath& 
PcpNodeRef::GetPath() const
{
    TF_VERIFY(_nodeIdx < _graph->_nodeSitePaths.size());
    return _graph->_nodeSitePaths[_nodeIdx];
}

PcpLayerStackSite
PcpNodeRef::GetSite() const
{
    return PcpLayerStackSite(GetLayerStack(), GetPath());
}

bool 
PcpNodeRef::IsDirect() const
{
    return GetArcType() == PcpArcTypeRoot;
}

bool
PcpNodeRef::IsInert() const
{
    const PcpPrimIndex_Graph::_Node& node = _graph->_GetNode(_nodeIdx);
    return node.smallInts.inert || node.smallInts.culled;
}

bool 
PcpNodeRef::CanContributeSpecs() const
{
    // No permissions in Usd mode, so skip restriction check.
    //
    // The logic here is equivalent to:
    //     (!IsInert() and (IsUsd() or not IsRestricted()))
    //
    // but it looks at the bits directly instead of going through those public
    // methods to avoid some unnecessary overhead.  This method is heavily used
    // so avoiding that overhead for the slight obfuscation is justified.

    const PcpPrimIndex_Graph::_Node& node = _graph->_GetNode(_nodeIdx);
    return !(node.smallInts.inert || node.smallInts.culled) &&
        (!node.smallInts.permissionDenied || _graph->_data->usd);
}

int
PcpNodeRef::GetDepthBelowIntroduction() const
{
    const PcpNodeRef parent = GetParentNode();
    if (!parent)
        return 0;

    return _GetNonVariantPathElementCount(parent.GetPath())
        - GetNamespaceDepth();
}

bool
PcpNodeRef::IsDueToAncestor() const
{
    return GetDepthBelowIntroduction() > 0;
}

SdfPath
PcpNodeRef::GetPathAtIntroduction() const
{
    SdfPath pathAtIntroduction = GetPath();
    for (int depth = GetDepthBelowIntroduction(); depth; --depth) {
        while (pathAtIntroduction.IsPrimVariantSelectionPath()) {
            // Skip over variant selections, since they do not
            // constitute levels of namespace depth. We do not simply
            // strip all variant selections here, because we want to
            // retain variant selections ancestral to the path where
            // this node was introduced.
            pathAtIntroduction = pathAtIntroduction.GetParentPath();
        }
        pathAtIntroduction = pathAtIntroduction.GetParentPath();
    }

    return pathAtIntroduction;
}

SdfPath
PcpNodeRef::GetIntroPath() const
{
    // Start with the parent node's current path.
    const PcpNodeRef parent = GetParentNode();
    if (!parent)
        return SdfPath::AbsoluteRootPath();
    SdfPath introPath = parent.GetPath();

    // Walk back up to the depth where this child was introduced.
    for (int depth = GetDepthBelowIntroduction(); depth; --depth) {
        while (introPath.IsPrimVariantSelectionPath()) {
            // Skip over variant selections, since they do not
            // constitute levels of namespace depth. We do not simply
            // strip all variant selections here, because we want to
            // retain variant selections ancestral to the path where
            // this node was introduced.
            introPath = introPath.GetParentPath();
        }
        introPath = introPath.GetParentPath();
    }

    return introPath;
}

PcpNodeRef::child_const_range
PcpNodeRef::GetChildrenRange() const
{
    PcpNodeRef node(_graph, _nodeIdx);
    return child_const_range(child_const_iterator(node, /* end = */ false),
                             child_const_iterator(node, /* end = */ true));
}

PcpNodeRef
PcpNodeRef::GetParentNode() const
{
    const size_t parentIndex = _GetParentIndex();
    return (parentIndex == PCP_INVALID_INDEX ? 
            PcpNodeRef() : PcpNodeRef(_graph, parentIndex));
}

PcpNodeRef
PcpNodeRef::GetOriginNode() const
{
    const size_t originIndex = _GetOriginIndex();
    return (originIndex == PCP_INVALID_INDEX ? 
            PcpNodeRef() : PcpNodeRef(_graph, originIndex));
}

////////////////////////////////////////////////////////////

PcpNodeRef_ChildrenIterator::PcpNodeRef_ChildrenIterator() :
    _index(PcpPrimIndex_Graph::_Node::_invalidNodeIndex)
{
    // Do nothing
}

PcpNodeRef_ChildrenIterator::PcpNodeRef_ChildrenIterator(
    const PcpNodeRef& node, bool end) :
    _node(node),
    _index(!end ?
            _node._graph->_GetNode(_node).smallInts.firstChildIndex :
            PcpPrimIndex_Graph::_Node::_invalidNodeIndex)
{
    // Do nothing
}

void 
PcpNodeRef_ChildrenIterator::increment()
{
    _index = _node._graph->_GetNode(_index).smallInts.nextSiblingIndex;
}

PcpNodeRef_ChildrenReverseIterator::PcpNodeRef_ChildrenReverseIterator() :
    _index(PcpPrimIndex_Graph::_Node::_invalidNodeIndex)
{
    // Do nothing
}

PcpNodeRef_ChildrenReverseIterator::PcpNodeRef_ChildrenReverseIterator(
    const PcpNodeRef_ChildrenIterator& i) :
    _node(i._node),
    _index(i._index)
{
    if (_index == PcpPrimIndex_Graph::_Node::_invalidNodeIndex) {
        _index = _node._graph->_GetNode(_node).smallInts.lastChildIndex;
    }
    else {
        increment();
    }
}

PcpNodeRef_ChildrenReverseIterator::PcpNodeRef_ChildrenReverseIterator(
    const PcpNodeRef& node, bool end) :
    _node(node),
    _index(!end ?
            _node._graph->_GetNode(_node).smallInts.lastChildIndex :
            PcpPrimIndex_Graph::_Node::_invalidNodeIndex)
{
    // Do nothing
}

void 
PcpNodeRef_ChildrenReverseIterator::increment()
{
    _index = _node._graph->_GetNode(_index).smallInts.prevSiblingIndex;
}

int
PcpNode_GetNonVariantPathElementCount(const SdfPath &path)
{
    return _GetNonVariantPathElementCount(path);
}

static inline int
_GetNonVariantPathElementCount(const SdfPath &path)
{
    //return path.StripAllVariantSelections().GetPathElementCount();
    if (ARCH_UNLIKELY(path.ContainsPrimVariantSelection())) {
        SdfPath cur(path);
        int result = (!cur.IsPrimVariantSelectionPath());
        cur = cur.GetParentPath();
        for (; cur.ContainsPrimVariantSelection(); cur = cur.GetParentPath())
            result += (!cur.IsPrimVariantSelectionPath());
        return result + static_cast<int>(cur.GetPathElementCount());
    } else {
        return static_cast<int>(path.GetPathElementCount());
    }
}

PXR_NAMESPACE_CLOSE_SCOPE
