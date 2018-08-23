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

#ifndef TRACE_EVENT_TREE_H
#define TRACE_EVENT_TREE_H

#include "pxr/pxr.h"

#include "pxr/base/trace/api.h"
#include "pxr/base/trace/event.h"
#include "pxr/base/trace/eventNode.h"
#include "pxr/base/trace/threads.h"
#include "pxr/base/tf/refBase.h"
#include "pxr/base/tf/refPtr.h"
#include "pxr/base/tf/token.h"
#include "pxr/base/tf/weakBase.h"
#include "pxr/base/tf/weakPtr.h"
#include "pxr/base/tf/declarePtrs.h"
#include "pxr/base/js/types.h"

#include <vector>
#include <unordered_map>

PXR_NAMESPACE_OPEN_SCOPE

class TraceCollection;
TF_DECLARE_WEAK_AND_REF_PTRS(TraceEventTree);

////////////////////////////////////////////////////////////////////////////////
/// \class TraceEventTree
///
/// This class contains a timeline call tree and a map of counters to their 
/// values over time.
///
///
class TraceEventTree : public TfRefBase, public TfWeakBase {
public:
    using CounterValues = std::vector<std::pair<TraceEvent::TimeStamp, double>>;
    using CounterValuesMap =
        std::unordered_map<TfToken, CounterValues, TfToken::HashFunctor>;
    using CounterMap =
        std::unordered_map<TfToken, double, TfToken::HashFunctor>;

    using MarkerValues = std::vector<std::pair<TraceEvent::TimeStamp, TraceThreadId>>;
    using MarkerValuesMap =
        std::unordered_map<TfToken, MarkerValues, TfToken::HashFunctor>;

    /// Creates a new TraceEventTree instance from the data in \p collection 
    /// and \p initialCounterValues.
    TRACE_API static TraceEventTreeRefPtr New(
        const TraceCollection& collection,
        const CounterMap* initialCounterValues = nullptr);

    static TraceEventTreeRefPtr New() {
        return TfCreateRefPtr( 
            new TraceEventTree(TraceEventNode::New()));
    }

    static TraceEventTreeRefPtr New(
            TraceEventNodeRefPtr root, 
            CounterValuesMap counters,
            MarkerValuesMap markers) {
        return TfCreateRefPtr( 
            new TraceEventTree(root, std::move(counters), std::move(markers)));
    }

    /// Returns the root node of the tree.
    const TraceEventNodeRefPtr& GetRoot() const { return _root; }

    /// Returns the map of counter values.
    const CounterValuesMap& GetCounters() const { return _counters; }

    /// Returns the map of markers values.
    const MarkerValuesMap& GetMarkers() const { return _markers; }

    /// Return the final value of the counters in the report.
    CounterMap GetFinalCounterValues() const;

    /// Returns a JSON object representing the data in the call tree that 
    /// conforms to the Chrome Trace format.
    TRACE_API JsObject CreateChromeTraceObject() const;

    /// Adds the contexts of \p tree to this tree.
    TRACE_API void Merge(const TraceEventTreeRefPtr& tree);

    /// Adds the data from \p collection to this tree.
    TRACE_API void Add(const TraceCollection& collection);

private:
    TraceEventTree(TraceEventNodeRefPtr root)
        : _root(root) {}

    TraceEventTree(  TraceEventNodeRefPtr root, 
                            CounterValuesMap counters,
                            MarkerValuesMap markers)
        : _root(root)
        , _counters(std::move(counters))
        , _markers(std::move(markers)) {}

    // Root of the call tree.
    TraceEventNodeRefPtr _root;
    // Counter data of the trace.
    CounterValuesMap _counters;
    // Marker data of the trace.
    MarkerValuesMap _markers;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // TRACE_EVENT_TREE_H
