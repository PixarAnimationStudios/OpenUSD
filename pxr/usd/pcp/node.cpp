//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
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

void* 
PcpNodeRef::GetUniqueIdentifier() const
{
    return _graph + _nodeIdx;
}

PcpNodeRef
PcpNodeRef::InsertChildSubgraph(
    const PcpPrimIndex_GraphRefPtr &subgraph, const PcpArc &arc,
    PcpErrorBasePtr *error)
{
    return _graph->InsertChildSubgraph(*this, subgraph, arc, error);
}

PcpNodeRef 
PcpNodeRef::InsertChild(const PcpLayerStackSite& site, const PcpArc& arc,
    PcpErrorBasePtr *error)
{
    return _graph->InsertChildNode(*this, site, arc, error);
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
PCP_DEFINE_API(bool, IsRestricted, _SetRestricted, smallInts.permissionDenied);

PCP_DEFINE_SET_API(bool, _SetInert, smallInts.inert);

PCP_DEFINE_GET_NODE_API(size_t, _GetParentIndex, indexes.arcParentIndex);
PCP_DEFINE_GET_NODE_API(size_t, _GetOriginIndex, indexes.arcOriginIndex);
PCP_DEFINE_GET_API(const PcpLayerStackRefPtr&, GetLayerStack, layerStack);

bool
PcpNodeRef::IsCulled() const
{
    TF_DEV_AXIOM(_nodeIdx < _graph->_unshared.size());
    return _graph->_unshared[_nodeIdx].culled;
}

void
PcpNodeRef::SetCulled(bool culled)
{
    TF_DEV_AXIOM(_nodeIdx < _graph->_unshared.size());
    
    const bool wasCulled = _graph->_unshared[_nodeIdx].culled;
    if (culled == wasCulled) {
        return;
    }

    // Have to set finalized to false if we cull anything.
    if (culled) {
        _graph->_finalized = false;
    }

    // If we've culled this node, we've definitely restricted contributions.
    // If we've unculled this node, some other flags may be restriction
    // contributions, so we don't know.
    _RecordRestrictionDepth(
        culled ? _Restricted::Yes : _Restricted::Unknown);

    _graph->_unshared[_nodeIdx].culled = culled;
}

void
PcpNodeRef::SetRestricted(bool restricted)
{
    const bool wasRestricted = IsRestricted();
    _SetRestricted(restricted);
    if (restricted != wasRestricted) {
        // If we set this node to restricted, we've definitely restricted
        // contributions. If we've unset restricted, some other flags
        // may be restricting contributions, so we don't know.
        _RecordRestrictionDepth(
            restricted ? _Restricted::Yes : _Restricted::Unknown);
    }
}

void
PcpNodeRef::SetInert(bool inert)
{
    const bool wasInert = IsInert();
    _SetInert(inert);
    if (inert != wasInert) {
        // If we set this node to inert, we've definitely restricted
        // contributions. If we've unset inert-ness, some other flags
        // may be restricting contributions, so we don't know.
        _RecordRestrictionDepth(
            inert ? _Restricted::Yes : _Restricted::Unknown);
    }
}

void
PcpNodeRef::_RecordRestrictionDepth(_Restricted isRestricted)
{
    // Determine if contributions have been restricted so we can
    // figure out what to record for the restriction depth. We
    // can avoid doing this extra check if the caller knows they
    // restricted contributions.
    const bool contributionRestricted = 
        isRestricted == _Restricted::Yes || !CanContributeSpecs();

    auto& currDepth = _graph->_unshared[_nodeIdx].restrictionDepth;

    if (!contributionRestricted) {
        currDepth = 0;
    }
    else {
        size_t newDepth = GetPath().GetPathElementCount();

        // XXX:
        // This should result in a "capacity exceeded" composition error
        // instead of just a warning.
        if (auto maxDepth =
            std::numeric_limits<std::decay_t<decltype(currDepth)>>::max();
            newDepth > maxDepth) {
            TF_WARN("Maximum restriction namespace depth exceeded");
            newDepth = maxDepth;
        }

        currDepth = newDepth;
    }
}

bool
PcpNodeRef::IsDueToAncestor() const
{
    TF_DEV_AXIOM(_nodeIdx < _graph->_unshared.size());
    return _graph->_unshared[_nodeIdx].isDueToAncestor;
}

void
PcpNodeRef::SetIsDueToAncestor(bool isDueToAncestor)
{
    TF_DEV_AXIOM(_nodeIdx < _graph->_unshared.size());
    _graph->_unshared[_nodeIdx].isDueToAncestor = isDueToAncestor;
}

bool
PcpNodeRef::HasSpecs() const
{
    TF_DEV_AXIOM(_nodeIdx < _graph->_unshared.size());
    return _graph->_unshared[_nodeIdx].hasSpecs;
}

void
PcpNodeRef::SetHasSpecs(bool hasSpecs)
{
    TF_DEV_AXIOM(_nodeIdx < _graph->_unshared.size());
    _graph->_unshared[_nodeIdx].hasSpecs = hasSpecs;
}

const SdfPath& 
PcpNodeRef::GetPath() const
{
    TF_DEV_AXIOM(_nodeIdx < _graph->_unshared.size());
    return _graph->_unshared[_nodeIdx].sitePath;
}

PcpLayerStackSite
PcpNodeRef::GetSite() const
{
    return PcpLayerStackSite(GetLayerStack(), GetPath());
}

bool 
PcpNodeRef::IsRootNode() const
{
    return GetArcType() == PcpArcTypeRoot;
}

bool
PcpNodeRef::IsInert() const
{
    const PcpPrimIndex_Graph::_Node& node = _graph->_GetNode(_nodeIdx);
    return node.smallInts.inert || _graph->_unshared[_nodeIdx].culled;
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
    return !(node.smallInts.inert || _graph->_unshared[_nodeIdx].culled) &&
        (!node.smallInts.permissionDenied || _graph->IsUsd());
}

size_t
PcpNodeRef::GetSpecContributionRestrictedDepth() const
{
    return _graph->_unshared[_nodeIdx].restrictionDepth;
}

void
PcpNodeRef::SetSpecContributionRestrictedDepth(size_t depth)
{
    _graph->_unshared[_nodeIdx].restrictionDepth = depth;
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

static SdfPath 
_GetPathAtIntroDepth(const SdfPath &path, int depthBelowIntro)
{
    SdfPath pathAtIntroduction = path;
    for ( ; depthBelowIntro; --depthBelowIntro) {
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
PcpNodeRef::GetPathAtIntroduction() const
{
    return _GetPathAtIntroDepth(GetPath(), GetDepthBelowIntroduction());
}

SdfPath
PcpNodeRef::GetIntroPath() const
{
    // Start with the parent node's current path.
    const PcpNodeRef parent = GetParentNode();
    if (!parent)
        return SdfPath::AbsoluteRootPath();

    return _GetPathAtIntroDepth(parent.GetPath(), GetDepthBelowIntroduction());
}

PCP_API 
SdfPath
PcpNodeRef::GetPathAtOriginRootIntroduction() const
{
    return _GetPathAtIntroDepth(GetPath(), 
        GetOriginRootNode().GetDepthBelowIntroduction());
}

PcpNodeRef::child_const_range
PcpNodeRef::GetChildrenRange() const
{
    PcpNodeRef node(_graph, _nodeIdx);
    return child_const_range(child_const_iterator(node, /* end = */ false),
                             child_const_iterator(node, /* end = */ true));
}

PcpNodeRef::child_const_reverse_range
PcpNodeRef::GetChildrenReverseRange() const
{
    PcpNodeRef node(_graph, _nodeIdx);
    return child_const_reverse_range(
        child_const_reverse_iterator(node, /* end = */ false),
        child_const_reverse_iterator(node, /* end = */ true));
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
            _node._graph->_GetNode(_node).indexes.firstChildIndex :
            PcpPrimIndex_Graph::_Node::_invalidNodeIndex)
{
    // Do nothing
}

void 
PcpNodeRef_ChildrenIterator::increment()
{
    _index = _node._graph->_GetNode(_index).indexes.nextSiblingIndex;
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
        _index = _node._graph->_GetNode(_node).indexes.lastChildIndex;
    }
    else {
        increment();
    }
}

PcpNodeRef_ChildrenReverseIterator::PcpNodeRef_ChildrenReverseIterator(
    const PcpNodeRef& node, bool end) :
    _node(node),
    _index(!end ?
            _node._graph->_GetNode(_node).indexes.lastChildIndex :
            PcpPrimIndex_Graph::_Node::_invalidNodeIndex)
{
    // Do nothing
}

void 
PcpNodeRef_ChildrenReverseIterator::increment()
{
    _index = _node._graph->_GetNode(_index).indexes.prevSiblingIndex;
}

int
PcpNode_GetNonVariantPathElementCount(const SdfPath &path)
{
    return _GetNonVariantPathElementCount(path);
}

static inline int
_GetNonVariantPathElementCount(const SdfPath &path)
{
    // The following code is equivalent to but more performant than:
    //
    // return path.StripAllVariantSelections().GetPathElementCount();

    int count = static_cast<int>(path.GetPathElementCount());
    if (path.ContainsPrimVariantSelection()) {
        SdfPath cur(path);

        // Walk up until we hit a variant selection node, then decrement count,
        // and keep going if there are more.
        do {
            while (!cur.IsPrimVariantSelectionPath()) {
                cur = cur.GetParentPath();
            }
            --count;
            cur = cur.GetParentPath();
        } while (cur.ContainsPrimVariantSelection());
    }

    return count;
}

std::ostream &
operator<<(std::ostream &out, const PcpNodeRef &node) 
{
    out << "(" << node._GetNodeIndex() << ") " << 
        TfEnum::GetDisplayName(node.GetArcType()) << " " << node.GetSite();
    return out;
}


PXR_NAMESPACE_CLOSE_SCOPE
