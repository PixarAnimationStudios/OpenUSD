//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_EXEC_VDF_DYNAMIC_TOPOLOGICAL_SORTER_H
#define PXR_EXEC_VDF_DYNAMIC_TOPOLOGICAL_SORTER_H

/// \file

#include "pxr/pxr.h"

#include "pxr/exec/vdf/api.h"
#include "pxr/exec/vdf/traits.h"

#include "pxr/base/trace/trace.h"
#include "pxr/base/tf/hash.h"
#include "pxr/base/tf/hashmap.h"
#include "pxr/base/tf/hashset.h"
#include "pxr/base/tf/stl.h"

#include <unordered_map>
#include <vector>

PXR_NAMESPACE_OPEN_SCOPE

/// Simple allocator for priorities.
///
class Vdf_TopologicalPriorityAllocator
{
public:
    VDF_API
    Vdf_TopologicalPriorityAllocator();
    VDF_API
    ~Vdf_TopologicalPriorityAllocator();

    /// Return the next unused priority.
    VDF_API
    int Allocate();

    /// Release \p priority for future re-use.
    VDF_API
    void Free(int priority);

    /// Reset the allocator to its initial state.
    VDF_API
    void Clear();

private:
    int _next;
    std::vector<int> _reusablePriorities;
};


/// Dynamic topological sorter.
///
/// Maintains a complete, up-to-date topological ordering of a DAG while
/// edges are inserted or removed.
///
/// `Vertex` must be hashable with TfHash.
///
/// Implements "Algorithm PK" (excluding cycle detection) from:
///
///   Pierce, D. & Kelly, P.  A dynamic topological sort algorithm for 
///   directed acyclic graphs. ACM Journal of Experimental Algorithmics (JEA),
///   volume 11, pages 1.7, 2007.
///
template <typename Vertex>
class VdfDynamicTopologicalSorter
{
public:
    static const int InvalidPriority = -1;
    static const int LastPriority = INT_MAX;

    /// Type used for passing \c Vertex as a parameter.  This allows simple
    /// vertex representations like \c int or \c T* to be passed with as little
    /// overhead as possible.
    using VertexParam = VdfByValueOrConstRef<Vertex>;

    /// Return the topological priority of \p v.
    int GetPriority(VertexParam v) const;

    /// Add the edge (\p source, \p target) to the graph.  
    void AddEdge(VertexParam source, VertexParam target);
    /// Remove the edge (\p source, \p target) from the graph.
    void RemoveEdge(VertexParam source, VertexParam target);

    /// Remove all edges and vertices from the graph.
    void Clear();

private:

    // Insert v, increment its reference count and return its priority.
    int _InsertVertex(VertexParam v);
    // Decrement v's reference count, removing it if the reference count
    // reaches zero.
    void _RemoveVertex(VertexParam v);

    // Pair of (Priority, Vertex), ordered only by Priority.
    struct _PrioritizedVertex
    {
        _PrioritizedVertex(
            int priority,
            Vertex vertex)
            : priority(priority)
            , vertex(vertex)
        {}

        bool operator<(const _PrioritizedVertex &rhs) const
        {
            return priority < rhs.priority;
        }

        int priority;
        Vertex vertex;
    };

    // Vertex mapping directions.
    enum _Direction { _Outgoing = 0, _Incoming, _NumDirections };

    // Depth-first search with a boundary.  Does not visit vertices, w, for
    // which boundaryComp(priority(w), boundary) returns false.
    //
    // Direction specifies the direction of the search: either along outgoing
    // edges (_Outgoing) or incoming edges (_Incoming.)
    template <typename PriorityBoundaryComp>
    std::vector<_PrioritizedVertex> _DFSWithBoundary(
        const int direction,
        VertexParam v,
        int boundary,
        const PriorityBoundaryComp &boundaryComp);

    // Combine the results of the backward and forward traversals
    // to assign new priorities for the affected vertices.
    void _Reorder(
        std::vector<_PrioritizedVertex> *deltaBackward,
        std::vector<_PrioritizedVertex> *deltaForward);

private:

    // Priority and reference count for a Vertex.
    struct _Rep
    {
        _Rep()
            : priority(InvalidPriority)
            , refCount(0)
        {}

        int priority;
        unsigned int refCount;
    };

