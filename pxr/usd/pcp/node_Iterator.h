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
#ifndef PXR_USD_PCP_NODE_ITERATOR_H
#define PXR_USD_PCP_NODE_ITERATOR_H

/// \file pcp/node_Iterator.h

#include "pxr/pxr.h"
#include "pxr/usd/pcp/node.h"
#include "pxr/usd/pcp/primIndex.h"
#include "pxr/usd/pcp/primIndex_Graph.h"

PXR_NAMESPACE_OPEN_SCOPE

// These classes exist because we want to optimize the iteration of a
// node's children while not exposing the PcpPrimIndex_Graph implementation
// detail outside of Pcp.  PcpNodeRef_ChildrenIterator and
// PcpNodeRef_ChildrenReverseIterator perform the same functions but can't
// inline access to PcpPrimIndex_Graph.

/// \class PcpNodeRef_PrivateChildrenConstIterator
///
/// Object used to iterate over child nodes (not all descendant nodes) of a
/// node in the prim index graph in strong-to-weak order.
///
class PcpNodeRef_PrivateChildrenConstIterator
{
public:
    using iterator_category = std::forward_iterator_tag;
    using value_type = const PcpNodeRef;
    using reference = const PcpNodeRef&;
    using pointer = const PcpNodeRef*;
    using difference_type = std::ptrdiff_t;

    // Required by TF_FOR_ALL but always assigned to afterwards.
    PcpNodeRef_PrivateChildrenConstIterator() = default;

    /// Constructs an iterator pointing to \p node's first or past its
    /// last child.
    PcpNodeRef_PrivateChildrenConstIterator(const PcpNodeRef& node,
                                            bool end = false) :
        _node(node),
        _nodes(&_node._graph->_GetNode(0))
    {
        _node._nodeIdx = end
            ? PcpPrimIndex_Graph::_Node::_invalidNodeIndex
            : _nodes[_node._nodeIdx].indexes.firstChildIndex;
    }

    reference operator*() const { return dereference(); }
    pointer operator->() const { return &(dereference()); }

    PcpNodeRef_PrivateChildrenConstIterator& operator++() {
        increment();
        return *this;
    }

    PcpNodeRef_PrivateChildrenConstIterator operator++(int) {
        PcpNodeRef_PrivateChildrenConstIterator result(*this);
        increment();
        return result;
    }

    bool operator==(
        const PcpNodeRef_PrivateChildrenConstIterator& other) const {
        return equal(other);
    }

    bool operator!=(
        const PcpNodeRef_PrivateChildrenConstIterator& other) const {
        return !equal(other);
    }

private:
    void increment()
    {
        _node._nodeIdx = _nodes[_node._nodeIdx].indexes.nextSiblingIndex;
    }

    bool equal(const PcpNodeRef_PrivateChildrenConstIterator& other) const
    {
        return _node == other._node;
    }

    reference dereference() const
    {
        return _node;
    }

private:
    // Current graph node this iterator is pointing at.
    PcpNodeRef _node;
    const PcpPrimIndex_Graph::_Node* _nodes;
};

/// \class PcpNodeRef_PrivateChildrenConstReverseIterator
///
/// Object used to iterate over child nodes (not all descendant nodes) of a
/// node in the prim index graph in weak-to-strong order.
///
class PcpNodeRef_PrivateChildrenConstReverseIterator
{
public:
    using iterator_category = std::forward_iterator_tag;
    using value_type = const PcpNodeRef;
    using reference = const PcpNodeRef&;
    using pointer = const PcpNodeRef*;
    using difference_type = std::ptrdiff_t;

    // Required by TF_FOR_ALL but always assigned to afterwards.
    PcpNodeRef_PrivateChildrenConstReverseIterator() = default;

    /// Constructs an iterator pointing to \p node's first or past its
    /// last child.
    PcpNodeRef_PrivateChildrenConstReverseIterator(const PcpNodeRef& node,
                                                   bool end = false) :
        _node(node),
        _nodes(&_node._graph->_GetNode(0))
    {
        _node._nodeIdx = end
            ? PcpPrimIndex_Graph::_Node::_invalidNodeIndex
            : _nodes[_node._nodeIdx].indexes.lastChildIndex;
    }

