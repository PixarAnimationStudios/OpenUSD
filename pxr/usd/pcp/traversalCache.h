//
// Copyright 2024 Pixar
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
#ifndef PXR_USD_PCP_TRAVERSAL_CACHE_H
#define PXR_USD_PCP_TRAVERSAL_CACHE_H

#include "pxr/pxr.h"

#include "pxr/usd/pcp/node.h"
#include "pxr/usd/pcp/node_Iterator.h"
#include "pxr/usd/pcp/primIndex_Graph.h"
#include "pxr/usd/sdf/path.h"

#include <optional>
#include <tuple>
#include <vector>

PXR_NAMESPACE_OPEN_SCOPE

/// \class Pcp_TraversalCache
///
/// Caches the traversal of a subtree in a prim index starting at a
/// given node and with a specified path within that node's layer stack.
/// As clients traverse through the subtree, the starting path will
/// be translated to each node and cached, so that repeated traversals
/// will not incur the same path translation costs. Clients may also
/// store data associated with each node in the subtree.
template <class Data>
class Pcp_TraversalCache
{
public:
    /// \class iterator
    /// Object for iterating over the subtree of nodes cached by
    /// the owning Pcp_TraversalCache.
    class iterator
    {
    public:
        /// value type is a tuple of (Node(), PathInNode(), AssociatedData()).
        /// See performance note on PathInNode().
        using value_type = std::tuple<PcpNodeRef const, SdfPath const, Data&>;

        /// Return the current node.
        PcpNodeRef Node()
        {
            return *_iter;
        }

        /// Return the original traversal path given to the owning
        /// Pcp_TraversalCache translated to the current node.
        ///
        /// This function will translate and cache the traversal path for
        /// this node and all parent nodes if they have not already been
        /// computed.
        SdfPath const& PathInNode()
        {
            return *_owner->_GetEntry(*_iter, /* computePaths = */ true).path;
        }

        /// Return a reference to the data associated with the current node.
        Data& AssociatedData()
        {
            return _owner->_GetEntry(*_iter, /* computePaths = */ false).data;
        }

        /// Return value_type. Note that this will incur the cost of path
        /// translations described in PathInNode(). If you don't need the
        /// translated path, use one of the other member functions to avoid
        /// this cost.
        value_type operator*()
        {
            _Entry& e = _owner->_GetEntry(*_iter, /* computePaths = */ true);
            return std::tie(*_iter, *e.path, e.data);
        }

        /// Causes the next increment of this iterator to ignore descendants
        /// of the current node.
        void PruneChildren()
        {
            _iter.PruneChildren();
        }

        iterator& operator++()
        {
            ++_iter;
            return *this;
        }

        iterator operator++(int)
        {
            iterator result(*this);
            ++_iter;
            return result;
        }

        bool operator==(iterator const& rhs) const
        { return std::tie(_owner, _iter) == std::tie(rhs._owner, rhs._iter); }

        bool operator!=(iterator const& rhs) const
        { return !(*this == rhs); }

    private:
        friend class Pcp_TraversalCache;
        iterator(
            Pcp_TraversalCache* owner,
            PcpNodeRef_PrivateSubtreeConstIterator iter)
            : _owner(owner)
            , _iter(iter)
        { }

        Pcp_TraversalCache* const _owner = nullptr;
        PcpNodeRef_PrivateSubtreeConstIterator _iter;
    };

    /// Construct a traversal cache for the subtree rooted at \p startNode and
    /// the path \p pathInNode. \p pathInNode must be in \p startNode's
    /// namespace.
    Pcp_TraversalCache(PcpNodeRef const& startNode, SdfPath const& pathInNode)
        : _startNode(startNode)
    {
        _ResizeForGraph();
        _cache[_startNode._GetNodeIndex()].path = pathInNode;
    }

    Pcp_TraversalCache(Pcp_TraversalCache const&) = delete;
    Pcp_TraversalCache(Pcp_TraversalCache &&) = delete;
    Pcp_TraversalCache& operator=(Pcp_TraversalCache const&) = delete;
    Pcp_TraversalCache& operator=(Pcp_TraversalCache &&) = delete;

    iterator begin()
    {
        _ResizeForGraph();
        return iterator(
            this, 
            PcpNodeRef_PrivateSubtreeConstIterator(
                _startNode, /* end = */ false));
    }

    iterator end()
    {
        _ResizeForGraph();
        return iterator(
            this, 
            PcpNodeRef_PrivateSubtreeConstIterator(
                _startNode, /* end = */ true));
    }

private:
    struct _Entry
    {
        // Traversal path translated to the entry's corresponding node
        std::optional<SdfPath> path;

        // Client data associated with this entry's corresponding node.
        Data data;
    };

    void _ResizeForGraph()
    {
        PcpPrimIndex_Graph const* graph = _startNode.GetOwningGraph();

        // We assume the graph will never shrink.
        TF_VERIFY(graph->_GetNumNodes() >= _cache.size());

        if (graph->_GetNumNodes() > _cache.size()) {
            _cache.resize(graph->_GetNumNodes());
        }
    }

    SdfPath _TranslatePathsForNode(PcpNodeRef const& node)
    {
        // "Recursively" map the path from the parent node to this node.
        // This terminates because we'll eventually reach _startNode,
        // and we populated its path in the c'tor.
        _Entry& entry = _cache[node._GetNodeIndex()];
        if (!entry.path) {
            PcpNodeRef const parentNode = node.GetParentNode();
            _Entry& parentEntry = _cache[parentNode._GetNodeIndex()];
            if (!parentEntry.path) {
                parentEntry.path = _TranslatePathsForNode(parentNode);
            }
            
            SdfPath const& pathInParent = *(parentEntry.path);
            entry.path = pathInParent.IsEmpty() ? SdfPath() : 
                node.GetMapToParent().MapTargetToSource(pathInParent);
        }

        return *entry.path;
    }

    _Entry& _GetEntry(PcpNodeRef const& node, bool computePaths)
    {
        TF_VERIFY(node._GetNodeIndex() < _cache.size());

        // If requested, make sure the translated path is populated before
        // returning the _Entry to the caller.
        if (computePaths) {
            _TranslatePathsForNode(node);
        }
        return _cache[node._GetNodeIndex()];
    }

    PcpNodeRef _startNode;
    std::vector<_Entry> _cache;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif
