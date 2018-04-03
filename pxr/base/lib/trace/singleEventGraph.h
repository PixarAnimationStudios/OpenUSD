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

#ifndef SINGLE_EVENT_GRAPH_H
#define SINGLE_EVENT_GRAPH_H

#include "pxr/pxr.h"

#include "pxr/base/trace/api.h"
#include "pxr/base/trace/event.h"
#include "pxr/base/trace/singleEventNode.h"
#include "pxr/base/trace/threads.h"
#include "pxr/base/tf/refBase.h"
#include "pxr/base/tf/refPtr.h"
#include "pxr/base/tf/token.h"
#include "pxr/base/tf/weakBase.h"
#include "pxr/base/tf/weakPtr.h"
#include "pxr/base/tf/declarePtrs.h"
#include "pxr/base/js/types.h"

#include <map>

PXR_NAMESPACE_OPEN_SCOPE

TF_DECLARE_WEAK_AND_REF_PTRS(TraceSingleEventGraph);

////////////////////////////////////////////////////////////////////////////////
/// \class TraceSingleEventGraph
///
/// This class contains a timeline call graph and a map of counters to their 
/// values over time.
///
///
class TraceSingleEventGraph : public TfRefBase, public TfWeakBase {
public:
    using CounterValues = std::map<TraceEvent::TimeStamp, double>;
    using CounterMap = std::map<TfToken, CounterValues>;

    static TraceSingleEventGraphRefPtr New() {
        return TfCreateRefPtr( 
            new TraceSingleEventGraph(TraceSingleEventNode::New()));
    }

    static TraceSingleEventGraphRefPtr New(
            TraceSingleEventNodeRefPtr root, 
            CounterMap counters) {
        return TfCreateRefPtr( 
            new TraceSingleEventGraph(root, std::move(counters)));
    }

    /// Returns the root node of the graph.
    const TraceSingleEventNodeRefPtr& GetRoot() const { return _root; }

    /// Returns the map of counter values.
    const CounterMap& GetCounters() const { return _counters; }

    /// Returns a JSON object representing the data in the call graph that 
    /// conforms to the Chrome Trace format.
    TRACE_API JsObject CreateChromeTraceObject() const;

    /// Adds the contexts of \p graph to this graph.
    TRACE_API void Merge(const TraceSingleEventGraphRefPtr& graph);

private:
    TraceSingleEventGraph(TraceSingleEventNodeRefPtr root)
        : _root(root) {}

    TraceSingleEventGraph(  TraceSingleEventNodeRefPtr root, 
                            CounterMap counters)
        : _root(root)
        , _counters(std::move(counters)) {}

    // Root of the call graph.
    TraceSingleEventNodeRefPtr _root;
    // Counter data of the trace.
    CounterMap _counters;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // SINGLE_EVENT_GRAPH_H