    reference operator*() const { return dereference(); }
    pointer operator->() const { return &(dereference()); }

    PcpNodeRef_PrivateChildrenConstReverseIterator& operator++() {
        increment();
        return *this;
    }

    PcpNodeRef_PrivateChildrenConstReverseIterator operator++(int) {
        PcpNodeRef_PrivateChildrenConstReverseIterator result(*this);
        increment();
        return result;
    }

    bool operator==(
        const PcpNodeRef_PrivateChildrenConstReverseIterator& other) const {
        return equal(other);
    }

    bool operator!=(
        const PcpNodeRef_PrivateChildrenConstReverseIterator& other) const {
        return !equal(other);
    }

private:
    void increment()
    {
        _node._nodeIdx = _nodes[_node._nodeIdx].indexes.prevSiblingIndex;
    }

    bool equal(const PcpNodeRef_PrivateChildrenConstReverseIterator& other)const
    {
        return _node == other._node;
    }

    reference dereference() const
    {
        return _node;
    }

private:
    // Current graph node this iterator is pointing at.
    PcpNodeRef _node;
    const PcpPrimIndex_Graph::_Node* _nodes;
};

// Wrapper type for TF_FOR_ALL().
class PcpNodeRef_PrivateChildrenConstRange {
public:
    PcpNodeRef_PrivateChildrenConstRange(const PcpNodeRef& node_):node(node_){}
    PcpNodeRef node;
};

// TF_FOR_ALL() traits.  We build the iterators on demand.
template <>
struct Tf_IteratorInterface<PcpNodeRef_PrivateChildrenConstRange, false> {
    typedef PcpNodeRef_PrivateChildrenConstRange RangeType;
    typedef PcpNodeRef_PrivateChildrenConstIterator IteratorType;
    static IteratorType Begin(RangeType const &c)
    {
        return IteratorType(c.node, /* end = */ false);
    }
    static IteratorType End(RangeType const &c)
    {
        return IteratorType(c.node, /* end = */ true);
    }
};
template <>
struct Tf_IteratorInterface<PcpNodeRef_PrivateChildrenConstRange, true> {
    typedef PcpNodeRef_PrivateChildrenConstRange RangeType;
    typedef PcpNodeRef_PrivateChildrenConstReverseIterator IteratorType;
    static IteratorType Begin(RangeType const &c)
    {
        return IteratorType(c.node, /* end = */ false);
    }
    static IteratorType End(RangeType const &c)
    {
        return IteratorType(c.node, /* end = */ true);
    }
};
template <>
struct Tf_ShouldIterateOverCopy<PcpNodeRef_PrivateChildrenConstRange> :
    std::true_type {};

// Wrap a node for use by TF_FOR_ALL().
inline
PcpNodeRef_PrivateChildrenConstRange
Pcp_GetChildrenRange(const PcpNodeRef& node)
{
    return PcpNodeRef_PrivateChildrenConstRange(node);
}

// Return all of a node's children, strong-to-weak.
inline
PcpNodeRefVector
Pcp_GetChildren(const PcpNodeRef& node)
{
    typedef PcpNodeRef_PrivateChildrenConstIterator IteratorType;
    return PcpNodeRefVector(IteratorType(node, /* end = */ false),
                            IteratorType(node, /* end = */ true));
}

/// \class PcpNodeRef_PrivateSubtreeConstIterator
///
/// Object used to iterate over all nodes in a subtree rooted at a
/// given node in the prim index graph in strong-to-weak order.
class PcpNodeRef_PrivateSubtreeConstIterator
{
public:
    using iterator_category = std::forward_iterator_tag;
    using value_type = const PcpNodeRef;
    using reference = const PcpNodeRef&;
    using pointer = const PcpNodeRef*;
    using difference_type = std::ptrdiff_t;