    using _PriorityMap = TfHashMap<Vertex, _Rep, TfHash>;
    _PriorityMap _priorities;

    Vdf_TopologicalPriorityAllocator _priorityAllocator;

    using _EdgeMap = std::unordered_multimap<Vertex, Vertex>;
    _EdgeMap _edges[_NumDirections];
};


template <typename Vertex>
inline int
VdfDynamicTopologicalSorter<Vertex>::GetPriority(
    VertexParam v) const
{
    typename _PriorityMap::const_iterator i = _priorities.find(v);
    if (i != _priorities.end()) {
        return i->second.priority;
    }
    return InvalidPriority;
}


template <typename Vertex>
void
VdfDynamicTopologicalSorter<Vertex>::AddEdge(
    VertexParam source,
    VertexParam target)
{
    _edges[_Outgoing].emplace(source, target);
    _edges[_Incoming].emplace(target, source);

    // Get priorities for source and target, creating new ones if the vertex
    // is brand new.
    const int sourcePriority = _InsertVertex(source);
    const int targetPriority = _InsertVertex(target);

    // We only need to do anything if adding the edge violates the existing
    // topological ordering.
    if (sourcePriority > targetPriority) {
        TRACE_SCOPE("VdfDynamicTopologicalSorter::AddEdge -- reordering");

        // "Forward search" -- traverse in the outgoing (_Outgoing) direction
        // from target, using source's priority as an upper bound to guide the
        // search.
        std::vector<_PrioritizedVertex> deltaForward =
            _DFSWithBoundary<>(
                _Outgoing, target, sourcePriority,
                std::less<int>());
        // "Backward search" -- traverse in the incoming (_Incoming) direction
        // from source, using target's priority as a lower bound to guide the
        // search.
        std::vector<_PrioritizedVertex> deltaBackward =
            _DFSWithBoundary<>(
                _Incoming, source, targetPriority,
                std::greater<int>());

        _Reorder(&deltaBackward, &deltaForward);
    }

    TF_DEV_AXIOM(GetPriority(source) < GetPriority(target));
}


template <typename Vertex>
void
VdfDynamicTopologicalSorter<Vertex>::RemoveEdge(
    VertexParam source,
    VertexParam target)
{
    // Removing an edge cannot invalidate the topological ordering, so there
    // nothing particularly fancy to do here.
    //
    // However, there is one subtlety: _edges.erase(value_type) would erase all
    // edges (source, target).  Because we allow clients to add the same edge
    // multiple times, we expect them to also remove it multiple times.

    using EdgeIter = typename _EdgeMap::iterator;
    using EdgeIterRange = std::pair<EdgeIter, EdgeIter>;
    using Edge = typename _EdgeMap::value_type;

    const EdgeIterRange outRange = _edges[_Outgoing].equal_range(source);
    const EdgeIter itl = std::find_if(outRange.first, outRange.second,
        [target](const Edge &e) { return e.second == target; });
    const bool foundOutgoing = itl != outRange.second;
    if (foundOutgoing) {
        _edges[_Outgoing].erase(itl);
    }
    
    const EdgeIterRange inRange = _edges[_Incoming].equal_range(target);
    const EdgeIter itr = std::find_if(inRange.first, inRange.second,
        [source](const Edge &e) { return e.second == source; });
    const bool foundIncoming = itr != inRange.second;
    if (foundIncoming) {
        _edges[_Incoming].erase(itr);
    }

    // If there is an incoming edge, we necessarily also expect and outgoing
    // edge for the same vertices.
    TF_DEV_AXIOM(foundOutgoing == foundIncoming);

    // If there is no edge, we do not need to remove and vertices.
    if (!foundOutgoing && !foundIncoming) {
        return;
    }

    _RemoveVertex(source);
    _RemoveVertex(target);
}


template <typename Vertex>
void
VdfDynamicTopologicalSorter<Vertex>::Clear()
{
    TfReset(_priorities);
    _priorityAllocator.Clear();
    TfReset(_edges[_Outgoing]);
    TfReset(_edges[_Incoming]);
}


template <typename Vertex>
inline int
VdfDynamicTopologicalSorter<Vertex>::_InsertVertex(
    VertexParam v)
{
    std::pair<typename _PriorityMap::iterator, bool> i =
        _priorities.insert(std::make_pair(v, _Rep()));
    _Rep &rep = i.first->second;
    ++rep.refCount;

    if (i.second) {
        rep.priority = _priorityAllocator.Allocate();
    }

    return rep.priority;
}


