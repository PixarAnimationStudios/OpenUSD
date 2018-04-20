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

#ifndef TRACE_REPORTER_H
#define TRACE_REPORTER_H

#include "pxr/pxr.h"

#include "pxr/base/trace/api.h"
#include "pxr/base/trace/collector.h"
#include "pxr/base/trace/event.h"
#include "pxr/base/trace/eventNode.h"
#include "pxr/base/trace/key.h"
#include "pxr/base/trace/reporterBase.h"

#include "pxr/base/tf/declarePtrs.h"
#include "pxr/base/tf/mallocTag.h"
#include "pxr/base/tf/staticTokens.h"

#include <tbb/concurrent_queue.h>

#include <map>
#include <ostream>
#include <string>
#include <vector>

PXR_NAMESPACE_OPEN_SCOPE

#define TRACE_REPORTER_TOKENS       \
    ((warningString, "WARNING:"))

TF_DECLARE_PUBLIC_TOKENS(TraceReporterTokens, TRACE_REPORTER_TOKENS);


TF_DECLARE_WEAK_AND_REF_PTRS(TraceEventNode);
TF_DECLARE_WEAK_AND_REF_PTRS(TraceSingleEventNode);
TF_DECLARE_WEAK_AND_REF_PTRS(TraceSingleEventGraph);

TF_DECLARE_WEAK_AND_REF_PTRS(TraceReporter);

class TraceEventNode;
class TraceCollectionAvailable;

