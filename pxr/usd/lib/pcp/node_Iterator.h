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
#ifndef PCP_NODE_ITERATOR_H
#define PCP_NODE_ITERATOR_H

/// \file pcp/node_Iterator.h

#include "pxr/pxr.h"
#include "pxr/usd/pcp/node.h"
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
    : public boost::iterator_facade<
                 /* Derived =   */ PcpNodeRef_PrivateChildrenConstIterator, 
                 /* ValueType = */ const PcpNodeRef,
                 /* Category =  */ boost::forward_traversal_tag
             >
{
public:
    // Required by TF_FOR_ALL but always assigned to afterwards.
    PcpNodeRef_PrivateChildrenConstIterator() { }

    /// Constructs an iterator pointing to \p node's first or past its
    /// last child.
    PcpNodeRef_PrivateChildrenConstIterator(const PcpNodeRef& node,
                                            bool end = false) :
        _node(node),
        _nodes(&_node._graph->_data->nodes[0])
    {
        _node._nodeIdx = end
            ? PcpPrimIndex_Graph::_Node::_invalidNodeIndex
            : _nodes[_node._nodeIdx].smallInts.firstChildIndex;
    }

private:
    friend class boost::iterator_core_access;
    void increment()
    {
        _node._nodeIdx = _nodes[_node._nodeIdx].smallInts.nextSiblingIndex;
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
    : public boost::iterator_facade<
                 /* Derived =   */ PcpNodeRef_PrivateChildrenConstReverseIterator, 
                 /* ValueType = */ const PcpNodeRef,
                 /* Category =  */ boost::forward_traversal_tag
             >
{
public:
    // Required by TF_FOR_ALL but always assigned to afterwards.
    PcpNodeRef_PrivateChildrenConstReverseIterator() { }

    /// Constructs an iterator pointing to \p node's first or past its
    /// last child.
    PcpNodeRef_PrivateChildrenConstReverseIterator(const PcpNodeRef& node,
                                                   bool end = false) :
        _node(node),
        _nodes(&_node._graph->_data->nodes[0])
    {
        _node._nodeIdx = end
            ? PcpPrimIndex_Graph::_Node::_invalidNodeIndex
            : _nodes[_node._nodeIdx].smallInts.lastChildIndex;
    }

private:
    friend class boost::iterator_core_access;
    void increment()
    {
        _node._nodeIdx = _nodes[_node._nodeIdx].smallInts.prevSiblingIndex;
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
    boost::true_type {};

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

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PCP_NODE_ITERATOR_H