template <typename Vertex>
inline void
VdfDynamicTopologicalSorter<Vertex>::_RemoveVertex(
    VertexParam v)
{
    typename _PriorityMap::iterator i = _priorities.find(v);
    if (i == _priorities.end()) {
        return;
    }

    _Rep &rep = i->second;
    if (--rep.refCount == 0) {
        _priorityAllocator.Free(rep.priority);
        _priorities.erase(i);
    }
}


template <typename Vertex>
template <typename PriorityBoundaryComp>
std::vector<typename VdfDynamicTopologicalSorter<Vertex>::_PrioritizedVertex>
VdfDynamicTopologicalSorter<Vertex>::_DFSWithBoundary(
    const int direction,
    VertexParam v,
    int boundary,
    const PriorityBoundaryComp &boundaryComp)
{
    // Alias for edge map iterator & value types
    using EdgeIter = typename _EdgeMap::iterator ;
    using Edge = typename _EdgeMap::value_type;

    TRACE_FUNCTION();

    std::vector<_PrioritizedVertex> delta;
    delta.push_back(_PrioritizedVertex(GetPriority(v), v));

    std::vector<Vertex> pending(1, v);
    TfHashSet< Vertex, TfHash > visited;
    visited.insert(v);

    while (!pending.empty()) {
        const Vertex u = pending.back();
        pending.pop_back();

        const std::pair<EdgeIter, EdgeIter> edges =
            _edges[direction].equal_range(u);

        for (EdgeIter it=edges.first; it!=edges.second; ++it) {
            const Edge &e = *it;
            VertexParam w = e.second;

            // Skip already visited vertices.
            if (!visited.insert(w).second) {
                continue;
            }

            const int priority = GetPriority(w);

            // Skip vertices whose priority is beyond our boundary.
            if (!boundaryComp(priority, boundary)) {
                continue;
            }

            delta.push_back(_PrioritizedVertex(priority, w));
            pending.push_back(w);
        }
    }

    return delta;
}


template <typename Vertex>
void
VdfDynamicTopologicalSorter<Vertex>::_Reorder(
    std::vector<_PrioritizedVertex> *deltaBackward,
    std::vector<_PrioritizedVertex> *deltaForward)
{
    TRACE_FUNCTION();

    const size_t numVertices = deltaBackward->size() + deltaForward->size();

    // The algorithm restores the topological ordering by reassigning the
    // existing priorities of nodes in delta{Backward, Forward}.
    std::vector<int> availablePriorities;
    availablePriorities.reserve(numVertices);

    // Sort the backward-set into topological order.
    std::sort(deltaBackward->begin(), deltaBackward->end());
    for (const _PrioritizedVertex &v : *deltaBackward) {
        availablePriorities.push_back(v.priority);
    }
    // Stash away an iterator that partitions availablePriorities by
    // (deltaBackward, deltaForward).
    std::vector<int>::iterator mid = availablePriorities.end();
    // Sort the forward-set into topological order.
    std::sort(deltaForward->begin(), deltaForward->end());
    for (const _PrioritizedVertex &v : *deltaForward) {
        availablePriorities.push_back(v.priority);
    }
    // Now merge the pool of available priorities into increasing order.
    // Because each of delta{Backward,Forward} were sorted by priority,
    // the correspond sections of availablePriorities are already sorted
    // by priority, so we can simply merge them in-place.
    std::inplace_merge(
        availablePriorities.begin(), mid, availablePriorities.end());

    std::vector<int>::const_iterator p = availablePriorities.begin();

    // The key here is that we've sorted the entire pool of
    // availablePriorities, but only sorted deltaBackward and deltaForward
    // within themselves.  Everything in deltaBackward is topologically-prior
    // to everything in deltaForward.  So, we can pool all of their existing
    // indices and redistribute them into this correct order.
    for (const _PrioritizedVertex &v : *deltaBackward) {
        _priorities[v.vertex].priority = *(p++);
    }
    for (const _PrioritizedVertex &v : *deltaForward) {
        _priorities[v.vertex].priority = *(p++);
    }
}

PXR_NAMESPACE_CLOSE_SCOPE

#endif