////////////////////////////////////////////////////////////////////////////////
/// \class TraceReporter
///
/// This class converters streams of TraceEvent objects into call graphs which
/// can then be used as a data source to a GUI or written out to a file.
///
class TraceReporter : 
    public TraceReporterBase, public TraceCollection::Visitor {
public:

    TF_MALLOC_TAG_NEW("Trace", "TraceReporter");
    
    using This = TraceReporter;
    using ThisPtr = TraceReporterPtr;
    using ThisRefPtr = TraceReporterRefPtr;

    using Event = TraceEvent;
    using TimeStamp = TraceEvent::TimeStamp;
    using CounterMap = TfHashMap<TfToken, double, TfToken::HashFunctor>;

    /// Create a new reporter with \a label and \a collector.
    static ThisRefPtr New(const std::string& label,
                          const TraceCollectorPtr& collector) {
        return TfCreateRefPtr(new This(label, collector));
    }

    /// Returns the global reporter.
    TRACE_API static TraceReporterPtr GetGlobalReporter();
   
    /// Destructor.
    TRACE_API virtual ~TraceReporter();

    /// Return the collector from which events are gathered.
    TraceCollectorPtr GetCollector() const {
        return _collector;
    }

    /// Return the label associated with this reporter.
    const std::string& GetLabel() {
        return _label;
    }

    /// \name Report Generation.
    /// @{

    /// Generates a report to the ostream \a s, dividing all times by 
    /// \a iterationCount.
    TRACE_API void Report(
        std::ostream &s,
        int iterationCount=1);

    /// Generates a report of the times to the ostream \a s.
    TRACE_API void ReportTimes(std::ostream &s);

    /// Generates a timeline trace report suitable for viewing in
    /// Chrome's trace viewer.
    TRACE_API void ReportChromeTracing(std::ostream &s);

    /// @}

    /// Returns the root node of the aggregated call graph.
    TRACE_API TraceEventNodePtr GetTreeRoot();

    /// Returns the root node of the timeline call graph.
    TRACE_API TraceSingleEventNodeRefPtr GetSingleEventRoot();

    /// Returns the timeline event graph
    TRACE_API TraceSingleEventGraphRefPtr GetSingleEventGraph();

    /// \name Counters
    /// @{

    /// Returns a map of counters (counter keys), associated with their total
    /// accumulated value. Each individual event node in the tree may also hold
    /// on to an inclusive and exclusive value for the given counter.
    TRACE_API const CounterMap & GetCounters() const;

    /// Returns the numeric index associated with a counter key. Counter values
    /// on the event nodes will have to be looked up by the numeric index.
    TRACE_API int GetCounterIndex(const TfToken &key) const;

    /// Add a counter to the reporter. This method can be used to restore a
    /// previous trace state and tree. Note, that the counter being added must
    /// have a unique key and index. The method will return false if a key or
    /// index already exists.
    TRACE_API bool AddCounter(const TfToken &key, int index, double totalValue);

    /// @}

    /// If we want to have multiple reporters per collector, this will need to
    /// be changed so that all reporters reporting on a collector update their
    /// respective trees
    TRACE_API void UpdateTree();

    /// Like UpdateTree() but also builds the single event tree.  This
    /// takes extra time and most clients don't need it so it's a separate
    /// method.
    TRACE_API void UpdateSingleEventTree();
    
    /// Clears event tree and counters.
    TRACE_API void ClearTree();

    /// \name Report options.
    /// @{

    /// This affects only stack trace event reporting.  If \c true then all
    /// events in a function are grouped together otherwise events are split
    /// out by address.
    TRACE_API void SetGroupByFunction(bool);

    /// Returns the current group-by-function state.
    TRACE_API bool GetGroupByFunction() const;

    /// When stack trace event reporting, this sets whether or not recursive 
    /// calls are folded in the output.  Recursion folding is useful when
    /// the stacks contain deep recursive structures.
    TRACE_API void SetFoldRecursiveCalls(bool);

    /// Returns the current setting for recursion folding for stack trace
    /// event reporting.
    TRACE_API bool GetFoldRecursiveCalls() const;

    /// @}

    /// Creates a valid TraceEventNode::Id object.
    /// This should be used by very few clients for certain special cases.
    /// For most cases, the TraceEventNode::Id object should be created and populated
    /// internally within the Reporter object itself.
    TRACE_API static TraceEventNode::Id CreateValidEventId();


    /// \name TraceCollection::Visitor Interface
    /// @{
    TRACE_API virtual void OnBeginCollection() override;
    TRACE_API virtual void OnEndCollection() override;
    TRACE_API virtual void OnBeginThread(const TraceThreadId&) override;
    TRACE_API virtual void OnEndThread(const TraceThreadId&) override;
    TRACE_API virtual bool AcceptsCategory(TraceCategoryId) override;
    TRACE_API virtual void OnEvent(
        const TraceThreadId&, const TfToken&, const TraceEvent&) override;
    /// @}

protected:

    TRACE_API TraceReporter(const std::string& label,
                   const TraceCollectorPtr& collector);

private:
    // Internal methods to traverse the collector's event log.
    using _EventTimes = std::map<TfToken, TimeStamp>;

    bool _IsAcceptingCollections() override;
    void _ProcessCollection(const TraceReporterBase::CollectionPtr&) override;
    void _ComputeInclusiveCounterValues();
    void _UpdateTree(bool buildSingleEventGraph);
    void _PrintRecursionMarker(std::ostream &s, const std::string &label, 
                               int indent);
    void _PrintLineTimes(std::ostream &s, double inclusive, double exclusive,
                    int count, const std::string& label, int indent,
                    bool recursive_node, int iterationCount=1);
    void _PrintNodeTimes(std::ostream &s, TraceEventNodeRefPtr node,  int indent,
                         int iterationCount=1);
    void _PrintLineCalls(std::ostream &s, int inclusive, int exclusive,
                         int total, const std::string& label, int indent);
    void _PrintTimes(std::ostream &s);
    void _AccumulateTime(const TraceEventNodeRefPtr& node);

    void _OnBeginEvent(const TraceThreadId&, const TfToken&, const TraceEvent&);
    void _OnEndEvent(const TraceThreadId&, const TfToken&, const TraceEvent&);
    void _OnCounterEvent(
        const TraceThreadId&, const TfToken&, const TraceEvent&);
    void _OnDataEvent(const TraceThreadId&, const TfToken&, const TraceEvent&);
    void _OnTimespanEvent(
        const TraceThreadId&, const TfToken&, const TraceEvent&);

    std::string _GetKeyName(const TfToken&) const;

    TraceCollectorPtr _collector;

    std::string _label;

    bool _groupByFunction;
    bool _foldRecursiveCalls;

    _EventTimes _eventTimes;
    TraceEventNodeRefPtr _rootNode;
    TraceSingleEventGraphRefPtr _singleEventGraph;

    CounterMap _counters;

    using _CounterIndexMap =TfHashMap<TfToken, int, TfToken::HashFunctor>;
    _CounterIndexMap _counterIndexMap;

    // Helper class for event graph creation
    struct _PendingEventNode {
        struct Child {
            TimeStamp start;
            TraceEventNodeRefPtr node;
        };

        struct CounterData {
            TimeStamp time;
            int index;
            double value;
        };

        _PendingEventNode(
            TraceEventNode::Id id, const TfToken& key, TimeStamp start);

        Child Close(TimeStamp end);

        TraceEventNode::Id id;
        TfToken key;
        TimeStamp start;
        std::vector<Child> children;
        std::vector<CounterData> counters;
    };
    
    using _PendingNodeStack = std::vector<_PendingEventNode>;
    using _ThreadStackMap = std::map<TraceThreadId, _PendingNodeStack>;
    _ThreadStackMap _threadStacks;
    int _counterIndex;
    bool _buildSingleEventGraph;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // TRACE_REPORTER_H