    /// If \p end is false, constructs an iterator representing the
    /// beginning of the subtree of nodes starting at \p node.
    ///
    /// If \p end is true, constructs an iterator representing the
    /// next weakest node after the subtree of nodes starting at \p node.
    /// This may be an invalid node if \p node is the root node.
    PcpNodeRef_PrivateSubtreeConstIterator(const PcpNodeRef& node, bool end)
        : _node(node)
        , _nodes(&_node._graph->_GetNode(0))
        , _pruneChildren(false)
    {
        if (end) {
            _MoveToNext();
        }
    }
    
    /// Causes the next increment of this iterator to ignore
    /// descendants of the current node.
    void PruneChildren()
    {
        _pruneChildren = true;
    }

    reference operator*() const { return _node; }
    pointer operator->() const { return &_node; }

    PcpNodeRef_PrivateSubtreeConstIterator& operator++()
    {
        if (_pruneChildren || !_MoveToFirstChild()) {
            _MoveToNext();
        }
        _pruneChildren = false;
        return *this;
    }

    PcpNodeRef_PrivateSubtreeConstIterator operator++(int)
    {
        PcpNodeRef_PrivateSubtreeConstIterator result(*this);
        ++(*this);
        return result;
    }

    bool operator==(const PcpNodeRef_PrivateSubtreeConstIterator& other) const
    { return _node == other._node; }

    bool operator!=(const PcpNodeRef_PrivateSubtreeConstIterator& other) const
    { return !(*this == other); }

private:
    // If the current node has child nodes, move this iterator to the
    // first child and return true. Otherwise return false.
    bool _MoveToFirstChild()
    {
        auto& curNodeIdx = _node._nodeIdx;
        const auto& nodeIndexes = _nodes[curNodeIdx].indexes;
        const auto& invalid = PcpPrimIndex_Graph::_Node::_invalidNodeIndex;

        if (nodeIndexes.firstChildIndex != invalid) {
            curNodeIdx = nodeIndexes.firstChildIndex;
            return true;
        }
        return false;
    }

    // If the current node has a direct sibling, move this iterator to
    // that node. Otherwise, move this iterator to the next sibling of
    // the nearest ancestor node with siblings. If no such node exists,
    // (i.e., the current node is the weakest node in the index), this
    // iterator will point to an invalid node.
    void _MoveToNext()
    {
        auto& curNodeIdx = _node._nodeIdx;
        const PcpPrimIndex_Graph::_Node::_Indexes* nodeIndexes = nullptr;
        const auto& invalid = PcpPrimIndex_Graph::_Node::_invalidNodeIndex;

        while (curNodeIdx != invalid) {
            // See if we can move to the current node's next sibling.
            nodeIndexes = &_nodes[curNodeIdx].indexes;
            if (nodeIndexes->nextSiblingIndex != invalid) {
                curNodeIdx = nodeIndexes->nextSiblingIndex;
                break;
            }

            // If we can't, move to the current node's parent and try again.
            curNodeIdx = nodeIndexes->arcParentIndex;
        }
    }

private:
    PcpNodeRef _node;
    const PcpPrimIndex_Graph::_Node* _nodes;
    bool _pruneChildren;
};

// Wrapper type for range-based for loops.
class PcpNodeRef_PrivateSubtreeConstRange
{
public:
    PcpNodeRef_PrivateSubtreeConstRange(const PcpNodeRef& node)
        : _begin(node, /* end = */ false)
        , _end(node, /* end = */ true)
    { }

    PcpNodeRef_PrivateSubtreeConstIterator begin() const { return _begin; }
    PcpNodeRef_PrivateSubtreeConstIterator end() const { return _end; }

private:
    PcpNodeRef_PrivateSubtreeConstIterator _begin, _end;
};

/// Return node range for subtree rooted at the given \p node.
inline
PcpNodeRef_PrivateSubtreeConstRange
Pcp_GetSubtreeRange(const PcpNodeRef& node)
{
    return PcpNodeRef_PrivateSubtreeConstRange(node);
}

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_USD_PCP_NODE_ITERATOR_H
