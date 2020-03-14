//
// Copyright 2018 Pixar
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

#ifndef PXR_BASE_TRACE_AGGREGATE_TREE_H
#define PXR_BASE_TRACE_AGGREGATE_TREE_H

#include "pxr/pxr.h"

#include "pxr/base/trace/api.h"
#include "pxr/base/trace/aggregateNode.h"

PXR_NAMESPACE_OPEN_SCOPE

class TraceCollection;

TF_DECLARE_WEAK_AND_REF_PTRS(TraceAggregateTree);
TF_DECLARE_WEAK_AND_REF_PTRS(TraceEventTree);

////////////////////////////////////////////////////////////////////////////////
/// \class TraceAggregateTree
///
/// A representation of a call tree. Each node represents one or more calls that
/// occurred in the trace. Multiple calls to a child node are aggregated into one
/// node.
///

class TraceAggregateTree : public TfRefBase, public TfWeakBase {
public:
    using This = TraceAggregateTree;
    using ThisPtr = TraceAggregateTreePtr;
    using ThisRefPtr = TraceAggregateTreeRefPtr;

    using TimeStamp = TraceEvent::TimeStamp;
    using EventTimes = std::map<TfToken, TimeStamp>;
    using CounterMap = TfHashMap<TfToken, double, TfToken::HashFunctor>;

    /// Create an empty tree
    static ThisRefPtr New() {
        return TfCreateRefPtr(new This());
    }

    /// Returns the root node of the tree.
    TraceAggregateNodePtr GetRoot() { return _root; }

    /// Returns a map of event keys to total inclusive time.
    const EventTimes& GetEventTimes() const { return _eventTimes; }

    /// Returns a map of counters (counter keys), associated with their total
    /// accumulated value. Each individual event node in the tree may also hold
    /// on to an inclusive and exclusive value for the given counter.
    const CounterMap& GetCounters() const { return _counters; }

    /// Returns the numeric index associated with a counter key. Counter values
    /// on the event nodes will have to be looked up by the numeric index.
    TRACE_API int GetCounterIndex(const TfToken &key) const;

    /// Add a counter to the tree. This method can be used to restore a
    /// previous trace state and tree. Note, that the counter being added must
    /// have a unique key and index. The method will return false if a key or
    /// index already exists.
    TRACE_API bool AddCounter(const TfToken &key, int index, double totalValue);

    /// Removes all data and nodes from the tree.
    TRACE_API void Clear();

    /// Creates new nodes and counter data from data in \p eventTree and \p 
    /// collection. 
    TRACE_API void Append(
        const TraceEventTreeRefPtr& eventTree,
        const TraceCollection& collection);

private:
    TRACE_API TraceAggregateTree();

    using _CounterIndexMap =TfHashMap<TfToken, int, TfToken::HashFunctor>;

    TraceAggregateNodeRefPtr _root;
    EventTimes _eventTimes;
    CounterMap _counters;
    _CounterIndexMap _counterIndexMap;
    int _counterIndex;

    friend class Trace_AggregateTreeBuilder;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_BASE_TRACE_AGGREGATE_TREE_H
